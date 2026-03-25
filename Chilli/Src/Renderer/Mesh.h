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
		SPHERE,
		CYLINDER,
		TORUS,
		CONE
	};

	struct MeshCreateInfo
	{
		void* Vertices = nullptr;
		uint32_t VertCount = 0;
		void* Indicies = nullptr;
		uint32_t IndexCount = 0;
		uint32_t InstanceCount = 0;

		BufferState IndexBufferState;

		IndexBufferType IndexType;
		VertexInputShaderLayout MeshLayout;
	};

	struct Mesh
	{
		std::array<BackBone::AssetHandle<Buffer>, 16> VertexBufferHandles;
		uint32_t ActiveVBHandlesCount = 0;

		BackBone::AssetHandle<Buffer> IBHandle;

		uint32_t VertexCount = 0;
		uint32_t InstanceCount = 0;
		uint32_t IndexCount = 0;

		IndexBufferType IBType = IndexBufferType::NONE;
		VertexInputShaderLayout MeshLayout;
		BufferState IndexBufferState;
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
		Chilli::Vec3 Color{ 0.0f };
	};

}
