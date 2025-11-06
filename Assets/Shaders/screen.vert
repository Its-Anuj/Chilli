#version 460
#include "Common.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InTexCoords;
layout(location = 2) in vec3 InNormals;

layout(location = 0) out vec2 OutTexCoord;

void main()
{
    Object Obj = ObjectData[DrawPushData.ObjectIndex];
    gl_Position = Obj.TransformationMat * vec4(InPosition, 1.0);
    
    OutTexCoord = InTexCoords;
}