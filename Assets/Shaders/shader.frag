#version 460
#include "Common.glsl"

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    int ObjectIndex;
    int MaterialIndex;
} DrawPushData;

void main()
{
    // Use the calculated light and the original alpha
    outColor = vec4(1,0 , 1, 1);
   // outColor = vec4(InColor, 1.0);
}