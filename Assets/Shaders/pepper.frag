#version 460
#include "Common.glsl"

layout(location = 0) in vec2 fragUV;
layout(location = 1) flat in uint InPepperMaterialIndex;

layout(location = 0) out vec4 outColor;

struct PepperMaterial
{
    // x for AlbedoTexture, y for AlbedoSampler
    int TextureIndex;
    int SamplerIndex;
    int Padding[2];
    vec4 Color;
};

layout(set = 4, binding = 0) buffer PepperMaterialStorageBufferObject{
    PepperMaterial[] PepperMaterials;
} PepperMaterialSSBO;

void main()
{
    PepperMaterial Mat = PepperMaterialSSBO.PepperMaterials[InPepperMaterialIndex];
    vec4 TextureColor = texture(sampler2D(Textures[Mat.TextureIndex], Samplers[Mat.SamplerIndex]),
    fragUV);
    
    outColor = TextureColor * Mat.Color;
}   