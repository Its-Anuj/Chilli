#version 460

#include "Common.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InTexCoords;

layout(location = 0) out vec2 fragUV;

void main() {
    gl_Position = vec4(InPosition, 1.0);
    fragUV = InTexCoords;
}
