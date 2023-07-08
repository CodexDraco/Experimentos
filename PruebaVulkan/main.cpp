#include <algorithm>
#include <cstdint>
#include <limits>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

#include <cstring>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

constexpr std::array<const char *, 1> validationLayers{
    "VK_LAYER_KHRONOS_validation"};

constexpr std::array<const char *, 1> deviceExtensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool enabledValidationLayers = false;
#else
const bool enabledValidationLayers = true;
#endif

class Window {
public:
  Window();
  ~Window();

  GLFWwindow *data() const { return m_window; }

private:
  GLFWwindow *m_window;
};

Window::Window() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  m_window = glfwCreateWindow(WIDTH, HEIGHT, "Prueba Vulkan", nullptr, nullptr);
};

Window::~Window() {
  glfwDestroyWindow(m_window);
  glfwTerminate();
}

class Instance {
public:
  Instance();
  ~Instance();

  VkInstance data() const { return m_instance; }

private:
  bool checkLayers();
  VkInstance m_instance;
};

Instance::Instance() {
  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pApplicationName = "Prueba Vulkan",
                            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                            .pEngineName = "No Engine",
                            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                            .apiVersion = VK_API_VERSION_1_0};
  VkInstanceCreateInfo createInfo{.sType =
                                      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                  .pApplicationInfo = &appInfo,
                                  .enabledLayerCount = 0};

  uint32_t extsCount = 0;
  const char **glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&extsCount);
  createInfo.enabledExtensionCount = extsCount;
  createInfo.ppEnabledExtensionNames = glfwExtensions;

  if (enabledValidationLayers && checkLayers()) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    throw std::runtime_error("Requested validation layers not available");
  }

  VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create instance");
  }
}

Instance::~Instance() { vkDestroyInstance(m_instance, nullptr); }

bool Instance::checkLayers() {
  uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *requested : validationLayers) {
    for (VkLayerProperties available : availableLayers) {
      if (strcmp(requested, available.layerName) == 0) {
        return true;
      }
    }
  }
  return false;
}

class Surface {
public:
  Surface(Instance const &, Window const &);
  ~Surface();

  VkSurfaceKHR data() const { return m_surface; }

private:
  VkSurfaceKHR m_surface;
  VkInstance m_instance;
};

Surface::Surface(Instance const &instance, Window const &window) {
  m_instance = instance.data();
  if (glfwCreateWindowSurface(m_instance, window.data(), nullptr, &m_surface) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create window surface");
}

Surface::~Surface() { vkDestroySurfaceKHR(m_instance, m_surface, nullptr); }

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class PhysicalDevice {
public:
  PhysicalDevice(VkInstance, VkSurfaceKHR);

  VkPhysicalDevice data() const { return m_device; }
  QueueFamilyIndices queueFamilyIndices() const { return m_queueFamilyIndices; }

private:
  VkPhysicalDevice m_device;
  QueueFamilyIndices m_queueFamilyIndices;
  VkSurfaceKHR m_surface;
  SwapChainSupportDetails m_swapChainSupportDetails;

  bool isDeviceSuitable(VkPhysicalDevice);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice);
  bool checkExtensionSupport(VkPhysicalDevice);
  SwapChainSupportDetails querySwapChainSupport();
  VkSurfaceFormatKHR chooseSwapSurfaceFormat();
  VkPresentModeKHR chooseSwapPresentMode();
  VkExtent2D chooseSwapExtent(GLFWwindow *);
};

PhysicalDevice::PhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
  m_device = VK_NULL_HANDLE;
  m_surface = surface;

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("No GPU with Vulkan support available");
  }
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      m_device = device;
      return;
    }
  }

  throw std::runtime_error("No GPU supported found");
}

bool PhysicalDevice::checkExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);
  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  uint32_t supported = 0;
  for (VkExtensionProperties const &extension : availableExtensions) {
    for (char const *required : deviceExtensions) {
      if (strcmp(required, extension.extensionName) == 0) {
        supported++;
      }
    }
  }

  return supported == static_cast<uint32_t>(deviceExtensions.size());
}

bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(device, &properties);

  m_queueFamilyIndices = findQueueFamilies(device);
  if (m_queueFamilyIndices.isComplete()) {
    if (checkExtensionSupport(device)) {
      m_swapChainSupportDetails = querySwapChainSupport();
      if (!m_swapChainSupportDetails.formats.empty() &&
          !m_swapChainSupportDetails.presentModes.empty()) {
        std::cout << "Using GPU: " << properties.deviceName << std::endl;
        return true;
      }
    }
  }
  return false;
}

QueueFamilyIndices PhysicalDevice::findQueueFamilies(VkPhysicalDevice device) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  QueueFamilyIndices result;

  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      result.graphicsFamily = i;

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
    if (presentSupport) {
      result.presentFamily = i;
    }
  }
  return result;
}

SwapChainSupportDetails PhysicalDevice::querySwapChainSupport() {
  SwapChainSupportDetails details{};

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device, m_surface,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(m_device, m_surface, &formatCount,
                                       nullptr);

  if (formatCount > 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_device, m_surface, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(m_device, m_surface,
                                            &presentModeCount, nullptr);

  if (presentModeCount > 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_device, m_surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

VkSurfaceFormatKHR PhysicalDevice::chooseSwapSurfaceFormat() {
  for (const auto &availableFormat : m_swapChainSupportDetails.formats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return availableFormat;
  }

  return m_swapChainSupportDetails.formats[0];
}

VkPresentModeKHR PhysicalDevice::chooseSwapPresentMode() {
  for (const auto &presentMode : m_swapChainSupportDetails.presentModes) {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
      return presentMode;
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D PhysicalDevice::chooseSwapExtent(GLFWwindow *window) {
  if (m_swapChainSupportDetails.capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return m_swapChainSupportDetails.capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent{static_cast<uint32_t>(width),
                            static_cast<uint32_t>(height)};
    actualExtent.width =
        std::clamp(actualExtent.width,
                   m_swapChainSupportDetails.capabilities.minImageExtent.width,
                   m_swapChainSupportDetails.capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(
        actualExtent.height,
        m_swapChainSupportDetails.capabilities.minImageExtent.height,
        m_swapChainSupportDetails.capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

class Device {
public:
  Device(PhysicalDevice const &);
  ~Device();

private:
  VkDevice m_device;
  VkQueue m_graphicsQueue;
  VkQueue m_presentQueue;
};

Device::Device(PhysicalDevice const &physDevice) {
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
      physDevice.queueFamilyIndices().graphicsFamily.value(),
      physDevice.queueFamilyIndices().presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};
    queueCreateInfos.push_back(queueCreateInfo);
  }
  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data(),
      .pEnabledFeatures = &deviceFeatures};
  if (enabledValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }
  if (vkCreateDevice(physDevice.data(), &createInfo, nullptr, &m_device) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create logical device");
  }
  vkGetDeviceQueue(m_device,
                   physDevice.queueFamilyIndices().graphicsFamily.value(), 0,
                   &m_graphicsQueue);
  vkGetDeviceQueue(m_device,
                   physDevice.queueFamilyIndices().presentFamily.value(), 0,
                   &m_presentQueue);
}

Device::~Device() { vkDestroyDevice(m_device, nullptr); }

class Main {
public:
  Main();
  ~Main();

  void mainLoop();

private:
  Window m_window;
  Instance m_instance;
  Surface m_surface;
  PhysicalDevice m_physicalDevice;
  Device m_device;
};

Main::Main()
    : m_surface(m_instance, m_window),
      m_physicalDevice(m_instance.data(), m_surface.data()),
      m_device(m_physicalDevice) {}

Main::~Main() {}

void Main::mainLoop() {
  while (!glfwWindowShouldClose(m_window.data())) {
    glfwPollEvents();
  }
}

int main() {
  try {
    Main m;
    m.mainLoop();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
