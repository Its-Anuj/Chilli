#version 460
#include "Common.glsl"

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    int ObjectIndex;
    int MaterialIndex;
} DrawPushData;

void main()
{
    // --- MATERIAL DATA ---
    Material ActiveMaterial = Materials[DrawPushData.MaterialIndex];
    int TexIdx = ActiveMaterial.AlbedoTextureIndex;
    int SampIdx = ActiveMaterial.AlbedoSamplerIndex;

    // Sample texture and multiply by material color
    vec4 Albedo = texture(sampler2D(Textures[TexIdx], Samplers[SampIdx]), fragUV) * ActiveMaterial.AlbedoColor;

    // --- TEMPORARY LIGHTING OVERRIDES ---
    // Hardcoded light direction (coming from top-right-front)
    vec3 L = normalize(vec3(0.5, 1.0, 0.5)); 
    
    // Hardcoded colors to ensure visibility
    vec3 FakeAmbient = vec3(0.2, 0.2, 0.2); // Constant low light everywhere
    vec3 FakeLightColor = vec3(1.0, 1.0, 0.9); // Warm white sun light

    // --- CALCULATIONS ---
    vec3 N = normalize(fragNormal);
    
    // Standard Lambertian Diffuse
    float DiffuseIntensity = max(dot(N, L), 0.0);
    vec3 DiffusePart = DiffuseIntensity * FakeLightColor;

    // Combine
    vec3 FinalRGB = (FakeAmbient + DiffusePart) * Albedo.rgb;
    
    // Output final color with texture alpha
    outColor = vec4(FinalRGB, Albedo.a);
}