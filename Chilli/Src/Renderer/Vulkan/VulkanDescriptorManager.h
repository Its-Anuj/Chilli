#pragma once

#include "SparseSet.h"
#include "Pipeline.h"
#include "Buffers.h"
#include "GraphicsBackend.h"

namespace Chilli
{
	using GetVkBufferFn = std::function<VkBuffer(uint32_t)>;
	using AllocateVkBufferFn = std::function<uint32_t(const BufferCreateInfo&)>;
	using FreeVkBufferFn = std::function<void(uint32_t)>;
	using MapVkBufferFn = std::function<void(uint32_t, void*, uint32_t, uint32_t)>;

	struct VulkanBindlessSetManagerCreateInfo
	{
		VulkanDevice* Device;
		int MaxFrameInFlight;

		GetVkBufferFn GetBuffer;
		AllocateVkBufferFn AllocateBuffer;
		FreeVkBufferFn FreeBuffer;
		MapVkBufferFn MapBufferData;
	};

	class VulkanBindlessSetManager
	{
	public:
		VulkanBindlessSetManager() {}
		~VulkanBindlessSetManager() {}

		void Init(VulkanBindlessSetManagerCreateInfo& Info);
		void Destroy(VulkanBindlessSetManagerCreateInfo& Info);

		void WriteBindlessTextures(VkDevice device, VkImageView Tex, uint32_t Index);
		void WriteBindlessSamplers(VkDevice device, VkSampler Sampler, uint32_t Index);

		const std::vector<std::array<VkDescriptorSet, int(BindlessSetTypes::COUNT)>>& Sets() {
			return _Sets;
		}

		SparseSet<uint32_t>& GetTextureMap() { return GetMap(BindlessSetTypes::TEX_MAP_USAGE); };
		SparseSet<uint32_t>& GetSamplerMap() { return GetMap(BindlessSetTypes::SAMPLER_MAP_USAGE);; };
		SparseSet<uint32_t>& GetMaterialMap() { return GetMap(BindlessSetTypes::MATERIAl); };
		SparseSet<uint32_t>& GetObjectMap() { return GetMap(BindlessSetTypes::PER_OBJECT); };

		SparseSet<uint32_t>& GetMap(BindlessSetTypes Type);

		inline std::array<uint32_t, int(BindlessSetTypes::COUNT_NON_USER)>& SetBuffers() {
			return _SetBuffers;
		}

		void UpdateGlobalShaderData(const GlobalShaderData& Data);
		void UpdateSceneShaderData(const SceneShaderData& Data);

		inline std::array<VkDescriptorSetLayout, int(BindlessSetTypes::COUNT_NON_USER)>& SetLayouts() {
			return _SetLayouts;
		}

		void UpdateMaterialShaderData(const Material& Mat, int MaterialID);
		void UpdateObjectShaderData(const glm::mat4& Transformation, int ObjectID);

	private:
		void _SetupManagerSetLayouts(VulkanBindlessSetManagerCreateInfo& Info);
		void _SetupManagerBuffers(VulkanBindlessSetManagerCreateInfo& Info);
		void _SetupBindlessSetManagerSets(VulkanBindlessSetManagerCreateInfo& Info);
		void _WriteBindlessSetManagerSets(VulkanBindlessSetManagerCreateInfo& Info);

	private:
		VkDescriptorPool _BindlessPool;
		std::vector<std::array<VkDescriptorSet, int(BindlessSetTypes::COUNT)>> _Sets;
		std::array<VkDescriptorSetLayout, int(BindlessSetTypes::COUNT_NON_USER)>_SetLayouts;
		std::array<uint32_t, int(BindlessSetTypes::COUNT_NON_USER)> _SetBuffers;

		size_t _GlobalBufferAlignedSize = 0;
		size_t _SceneBufferAlignedSize = 0;

		// Takes Raw Texture Handle then Gives out the index mapping in the Textures[] shader array
		// Takes Raw Sampler Handle then Gives out index mapping in Samplers[] in shader
		std::array< SparseSet<uint32_t>, int(BindlessSetTypes::COUNT_NON_USER)> _Maps;

		VulkanBindlessSetManagerCreateInfo _CreateInfo;
	};

	class VulkanMaterialDescriptorManager
	{
	public:
		VulkanMaterialDescriptorManager() {}
		~VulkanMaterialDescriptorManager() {}

	private:
		VkDescriptorPool _Pool;
		std::vector<std::array<VkDescriptorSet, int(BindlessSetTypes::COUNT_NON_USER)>> _Sets;

	};
}
