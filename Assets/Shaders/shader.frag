#version 460
#include "Common.glsl"

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 OutTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // Combine the separate texture + sampler
    Material Mat = MaterialsData[DrawPushData.MaterialIndex];
    
    outColor = SampleMaterialAlbedo(Mat, OutTexCoord) * Mat.AlbedoColor;
    //outColor = vec4(1.0, 1.0, 1.0, 1.0) * Mat.AlbedoTextureIndex;
}