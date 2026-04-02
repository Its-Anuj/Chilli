#version 460
#include "Common.glsl"

layout(location = 0) in vec3 OutColor;
layout(location = 1) in vec2 InTexCoords;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    int ObjectIndex;
    int MaterialIndex;
    int ActiveSceneIndex;
    int Padding;
} DrawPushData;

void main()
{
    Material ActiveMaterial = Materials[DrawPushData.MaterialIndex];
    vec4 TextureColor = texture(sampler2D(Textures[int(ActiveMaterial.AlbedoTextureIndex)], 
                                          Samplers[int(ActiveMaterial.AlbedoSamplerIndex)]), InTexCoords);
    // Use the calculated light and the original alpha
    outColor = ActiveMaterial.AlbedoColor * TextureColor;
}