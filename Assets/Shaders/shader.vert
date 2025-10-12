#version 450

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 OutTexCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(InPosition, 1.0);
    OutTexCoord = InTexCoord;
    fragColor = vec3(InTexCoord, 0.0);
}