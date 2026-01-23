#pragma once

#include "Buffers.h"
#include "Maths.h"
#include "Pipeline.h"

namespace Chilli
{
	enum class BasicShapes
	{
		TRIANGLE,
		QUAD,
		CUBE,
		SPHERE
	};

	struct MeshCreateInfo
	{
		void* Vertices = nullptr;
		uint32_t VertCount = 0;
        size_t VerticesSize = 0;
		void* Indicies = nullptr;
		uint32_t IndexCount = 0;
        size_t IndiciesSize = 0;

		IndexBufferType IndexType;
		BufferState State;
	};

	struct Mesh
	{
		BackBone::AssetHandle<Buffer> VBHandle, IBHandle;
		uint32_t VertexCount = 0;
		uint32_t IndexCount = 0;
		IndexBufferType IBType = IndexBufferType::NONE;
	};

	struct Vertex2D
	{
		Chilli::Vec3 Position;
		Chilli::Vec2 UV;
	};

	struct Vertex
	{
		Chilli::Vec3 Position;
		Chilli::Vec3 Normal;
		Chilli::Vec2 UV;
	};

}
