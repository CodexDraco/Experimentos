use std::{sync::Arc, usize};

use vulkano::{
    buffer::{Buffer, BufferContents, BufferCreateInfo, BufferUsage, Subbuffer},
    command_buffer::{
        allocator::StandardCommandBufferAllocator, AutoCommandBufferBuilder,
        CommandBufferExecFuture, CommandBufferUsage, PrimaryAutoCommandBuffer, RenderPassBeginInfo,
        SubpassContents,
    },
    device::{
        physical::{PhysicalDevice, PhysicalDeviceType},
        Device, DeviceCreateInfo, DeviceExtensions, Queue, QueueCreateInfo, QueueFlags,
    },
    image::{view::ImageView, ImageUsage, SwapchainImage},
    instance::{Instance, InstanceCreateInfo},
    memory::allocator::{AllocationCreateInfo, MemoryUsage, StandardMemoryAllocator},
    pipeline::{
        graphics::{
            input_assembly::InputAssemblyState,
            vertex_input::Vertex,
            viewport::{Viewport, ViewportState},
        },
        GraphicsPipeline, Pipeline,
    },
    render_pass::{Framebuffer, FramebufferCreateInfo, RenderPass, Subpass},
    shader::ShaderModule,
    swapchain::{
        self, AcquireError, PresentFuture, Surface, Swapchain, SwapchainAcquireFuture,
        SwapchainCreateInfo, SwapchainCreationError, SwapchainPresentInfo,
    },
    sync::{
        self,
        future::{FenceSignalFuture, JoinFuture},
        FlushError, GpuFuture,
    },
    VulkanLibrary,
};
use vulkano_win::VkSurfaceBuild;
use winit::{
    event::{Event, WindowEvent},
    event_loop::{ControlFlow, EventLoop},
    window::{Window, WindowBuilder},
};

mod vs {
    vulkano_shaders::shader! {
        ty: "vertex",
        path: "shaders/vertex.glsl",
    }
}

mod fs {
    vulkano_shaders::shader! {
        ty: "fragment",
        path: "shaders/fragment.glsl",
    }
}

pub type Fence = FenceSignalFuture<
    PresentFuture<CommandBufferExecFuture<JoinFuture<Box<dyn GpuFuture>, SwapchainAcquireFuture>>>,
>;

pub struct Engine {
    _instance: Arc<Instance>,
    event_loop: EventLoop<()>,
    device: Arc<Device>,
    window: Arc<Window>,
    queue: Arc<Queue>,
    viewport: Viewport,
    swapchain: Arc<Swapchain>,
    vs: Arc<ShaderModule>,
    fs: Arc<ShaderModule>,
    render_pass: Arc<RenderPass>,
    vertex_buffer: Subbuffer<[MyVertex]>,
    command_buffers: Vec<Arc<PrimaryAutoCommandBuffer>>,
    window_resized: bool,
    recreate_swapchain: bool,
    fences: Vec<Option<Arc<Fence>>>,
    previous_fence_i: usize,
    command_buffer_allocator: StandardCommandBufferAllocator,
    fragment_constant_data: fs::PushConstantData,
}

impl Engine {
    pub fn run(mut self) {
        self.event_loop
            .run(move |event, _, control_flow| match event {
                Event::WindowEvent {
                    event: WindowEvent::CloseRequested,
                    ..
                } => {
                    *control_flow = ControlFlow::Exit;
                }
                Event::WindowEvent {
                    event: WindowEvent::Resized(_),
                    ..
                } => {
                    self.window_resized = true;
                }
                Event::MainEventsCleared => {
                    if self.window_resized || self.recreate_swapchain {
                        self.recreate_swapchain = false;

                        let new_dimensions = self.window.inner_size();

                        let (new_swapchain, new_images) =
                            match self.swapchain.recreate(SwapchainCreateInfo {
                                image_extent: new_dimensions.into(),
                                ..self.swapchain.create_info()
                            }) {
                                Ok(r) => r,
                                Err(SwapchainCreationError::ImageExtentNotSupported { .. }) => {
                                    return
                                }
                                Err(e) => panic!("failed to recreate swapchain: {e}"),
                            };
                        self.swapchain = new_swapchain;
                        let new_framebuffers = get_framebuffer(&new_images, &self.render_pass);

                        if self.window_resized {
                            self.window_resized = false;

                            self.viewport.dimensions = new_dimensions.into();
                            let new_pipeline = get_pipeline(
                                self.device.clone(),
                                self.vs.clone(),
                                self.fs.clone(),
                                self.render_pass.clone(),
                                self.viewport.clone(),
                            );
                            self.command_buffers = get_command_buffers(
                                &self.command_buffer_allocator,
                                &self.queue,
                                &new_pipeline,
                                &new_framebuffers,
                                &self.vertex_buffer,
                                self.fragment_constant_data,
                            );
                        }
                    }

                    let (image_i, suboptimal, acquire_future) =
                        match swapchain::acquire_next_image(self.swapchain.clone(), None) {
                            Ok(r) => r,
                            Err(AcquireError::OutOfDate) => {
                                self.recreate_swapchain = true;
                                return;
                            }
                            Err(e) => panic!("failed to acquire next image: {e}"),
                        };

                    if suboptimal {
                        self.recreate_swapchain = true;
                    }

                    if let Some(image_fence) = &self.fences[image_i as usize] {
                        image_fence.wait(None).unwrap();
                    }

                    let previous_future = match self.fences[self.previous_fence_i as usize].clone()
                    {
                        None => {
                            let mut now = sync::now(self.device.clone());
                            now.cleanup_finished();

                            now.boxed()
                        }
                        Some(fence) => fence.boxed(),
                    };

                    let future = previous_future
                        .join(acquire_future)
                        .then_execute(
                            self.queue.clone(),
                            self.command_buffers[image_i as usize].clone(),
                        )
                        .unwrap()
                        .then_swapchain_present(
                            self.queue.clone(),
                            SwapchainPresentInfo::swapchain_image_index(
                                self.swapchain.clone(),
                                image_i,
                            ),
                        )
                        .then_signal_fence_and_flush();

                    self.fences[image_i as usize] = match future {
                        Ok(value) => Some(Arc::new(value)),
                        Err(FlushError::OutOfDate) => {
                            self.recreate_swapchain = true;
                            None
                        }
                        Err(e) => {
                            println!("failed to flush future: {e}");
                            None
                        }
                    };

                    self.previous_fence_i = image_i as usize;
                }
                _ => (),
            });
    }
}

