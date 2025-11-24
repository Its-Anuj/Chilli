#version 460

#include "Common.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InTexCoords;

layout(location = 0) out vec2 OutTexCoords;

void main()
{
    Object Obj = Objects[DrawPushData.ObjectIndex];
    gl_Position = Obj.TransformationMat * vec4(InPosition, 1.0);
    OutTexCoords = InTexCoords;
}