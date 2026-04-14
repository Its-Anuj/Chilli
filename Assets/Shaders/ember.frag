#version 450

#include "Common.glsl"

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in uint FontIndex;

layout(location = 0) out vec4 outColor;

#define MAX_UNIQUE_FONT_COUNT 32

layout(set = 4, binding = 0) uniform texture2D fontAtlas[MAX_UNIQUE_FONT_COUNT];
layout(set = 4, binding = 1) uniform sampler fontAtlasSampler;

struct FlameMaterial
{
    vec4 Color;
};

layout(push_constant) uniform Push {
    mat4 projection;
    vec4 textColor;
    float pixelRange; 
} pc;

// MSDF core math: find the median of RGB to get the signed distance
float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    // 1. Sample the MSDF texture (Must be UNORM, not SRGB!)
    // Assuming you are currently using FontIndex 0 or passing it via another means
    // If you have multiple fonts, you'll need to pass FontIndex via a vertex attribute
    vec3 msd = texture(sampler2D(fontAtlas[FontIndex], fontAtlasSampler), fragTexCoord).rgb;
    
    // 2. Extract the signed distance
    float sd = median(msd.r, msd.g, msd.b);
    
    // 3. Convert distance to screen pixels for perfectly sharp anti-aliasing
    // fwidth calculates the derivative (how much the UV changes per screen pixel)
    float screenPxDistance = pc.pixelRange * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

    // 4. Discard invisible fragments to save on blending cost
    if (opacity < 0.01) discard;

    outColor = vec4(pc.textColor.rgb, pc.textColor.a * opacity);
}