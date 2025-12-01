#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_descriptor_indexing : enable

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    vec4 ResolutionTime; // x and y for resolution of rendering framebuffer ,z for time, w is nothing
} GlobalUBO;

layout(set = 0, binding = 1) uniform SceneUniformBufferObject {
    mat4 ViewProjMatrix;
    vec4 CameraPos; // x and y for resolution of rendering framebuffer ,z for time, w is nothing
} SceneUBO;

const int MAX_SAMPLER_COUNT = 16;

layout(set = 1, binding = 0) uniform sampler Sampler[MAX_SAMPLER_COUNT];
layout(set = 1, binding = 1) uniform texture2D Textures[];

struct Material
{
    // x for AlbedoTexture, y for AlbedoSampler
    ivec4 Indicies;
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

layout(push_constant) uniform PushConstants {
    int ObjectIndex;
    int MaterialIndex;
} DrawPushData;
