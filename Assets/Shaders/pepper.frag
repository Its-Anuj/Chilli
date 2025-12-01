#version 460

#include "Common.glsl"

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 InTexCoords;

void main() {
    Material Mat = Materials[DrawPushData.MaterialIndex];

    // Sample it
    outColor = texture(sampler2D(Textures[nonuniformEXT(Mat.Indicies.x)], Sampler[Mat.Indicies.y])
    , InTexCoords) * Mat.AlbedoColor;
}
