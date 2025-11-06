#pragma once

#include "Buffer.h"
#include "Shader.h"
#include "Texture.h"
#include "Object.h"
#include "Maths.h"

namespace Chilli
{
	struct Material
	{
		UUID ID;
		UUID UsingPipelineID;

		UUID AlbedoTexture;
		UUID AlbedoSampler;
		Vec4 AlbedoColor;
	};

	struct MaterialShaderData
	{
		Vec4 AlbedoColor;
		// x for AlbedoTexture Index, y for AlbedoSampler Index
		IVec4 Indicies;
	};

	class MaterialManager
	{
	public:
		MaterialManager() {}
		~MaterialManager() {}

		const UUID& Add(const Material& Mat);
		Material& Get(UUID ID);
		bool Exist(UUID ID) const;

		void Flush();
		void Destroy(const UUID& ID);

		size_t Count() const { return _Materials.size(); }

	private:
		std::unordered_map<UUID, Material> _Materials;
	};

	class BindlessSetTextureManager
	{
	public:
		BindlessSetTextureManager() {}
		~BindlessSetTextureManager() {}

		uint32_t Add(const UUID& ID);
		void Update(const UUID& ID, uint32_t Index);
		uint32_t GetIndex(const UUID& ID);
		void Remove(const UUID& ID);
		uint32_t GetCount() const { return _Textures.size(); }

		bool IsTexPresent(const UUID& ID) const;
	private:
		std::unordered_map<UUID, uint32_t> _Textures;
	};

	class BindlessSetSamplerManager
	{
	public:
		BindlessSetSamplerManager() {}
		~BindlessSetSamplerManager() {}

		uint32_t Add(const UUID& ID);
		uint32_t GetIndex(const UUID& ID);
		void Remove(const UUID& ID);
		uint32_t GetCount() const { return _Samplers.size(); }

		bool IsPresent(const UUID& ID) const;
	private:
		std::unordered_map<UUID, uint32_t> _Samplers;
	};

	class BindlessSetMaterialManager
	{
	public:
		BindlessSetMaterialManager() {}
		~BindlessSetMaterialManager() {}

		uint32_t Add(const UUID& ID);
		void Add(const UUID& ID, uint32_t Index);
		uint32_t GetIndex(const UUID& ID);
		void Remove(const UUID& ID);
		uint32_t GetCount() const { return _Materials.size(); }

		bool IsPresent(const UUID& ID) const;
	private:
		std::unordered_map<UUID, uint32_t> _Materials;
	};

	class BindlessSetObjectManager
	{
	public:
		BindlessSetObjectManager() {}
		~BindlessSetObjectManager() {}

		uint32_t Add(const UUID& ID);
		void Add(const UUID& ID, uint32_t Index);
		uint32_t GetIndex(const UUID& ID);
		void Remove(const UUID& ID);
		uint32_t GetCount() const { return _Objects.size(); }

		bool IsPresent(const UUID& ID) const;
	private:
		std::unordered_map<UUID, uint32_t> _Objects;
	};

	class BindlessSetManager
	{
	public:
		virtual void UpdateGlobalUBO(void* Data) = 0;
		virtual void SetGlobalUBO(size_t GlobalUBOSize) = 0;

		virtual void UpdateSceneUBO(void* Data) = 0;
		virtual void SetSceneUBO(size_t SceneUBOSize) = 0;

		virtual void CreateMaterialSBO(size_t SizeInBytes, uint32_t MaxIndex) = 0;
		virtual void CreateObjectSBO(size_t SizeInBytes, uint32_t MaxIndex) = 0;

		virtual uint32_t AddTexture(const std::shared_ptr<Texture>& Tex) = 0;
		virtual uint32_t GetTextureIndex(const std::shared_ptr<Texture>& Tex) = 0;
		virtual void RemoveTexture(const std::shared_ptr<Texture>& Tex) = 0;
		virtual uint32_t GetTextureCount() const = 0;

		virtual bool IsTexPresent(const std::shared_ptr<Texture>& Tex) const = 0;

		virtual void AddSampler(const std::shared_ptr<Sampler>& Sam) = 0;

		virtual void UpdateMaterial(const Material& Mat, uint32_t Index) = 0;
		virtual void UpdateObject(Object& Obj, uint32_t Index) = 0;

	private:
	};
}

