#version 460
#include "Common.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InTexCoords;
layout(location = 3) in vec3 InColor;

layout(location = 1) out vec3 OutColor;

layout(push_constant) uniform PushConstants {
    int ObjectIndex;
    int MaterialIndex;
    int ActiveSceneIndex;
} DrawPushData;

void main() {
    //gl_Position = ViewProjMatrix * worldPos;
    gl_Position = vec4(InPosition, 1.0);
    OutColor = InColor;
}