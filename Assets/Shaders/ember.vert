#version 450

// Binding 0: Unit Quad (0.0 to 1.0)
layout(location = 0) in vec2 inVertexPos;

// Binding 1: Instance Data
layout(location = 1) in vec2 instPos;
layout(location = 2) in vec2 instSize;
layout(location = 3) in vec2 instUVOffset;
layout(location = 4) in vec2 instUVRange;
layout(location = 5) in int instFontIndex;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out int FontIndex;

layout(push_constant) uniform Push {
    mat4 projection;
    vec4 textColor;
} pc;

void main()
{
    // --------------------------------------------------
    // Flip quad Y (Y-up -> Y-down)
    // --------------------------------------------------
    vec2 quadPos = inVertexPos;
    quadPos.y = 1.0 - quadPos.y;

    // --------------------------------------------------
    // Pixel space position
    // --------------------------------------------------
    vec2 pixelPos = instPos + quadPos * instSize;

    // --------------------------------------------------
    // Project to NDC
    // --------------------------------------------------
    gl_Position = pc.projection * vec4(pixelPos, 0.0, 1.0);

    // --------------------------------------------------
    // UVs (keep original orientation!)
    // --------------------------------------------------
    fragTexCoord = instUVOffset + inVertexPos * instUVRange;

    FontIndex = instFontIndex;
}
