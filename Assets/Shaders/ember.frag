#version 450

#include "Common.glsl"

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in int FontIndex;

layout(location = 0) out vec4 outColor;

#define MAX_UNIQUE_FONT_COUNT 32

layout(set = 4, binding = 0) uniform texture2D fontAtlas[MAX_UNIQUE_FONT_COUNT];
layout(set = 4, binding = 1) uniform sampler fontAtlasSampler;

layout(push_constant) uniform Push {
    mat4 projection;
    vec4 textColor;
} pc;

void main() {
    // Sample using the correct atlas index passed from Instance Data
    float alpha = texture(sampler2D(fontAtlas[FontIndex], fontAtlasSampler), fragTexCoord).r;

    // Use the push constant color for the RGB, and the texture's R channel for transparency
    // We multiply pc.textColor.a by alpha so you can fade text in/out
    outColor = vec4(pc.textColor.rgb, pc.textColor.a * alpha);

    // Optimization: If alpha is basically 0, you can discard to save on blending
    // if(alpha < 0.005) discard;
}