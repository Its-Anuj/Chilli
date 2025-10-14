#version 450

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 OutTexCoord;

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    mat4 ViewProjMatrix;
} GlobalUBO;

layout(set = 0, binding = 1) uniform UniformBufferObject {
    mat4 model;
} ubo;

void main() {
    gl_Position = GlobalUBO.ViewProjMatrix * ubo.model * vec4(InPosition, 1.0);
    OutTexCoord = InTexCoord;
    fragColor = vec3(InTexCoord, 0.0);
}