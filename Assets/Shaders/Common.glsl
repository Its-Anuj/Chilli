#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    vec4 ResolutionTime; // x and y for resolution of rendering framebuffer ,z for time, w is nothing
} GlobalUBO;

layout(set = 1, binding = 0) uniform SceneUniformBufferObject{
    mat4 ViewProjMatrix;
    vec4 CameraPos;
} SceneData;

layout(set = 2, binding = 0) uniform sampler TexturesSamplers[16]; // fixed small number, like 4 or 8
layout(set = 2, binding = 1) uniform texture2D Textures[];

struct Material
{
    vec4 AlbedoColor; // rgba
    ivec4 Indicies; // x for AlbedoTexture, y for AlbedoSampler, zw for future 
};

struct Object
{
    mat4 TransformationMat;
};

layout(set = 3, binding = 0) buffer MaterialBuffer {
    Material MaterialsData[];
};

layout(set = 4, binding = 0) buffer ObjectBuffer{
    Object ObjectData[];
};

// Push constant for per-draw sampler selection
layout(push_constant) uniform PushConstants {
    int ObjectIndex;
    int MaterialIndex;
} DrawPushData;

// Basic Functions
// In Common.glsl
vec4 SampleMaterialAlbedo(Material mat, vec2 texCoord) {
    return texture(sampler2D(Textures[mat.Indicies.x], TexturesSamplers[mat.Indicies.y]), texCoord);
}
