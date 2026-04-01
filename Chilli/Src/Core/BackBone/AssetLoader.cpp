#include "AssetLoader.h"
#include "AssetLoader.h"
#include "AssetLoader.h"
#include "AssetLoader.h"
#include "AssetLoader.h"
#include "AssetLoader.h"
#include "AssetLoader.h"
#include "Ch_PCH.h"
#include "AssetLoader.h"
#include "Profiling\Timer.h"

#include "DeafultExtensions.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "ufbx.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <unordered_map>
#include <functional>

// A temporary struct just for hashing 8 floats together
struct TempVertex {
	float px, py, pz;
	float nx, ny, nz;
	float tx, ty;

	// Tell the map how to check if two vertices are EXACTLY identical
	bool operator==(const TempVertex& other) const {
		return px == other.px && py == other.py && pz == other.pz &&
			nx == other.nx && ny == other.ny && nz == other.nz &&
			tx == other.tx && ty == other.ty;
	}
};

// Tell the map how to generate a unique ID (hash) for these 8 floats
struct VertexHasher {
	size_t operator()(const TempVertex& v) const {
		size_t seed = 0;
		std::hash<float> hasher;

		// The standard "hash combine" magic formula
		auto hash_combine = [&](float val) {
			seed ^= hasher(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			};

		hash_combine(v.px); hash_combine(v.py); hash_combine(v.pz);
		hash_combine(v.nx); hash_combine(v.ny); hash_combine(v.nz);
		hash_combine(v.tx); hash_combine(v.ty);

		return seed;
	}
};

namespace Chilli
{
	uint32_t GetNewAssetLoaderID() {
		static uint32_t NewID = 0;
		return NewID++;
	}

	AssetLoader::AssetLoader(BackBone::SystemContext& Ctxt)
	{
		Ctxt.AssetRegistry->RegisterStore<ImageData>();
		_Ctxt = Ctxt;
	}

	ImageLoader::~ImageLoader()
	{
		for (int i = 0; i < _IndexMaps.size(); i++)
		{
			if (_IndexMaps[i].IsValid())
				CH_CORE_ERROR("All Image Data Must be Unloaded!");
		}
	}

	BackBone::AssetHandle<ImageData> ImageLoader::LoadTyped(BackBone::SystemContext& Ctxt, const std::string& Path)
	{
		auto Command = Chilli::Command(Ctxt);
		auto ImageDataStore = Command.GetStore<ImageData>();

		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));

		int Index = IndexEntry::FindIndex(_IndexMaps, HashValue);
		if (Index != -1)
			return _ImageDataHandles[Index];

		ImageData NewData{};
		int Width, Height, NumChannels;

		NewData.Pixels = stbi_load(Path.c_str(), &Width, &Height, &NumChannels, STBI_rgb_alpha);
		if (NewData.Pixels == nullptr)
			CH_CORE_ERROR("Given Path {} doesnot result any pixel data", Path);
		NewData.Resolution = { Width, Height };
		NewData.NumChannels = NumChannels;
		NewData.FileName = GetFileNameWithExtension(Path);
		NewData.FilePath = Path;
		NewData.Format = ImageFormat::RGBA8;

		auto ImageDataHandle = ImageDataStore->Add(NewData);

		uint32_t NewIndex = static_cast<uint32_t>(_ImageDataHandles.size());
		_ImageDataHandles.push_back(ImageDataHandle);

		IndexEntry::AddOrUpdateSorted(_IndexMaps, { HashValue, NewIndex });

