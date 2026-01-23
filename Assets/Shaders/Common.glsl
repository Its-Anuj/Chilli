#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_descriptor_indexing : enable

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    vec4 ResolutionTime; // x and y for resolution of rendering framebuffer ,z for time, w is nothing
} GlobalUBO;

#define MAX_SCENE_DATA_COUNT 16

struct Scene
{
    // --- Camera Data ---
    mat4 ViewProjMatrix;
    vec4 CameraPos; 
    
    // --- Environment / Properties ---
    vec4 AmbientColor;    // Global light color/intensity
    vec4 FogProperties;   // x: density, y: start, z: end
    vec4 ScreenData;      // x: gamma, y: exposure, etc.
    
    // --- Light Data (Example) ---
    vec4 GlobalLightDir;  // Directional light for the whole scene
};

layout(set = 0, binding = 1) uniform SceneUniformBufferObject {
    Scene Scenes[MAX_SCENE_DATA_COUNT];
} SceneUBO;

const int MAX_SAMPLER_COUNT = 16;

layout(set = 1, binding = 0) uniform sampler Samplers[MAX_SAMPLER_COUNT];
layout(set = 1, binding = 1) uniform texture2D Textures[];

struct Material
{
    // x for AlbedoTexture, y for AlbedoSampler
    int AlbedoTextureIndex;
    int AlbedoSamplerIndex;
    int Padding[2];
    vec4 AlbedoColor;
};

layout(set = 2, binding = 0) buffer MaterialStorageBufferObject{
    Material[] Materials;
};

struct Object
{
    mat4 TransformationMat;
	mat4 InverseTransformationMat;
};

layout(set = 3, binding = 0) buffer ObjectStorageBufferObject{
    Object[] Objects;
};
