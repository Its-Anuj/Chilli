#version 450

#include "Common.glsl"

// Binding 0: Unit Quad (0.0 to 1.0)
layout(location = 0) in vec3 InVertexPos;

const uint OVERLAY = 0;
const uint WORLDSPACE = 1;
const uint XRAY = 2;
const uint GiIZMO = 3;

layout(push_constant) uniform Push {
    uint ModeFlag;
	vec4 Color;
} pc;

void main()
{
	if(pc.ModeFlag == WORLDSPACE)
	{
		gl_Position = SceneUBO.Scenes[0].ViewProjMatrix * vec4(InVertexPos, 1.0); 
	}
	else{
		gl_Position = vec4(InVertexPos, 1.0); 
	}
}
