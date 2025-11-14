#pragma once

#include "Buffers.h"
#include "Pipeline.h"

namespace Chilli
{
	struct MeshShaderLayout
	{
		ShaderObjectTypes Type;
		const char* Name;
	};

	struct Mesh
	{
		size_t VBHandle;
		std::vector< MeshShaderLayout> Layout;
	};
}
