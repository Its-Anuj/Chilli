#version 450

#include "Common.glsl"

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    uint ModeFlag;
	vec4 Color;
} pc;

void main() {
    outColor = pc.Color;
}