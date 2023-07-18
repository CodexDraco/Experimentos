#version 460

layout(push_constant) uniform PushConstantData {
    float gradient;
} pc;

layout(location = 0) out vec4 f_color;

void main() {
    f_color = vec4(1.0, 0.0, 0.0, 1.0)*pc.gradient;
}
