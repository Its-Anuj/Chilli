#include "ChV_PCH.h"
#include "vulkan\vulkan.h"

#include "vk_mem_alloc.h"
#include "VulkanBackend.h"
#include "VulkanConversions.h"
#include <unordered_set>
#include <algorithm>

#include "spirv_reflect.h"

PFN_vkCreateShadersEXT pfn_vkCreateShadersEXT;
PFN_vkCmdBindShadersEXT pfn_vkCmdBindShadersEXT;
PFN_vkDestroyShaderEXT pfn_vkDestroyShaderEXT;

void LoadShaderObjectEXTFunctions(VkDevice Device)
{
	pfn_vkCreateShadersEXT = (PFN_vkCreateShadersEXT)vkGetDeviceProcAddr(Device, "vkCreateShadersEXT");
	pfn_vkCmdBindShadersEXT = (PFN_vkCmdBindShadersEXT)vkGetDeviceProcAddr(Device, "vkCmdBindShadersEXT");
	pfn_vkDestroyShaderEXT = (PFN_vkDestroyShaderEXT)vkGetDeviceProcAddr(Device, "vkDestroyShaderEXT");
}

namespace Chilli
{
	Chilli::ShaderUniformTypes SpirvTypeToShaderUniformTypes(SpvReflectDescriptorType Type);
	// Helper to map SPIR-V format to VkFormat
	VkFormat SPVFormatToVkFormat(SpvReflectFormat fmt);

	Chilli::ReflectedShaderInfo ReflectShaderModule(VkDevice device, const std::vector<char>& code, VkShaderStageFlagBits stageFlag);

	void CombineUniformReflection(
		std::vector<ReflectedSetUniformInput>& OutCombined,
		const std::vector<ReflectedSetUniformInput>& InfoA,
		const std::vector<ReflectedSetUniformInput>& InfoB,
		ShaderStageType StageA,
		ShaderStageType StageB
	);
	void CombinePushConstantReflection(
		std::vector<ReflectedInlineUniformData>& OutCombined,
		const std::vector<ReflectedInlineUniformData>& InfoA,
		const std::vector<ReflectedInlineUniformData>& InfoB,
		ShaderStageType StageA,
		ShaderStageType StageB);

	inline uint64_t CombineHash(uint64_t a, uint64_t b)
	{
		// Simple mixing
		a ^= b + 0x9e3779b97f4a7c15 + (a << 12) + (a >> 4);
		return a;
	}

	uint64_t HashPipelineLayout(const ReflectedShaderInfo& info)
	{
		uint64_t hash = 0;

		for (auto& set : info.UniformInputs)
		{
			hash = CombineHash(hash, set.Set);
			for (auto& binding : set.Bindings)
			{
				hash = CombineHash(hash, binding.Binding);
				hash = CombineHash(hash, static_cast<uint64_t>(binding.Type));
				hash = CombineHash(hash, binding.Count);
				hash = CombineHash(hash, static_cast<uint64_t>(binding.Stage));
			}
		}

		for (auto& pc : info.PushConstants)
		{
			hash = CombineHash(hash, pc.Offset);
			hash = CombineHash(hash, pc.Size);
			hash = CombineHash(hash, static_cast<uint64_t>(pc.Stage));
		}

		return hash;
	}

	VkPipelineShaderStageCreateInfo _CreateVkShaderStages(VkShaderModule module, VkShaderStageFlagBits type, const char* Name)
	{
		VkPipelineShaderStageCreateInfo stage{};
		stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage.module = module;
		stage.pName = Name;
		stage.stage = type;

		return stage;
	}

	VkShaderModule _CreateVkShaderModule(VkDevice device, std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createinfo.codeSize = code.size();
		createinfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createinfo, nullptr, &shaderModule) != VK_SUCCESS)
			throw std::runtime_error("failed to create shader module!");

		return shaderModule;
	}

