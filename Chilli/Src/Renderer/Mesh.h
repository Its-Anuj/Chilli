#pragma once

#include "UUID/UUID.h"
#include "Buffer.h"

namespace Chilli
{
	struct Mesh
	{
		std::vector<std::shared_ptr<VertexBuffer>> VertexBuffers;
		std::shared_ptr<IndexBuffer> IndexBuffer;
		UUID ID;
	};

	class MeshManager
	{
	public:
		MeshManager() {}
		~MeshManager(){}

		const UUID& AddMesh(const VertexBufferSpec& VBSpec, const IndexBufferSpec& IBSpec);
		const Mesh& GetMesh(UUID ID);
		bool DoesMeshExist(UUID ID);

		void Flush();
		void DestroyMesh(const UUID& ID);

		size_t Count() const { return _Meshes.size(); }

	private:
		std::unordered_map<UUID, Mesh> _Meshes;
	};
}
