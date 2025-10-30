#version 460

#include "Common.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InTexCoords;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 OutTexCoord;

void main()
{
    Object Obj = ObjectData[DrawPushData.ObjectIndex];
    gl_Position = SceneData.ViewProjMatrix * Obj.TransformationMat * vec4(InPosition, 1.0);
    
    // Create colorful gradient based on position and time
    fragColor = vec3(1.0, 1.0, 0.0f);
    OutTexCoord = InTexCoords;
}