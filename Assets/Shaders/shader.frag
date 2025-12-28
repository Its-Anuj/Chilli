#version 460
#include "Common.glsl"

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main()
{
    Material ActiveMat = Materials[DrawPushData.MaterialIndex];
    vec4 texColor = texture(sampler2D(Textures[ActiveMat.AlbedoTextureIndex], Samplers[ActiveMat.AlbedoSamplerIndex]), fragUV);

    outColor = texColor;
}
