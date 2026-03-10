#version 460
#include "Common.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InTexCoords;
layout(location = 3) in vec3 InColor;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec3 OutColor;

layout(push_constant) uniform PushConstants {
    int ObjectIndex;
    int MaterialIndex;
} DrawPushData;

void main() {
    mat4 ViewProjMatrix = SceneUBO.Scenes[0].ViewProjMatrix;
    mat4 ModelMat = Objects[DrawPushData.ObjectIndex].TransformationMat;
    mat4 InvModelMat = Objects[DrawPushData.ObjectIndex].InverseTransformationMat;

    vec4 worldPos = ModelMat * vec4(InPosition, 1.0);
    
    gl_Position = ViewProjMatrix * worldPos;
    
    fragUV = InTexCoords;
    fragWorldPos = worldPos.xyz;

    // Transform normal to world space using the Inverse Transpose
    // transpose(InverseTransformationMat) because InverseTransformationMat is usually (M^-1)
    // If your InverseTransformationMat is already provided, use it to keep normals perpendicular
    fragNormal = normalize(mat3(transpose(InvModelMat)) * InNormal);

    OutColor = InColor;
}