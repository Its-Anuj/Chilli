#version 460
#include "Common.glsl"

layout(location = 0) in vec2 OutTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // Combine the separate texture + sampler
    Material Mat = MaterialsData[DrawPushData.MaterialIndex];
    
    outColor = SampleMaterialAlbedo(Mat, OutTexCoord) * Mat.AlbedoColor;
}