#[derive(BufferContents, Vertex)]
#[repr(C)]
struct MyVertex {
    #[format(R32G32_SFLOAT)]
    position: [f32; 2],
}

pub fn init_engine() -> Engine {
    let library = VulkanLibrary::new().expect("no local Vulkan library/DLL");
    let required_extensions = vulkano_win::required_extensions(&library);
    let instance = Instance::new(
        library,
        InstanceCreateInfo {
            enabled_extensions: required_extensions,
            ..Default::default()
        },
    )
    .expect("failed to create instance");

    let event_loop = EventLoop::new();
    let surface = WindowBuilder::new()
        .build_vk_surface(&event_loop, instance.clone())
        .unwrap();

    let window = surface
        .object()
        .unwrap()
        .clone()
        .downcast::<Window>()
        .unwrap();

    let device_extensions = DeviceExtensions {
        khr_swapchain: true,
        ..DeviceExtensions::empty()
    };

    let (physical_device, queue_family_index) =
        select_device(&instance, &surface, &device_extensions);

    let (device, mut queues) = Device::new(
        physical_device.clone(),
        DeviceCreateInfo {
            queue_create_infos: vec![QueueCreateInfo {
                queue_family_index,
                ..Default::default()
            }],
            enabled_extensions: device_extensions,
            ..Default::default()
        },
    )
    .expect("failed to create device");

    let queue = queues.next().unwrap();

    let caps = physical_device
        .surface_capabilities(&surface, Default::default())
        .expect("failed to get surface capabilities");

    let dimensions = window.inner_size();
    let composite_alpha = caps.supported_composite_alpha.into_iter().next().unwrap();
    let image_format = Some(
        physical_device
            .surface_formats(&surface, Default::default())
            .unwrap()[0]
            .0,
    );

    let (swapchain, images) = Swapchain::new(
        device.clone(),
        surface.clone(),
        SwapchainCreateInfo {
            min_image_count: caps.min_image_count + 1,
            image_format,
            image_extent: dimensions.into(),
            image_usage: ImageUsage::COLOR_ATTACHMENT,
            composite_alpha,
            ..Default::default()
        },
    )
    .unwrap();

    let render_pass = get_render_pass(device.clone(), &swapchain);
    let framebuffers = get_framebuffer(&images, &render_pass);

    let mesh = vec![
        MyVertex {
            position: [-0.5, -0.5],
        },
        MyVertex {
            position: [0.0, 0.5],
        },
        MyVertex {
            position: [0.5, -0.25],
        },
    ];

    let memory_allocator = StandardMemoryAllocator::new_default(device.clone());

    let vertex_buffer = Buffer::from_iter(
        &memory_allocator,
        BufferCreateInfo {
            usage: BufferUsage::VERTEX_BUFFER,
            ..Default::default()
        },
        AllocationCreateInfo {
            usage: MemoryUsage::Upload,
            ..Default::default()
        },
        mesh,
    )
    .unwrap();

    let vs = vs::load(device.clone()).expect("failed to create vertex shader module");
    let fs = fs::load(device.clone()).expect("failed to create fragment shader module");

    let fragment_constant_data = fs::PushConstantData { gradient: 0.0 };

    let viewport = Viewport {
        origin: [0.0, 0.0],
        dimensions: window.inner_size().into(),
        depth_range: 0.0..1.0,
    };

    let pipeline = get_pipeline(
        device.clone(),
        vs.clone(),
        fs.clone(),
        render_pass.clone(),
        viewport.clone(),
    );

    let command_buffer_allocator =
        StandardCommandBufferAllocator::new(device.clone(), Default::default());

    let command_buffers = get_command_buffers(
        &command_buffer_allocator,
        &queue,
        &pipeline,
        &framebuffers,
        &vertex_buffer,
        fragment_constant_data,
    );

    let frames_in_flight = images.len();
    let fences: Vec<Option<Arc<Fence>>> = vec![None; frames_in_flight];

    Engine {
        _instance: instance,
        event_loop,
        device,
        queue,
        window,
        viewport,
        swapchain,
        vs,
        fs,
        render_pass,
        vertex_buffer,
        command_buffers,
        window_resized: false,
        recreate_swapchain: false,
        fences,
        previous_fence_i: 0,
        command_buffer_allocator,
        fragment_constant_data,
    }
}

