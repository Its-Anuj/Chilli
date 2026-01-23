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

void main() {
    // 1. Position Math:
    // Scale the unit quad by glyph size and offset by character position.
    // instPos already includes the curY + (bc.yoff * s) from your C++ code.
    vec2 pixelPos = (inVertexPos * instSize) + instPos;
    
    // 2. Project to NDC:
    // This uses your manual matrix to map 0..Width to -1..1
    gl_Position = pc.projection * vec4(pixelPos, 0.0, 1.0);
    
    // 3. UV Math:
    // stb_truetype provides s0, t0 (UVOffset) and s1, t1.
    // We lerp between them using our 0..1 vertex positions.
    fragTexCoord = instUVOffset + (inVertexPos * instUVRange);
    
    FontIndex = instFontIndex;
}