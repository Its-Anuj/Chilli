#version 460
#include "Common.glsl"

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    int ObjectIndex;
    int MaterialIndex;
} DrawPushData;

void main()
{
    Material ActiveMaterial = Materials[DrawPushData.MaterialIndex];
    vec4 TextureColor = texture(sampler2D(Textures[ActiveMaterial.AlbedoTextureIndex], Samplers[ActiveMaterial.AlbedoSamplerIndex]),
        fragUV);
    
    outColor = TextureColor * ActiveMaterial.AlbedoColor;
}   