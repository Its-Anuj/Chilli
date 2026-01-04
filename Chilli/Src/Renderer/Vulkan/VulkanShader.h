#pragma once

#include "SparseSet.h"
#include "Pipeline.h"
#include <GraphicsBackend.h>

void LoadShaderObjectEXTFunctions(VkDevice Device);

namespace Chilli
{
	// stageFlag = VK_SHADER_STAGE_VERTEX_BIT / FRAGMENT_BIT etc.
	Chilli::ReflectedShaderInfo ReflectShaderModule(VkDevice device, const std::vector<char>& code, VkShaderStageFlagBits stageFlag);
	class VulkanBindlessRenderingManager;

	struct VulkanShaderModule
	{
		ReflectedShaderInfo ReflectedInfo;
		const char* FilePath;
		std::vector<char> SpirvCode;
		ShaderStageType Stage;
	};

	struct VulkanShaderProgram
	{
		std::vector<uint32_t> ModuleHandles;
		ReflectedShaderInfo CombinedInfo;
		uint32_t ActiveMask = 0;

		VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
		std::vector< VkShaderEXT> ObjectHandles;
		std::array<VkDescriptorSetLayout, int(BindlessSetTypes::COUNT_USER)> SetLayouts;
	};

	using VulkanBindlessSetLayoutType = std::array<VkDescriptorSetLayout, int(BindlessSetTypes::COUNT_NON_USER)>;
	// Mangages Everything related to shaders
	class VulkanShaderDataManager
	{
	public:
		VulkanShaderDataManager() {
		}
		~VulkanShaderDataManager();

		uint32_t CreateShaderModule(VkDevice Device, const char* FilePath, ShaderStageType Type);
		void DestroyShaderModule(VkDevice Device, uint32_t Handle);

		uint32_t MakeShaderProgram();
		void AttachShader(uint32_t ProgramHandle, const ShaderModule& Shader);
		void LinkShaderProgram(VkDevice Device, uint32_t ProgramHandle,
			const VulkanBindlessSetLayoutType& BindlessSetLayouts);
		void ClearShaderProgram(VkDevice Device, uint32_t ProgramHandle);
		void BindShaderProgram(VkCommandBuffer CmdBuffer, uint32_t ProgramHandle);

		void PushConstants(VkCommandBuffer CmdBuffer, uint32_t ProgramHandle, uint32_t Stage, void* Data, size_t Size, size_t Offset);

		void BindBindlessSets(VkCommandBuffer CmdBuffer, uint32_t ProgramHandle, uint32_t FrameIndex,
			const VulkanBindlessRenderingManager& BindlessManager);

		void BindUserSets(VkCommandBuffer CmdBuffer, uint32_t ProgramHandle,
			const std::array<VkDescriptorSet, int(BindlessSetTypes::COUNT_USER)>& Sets);

		struct VulkanShaderLocationData
		{
			ReflectedBindingUniformInput BindingInfo;
			bool ReturnState;
		};
		// For User 
		VulkanShaderLocationData FindUserShaderDataLocation(uint32_t ProgramID, const char* Name);

		const ReflectedShaderInfo& GetProgramInfo(uint32_t ProgramID) { 
			return _ShaderPrograms.Get(ProgramID)->CombinedInfo; 
		}

		const std::array<VkDescriptorSetLayout, int(BindlessSetTypes::COUNT_USER)>& GetProgramSetLayouts(uint32_t ID) {
			return _ShaderPrograms.Get(ID)->SetLayouts;
		}

	private:
		SparseSet<VulkanShaderModule> _ShaderModules;
		SparseSet<VulkanShaderProgram> _ShaderPrograms;
	};

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

	struct VulkanDescriptorPoolSize
	{
		ShaderUniformTypes Type;
		uint32_t Count = 0;
	};

	class VulkanBindlessRenderingManager
	{
	public:
		VulkanBindlessRenderingManager() {}
		~VulkanBindlessRenderingManager() {}

		void Init(const VulkanBindlessSetManagerCreateInfo& Info);
		void Destroy(const VulkanBindlessSetManagerCreateInfo& Info);

		const VulkanBindlessSetLayoutType& GetBindlessSetLayouts() { return _BindlessSetLayouts; }
		const std::vector<std::array<VkDescriptorSet, int(BindlessSetTypes::COUNT_NON_USER)>>& GetBindlessSets() const {
			return _BindlessSets;
		}

		void UpdateGlobalShaderData(const GlobalShaderData& Data);
		void UpdateSceneShaderData(const SceneShaderData& Data);

		void PrepareForMaterial(uint32_t MaterialIndex);

		void PrepareForTexture(uint32_t TexIndex);
		uint32_t UpdateTexture(VkDevice device, uint32_t HandleId, VkImageView Txt);

		void PrepareForSampler(uint32_t SamplerIndex);
		uint32_t UpdateSampler(VkDevice device, uint32_t HandleId, VkSampler Sampler);
		
		void UpdateMaterialShaderData(uint32_t MaterialHandle, const MaterialShaderData& Data);
		void UpdateObjectShaderData(const ObjectShaderData& Data);

		uint32_t GetMaterialShaderIndex(uint32_t RawMaterialHandle) { return _MaterialMap.Get(RawMaterialHandle)->ShaderIndex; }

	private:
		void _SetupBindlessSetLayouts(VkDevice Device);
		void _SetupBindlessSetManagerSets(const VulkanBindlessSetManagerCreateInfo& Info);
		void _SetupManagerBuffers(const VulkanBindlessSetManagerCreateInfo& Info);
		void _WriteBindlessSetManagerSets(const VulkanBindlessSetManagerCreateInfo& Info);
		;
		void _WriteShaderTexture(VkDevice device, uint32_t Index, VkImageView Texture);
		void _WriteShaderSampler(VkDevice device, uint32_t Index, VkSampler Sampler);
	private:
		VulkanBindlessSetLayoutType _BindlessSetLayouts = { VK_NULL_HANDLE };
		std::vector< VkDescriptorPool> _BindlessDescPools;
		VkDescriptorPool _BindlessTexPool;
		std::vector<std::array<VkDescriptorSet, int(BindlessSetTypes::COUNT_NON_USER)>> _BindlessSets;

		struct BindlessTextureMetaData
		{
			uint32_t TexIndex;
			uint32_t ShaderIndex;
		};

		struct BindlessSamplerMetaData
		{
			uint32_t SamplerIndex;
			uint32_t ShaderIndex;
		};

		struct BindlessMaterialMetaData
		{
			uint32_t MaterialIndex;
			uint32_t ShaderIndex;
		};

		SparseSet<BindlessTextureMetaData> _TextureMap;
		SparseSet<BindlessSamplerMetaData> _SamplerMap;
		SparseSet<BindlessMaterialMetaData> _MaterialMap;

		std::array<uint32_t, int(BindlessSetTypes::COUNT_NON_USER)> _SetBuffers;

		size_t _GlobalBufferAlignedSize = 0;
		size_t _SceneBufferAlignedSize = 0;

		VulkanBindlessSetManagerCreateInfo _CreateInfo;
		uint32_t _ActiveTextureCounter = 0;
		uint32_t _ActiveSamplerCounter = 0;
		uint32_t _ActiveMaterialCounter = 0;
		uint32_t _ActiveObjectCounter = 0;
	};
}




