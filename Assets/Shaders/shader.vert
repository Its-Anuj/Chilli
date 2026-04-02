#version 460
#include "Common.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InTexCoords;
layout(location = 3) in vec3 InColor;

layout(location = 0) out vec3 OutColor;
layout(location = 1) out vec2 OutTexCoords;

layout(push_constant) uniform PushConstants {
    int ObjectIndex;
    int MaterialIndex;
    int ActiveSceneIndex;
    int Padding;
} DrawPushData;

void main() {
    Object ActiveObject = Objects[DrawPushData.ObjectIndex];
    Scene ActiveScene = SceneUBO.Scenes[DrawPushData.ActiveSceneIndex];
    
    mat4 ModelMatrix = ActiveObject.TransformationMat;

    vec4 WorldPos = ModelMatrix * vec4(InPosition, 1.0);

    gl_Position = ActiveScene.ViewProjMatrix * WorldPos;

    OutColor = InColor;
    OutTexCoords = InTexCoords;
}