#pragma region ShaderManager

	void _SpirvShaderReadFile(const char* FilePath, std::vector<char>& CharData)
	{
		std::ifstream file(FilePath, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			throw std::runtime_error("failed to open file!");

		size_t fileSize = (size_t)file.tellg();
		CharData.resize(fileSize);

		file.seekg(0);
		file.read(CharData.data(), fileSize);
		file.close();
	}

	VulkanShaderDataManager::~VulkanShaderDataManager()
	{
		VULKAN_ASSERT(_ShaderPrograms.GetActiveCount() == 0, "All Shader Program Have not been Cleared!");
	}

	uint32_t VulkanShaderDataManager::CreateShaderModule(VkDevice Device, const char* FilePath, ShaderStageType Type)
	{
		VulkanShaderModule RawModule;
		_SpirvShaderReadFile(FilePath, RawModule.SpirvCode);
		RawModule.FilePath = FilePath;
		RawModule.ReflectedInfo = ReflectShaderModule(Device, RawModule.SpirvCode,
			ShaderStageTypeToVkFlagBit(Type));
		RawModule.Stage = Type;

		return _ShaderModules.Create(RawModule);
	}

	void VulkanShaderDataManager::DestroyShaderModule(VkDevice Device, uint32_t Handle)
	{
		auto RawModule = _ShaderModules.Get(Handle);
		if (RawModule == nullptr)
			VULKAN_ERROR("No Valid Raw VK Module Given!");
		_ShaderModules.Destroy(Handle);
	}

	uint32_t VulkanShaderDataManager::MakeShaderProgram()
	{
		return _ShaderPrograms.Create(VulkanShaderProgram{});
	}

	void VulkanShaderDataManager::AttachShader(uint32_t ProgramHandle, const ShaderModule& Shader)
	{
		auto Program = _ShaderPrograms.Get(ProgramHandle);
		Program->ModuleHandles.push_back(Shader.RawModuleHandle);

		std::vector<ReflectedSetUniformInput> OutUniformInput;
		CombineUniformReflection(OutUniformInput, Program->CombinedInfo.UniformInputs, _ShaderModules.Get(Shader.RawModuleHandle)->ReflectedInfo.UniformInputs, (ShaderStageType)Program->ActiveMask, Shader.Stage);

		if (Shader.Stage == SHADER_STAGE_VERTEX)
			Program->CombinedInfo.VertexInputs = _ShaderModules.Get(Shader.RawModuleHandle)->ReflectedInfo.VertexInputs;

		std::vector<ReflectedInlineUniformData> OutPushConstantRange;
		CombinePushConstantReflection(OutPushConstantRange, Program->CombinedInfo.PushConstants, _ShaderModules.Get(Shader.RawModuleHandle)->ReflectedInfo.PushConstants, (ShaderStageType)Program->ActiveMask, Shader.Stage);

		Program->CombinedInfo.UniformInputs = OutUniformInput;
		Program->CombinedInfo.PushConstants = OutPushConstantRange;
	}

	void VulkanShaderDataManager::LinkShaderProgram(VkDevice Device, uint32_t ProgramHandle, const VulkanBindlessSetLayoutType& BindlessSetLayouts)
	{
		auto Program = _ShaderPrograms.Get(ProgramHandle);
		uint32_t setLayoutCount = BindlessSetLayouts.size();

		std::vector< VkShaderCreateInfoEXT> CreateInfos;

		// Create Set Layouts and fill PushConstants Ranges

		for (auto& BindingInfo : Program->CombinedInfo.UniformInputs)
		{
			if (BindingInfo.Set < int(BindlessSetTypes::USER_0))
				continue;

			std::vector<VkDescriptorSetLayoutBinding> BindingCreateInfos;

			for (auto& PerBinding : BindingInfo.Bindings)
			{
				VkDescriptorSetLayoutBinding VkBinding{};
				VkBinding.binding = PerBinding.Binding;
				VkBinding.descriptorCount = PerBinding.Count;
				VkBinding.descriptorType = ShaderUniformTypeToVk(PerBinding.Type);
				VkBinding.stageFlags = ShaderStageTypeToVk(PerBinding.Stage);
				BindingCreateInfos.push_back(VkBinding);
			}

			VkDescriptorSetLayoutCreateInfo CreateInfo{};
			CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			CreateInfo.bindingCount = BindingCreateInfos.size();
			CreateInfo.pBindings = BindingCreateInfos.data();
			CreateInfo.pNext = nullptr;

			VkDescriptorSetLayout Layout;
			VULKAN_SUCCESS_ASSERT(vkCreateDescriptorSetLayout(Device, &CreateInfo, nullptr, &Layout), "Failed to create vkCreateDescriptorSetLayout");
			Program->SetLayouts[int(BindingInfo.Set - int(BindlessSetTypes::USER_0))] = Layout;
			setLayoutCount++;
		}

		std::vector<VkDescriptorSetLayout> Layouts;
		for (auto& SetLayout : BindlessSetLayouts)
			Layouts.push_back(SetLayout);
		for (int i = int(BindlessSetTypes::USER_0); i < int(BindlessSetTypes::COUNT); i++)
			if (Program->SetLayouts[int(i - int(BindlessSetTypes::USER_0))] != VK_NULL_HANDLE)
				Layouts.push_back(Program->SetLayouts[int(i - int(BindlessSetTypes::USER_0))]);

		std::vector<VkPushConstantRange> VkPushConstants;
		for (auto& PushConstant : Program->CombinedInfo.PushConstants)
		{
			VkPushConstantRange Range{};
			Range.offset = PushConstant.Offset;
			Range.size = PushConstant.Size;
			Range.stageFlags = ShaderStageTypeToVk(PushConstant.Stage);
			VkPushConstants.push_back(Range);
		}

		{
			// Create Pipeline Layouts
			VkPipelineLayoutCreateInfo LayoutCreateInfo{};
			LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			LayoutCreateInfo.setLayoutCount = setLayoutCount; // Or whatever descriptor sets you use
			LayoutCreateInfo.pSetLayouts = Layouts.data();
			LayoutCreateInfo.pushConstantRangeCount = VkPushConstants.size();
			LayoutCreateInfo.pPushConstantRanges = VkPushConstants.data();
			vkCreatePipelineLayout(Device, &LayoutCreateInfo, nullptr, &Program->PipelineLayout);
		}

		for (int i = 0; i < Program->ModuleHandles.size(); i++)
		{
			auto VkShaderModuleData = _ShaderModules.Get(Program->ModuleHandles[i]);
			// 2. Create the VkShaderEXT (Driver object)
			VkShaderCreateInfoEXT createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;

			createInfo.flags = 0; // No special flags needed usually
			createInfo.stage = ShaderStageTypeToVkFlagBit(VkShaderModuleData->Stage);
			// nextStage: Tessellation Evaluation

			if (i + 1 < Program->ModuleHandles.size())
				createInfo.nextStage = ShaderStageTypeToVkFlagBit(
					_ShaderModules.Get(Program->ModuleHandles[i + 1])->Stage);
			else
				createInfo.nextStage = 0;
			// Code Source
			createInfo.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
			createInfo.codeSize = VkShaderModuleData->SpirvCode.size();
			createInfo.pCode = VkShaderModuleData->SpirvCode.data();

			// Entry Point
			createInfo.pName = "main";
			createInfo.pSpecializationInfo = nullptr;

			// Resource Layout (MUST BE IDENTICAL TO ALL OTHER STAGES)
			createInfo.setLayoutCount = setLayoutCount;
			createInfo.pSetLayouts = Layouts.data();

			createInfo.pushConstantRangeCount = VkPushConstants.size();
			createInfo.pPushConstantRanges = VkPushConstants.data();
			CreateInfos.push_back(createInfo);
		}

		VkResult result;
		std::vector<VkShaderEXT> ShaderObjects;
		ShaderObjects.resize(Program->ModuleHandles.size());

		result = pfn_vkCreateShadersEXT(Device, CreateInfos.size(), CreateInfos.data(), NULL, ShaderObjects.data());
		if (result != VK_SUCCESS)
		{
			VULKAN_ERROR("Shader Creation Error!");
		}
		Program->ObjectHandles = ShaderObjects;
	}

	void VulkanShaderDataManager::ClearShaderProgram(VkDevice Device, uint32_t Program)
	{
		auto VkProgram = _ShaderPrograms.Get(Program);
		VkProgram->CombinedInfo.PushConstants.clear();
		VkProgram->CombinedInfo.UniformInputs.clear();
		VkProgram->CombinedInfo.VertexInputs.clear();
		VkProgram->ActiveMask = 0;

		for (auto& Layout : VkProgram->SetLayouts)
		{
			vkDestroyDescriptorSetLayout(Device, Layout, nullptr);
			Layout = VK_NULL_HANDLE;
		}

		for (auto& Object : VkProgram->ObjectHandles)
			pfn_vkDestroyShaderEXT(Device, Object, nullptr);

		if (VkProgram->PipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(Device, VkProgram->PipelineLayout, nullptr);

		_ShaderPrograms.Destroy(Program);
	}

	void VulkanShaderDataManager::BindShaderProgram(VkCommandBuffer CmdBuffer, uint32_t ProgramHandle)
	{
		auto Program = _ShaderPrograms.Get(ProgramHandle);

		const VkShaderStageFlagBits stages[2] =
		{
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT
		};

		VkShaderEXT shaders[2] = { Program->ObjectHandles[0], Program->ObjectHandles[1] };

		pfn_vkCmdBindShadersEXT(CmdBuffer, 2, stages, shaders);
	}

	void VulkanShaderDataManager::PushConstants(VkCommandBuffer CmdBuffer, uint32_t ProgramHandle, uint32_t Stage,
		void* Data, size_t Size, size_t Offset)
	{// 1. Get the program handle (required for PipelineLayout reference)
		auto Program = _ShaderPrograms.Get(ProgramHandle);

		// 2. Convert engine stage type to Vulkan flags
		VkShaderStageFlags requestedStage = ShaderStageTypeToVk(Stage);

		// --- SIMPLIFIED VALIDATION AGAINST FIXED MAX SIZE ---

		// Check 1: Does the requested area go past the absolute maximum size?
		if (Offset + Size > MAX_INLINE_UNIFORM_DATA_SIZE)
		{
			// Log error: Push constant command goes past the defined MAX_PUSH_CONSTANT_SIZE.
			// This indicates an error in the application logic.
			return;
		}
		// If a valid range was found, execute the command.
		// NOTE: The stage passed to vkCmdPushConstants should be the specific requested stage,
		// not the combined stage from the range.
		vkCmdPushConstants(CmdBuffer, Program->PipelineLayout, ShaderStageTypeToVk(Stage),
			Offset, Size, Data);
	}

	void VulkanShaderDataManager::BindBindlessSets(VkCommandBuffer CmdBuffer, uint32_t ProgramHandle, uint32_t FrameIndex,
		const VulkanBindlessRenderingManager& BindlessManager)
	{
		auto Program = _ShaderPrograms.Get(ProgramHandle);

		std::array<VkDescriptorSet, int(BindlessSetTypes::COUNT_NON_USER)> Sets = BindlessManager.GetBindlessSets()[FrameIndex];

		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Program->PipelineLayout,
			0, int(BindlessSetTypes::COUNT_NON_USER), Sets.data(), 0, nullptr);
	}

	void VulkanShaderDataManager::BindUserSets(VkCommandBuffer CmdBuffer, uint32_t ProgramHandle, const std::array<VkDescriptorSet, int(BindlessSetTypes::COUNT_USER)>& Sets)
	{
		auto Program = _ShaderPrograms.Get(ProgramHandle);

		for (int i = 0; i < Sets.size(); i++)
		{
			if (Sets[i] == VK_NULL_HANDLE)
				continue;

			vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Program->PipelineLayout,
				int(BindlessSetTypes::USER_0) + i, 1, &Sets[i], 0, nullptr);
		}
	}

	VulkanShaderDataManager::VulkanShaderLocationData VulkanShaderDataManager::FindUserShaderDataLocation(uint32_t ProgramID, const char* Name)
	{
		auto Program = _ShaderPrograms.Get(ProgramID);

		for (int i = int(BindlessSetTypes::USER_0); i < Program->CombinedInfo.UniformInputs.size(); i++)
		{
			auto& SetData = Program->CombinedInfo.UniformInputs[i];

			for (int b = 0; b < SetData.Bindings.size(); b++)
			{
				if (memcmp(SetData.Bindings[b].Name, Name, SHADER_UNIFORM_BINDING_NAME_SIZE) == 0) {
					return { SetData.Bindings[b], true };
				}
			}
		}
		return { ReflectedBindingUniformInput(), false };
	}