fn select_device(
    instance: &Arc<Instance>,
    surface: &Arc<Surface>,
    device_extensions: &DeviceExtensions,
) -> (Arc<PhysicalDevice>, u32) {
    instance
        .enumerate_physical_devices()
        .expect("could not enumerate devices")
        .filter(|p| p.supported_extensions().contains(&device_extensions))
        .filter_map(|p| {
            p.queue_family_properties()
                .iter()
                .enumerate()
                .position(|(i, q)| {
                    q.queue_flags.contains(QueueFlags::GRAPHICS)
                        && p.surface_support(i as u32, &surface).unwrap_or(false)
                })
                .map(|q| (p, q as u32))
        })
        .min_by_key(|(p, _)| match p.properties().device_type {
            PhysicalDeviceType::DiscreteGpu => 0,
            PhysicalDeviceType::IntegratedGpu => 1,
            PhysicalDeviceType::VirtualGpu => 2,
            PhysicalDeviceType::Cpu => 3,
            _ => 4,
        })
        .expect("no device available")
}

fn get_render_pass(device: Arc<Device>, swapchain: &Arc<Swapchain>) -> Arc<RenderPass> {
    vulkano::single_pass_renderpass!(
        device,
        attachments: {
            color: {
                load: Clear,
                store: Store,
                format: swapchain.image_format(),
                samples: 1,
            },
        },
        pass: {
            color: [color],
            depth_stencil: {},
        },
    )
    .unwrap()
}

fn get_framebuffer(
    images: &[Arc<SwapchainImage>],
    render_pass: &Arc<RenderPass>,
) -> Vec<Arc<Framebuffer>> {
    images
        .iter()
        .map(|image| {
            let view = ImageView::new_default(image.clone()).unwrap();
            Framebuffer::new(
                render_pass.clone(),
                FramebufferCreateInfo {
                    attachments: vec![view],
                    ..Default::default()
                },
            )
            .unwrap()
        })
        .collect::<Vec<_>>()
}

fn get_pipeline(
    device: Arc<Device>,
    vs: Arc<ShaderModule>,
    fs: Arc<ShaderModule>,
    render_pass: Arc<RenderPass>,
    viewport: Viewport,
) -> Arc<GraphicsPipeline> {
    GraphicsPipeline::start()
        .vertex_input_state(MyVertex::per_vertex())
        .vertex_shader(vs.entry_point("main").unwrap(), ())
        .input_assembly_state(InputAssemblyState::new())
        .viewport_state(ViewportState::viewport_fixed_scissor_irrelevant([viewport]))
        .fragment_shader(fs.entry_point("main").unwrap(), ())
        .render_pass(Subpass::from(render_pass, 0).unwrap())
        .build(device)
        .unwrap()
}

fn get_command_buffers(
    command_buffer_allocator: &StandardCommandBufferAllocator,
    queue: &Arc<Queue>,
    pipeline: &Arc<GraphicsPipeline>,
    framebuffers: &Vec<Arc<Framebuffer>>,
    vertex_buffer: &Subbuffer<[MyVertex]>,
    push_constants: fs::PushConstantData,
) -> Vec<Arc<PrimaryAutoCommandBuffer>> {
    framebuffers
        .iter()
        .map(|framebuffer| {
            let mut builder = AutoCommandBufferBuilder::primary(
                command_buffer_allocator,
                queue.queue_family_index(),
                CommandBufferUsage::MultipleSubmit,
            )
            .unwrap();

            builder
                .begin_render_pass(
                    RenderPassBeginInfo {
                        clear_values: vec![Some([0.1, 0.1, 0.1, 1.0].into())],
                        ..RenderPassBeginInfo::framebuffer(framebuffer.clone())
                    },
                    SubpassContents::Inline,
                )
                .unwrap()
                .bind_pipeline_graphics(pipeline.clone())
                .bind_vertex_buffers(0, vertex_buffer.clone())
                .push_constants(pipeline.layout().clone(), 0, push_constants)
                .draw(vertex_buffer.len() as u32, 1, 0, 0)
                .unwrap()
                .end_render_pass()
                .unwrap();

            Arc::new(builder.build().unwrap())
        })
        .collect()
}
