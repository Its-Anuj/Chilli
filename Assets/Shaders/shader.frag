#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 InTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2D  Tex;

void main() {
    outColor = texture(Tex, InTexCoord);
}