#version 460
#include "Common.glsl"

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 4, binding = 0) uniform texture2D ScreenTexture;
layout(set = 4, binding = 1) uniform sampler ScreenSampler;

void main()
{
    vec4 TextureColor = texture(sampler2D(Textures[1], ScreenSampler), fragUV);

    outColor = TextureColor;
}