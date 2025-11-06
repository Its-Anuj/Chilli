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
		size_t VertexCount = 0;
		size_t IndexCount = 0;

		bool IsValid() const { return !VertexBuffers.empty() && IndexBuffer; }
	};

	class MeshManager
	{
	public:
		MeshManager() {}
		~MeshManager() {}

		// Construction
		const UUID& AddMesh(const VertexBufferSpec& VBSpec, const IndexBufferSpec& IBSpec);
		const UUID& AddMesh(const Mesh& mesh);

		// Query
		bool DoesMeshExist(const UUID& ID) const;
		Mesh& GetMesh(const UUID& ID);
		const Mesh& GetMesh(const UUID& ID) const;

		// Bulk operations
		void DestroyMesh(const UUID& ID);
		void Flush();
		size_t GetMeshCount() const { return _Meshes.size(); }

		// Iteration support
		auto begin() { return _Meshes.begin(); }
		auto end() { return _Meshes.end(); }
		auto begin() const { return _Meshes.begin(); }
		auto end() const { return _Meshes.end(); }

		// Statistics
		size_t GetTotalVertexCount() const;
		size_t GetTotalIndexCount() const;

		size_t Count() const { return _Meshes.size(); }

	private:
		std::unordered_map<UUID, Mesh> _Meshes;
	};
}
