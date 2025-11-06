#include "Ch_PCH.h"

#include "Mesh.h"

namespace Chilli
{
#pragma region Mesh_Manager
	const UUID& MeshManager::AddMesh(const VertexBufferSpec& VBSpec, const IndexBufferSpec& IBSpec)
	{
		UUID NewID;

		Mesh NewMesh;
		NewMesh.ID = NewID;
		NewMesh.IndexBuffer = IndexBuffer::Create(IBSpec);
		NewMesh.VertexBuffers.push_back(VertexBuffer::Create(VBSpec));

		_Meshes[NewID] = NewMesh;
		return NewID;
	}

	const UUID& MeshManager::AddMesh(const Mesh& Mesh)
	{
		if (!Mesh.IsValid())
			CH_CORE_ERROR("MeshManager: Attempted to add invalid mesh");
		const auto& id = Mesh.ID;
		if (DoesMeshExist(id))
			DestroyMesh(id);

		_Meshes[id] = Mesh;
		return id;
	}

	bool MeshManager::DoesMeshExist(const UUID& ID) const
	{
		return _Meshes.find(ID) != _Meshes.end();
	}

	Mesh& MeshManager::GetMesh(const UUID& ID)
	{
		auto it = _Meshes.find(ID);
		if (it != _Meshes.end())
			return it->second;
		CH_CORE_ERROR("Mesh Manager: ID: {0} not Found",(uint64_t)ID);
	}

	const Mesh& MeshManager::GetMesh(const UUID& ID) const
	{
		auto it = _Meshes.find(ID);
		if (it != _Meshes.end())
			return it->second;
		CH_CORE_ERROR("Mesh Manager: ID: {0} not Found", (uint64_t)ID);
	}

	void MeshManager::Flush()
	{
		for (auto& It : _Meshes)
		{
			It.second.IndexBuffer->Destroy();
			for (auto& VB : It.second.VertexBuffers)
				VB->Destroy();
		}
		_Meshes.clear();
	}

	size_t MeshManager::GetTotalVertexCount() const
	{
		size_t Count = 0;
		for (auto& It : _Meshes)
			Count += It.second.VertexCount;
		return Count;
	}

	size_t MeshManager::GetTotalIndexCount() const
	{
		size_t Count = 0;
		for (auto& It : _Meshes)
			Count += It.second.IndexCount;
		return Count;;
	}

	void MeshManager::DestroyMesh(const UUID& ID)
	{
		auto it = _Meshes.find(ID);
		if (it != _Meshes.end())
		{
			if (it->second.IndexBuffer)
			{
				it->second.IndexBuffer->Destroy();
				for (auto& VB : it->second.VertexBuffers)
					VB->Destroy();
			}
			_Meshes.erase(it);
		}
	}
#pragma endregion 

}
