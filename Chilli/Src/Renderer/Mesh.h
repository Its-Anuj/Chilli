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
		BufferCreateInfo IBSpec;
		IndexBufferType IBType = IndexBufferType::NONE;
		std::vector< MeshShaderLayout> Layout;
	};

	enum class BasicShapes
	{
		TRIANGLE,
		QUAD,
		CUBE,
	};
	
	struct BetterMeshCreateInfo
	{
		void* Vertices = nullptr;
		uint32_t VertCount = 0;
		void* Indicies = nullptr;
		uint32_t IndexCount = 0;

		std::vector< MeshShaderLayout> Layouts;
		IndexBufferType IndexType;
		BufferState State;
	};

	struct Mesh
	{
		uint32_t VBHandle;
		uint32_t IBHandle = static_cast<uint32_t>(-1);
		uint32_t VertexCount = 0;
		uint32_t IndexCount = 0;
		IndexBufferType IBType = IndexBufferType::NONE;
		std::vector< MeshShaderLayout> Layout;
	};
}
