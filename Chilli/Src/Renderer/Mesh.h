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

	struct MeshCreateInfo
	{
		std::vector<BufferCreateInfo> VBs;
		BufferCreateInfo IndexBufferSpec;
		std::vector< MeshShaderLayout> Layout;
	};

	struct Mesh
	{
		uint32_t VBHandle;
		uint32_t VertexCount = 0;
		std::vector< MeshShaderLayout> Layout;
	};
}
