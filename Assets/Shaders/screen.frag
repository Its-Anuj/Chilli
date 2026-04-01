#version 460
#include "Common.glsl"

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 4, binding = 0) uniform texture2D  InColorTarget;
layout(set = 4, binding = 1) uniform sampler InSampler;
//layout(set = 4, binding = 2) uniform ScreenDrawBlock {
//    vec4 ScreenColor; // x and y for resolution of rendering framebuffer ,z for time, w is nothing
//} ScreenData;

void main()
{
    vec4 TextureColor = texture(sampler2D(InColorTarget, InSampler),fragUV);
    
    outColor = TextureColor; 
}   