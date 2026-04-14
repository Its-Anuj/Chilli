#version 450

// Binding 0: FlameVertex (Pos[2], TexCoords[2])
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTexCoords;
layout(location = 2) in uint inFontIndex;
layout(location = 3) in uint inMaterailIndex;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out uint OutFontIndex;

layout(push_constant) uniform Push {
    mat4 projection;
    vec4 textColor;
    float pixelRange; // Pass MSDFData::PixelRange here!
} pc;

void main()
{
    // Positions are already in pixel space from the C++ builder
    gl_Position = pc.projection * vec4(inPos, 0.0, 1.0);
    
    fragTexCoord = inTexCoords;
    OutFontIndex = inFontIndex;
}