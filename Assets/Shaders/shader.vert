#version 450

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InTexCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model *  vec4(InPosition, 1.0);
    fragTexCoord = InTexCoord;
}