#pragma endregion 

#pragma region Bindless Rendering Manager

	VkDescriptorPool CreateVkDescriptorPool(VkDevice Device,
		const std::vector< VulkanDescriptorPoolSize>& Sizes,
		void* pNextChain, uint32_t MaxSets, VkDescriptorPoolCreateFlagBits CreateFlag)
	{
		std::vector< VkDescriptorPoolSize> poolSizes;
		for (auto& Size : Sizes)
		{
			VkDescriptorPoolSize PoolSize;
			PoolSize.descriptorCount = Size.Count;
			PoolSize.type = ShaderUniformTypeToVk(Size.Type);
			poolSizes.push_back(PoolSize);
		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.pNext = pNextChain; // Inserted pNext chain
		poolInfo.flags = CreateFlag; // Allows individual sets to be freed
		poolInfo.maxSets = MaxSets;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		VkDescriptorPool descriptorPool;
		vkCreateDescriptorPool(Device, &poolInfo, nullptr, &descriptorPool);
		return descriptorPool;
	}

	VkDescriptorSetLayoutBinding __CreateSetLayoutBinding(uint32_t Binding, ShaderUniformTypes Type,
		uint32_t DesCount, VkShaderStageFlags ShaderFlags, VkSampler* pImmutableSamplers)
	{
		// Create Layout
		VkDescriptorSetLayoutBinding BindingInfo{};
		BindingInfo.binding = Binding;
		BindingInfo.descriptorType = ShaderUniformTypeToVk(Type);
		BindingInfo.descriptorCount = DesCount;
		BindingInfo.stageFlags = ShaderFlags;
		BindingInfo.pImmutableSamplers = pImmutableSamplers; // Optional

		return BindingInfo;
	}

	VkDescriptorSetLayout __CreateSetLayout(VkDevice Device, uint32_t BindingCount, VkDescriptorSetLayoutBinding* pBindings, void* pNext = nullptr)
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = BindingCount;
		layoutInfo.pBindings = pBindings;
		layoutInfo.pNext = pNext;
		VkDescriptorSetLayout Layout;

		if (vkCreateDescriptorSetLayout(Device, &layoutInfo, nullptr, &Layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		return Layout;
	}

	void VulkanBindlessRenderingManager::Init(const VulkanBindlessSetManagerCreateInfo& Info)
	{
		_CreateInfo = Info;
		// Calculate the maximum capacity required for UBs/SSBOs for one frame's dynamic sets.
	// NOTE: MAX_TEXTURES/MAX_SAMPLERS are set to 0 or a low default value here.
		const std::vector<VulkanDescriptorPoolSize> DynamicPoolSizes = {
			VulkanDescriptorPoolSize(ShaderUniformTypes::UNIFORM_BUFFER,
				int(BindlessRenderingLimits::MAX_UNIFORM_BUFFERS) / Info.MaxFrameInFlight),
			VulkanDescriptorPoolSize(ShaderUniformTypes::STORAGE_BUFFER,
				int(BindlessRenderingLimits::MAX_STORAGE_BUFFERS) / Info.MaxFrameInFlight),
		};

		for (int i = 0; i < Info.MaxFrameInFlight; i++)
		{
			// Use a Create function that takes the pool sizes, max sets, and flags.
			this->_BindlessDescPools.push_back(
				CreateVkDescriptorPool(
					Info.Device->GetHandle(),
					DynamicPoolSizes,
					nullptr,
					1000,
					VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT // Essential for per-frame reset
				)
			);
		}


		// --- 2. Create 1x Global Bindless Pool (HIGH Capacity) ---
		// This pool is for the static Set 1 (TEX_SAMPLERS).
		// It must be sized for the full MAX_TEXTURES/MAX_SAMPLERS count.

		const std::vector<VulkanDescriptorPoolSize> BindlessPoolSizes = {
			VulkanDescriptorPoolSize(ShaderUniformTypes::SAMPLED_IMAGE,
				BindlessRenderingLimits::MAX_TEXTURES), // Full Capacity
			VulkanDescriptorPoolSize(ShaderUniformTypes::SAMPLER,
				BindlessRenderingLimits::MAX_SAMPLERS)
		};

		// Only 1 set (the TEX_SAMPLERS set) is allocated from this pool.
		uint32_t bindlessMaxSets = 1;

		// Use a member variable for the single global bindless pool (e.g., _BindlessPool instead of _BindlessDescPools)
		this->_BindlessTexPool =
			CreateVkDescriptorPool(
				Info.Device->GetHandle(),
				BindlessPoolSizes,
				nullptr,
				bindlessMaxSets,
				VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT // Essential for true bindless
			);

		_SetupBindlessSetLayouts(Info.Device->GetHandle());
		_SetupBindlessSetManagerSets(Info);
		_SetupManagerBuffers(Info);
		_WriteBindlessSetManagerSets(Info);
	}

	void VulkanBindlessRenderingManager::Destroy(const VulkanBindlessSetManagerCreateInfo& Info)
	{
		auto Device = Info.Device->GetHandle();
		for (auto& descriptorPool : this->_BindlessDescPools)
			// All allocated descriptor sets are automatically freed when the pool is destroyed
			if (descriptorPool != VK_NULL_HANDLE) {
				vkDestroyDescriptorPool(Device, descriptorPool, nullptr);
				descriptorPool = VK_NULL_HANDLE; // Good practice to reset handle
			}

		vkDestroyDescriptorPool(Device, _BindlessTexPool, nullptr);

		for (auto& BufferHandle : _SetBuffers)
			if (BufferHandle != SparseSet<uint32_t>::npos)
				Info.FreeBuffer(BufferHandle);

		for (auto& Layout : this->_BindlessSetLayouts)
			vkDestroyDescriptorSetLayout(Device, Layout, nullptr);
	}

	void VulkanBindlessRenderingManager::_SetupBindlessSetLayouts(VkDevice device)
	{
		// Create Layouts and Sets
		{
			// Global Set - 0 Binding 0
			auto GlobalUBOLayoutBinding = __CreateSetLayoutBinding(0, ShaderUniformTypes::UNIFORM_BUFFER,
				1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

			// Global Set - 0 Binding 1
			auto SceneUBOLayoutBinding = __CreateSetLayoutBinding(1, ShaderUniformTypes::UNIFORM_BUFFER,
				1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

			VkDescriptorSetLayoutBinding Bindings[] = { GlobalUBOLayoutBinding, SceneUBOLayoutBinding };

			_BindlessSetLayouts[int(BindlessSetTypes::GLOBAL_SCENE)] = __CreateSetLayout(
				device, 2, Bindings);
		} {
			// Tex Sampler Set - CORRECT ORDER for bindless
			// Binding 0: Samplers (fixed count)
			auto SamplerBinding = __CreateSetLayoutBinding(0, ShaderUniformTypes::SAMPLER,
				int(BindlessRenderingLimits::MAX_SAMPLERS), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

			// Binding 1: Textures (variable count - MUST BE HIGHEST BINDING)
			auto TextureBinding = __CreateSetLayoutBinding(1, ShaderUniformTypes::SAMPLED_IMAGE,
				int(BindlessRenderingLimits::MAX_TEXTURES), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

			VkDescriptorSetLayoutBinding Bindings[] = { SamplerBinding, TextureBinding }; // REVERSED ORDER!

			// ---- Binding flags ----
			VkDescriptorBindingFlags bindingFlags[2] = {};
			bindingFlags[0] = 0; // No flags for samplers (binding 0)
			bindingFlags[1] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |  // for bindless textures (binding 1)
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

			VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsInfo{};
			bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
			bindingFlagsInfo.bindingCount = 2;
			bindingFlagsInfo.pBindingFlags = bindingFlags;

			_BindlessSetLayouts[int(BindlessSetTypes::TEX_SAMPLERS)] = __CreateSetLayout(
				device, 2, Bindings, &bindingFlagsInfo);
		}

		{
			// Material Set - 0 Binding 0
			auto MaterialBinding = __CreateSetLayoutBinding(0, ShaderUniformTypes::STORAGE_BUFFER,
				1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

			_BindlessSetLayouts[int(BindlessSetTypes::MATERIAl)] = __CreateSetLayout(
				device, 1, &MaterialBinding, nullptr);
		}
		{
			// Object Set - 0 Binding 0
			auto ObjectBinding = __CreateSetLayoutBinding(0, ShaderUniformTypes::STORAGE_BUFFER,
				1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

			_BindlessSetLayouts[int(BindlessSetTypes::PER_OBJECT)] = __CreateSetLayout(
				device, 1, &ObjectBinding, nullptr);

		}
	}

	void VulkanBindlessRenderingManager::_SetupBindlessSetManagerSets(const VulkanBindlessSetManagerCreateInfo& Info)
	{
		auto device = Info.Device->GetHandle();

		// Ensure _BindlessSets is sized to Info.MaxFrameInFlight
		if (_BindlessSets.size() < Info.MaxFrameInFlight) {
			_BindlessSets.resize(Info.MaxFrameInFlight);
		}

		// Single layout pointers for convenience
		VkDescriptorSetLayout globalSceneLayout = _BindlessSetLayouts[int(BindlessSetTypes::GLOBAL_SCENE)];
		VkDescriptorSetLayout texSamplersLayout = _BindlessSetLayouts[int(BindlessSetTypes::TEX_SAMPLERS)];
		VkDescriptorSetLayout materialLayout = _BindlessSetLayouts[int(BindlessSetTypes::MATERIAl)];
		VkDescriptorSetLayout perObjectLayout = _BindlessSetLayouts[int(BindlessSetTypes::PER_OBJECT)];

		// --- 2. Bindless Texture Samplers Set (TEX_SAMPLERS) ---
		{
			// 1. Prepare Layouts (Required for batch allocation)

			uint32_t BindlessMaxTexs = BindlessRenderingLimits::MAX_TEXTURES;

			VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo{};
			variableCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
			// CRITICAL FIX: The count must match the number of sets being allocated
			variableCountInfo.descriptorSetCount = 1;
			variableCountInfo.pDescriptorCounts = &BindlessMaxTexs; // Use the dynamically sized vector data

			// 3. Prepare Allocation Info
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = this->_BindlessTexPool; // The single global pool
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &texSamplersLayout;
			allocInfo.pNext = &variableCountInfo;

			// 4. Allocate Sets
			VkDescriptorSet Sets;
			if (vkAllocateDescriptorSets(device, &allocInfo, &Sets) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate TEX_SAMPLERS set!");
			}

			// 5. Store Sets in per-frame slots
			for (int i = 0; i < Info.MaxFrameInFlight; ++i)
				this->_BindlessSets[i][int(BindlessSetTypes::TEX_SAMPLERS)] = Sets;
		}
		// Loop through ALL frames in flight (i)
		for (int i = 0; i < Info.MaxFrameInFlight; i++)
		{
			VkDescriptorPool currentPool = _BindlessDescPools[i];

			// --- 1. Global Scene Set (Set 0) ---
			{
				VkDescriptorSetAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = currentPool;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts = &globalSceneLayout;

				if (vkAllocateDescriptorSets(device, &allocInfo,
					&_BindlessSets[i][int(BindlessSetTypes::GLOBAL_SCENE)]) != VK_SUCCESS) {
					throw std::runtime_error("failed to allocate Global Scene set!");
				}
			}

			// --- 3. Material Scene Set (MATERIAL) ---
			{
				VkDescriptorSetAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = currentPool;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts = &materialLayout;

				if (vkAllocateDescriptorSets(device, &allocInfo,
					&_BindlessSets[i][int(BindlessSetTypes::MATERIAl)]) != VK_SUCCESS) {
					throw std::runtime_error("failed to allocate MATERIAL set!");
				}
			}

			// --- 4. Per Object Scene Set (PER_OBJECT) ---
			{
				VkDescriptorSetAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = currentPool;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts = &perObjectLayout;

				if (vkAllocateDescriptorSets(device, &allocInfo,
					&_BindlessSets[i][int(BindlessSetTypes::PER_OBJECT)]) != VK_SUCCESS) {
					throw std::runtime_error("failed to allocate PER_OBJECT set!");
				}
			}
		}
	}

	void VulkanBindlessRenderingManager::_SetupManagerBuffers(const VulkanBindlessSetManagerCreateInfo& Info)
	{
		// Global Buffers
		size_t alignment = Info.Device->GetPhysicalDevice()->Info.properties.limits.minUniformBufferOffsetAlignment;
		{
			size_t rawSize = sizeof(GlobalShaderData);
			_GlobalBufferAlignedSize = (rawSize + alignment - 1) & ~(alignment - 1);

			BufferCreateInfo BufferInfo{};
			BufferInfo.Data = nullptr;
			BufferInfo.SizeInBytes = _GlobalBufferAlignedSize * Info.MaxFrameInFlight;
			BufferInfo.State = BufferState::STREAM_DRAW;
			BufferInfo.Type = BUFFER_TYPE_UNIFORM;

			_SetBuffers[int(BindlessSetTypes::GLOBAL_SCENE)] = Info.AllocateBuffer(BufferInfo);
		}
		{
			// Scene Buffers
			size_t rawSize = sizeof(SceneShaderData);
			_SceneBufferAlignedSize = (rawSize + alignment - 1) & ~(alignment - 1);

			BufferCreateInfo BufferInfo{};
			BufferInfo.Data = nullptr;
			BufferInfo.SizeInBytes = _SceneBufferAlignedSize * Info.MaxFrameInFlight;
			BufferInfo.State = BufferState::STREAM_DRAW;
			BufferInfo.Type = BUFFER_TYPE_UNIFORM;

			_SetBuffers[int(BindlessSetTypes::SCENE_BUFFER_USAGE)] = Info.AllocateBuffer(BufferInfo);
		}
		{
			// Material Buffers
			constexpr size_t rawSize = sizeof(MaterialShaderData);

			std::vector< MaterialShaderData> DeafultData;
			DeafultData.resize(CH_MATERIAL_SHADER_DATA_AMOUNT * Info.MaxFrameInFlight);

			BufferCreateInfo BufferInfo{};
			BufferInfo.Data = DeafultData.data();
			BufferInfo.SizeInBytes = rawSize * CH_MATERIAL_SHADER_DATA_AMOUNT * Info.MaxFrameInFlight;
			BufferInfo.State = BufferState::DYNAMIC_DRAW;
			BufferInfo.Type = BUFFER_TYPE_STORAGE;

			_SetBuffers[int(BindlessSetTypes::MATERIAl)] = Info.AllocateBuffer(BufferInfo);
		}
		{
			// Material Buffers
			constexpr size_t rawSize = sizeof(ObjectShaderData);

			std::vector< ObjectShaderData> DeafultData;
			DeafultData.resize(CH_OBJECT_SHADER_DATA_AMOUNT * Info.MaxFrameInFlight);

			BufferCreateInfo BufferInfo{};
			BufferInfo.Data = DeafultData.data();
			BufferInfo.SizeInBytes = rawSize * CH_OBJECT_SHADER_DATA_AMOUNT * Info.MaxFrameInFlight;
			BufferInfo.State = BufferState::DYNAMIC_DRAW;
			BufferInfo.Type = BUFFER_TYPE_STORAGE;

			_SetBuffers[int(BindlessSetTypes::PER_OBJECT)] = Info.AllocateBuffer(BufferInfo);
		}
	}

	void VulkanBindlessRenderingManager::_WriteBindlessSetManagerSets(const VulkanBindlessSetManagerCreateInfo& Info)
	{
		auto device = Info.Device->GetHandle();
		for (size_t i = 0; i < Info.MaxFrameInFlight; i++) {
			{
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = Info.GetBuffer(_SetBuffers[int(BindlessSetTypes::GLOBAL_SCENE)]);
				bufferInfo.offset = i * _GlobalBufferAlignedSize;
				bufferInfo.range = sizeof(GlobalShaderData);

				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = _BindlessSets[i][int(BindlessSetTypes::GLOBAL_SCENE)];
				descriptorWrite.dstBinding = 0;
				descriptorWrite.dstArrayElement = 0;

				descriptorWrite.descriptorType = ShaderUniformTypeToVk(ShaderUniformTypes::UNIFORM_BUFFER);
				descriptorWrite.descriptorCount = 1;

				descriptorWrite.pBufferInfo = &bufferInfo;
				descriptorWrite.pImageInfo = nullptr; // Optional
				descriptorWrite.pTexelBufferView = nullptr; // Optional

				vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
			}
			{
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = Info.GetBuffer(_SetBuffers[int(BindlessSetTypes::SCENE_BUFFER_USAGE)]);
				bufferInfo.offset = i * _SceneBufferAlignedSize;
				bufferInfo.range = sizeof(SceneShaderData);

				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = _BindlessSets[i][int(BindlessSetTypes::GLOBAL_SCENE)];
				descriptorWrite.dstBinding = 1;
				descriptorWrite.dstArrayElement = 0;

				descriptorWrite.descriptorType = ShaderUniformTypeToVk(ShaderUniformTypes::UNIFORM_BUFFER);
				descriptorWrite.descriptorCount = 1;

				descriptorWrite.pBufferInfo = &bufferInfo;
				descriptorWrite.pImageInfo = nullptr; // Optional
				descriptorWrite.pTexelBufferView = nullptr; // Optional

				vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
			}
			{
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = Info.GetBuffer(_SetBuffers[int(BindlessSetTypes::MATERIAl)]);
				bufferInfo.offset = i * CH_MATERIAL_SHADER_DATA_AMOUNT * sizeof(MaterialShaderData);
				bufferInfo.range = CH_MATERIAL_SHADER_DATA_AMOUNT * sizeof(MaterialShaderData);

				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = _BindlessSets[i][int(BindlessSetTypes::MATERIAl)];
				descriptorWrite.dstBinding = 0;
				descriptorWrite.dstArrayElement = 0;

				descriptorWrite.descriptorType = ShaderUniformTypeToVk(ShaderUniformTypes::STORAGE_BUFFER);
				descriptorWrite.descriptorCount = 1;

				descriptorWrite.pBufferInfo = &bufferInfo;
				descriptorWrite.pImageInfo = nullptr; // Optional
				descriptorWrite.pTexelBufferView = nullptr; // Optional

				vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
			}
			{
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = Info.GetBuffer(_SetBuffers[int(BindlessSetTypes::PER_OBJECT)]);
				bufferInfo.offset = i * CH_OBJECT_SHADER_DATA_AMOUNT * sizeof(ObjectShaderData);
				bufferInfo.range = CH_OBJECT_SHADER_DATA_AMOUNT * sizeof(ObjectShaderData);

				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = _BindlessSets[i][int(BindlessSetTypes::PER_OBJECT)];
				descriptorWrite.dstBinding = 0;
				descriptorWrite.dstArrayElement = 0;

				descriptorWrite.descriptorType = ShaderUniformTypeToVk(ShaderUniformTypes::STORAGE_BUFFER);
				descriptorWrite.descriptorCount = 1;

				descriptorWrite.pBufferInfo = &bufferInfo;
				descriptorWrite.pImageInfo = nullptr; // Optional
				descriptorWrite.pTexelBufferView = nullptr; // Optional

				vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
			}
		}
	}

	void VulkanBindlessRenderingManager::_WriteShaderTexture(VkDevice device, uint32_t Index, VkImageView Texture)
	{
		VkDescriptorImageInfo imageinfo{};
		imageinfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageinfo.sampler = VK_NULL_HANDLE;
		imageinfo.imageView = Texture;

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = _BindlessSets[0][int(BindlessSetTypes::TEX_SAMPLERS)];
		descriptorWrite.dstBinding = 1;
		descriptorWrite.dstArrayElement = Index;

		descriptorWrite.descriptorType = ShaderUniformTypeToVk(ShaderUniformTypes::SAMPLED_IMAGE);
		descriptorWrite.descriptorCount = 1;

		descriptorWrite.pBufferInfo = nullptr;
		descriptorWrite.pImageInfo = &imageinfo; // Optional
		descriptorWrite.pTexelBufferView = nullptr; // Optional

		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
	}

	void VulkanBindlessRenderingManager::_WriteShaderSampler(VkDevice device, uint32_t Index, VkSampler Sampler)
	{
		VkDescriptorImageInfo imageinfo{};
		imageinfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageinfo.sampler = Sampler;
		imageinfo.imageView = VK_NULL_HANDLE;

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = _BindlessSets[0][int(BindlessSetTypes::TEX_SAMPLERS)];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = Index;

		descriptorWrite.descriptorType = ShaderUniformTypeToVk(ShaderUniformTypes::SAMPLER);
		descriptorWrite.descriptorCount = 1;

		descriptorWrite.pBufferInfo = nullptr;
		descriptorWrite.pImageInfo = &imageinfo; // Optional
		descriptorWrite.pTexelBufferView = nullptr; // Optional

		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
	}

	void VulkanBindlessRenderingManager::UpdateGlobalShaderData(const GlobalShaderData& Data)
	{
		for (int i = 0; i < _CreateInfo.MaxFrameInFlight; i++)
		{
			_CreateInfo.MapBufferData(_SetBuffers[int(BindlessSetTypes::GLOBAL_SCENE)],
				(void*)&Data,
				sizeof(GlobalShaderData),
				i * _GlobalBufferAlignedSize);
		}
	}

	void VulkanBindlessRenderingManager::UpdateSceneShaderData(const SceneShaderData& Data)
	{
		for (int i = 0; i < _CreateInfo.MaxFrameInFlight; i++)
		{
			_CreateInfo.MapBufferData(_SetBuffers[int(BindlessSetTypes::SCENE_BUFFER_USAGE)],
				(void*)&Data,
				sizeof(SceneShaderData),
				i * _SceneBufferAlignedSize);
		}
	}

	void VulkanBindlessRenderingManager::PrepareForMaterial(uint32_t MaterialIndex)
	{
		_MaterialMap.Create({ MaterialIndex, UINT32_MAX });
	}

	void VulkanBindlessRenderingManager::PrepareForTexture(uint32_t TexIndex)
	{
		_TextureMap.Create({ TexIndex, UINT32_MAX });
	}

	uint32_t VulkanBindlessRenderingManager::UpdateTexture(VkDevice device, uint32_t HandleId, VkImageView Txt)
	{
		auto MetaData = _TextureMap.Get(HandleId);

		if (MetaData)
		{
			if (MetaData->ShaderIndex != UINT32_MAX)
				return MetaData->ShaderIndex;
			// 1. Assign the current counter as the slot in the descriptor set
			MetaData->ShaderIndex = _ActiveTextureCounter;

			// Update Descritpor Set
			_WriteShaderTexture(device, _ActiveTextureCounter, Txt);

			// 2. Increment the counter for the NEXT texture
			_ActiveTextureCounter++;

			// 3. Return the index so your Material knows which slot to use in the shader
			return MetaData->ShaderIndex;
		}

		return UINT32_MAX;
	}

	void VulkanBindlessRenderingManager::PrepareForSampler(uint32_t SamplerIndex)
	{
		_SamplerMap.Create({ SamplerIndex, UINT32_MAX });
	}

	uint32_t VulkanBindlessRenderingManager::UpdateSampler(VkDevice device, uint32_t HandleId, VkSampler Sampler)
	{
		auto MetaData = _SamplerMap.Get(HandleId);

		if (MetaData)
		{
			if (MetaData->ShaderIndex != UINT32_MAX)
				return MetaData->ShaderIndex;
			// 1. Assign the current counter as the slot in the descriptor set
			MetaData->ShaderIndex = _ActiveSamplerCounter;

			// Update Descritpor Set
			_WriteShaderSampler(device, _ActiveSamplerCounter, Sampler);

			// 2. Increment the counter for the NEXT texture
			_ActiveSamplerCounter++;

			// 3. Return the index so your Material knows which slot to use in the shader
			return MetaData->ShaderIndex;
		}

		return UINT32_MAX;
	}

	void VulkanBindlessRenderingManager::UpdateMaterialShaderData(uint32_t MaterialHandle, const MaterialShaderData& Data)
	{
		int MaterialOffset = _ActiveMaterialCounter;
		auto MetaData = _MaterialMap.Get(MaterialHandle);

		if (!MetaData)
		{
			VULKAN_ERROR("Given materail index is not been prepared!");
			return;
		}

		bool Increment = false;
		if (MetaData)
		{
			if (MetaData->ShaderIndex == UINT32_MAX)
			{
				MetaData->ShaderIndex = MaterialOffset;
				Increment = true;
			}
			else
				MaterialOffset = MetaData->ShaderIndex;
		}

		for (uint32_t i = 0; i < _CreateInfo.MaxFrameInFlight; i++)
		{
			const uint32_t frameOffset =
				i * CH_MATERIAL_SHADER_DATA_AMOUNT * sizeof(MaterialShaderData);

			_CreateInfo.MapBufferData(
				_SetBuffers[int(BindlessSetTypes::MATERIAl)],
				(void*)&Data,
				sizeof(MaterialShaderData),
				frameOffset + (MaterialOffset * sizeof(MaterialShaderData))
			);
		}

		if (Increment)
			_ActiveMaterialCounter++;
	}

	// USe Entity Handle for Objects
	void VulkanBindlessRenderingManager::UpdateObjectShaderData(const ObjectShaderData& Data)
	{
		const uint32_t ObjectOffset = _ActiveObjectCounter * sizeof(ObjectShaderData);
		for (uint32_t i = 0; i < _CreateInfo.MaxFrameInFlight; i++)
		{
			const uint32_t frameOffset =
				i * CH_OBJECT_SHADER_DATA_AMOUNT * sizeof(ObjectShaderData);

			_CreateInfo.MapBufferData(
				_SetBuffers[int(BindlessSetTypes::PER_OBJECT)],
				(void*)&Data,
				sizeof(ObjectShaderData),
				frameOffset + ObjectOffset
			);
		}
		_ActiveObjectCounter++;
	}
#pragma endregion 

}

namespace Chilli
{
	VkFormat SPVFormatToVkFormat(SpvReflectFormat fmt)
	{
		switch (fmt) {
		case SPV_REFLECT_FORMAT_R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
		case SPV_REFLECT_FORMAT_R32G32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
		case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
		case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
		default: return VK_FORMAT_UNDEFINED;
		}
	}

	Chilli::ShaderUniformTypes SpirvTypeToShaderUniformTypes(SpvReflectDescriptorType Type)
	{
		switch (Type)
		{
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
			return Chilli::ShaderUniformTypes::SAMPLER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			return Chilli::ShaderUniformTypes::COMBINED_IMAGE_SAMPLER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return Chilli::ShaderUniformTypes::SAMPLED_IMAGE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			return Chilli::ShaderUniformTypes::UNIFORM_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			return Chilli::ShaderUniformTypes::STORAGE_BUFFER;
		default:
			return Chilli::ShaderUniformTypes::UNKNOWN;
		}
	}

	Chilli::ReflectedShaderInfo ReflectShaderModule(VkDevice device, const std::vector<char>& code, VkShaderStageFlagBits stageFlag)
	{
		ReflectedShaderInfo info;

		SpvReflectShaderModule module;
		SpvReflectResult result = spvReflectCreateShaderModule(code.size(), code.data(), &module);
		assert(result == SPV_REFLECT_RESULT_SUCCESS);

		// ---------- Reflect Vertex Input Layout -----
		uint32_t VertexInputCount = 0;
		spvReflectEnumerateInputVariables(&module, &VertexInputCount, nullptr);
		std::vector<SpvReflectInterfaceVariable*> VertexInputs(VertexInputCount);
		spvReflectEnumerateInputVariables(&module, &VertexInputCount, VertexInputs.data());

		uint32_t VertexInputOffset = 0;

		for (auto* VertexInputInfo : VertexInputs)
		{
			// Skip built-ins like gl_VertexIndex, gl_InstanceIndex
			if (VertexInputInfo->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
				continue;

			ReflectedVertexInput input{};
			input.Name = VertexInputInfo->name ? VertexInputInfo->name : "";
			input.Location = VertexInputInfo->location;
			input.Format = VkFormatToShaderObjectType(SPVFormatToVkFormat(VertexInputInfo->format));
			input.Offset = VertexInputOffset; // You'll compute offsets later
			VertexInputOffset += ShaderTypeToSize(FormatToShaderType(SPVFormatToVkFormat(VertexInputInfo->format)));

			info.VertexInputs.push_back(input);
		}

		// -------- Descriptor Sets --------
		uint32_t set_count = 0;
		spvReflectEnumerateDescriptorSets(&module, &set_count, nullptr);

		std::vector<SpvReflectDescriptorSet*> sets(set_count);
		spvReflectEnumerateDescriptorSets(&module, &set_count, sets.data());
		info.UniformInputs.reserve(set_count);

		for (uint32_t si = 0; si < set_count; ++si) {
			SpvReflectDescriptorSet* s = sets[si];
			ReflectedSetUniformInput mySet;
			mySet.Set = s->set;
			mySet.Bindings.reserve(s->binding_count);

			for (uint32_t bi = 0; bi < s->binding_count; ++bi) {
				SpvReflectDescriptorBinding* b = s->bindings[bi];

				ReflectedBindingUniformInput rb;
				rb.Binding = b->binding;
				rb.Set = b->set;
				rb.Count = b->count;
				rb.Type = SpirvTypeToShaderUniformTypes(b->descriptor_type);
				rb.Stage = VkFlagBitToShaderStageType(stageFlag);

				// FIX: Copy name into char[32]
				if (b->name) {
					strncpy(rb.Name, b->name, SHADER_UNIFORM_BINDING_NAME_SIZE - 1);
					rb.Name[SHADER_UNIFORM_BINDING_NAME_SIZE - 1] = '\0';
				}
				else
					rb.Name[0] = '\0';

				mySet.Bindings.push_back(rb);
			}

			info.UniformInputs.push_back(mySet);
		}

		// ---------- Reflect Push Constants ----------
		uint32_t pcCount = 0;
		spvReflectEnumeratePushConstantBlocks(&module, &pcCount, nullptr);
		std::vector<SpvReflectBlockVariable*> pcs(pcCount);
		spvReflectEnumeratePushConstantBlocks(&module, &pcCount, pcs.data());

		for (auto* pc : pcs)
		{
			ReflectedInlineUniformData range{};
			range.Offset = 0;
			range.Size = pc->size;
			range.Stage = VkFlagBitToShaderStageType(stageFlag);
			info.PushConstants.push_back(range);
		}

		spvReflectDestroyShaderModule(&module);
		return info;
	}

	void CombineUniformReflection(
		std::vector<ReflectedSetUniformInput>& OutCombined,
		const std::vector<ReflectedSetUniformInput>& InfoA,
		const std::vector<ReflectedSetUniformInput>& InfoB,
		ShaderStageType StageA,
		ShaderStageType StageB
	)
	{
		std::unordered_set<uint32_t> allSets;
		for (auto& setA : InfoA) allSets.insert(setA.Set);
		for (auto& setB : InfoB) allSets.insert(setB.Set);

		for (uint32_t setIndex : allSets)
		{
			ReflectedSetUniformInput combinedSet;
			combinedSet.Set = setIndex;

			const ReflectedSetUniformInput* setA = nullptr;
			const ReflectedSetUniformInput* setB = nullptr;

			// Fetch matching set
			for (auto& s : InfoA)
				if (s.Set == setIndex) { setA = &s; break; }

			for (auto& s : InfoB)
				if (s.Set == setIndex) { setB = &s; break; }

			std::unordered_set<uint32_t> allBindings;

			if (setA)
				for (auto& b : setA->Bindings)
					allBindings.insert(b.Binding);

			if (setB)
				for (auto& b : setB->Bindings)
					allBindings.insert(b.Binding);

			for (uint32_t bindingIndex : allBindings)
			{
				ReflectedBindingUniformInput combinedBinding{};
				combinedBinding.Binding = bindingIndex;
				combinedBinding.Set = setIndex;

				const ReflectedBindingUniformInput* bindA = nullptr;
				const ReflectedBindingUniformInput* bindB = nullptr;

				if (setA)
				{
					for (auto& b : setA->Bindings)
						if (b.Binding == bindingIndex) { bindA = &b; break; }
				}

				if (setB)
				{
					for (auto& b : setB->Bindings)
						if (b.Binding == bindingIndex) { bindB = &b; break; }
				}

				if (bindA && bindB)
				{
					// VALIDATION: Check if same binding in different stages is actually the same data
					bool typeMismatch = (bindA->Type != bindB->Type);
					bool nameMismatch = (strcmp(bindA->Name, bindB->Name) != 0);

					if (typeMismatch || nameMismatch)
					{
						// ERROR: Log mismatch and skip pushing this binding
						// You might want to use your engine's specific logging here
						assert(false && "Binding conflict: Type or Name mismatch for same set/binding across stages");
						continue;
					}

					// Exists in both shaders -> merge stages
					strncpy(combinedBinding.Name, bindA->Name, SHADER_UNIFORM_BINDING_NAME_SIZE - 1);
					combinedBinding.Name[SHADER_UNIFORM_BINDING_NAME_SIZE - 1] = '\0';

					combinedBinding.Type = bindA->Type;
					combinedBinding.Count = bindA->Count;

					// Use OR to combine the bits. If bindA already had multiple bits, we preserve them.
					combinedBinding.Stage = (ShaderStageType)(bindA->Stage | bindB->Stage);
				}
				else if (bindA)
				{
					// Only in Shader A: Work for individual set/binding
					combinedBinding = *bindA;
					// Preserve existing flags or ensure StageA bit is set
					combinedBinding.Stage = (ShaderStageType)(bindA->Stage | StageA);
				}
				else if (bindB)
				{
					// Only in Shader B: Work for individual set/binding
					combinedBinding = *bindB;
					combinedBinding.Stage = (ShaderStageType)(bindB->Stage | StageB);

					// Safe name copy
					char tmp[SHADER_UNIFORM_BINDING_NAME_SIZE];
					strncpy(tmp, bindB->Name, SHADER_UNIFORM_BINDING_NAME_SIZE);
					memcpy(combinedBinding.Name, tmp, SHADER_UNIFORM_BINDING_NAME_SIZE);
				}

				combinedSet.Bindings.push_back(combinedBinding);
			}

			std::sort(
				combinedSet.Bindings.begin(),
				combinedSet.Bindings.end(),
				[](const ReflectedBindingUniformInput& a,
					const ReflectedBindingUniformInput& b)
				{
					return a.Binding < b.Binding;
				}
			);

			OutCombined.push_back(combinedSet);
		}

		std::sort(
			OutCombined.begin(),
			OutCombined.end(),
			[](const ReflectedSetUniformInput& a,
				const ReflectedSetUniformInput& b)
			{
				return a.Set < b.Set;
			}
		);
	}

	void CombinePushConstantReflection(
		std::vector<ReflectedInlineUniformData>& OutCombined,
		const std::vector<ReflectedInlineUniformData>& InfoA,
		const std::vector<ReflectedInlineUniformData>& InfoB,
		ShaderStageType StageA,
		ShaderStageType StageB)
	{
		OutCombined.clear();

		// Temp list to merge
		std::vector<ReflectedInlineUniformData> temp;

		auto AddRange = [&](const std::vector<ReflectedInlineUniformData>& src, ShaderStageType stage)
			{
				for (const auto& r : src)
				{
					ReflectedInlineUniformData pc = r;
					pc.Stage |= (uint64_t)stage;
					temp.push_back(pc);
				}
			};

		// Add ranges with stage flags
		AddRange(InfoA, StageA);
		AddRange(InfoB, StageB);

		if (temp.empty())
			return;

		// Sort by offset (required for merging)
		std::sort(temp.begin(), temp.end(),
			[](const auto& a, const auto& b)
			{
				return a.Offset < b.Offset;
			});

		// Merge overlapping ranges
		ReflectedInlineUniformData current = temp[0];

		for (size_t i = 1; i < temp.size(); i++)
		{
			const auto& next = temp[i];

			uint32_t currentEnd = current.Offset + current.Size;
			uint32_t nextEnd = next.Offset + next.Size;

			bool overlap =
				next.Offset <= currentEnd; // overlapping or touching

			if (overlap)
			{
				// Merge the size
				current.Size = std::max(currentEnd, nextEnd) - current.Offset;
				// Merge stage flags
				current.Stage |= next.Stage;
			}
			else
			{
				// store completed range
				OutCombined.push_back(current);
				current = next;
			}
		}

		// push last
		OutCombined.push_back(current);
	}
}