		return ImageDataHandle;
	}

	void ImageLoader::Unload(BackBone::SystemContext& Ctxt, const std::string& Path)
	{
		auto Command = Chilli::Command(Ctxt);
		auto ImageDataStore = Command.GetStore<ImageData>();

		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));

		int Index = IndexEntry::FindIndex(_IndexMaps, HashValue);
		if (Index == -1)
			return;

		auto Handle = ImageDataStore->Get(_ImageDataHandles[Index]);
		if (!Handle)
			return;

		stbi_image_free(Handle->Pixels);

		ImageDataStore->Remove(_ImageDataHandles[Index]);

		// Invalidate index entry
		IndexEntry::Invalidate(_IndexMaps, HashValue);
	}

	void ImageLoader::Unload(BackBone::SystemContext& Ctxt, const BackBone::AssetHandle<ImageData>& Data)
	{
		Unload(Ctxt, Data.ValPtr->FilePath);
	}

	CGLFTMeshLoader::~CGLFTMeshLoader()
	{
		for (int i = 0; i < _IndexMaps.size(); i++)
		{
			if (_IndexMaps[i].IsValid())
				CH_CORE_ERROR("All CGLTF Mesh Data Must be Unloaded!");
		}
	}

	void LoadGLB(const std::string& Path, MeshLoaderData& Collection, BackBone::AssetStore<RawMeshData>* Store) {
		cgltf_options options = {};
		cgltf_data* data = nullptr;
		cgltf_result result = cgltf_parse_file(&options, Path.c_str(), &data);

		if (result != cgltf_result_success) return;

		cgltf_load_buffers(&options, data, Path.c_str());

		Collection.Path = Path;

		for (size_t m = 0; m < data->meshes_count; ++m) {
			cgltf_mesh& gltfMesh = data->meshes[m];

			for (size_t p = 0; p < gltfMesh.primitives_count; ++p) {
				cgltf_primitive& prim = gltfMesh.primitives[p];

				RawMeshData NewMesh;
				NewMesh.Path = Path;
				NewMesh.Name = gltfMesh.name ? gltfMesh.name : "Mesh_" + std::to_string(m) + "_" + std::to_string(p);
				NewMesh.IBType = IndexBufferType::UINT32_T;

				NewMesh.FileLayout.BeginBinding(0);

				cgltf_accessor* pos_acc = nullptr;
				cgltf_accessor* norm_acc = nullptr;
				cgltf_accessor* uv_acc = nullptr;
				cgltf_accessor* col_acc = nullptr;

				// 1. Identify what we have
				for (size_t a = 0; a < prim.attributes_count; ++a) {
					cgltf_attribute& attr = prim.attributes[a];
					if (attr.type == cgltf_attribute_type_position) pos_acc = attr.data;
					if (attr.type == cgltf_attribute_type_normal)   norm_acc = attr.data;
					if (attr.type == cgltf_attribute_type_texcoord) uv_acc = attr.data;
					if (attr.type == cgltf_attribute_type_color)    col_acc = attr.data;
				}

				// 2. Build Layout (This automatically handles Offsets internally)
				if (pos_acc)  NewMesh.FileLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPos", (uint32_t)MeshAttribute::POSITION);
				if (norm_acc) NewMesh.FileLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InNorm", (uint32_t)MeshAttribute::NORMAL);
				if (uv_acc)   NewMesh.FileLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "InUV", (uint32_t)MeshAttribute::TEXCOORD);
				if (col_acc)  NewMesh.FileLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InCol", (uint32_t)MeshAttribute::COLOR);

				auto& binding = NewMesh.FileLayout.Bindings[0];
				uint32_t stride = binding.Stride;
				size_t vertexCount = pos_acc->count;

				NewMesh.Vertices.resize(vertexCount * stride);
				uint8_t* vPtr = NewMesh.Vertices.data();

				// 3. Map Attributes to their layout offsets
				// We find the index of each attribute in our vector to get the correct offset
				int posIdx = -1, normIdx = -1, uvIdx = -1, colIdx = -1;
				for (int i = 0; i < binding.Attribs.size(); i++) {
					uint32_t loc = binding.Attribs[i].Location;
					if (loc == (uint32_t)MeshAttribute::POSITION) posIdx = i;
					if (loc == (uint32_t)MeshAttribute::NORMAL)   normIdx = i;
					if (loc == (uint32_t)MeshAttribute::TEXCOORD) uvIdx = i;
					if (loc == (uint32_t)MeshAttribute::COLOR)    colIdx = i;
				}

				// 4. Interleave Vertex Data using the offsets we calculated in AddAttribute
				for (size_t i = 0; i < vertexCount; ++i) {
					uint8_t* currentVert = vPtr + (i * stride);

					if (posIdx != -1)
						cgltf_accessor_read_float(pos_acc, i, (float*)(currentVert + binding.Attribs[posIdx].Offset), 3);

					if (normIdx != -1)
						cgltf_accessor_read_float(norm_acc, i, (float*)(currentVert + binding.Attribs[normIdx].Offset), 3);

					if (uvIdx != -1)
						cgltf_accessor_read_float(uv_acc, i, (float*)(currentVert + binding.Attribs[uvIdx].Offset), 2);

					if (colIdx != -1)
						cgltf_accessor_read_float(col_acc, i, (float*)(currentVert + binding.Attribs[colIdx].Offset), 3);
					else if (colIdx == -1 && false) {
						/* If you ever want to force-pad colors when missing, handle here */
					}
				}

				// 5. Pack Indices (Remains the same)
				size_t indexCount = prim.indices->count;
				NewMesh.Indices.resize(indexCount * sizeof(uint32_t));
				uint32_t* iPtr = (uint32_t*)NewMesh.Indices.data();
				for (size_t i = 0; i < indexCount; ++i) {
					iPtr[i] = (uint32_t)cgltf_accessor_read_index(prim.indices, i);
				}

				auto Handle = Store->Add(NewMesh);
				Collection.Meshes.push_back(Handle);
			}
		}
		cgltf_free(data);
	}

	BackBone::AssetHandle<MeshLoaderData> CGLFTMeshLoader::LoadTyped(BackBone::SystemContext& Ctxt, const std::string& Path)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Store = Command.GetStore<MeshLoaderData>();

		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));

		int Index = IndexEntry::FindIndex(_IndexMaps, HashValue);
		if (Index != -1)
			return _MeshDataHandles[Index];

		MeshLoaderData NewData{};
		NewData.Path = Path;
		LoadGLB(Path, NewData, Command.GetStore<RawMeshData>());

		auto Handle = Store->Add(NewData);

		uint32_t NewIndex = static_cast<uint32_t>(_MeshDataHandles.size());
		_MeshDataHandles.push_back(Handle);

		IndexEntry::AddOrUpdateSorted(_IndexMaps, { HashValue, NewIndex });

		return Handle;
	}

	void Chilli::CGLFTMeshLoader::Unload(BackBone::SystemContext& Ctxt, const std::string& Path)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Store = Command.GetStore<MeshLoaderData>();

		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));

		int Index = IndexEntry::FindIndex(_IndexMaps, HashValue);
		if (Index == -1)
			return;

		auto Handle = Store->Get(_MeshDataHandles[Index]);
		if (!Handle)
			return;

		auto RawMeshStore = Command.GetStore<RawMeshData>();
		for (auto& i : Handle->Meshes)
		{
			RawMeshStore->Remove(i);
		}

		Store->Remove(_MeshDataHandles[Index]);

		// Invalidate index entry
		IndexEntry::Invalidate(_IndexMaps, HashValue);
	}

	void Chilli::CGLFTMeshLoader::Unload(BackBone::SystemContext& Ctxt, const BackBone::AssetHandle<MeshLoaderData>& Data)
	{
		Unload(Ctxt, Data.ValPtr->Path);
	}

	void LoadOBJ(const std::string& Path, MeshLoaderData& Collection, BackBone::AssetStore<RawMeshData>* Store) {
		tinyobj::ObjReaderConfig reader_config;
		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(Path, reader_config)) {
			if (!reader.Error().empty()) {
				CH_CORE_ERROR("TinyObjReader error: {}", reader.Error());
			}
			return;
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		Collection.Path = Path;

		// 1. Iterate through each "Shape" (Object) in the OBJ
		for (size_t s = 0; s < shapes.size(); s++) {
			RawMeshData NewMesh;
			NewMesh.Path = Path;
			NewMesh.Name = shapes[s].name.empty() ? "OBJ_Mesh_" + std::to_string(s) : shapes[s].name;
			NewMesh.IBType = IndexBufferType::UINT32_T;

			// 2. Identify available attributes
			bool hasNormals = !attrib.normals.empty();
			bool hasUVs = !attrib.texcoords.empty();
			bool hasColors = !attrib.colors.empty();

			// 3. Setup FileLayout
			NewMesh.FileLayout.BeginBinding(0);
			NewMesh.FileLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPos", (uint32_t)MeshAttribute::POSITION);
			if (hasNormals) NewMesh.FileLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InNorm", (uint32_t)MeshAttribute::NORMAL);
			if (hasUVs)     NewMesh.FileLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "InUV", (uint32_t)MeshAttribute::TEXCOORD);
			if (hasColors)  NewMesh.FileLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InCol", (uint32_t)MeshAttribute::COLOR);

			auto& binding = NewMesh.FileLayout.Bindings[0];
			uint32_t stride = binding.Stride;

			// OBJ indices are unique per attribute (pos, norm, uv). 
			// To interleave them, we must treat every unique combination as a new vertex.
			std::vector<uint8_t> interleavedVertices;
			std::vector<uint32_t> indices;
			std::unordered_map<std::string, uint32_t> uniqueVertices; // Simple string-key map for vertex deduplication

			for (size_t f = 0; f < shapes[s].mesh.indices.size(); f++) {
				tinyobj::index_t idx = shapes[s].mesh.indices[f];

				// Generate a unique key for this vertex combination
				std::string key = std::to_string(idx.vertex_index) + "_" +
					std::to_string(idx.normal_index) + "_" +
					std::to_string(idx.texcoord_index);

				if (uniqueVertices.count(key) == 0) {
					uniqueVertices[key] = static_cast<uint32_t>(interleavedVertices.size() / stride);

					// Pack the vertex data
					size_t start = interleavedVertices.size();
					interleavedVertices.resize(start + stride);
					uint8_t* vPtr = &interleavedVertices[start];
					uint32_t offset = 0;

					// Position
					memcpy(vPtr + offset, &attrib.vertices[3 * idx.vertex_index], 12);
					offset += 12;

					// Normal
					if (hasNormals && idx.normal_index >= 0) {
						memcpy(vPtr + offset, &attrib.normals[3 * idx.normal_index], 12);
						offset += 12;
					}

					// UV
					if (hasUVs && idx.texcoord_index >= 0) {
						memcpy(vPtr + offset, &attrib.texcoords[2 * idx.texcoord_index], 8);
						offset += 8;
					}

					// Color (OBJ often includes colors after vertices)
					if (hasColors) {
						memcpy(vPtr + offset, &attrib.colors[3 * idx.vertex_index], 12);
						offset += 12;
					}
				}
				indices.push_back(uniqueVertices[key]);
			}

			NewMesh.Vertices = std::move(interleavedVertices);

			// Copy indices to NewMesh
			NewMesh.Indices.resize(indices.size() * sizeof(uint32_t));
			memcpy(NewMesh.Indices.data(), indices.data(), NewMesh.Indices.size());

			auto Handle = Store->Add(NewMesh);
			Collection.Meshes.push_back(Handle);
		}
	}

	TinyObjMeshLoader::~TinyObjMeshLoader()
	{
		for (int i = 0; i < _IndexMaps.size(); i++)
		{
			if (_IndexMaps[i].IsValid())
				CH_CORE_ERROR("All CGLTF Mesh Data Must be Unloaded!");
		}
	}

	BackBone::AssetHandle<MeshLoaderData> TinyObjMeshLoader::LoadTyped(BackBone::SystemContext& Ctxt, const std::string& Path)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Store = Command.GetStore<MeshLoaderData>();

		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));

		int Index = IndexEntry::FindIndex(_IndexMaps, HashValue);
		if (Index != -1)
			return _MeshDataHandles[Index];

		MeshLoaderData NewData{};
		NewData.Path = Path;
		LoadOBJ(Path, NewData, Command.GetStore<RawMeshData>());

		auto Handle = Store->Add(NewData);

		uint32_t NewIndex = static_cast<uint32_t>(_MeshDataHandles.size());
		_MeshDataHandles.push_back(Handle);

		IndexEntry::AddOrUpdateSorted(_IndexMaps, { HashValue, NewIndex });

		return Handle;
	}

	void Chilli::TinyObjMeshLoader::Unload(BackBone::SystemContext& Ctxt, const std::string& Path)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Store = Command.GetStore<MeshLoaderData>();

		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));

		int Index = IndexEntry::FindIndex(_IndexMaps, HashValue);
		if (Index == -1)
			return;

		auto Handle = Store->Get(_MeshDataHandles[Index]);
		if (!Handle)
			return;

		auto RawMeshStore = Command.GetStore<RawMeshData>();
		for (auto& i : Handle->Meshes)
		{
			RawMeshStore->Remove(i);
		}

		Store->Remove(_MeshDataHandles[Index]);

		// Invalidate index entry
		IndexEntry::Invalidate(_IndexMaps, HashValue);
	}

	void Chilli::TinyObjMeshLoader::Unload(BackBone::SystemContext& Ctxt, const BackBone::AssetHandle<MeshLoaderData>& Data)
	{
		Unload(Ctxt, Data.ValPtr->Path);
	}

}
