#include "ChV_PCH.h"

#define USE_PLATFORM_WIN32_KHR
#define VK_ENABLE_BETA_EXTENSIONS
#include "C:\VulkanSDK\1.3.275.0\Include\vulkan\vulkan.h"

#include "vk_mem_alloc.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "VulkanShader.h"
#include "VulkanConversions.h"

#include "spirv_reflect.h"

namespace Chilli
{
	static VulkanPipelineLayoutCache s_LayoutCache;

	void _ReadFile(const char* FilePath, std::vector<char>& CharData)
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

	VkShaderModule _CreateShaderModule(VkDevice device, std::vector<char>& code)
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

	VkPipelineShaderStageCreateInfo _CreateShaderStages(VkShaderModule module, VkShaderStageFlagBits type, const char* Name)
	{
		VkPipelineShaderStageCreateInfo stage{};
		stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage.module = module;
		stage.pName = Name;
		stage.stage = type;

		return stage;
	}

	VkVertexInputAttributeDescription GetAttributeDescription(int Binding, int Location, int Offset, VkFormat Format)
	{
		VkVertexInputAttributeDescription attributeDescriptions{};
		attributeDescriptions.binding = Binding;
		attributeDescriptions.location = Location;
		attributeDescriptions.format = Format; // vec3
		attributeDescriptions.offset = Offset;
		return attributeDescriptions;
	}

	// Helper to map SPIR-V format to VkFormat
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

	int ShaderTypeToSize(ShaderVertexTypes Type)
	{
		switch (Type)
		{
		case ShaderVertexTypes::FLOAT1:
			return sizeof(float) * 1;
		case ShaderVertexTypes::FLOAT2:
			return sizeof(float) * 2;
		case ShaderVertexTypes::FLOAT3:
			return sizeof(float) * 3;
		case ShaderVertexTypes::FLOAT4:
			return sizeof(float) * 4;
		case ShaderVertexTypes::INT1:
			return sizeof(int) * 1;
		case ShaderVertexTypes::INT2:
			return sizeof(int) * 2;
		case ShaderVertexTypes::INT3:
			return sizeof(int) * 3;
		case ShaderVertexTypes::INT4:
			return sizeof(int) * 4;
		case ShaderVertexTypes::UINT1:
			return sizeof(unsigned int) * 1;
		case ShaderVertexTypes::UINT2:
			return sizeof(unsigned int) * 2;
		case ShaderVertexTypes::UINT3:
			return sizeof(unsigned int) * 3;
		case ShaderVertexTypes::UINT4:
			return sizeof(unsigned int) * 4;
		default:
			return -1;
		}
	}

	VkFormat ShaderTypeToFormat(ShaderVertexTypes Type)
	{
		switch (Type)
		{
		case ShaderVertexTypes::FLOAT1:
			return VkFormat::VK_FORMAT_R32_SFLOAT;
		case ShaderVertexTypes::FLOAT2:
			return VkFormat::VK_FORMAT_R32G32_SFLOAT;
		case ShaderVertexTypes::FLOAT3:
			return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
		case ShaderVertexTypes::FLOAT4:
			return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		case ShaderVertexTypes::INT1:
			return VkFormat::VK_FORMAT_R32_SINT;
		case ShaderVertexTypes::INT2:
			return VkFormat::VK_FORMAT_R32G32_SINT;
		case ShaderVertexTypes::INT3:
			return VkFormat::VK_FORMAT_R32G32B32_SINT;
		case ShaderVertexTypes::INT4:
			return VkFormat::VK_FORMAT_R32G32B32A32_SINT;
		case ShaderVertexTypes::UINT1:
			return VkFormat::VK_FORMAT_R32_UINT;
		case ShaderVertexTypes::UINT2:
			return VkFormat::VK_FORMAT_R32G32_UINT;
		case ShaderVertexTypes::UINT3:
			return VkFormat::VK_FORMAT_R32G32B32_UINT;
		case ShaderVertexTypes::UINT4:
			return VkFormat::VK_FORMAT_R32G32B32A32_UINT;
		default:
			return VK_FORMAT_UNDEFINED;
		}
	}


	ShaderVertexTypes FormatToShaderType(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R32_SFLOAT:
			return ShaderVertexTypes::FLOAT1;
		case VK_FORMAT_R32G32_SFLOAT:
			return ShaderVertexTypes::FLOAT2;
		case VK_FORMAT_R32G32B32_SFLOAT:
			return ShaderVertexTypes::FLOAT3;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return ShaderVertexTypes::FLOAT4;
		case VK_FORMAT_R32_SINT:
			return ShaderVertexTypes::INT1;
		case VK_FORMAT_R32G32_SINT:
			return ShaderVertexTypes::INT2;
		case VK_FORMAT_R32G32B32_SINT:
			return ShaderVertexTypes::INT3;
		case VK_FORMAT_R32G32B32A32_SINT:
			return ShaderVertexTypes::INT4;
		case VK_FORMAT_R32_UINT:
			return ShaderVertexTypes::UINT1;
		case VK_FORMAT_R32G32_UINT:
			return ShaderVertexTypes::UINT2;
		case VK_FORMAT_R32G32B32_UINT:
			return ShaderVertexTypes::UINT3;
		case VK_FORMAT_R32G32B32A32_UINT:
			return ShaderVertexTypes::UINT4;
		}
	}

	// stageFlag = VK_SHADER_STAGE_VERTEX_BIT / FRAGMENT_BIT etc.
	ReflectedShaderInfo ReflectShaderModule(VkDevice device, const std::string& path, VkShaderStageFlagBits stageFlag)
	{
		ReflectedShaderInfo info;
		std::vector<char> code;
		_ReadFile(path.c_str(), code);

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
			input.Format = SPVFormatToVkFormat(VertexInputInfo->format);
			input.Offset = VertexInputOffset; // You'll compute offsets later
			VertexInputOffset += ShaderTypeToSize(FormatToShaderType(SPVFormatToVkFormat(VertexInputInfo->format)));

			info.VertexInputs.push_back(input);
		}

		// ---------- Reflect Descriptor Bindings ----------
		uint32_t setCount = 0;
		spvReflectEnumerateDescriptorSets(&module, &setCount, nullptr);
		std::vector<SpvReflectDescriptorSet*> sets(setCount);
		spvReflectEnumerateDescriptorSets(&module, &setCount, sets.data());

		for (auto* set : sets)
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			for (uint32_t i = 0; i < set->binding_count; i++)
			{
				const SpvReflectDescriptorBinding& refl = *(set->bindings[i]);

				VkDescriptorSetLayoutBinding layoutBinding{};
				layoutBinding.binding = refl.binding;
				layoutBinding.descriptorType = (VkDescriptorType)refl.descriptor_type;
				layoutBinding.descriptorCount = refl.count;
				layoutBinding.stageFlags = stageFlag;
				layoutBinding.pImmutableSamplers = nullptr;

				ReflectedUniformInput UniformInfo{};
				UniformInfo.Binding = refl.binding;

				if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
					UniformInfo.Type = ShaderUniformTypes::UNIFORM_BUFFER;
				if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					UniformInfo.Type = ShaderUniformTypes::SAMPLED_IMAGE;

				if (layoutBinding.stageFlags == VK_SHADER_STAGE_VERTEX_BIT)
					UniformInfo.Stage = ShaderStageType::VERTEX;
				if (layoutBinding.stageFlags == VK_SHADER_STAGE_FRAGMENT_BIT)
					UniformInfo.Stage = ShaderStageType::FRAGMENT;

				UniformInfo.Count = 1;
				UniformInfo.pImmutableSamplers = nullptr;
				UniformInfo.SetLayoutIndex = refl.set;
				UniformInfo.Name = refl.name ? refl.name : "";

				info.Bindings.push_back(UniformInfo);
			}
		}

		// ---------- Reflect Push Constants ----------
		uint32_t pcCount = 0;
		spvReflectEnumeratePushConstantBlocks(&module, &pcCount, nullptr);
		std::vector<SpvReflectBlockVariable*> pcs(pcCount);
		spvReflectEnumeratePushConstantBlocks(&module, &pcCount, pcs.data());

		for (auto* pc : pcs)
		{
			VkPushConstantRange range{};
			range.offset = 0;
			range.size = pc->size;
			range.stageFlags = stageFlag;
			info.PushConstantRanges.push_back(range);
		}

		spvReflectDestroyShaderModule(&module);
		return info;
	}

	bool MergeBindingStage(std::vector<ReflectedUniformInput>& bindings,
		const ReflectedUniformInput& candidate)
	{
		for (auto& existing : bindings)
		{
			// Match by set and binding index
			if (existing.Binding == candidate.Binding && existing.SetLayoutIndex == candidate.SetLayoutIndex)
			{
				// Merge stage flags: if one is vertex and one is fragment → set to ALL
				if ((existing.Stage == ShaderStageType::VERTEX && candidate.Stage == ShaderStageType::FRAGMENT) ||
					(existing.Stage == ShaderStageType::FRAGMENT && candidate.Stage == ShaderStageType::VERTEX))
				{
					existing.Stage = ShaderStageType::ALL;
				}
				return true; // already exists and merged
			}
		}
		return false;
	}

	void MergeReflectedShaderInfos(ReflectedShaderInfo& base, const ReflectedShaderInfo& extra)
	{
		// Merge uniform bindings
		for (const auto& b : extra.Bindings)
		{
			if (!MergeBindingStage(base.Bindings, b))
				base.Bindings.push_back(b);
		}

		// Merge push constants (avoid duplicates)
		for (const auto& pc : extra.PushConstantRanges)
		{
			bool exists = false;
			for (const auto& existing : base.PushConstantRanges)
			{
				if (existing.offset == pc.offset && existing.size == pc.size)
				{
					exists = true;
					break;
				}
			}
			if (!exists)
				base.PushConstantRanges.push_back(pc);
		}

		// (Optional) Vertex inputs only exist for vertex shader — no need to merge
	}

	void VulkanGraphiscPipeline::Init(const GraphicsPipelineSpec& Spec)
	{
		auto Device = VulkanUtils::GetLogicalDevice();

		std::vector<char> VertShaderCode, FragShaderCode;
		_ReadFile(Spec.VertPath.c_str(), VertShaderCode);
		_ReadFile(Spec.FragPath.c_str(), FragShaderCode);
		// Create shader modules
		VkShaderModule vertShaderModule = _CreateShaderModule(Device, VertShaderCode);
		VkShaderModule fragShaderModule = _CreateShaderModule(Device, FragShaderCode);

		// Shader stages
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);
		shaderStages[0] = _CreateShaderStages(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT, "main");
		shaderStages[1] = _CreateShaderStages(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

		_Spec = Spec;
		_FillInfo();

		uint32_t Stride = 0;

		std::vector<VkVertexInputAttributeDescription> NewAttributeDescriptions;
		NewAttributeDescriptions.reserve(_Info.VertexInputs.size());

		for (auto& InputInfo : _Info.VertexInputs)
		{
			NewAttributeDescriptions.push_back(GetAttributeDescription(0, InputInfo.Location, InputInfo.Offset,
				InputInfo.Format));
			Stride += ShaderTypeToSize(FormatToShaderType(InputInfo.Format));
		}

		// 🆕 UPDATED: Vertex input descriptions for the new shader
		VkVertexInputBindingDescription bindingDescriptions;
		bindingDescriptions.binding = 0;
		bindingDescriptions.stride = Stride;
		bindingDescriptions.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescriptions;
		vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)1;
		vertexInputInfo.pVertexAttributeDescriptions = NewAttributeDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)NewAttributeDescriptions.size();

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.primitiveRestartEnable = VK_FALSE;
		inputAssembly.topology = TopologyToVk(Spec.TopologyMode);

		// Viewport and scissor (dynamic)
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.lineWidth = 1.0f;
		rasterizer.frontFace = FrontFaceToVk(Spec.FrontFace);
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.cullMode = CullModeToVk(Spec.ShaderCullMode);
		rasterizer.polygonMode = PolygonModeToVk(Spec.ShaderFillMode);

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// Color blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.pAttachments = nullptr;

		if (Spec.ColorBlend)
		{
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlendAttachment;
		}

		// Dynamic state
		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		// Dynamic rendering
		VkPipelineRenderingCreateInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		renderingInfo.colorAttachmentCount = 1;
		auto SwapChainFormat = VulkanUtils::GetSwapChainKHR().GetFormat();
		renderingInfo.pColorAttachmentFormats = &SwapChainFormat;

		_PipelineLayoutHash = 0;
		auto PipelineLayout = VulkanPipelineLayoutCache::GetPipelineLayoutCache().GetOrCreate(0);

		// Create graphics pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = &renderingInfo; // Dynamic rendering
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = PipelineLayout;
		pipelineInfo.renderPass = VK_NULL_HANDLE; // No render pass with dynamic rendering
		pipelineInfo.subpass = 0;
		
		VkPipelineDepthStencilStateCreateInfo depthStencil{};

		if (Spec.EnableDepthStencil)
		{
			// Depth stencil state - THIS IS KEY!
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = Spec.EnableDepthTest;
			depthStencil.depthWriteEnable = Spec.EnableDepthWrite;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = Spec.EnableStencilTest;
			pipelineInfo.pDepthStencilState = &depthStencil;
		}

		vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_Pipeline);

		// Cleanup shader modules
		vkDestroyShaderModule(Device, vertShaderModule, nullptr);
		vkDestroyShaderModule(Device, fragShaderModule, nullptr);
	}

	void VulkanGraphiscPipeline::ReCreate(const GraphicsPipelineSpec& Spec)
	{
		Destroy();
		Init(Spec);
	}

	void VulkanGraphiscPipeline::Destroy()
	{
		vkDestroyPipeline(VulkanUtils::GetLogicalDevice(), _Pipeline, nullptr);
	}

	void VulkanGraphiscPipeline::Bind() const
	{
		vkCmdBindPipeline(VulkanUtils::GetActiveCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, _Pipeline);
	}

	void VulkanGraphiscPipeline::_FillInfo()
	{
		_Info = ReflectShaderModule(VulkanUtils::GetLogicalDevice(), _Spec.VertPath, VK_SHADER_STAGE_VERTEX_BIT);
		auto FragInfo = ReflectShaderModule(VulkanUtils::GetLogicalDevice(), _Spec.FragPath, VK_SHADER_STAGE_FRAGMENT_BIT);

		MergeReflectedShaderInfos(_Info, FragInfo);
	}

	VkDescriptorPool _CreatePool(VkDevice device, uint32_t MaxSetCount
		, VkDescriptorPoolCreateFlags Flag, const std::vector<std::tuple< ShaderUniformTypes, uint32_t>>& PoolSizeInfos,
		void* pNext = nullptr)
	{
		VkDescriptorPoolCreateInfo PoolInfo{};
		PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		PoolInfo.flags = Flag;
		PoolInfo.maxSets = MaxSetCount;

		std::vector<VkDescriptorPoolSize> PoolSizes;
		PoolSizes.reserve(PoolSizeInfos.size());

		for (auto& SizeInfo : PoolSizeInfos)
		{
			auto& [Type, Count] = SizeInfo;
			PoolSizes.push_back({ UniformTypeToVk(Type), Count });
		}
		PoolInfo.poolSizeCount = PoolSizes.size();
		PoolInfo.pPoolSizes = PoolSizes.data();
		PoolInfo.pNext = pNext;

		VkDescriptorPool Pool;
		VULKAN_SUCCESS_ASSERT(vkCreateDescriptorPool(device, &PoolInfo, nullptr, &Pool), "Descrptor pool Failed to Create!");
		return Pool;
	}

	VkDescriptorSet _CreateDescSet(VkDevice device, uint32_t Count, VkDescriptorPool Pool, const VkDescriptorSetLayout* pLayout
		, void* pNext = nullptr)
	{
		VkDescriptorSetAllocateInfo AllocInfo{};
		AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfo.descriptorPool = Pool;
		AllocInfo.descriptorSetCount = Count;;
		AllocInfo.pSetLayouts = pLayout;;
		AllocInfo.pNext = pNext;
		VkDescriptorSet Set;

		VULKAN_SUCCESS_ASSERT(vkAllocateDescriptorSets(device, &AllocInfo, &Set), "Descriptor Set Failed to Create!");
		return Set;
	}

	void VulkanBindlessSetManager::Init()
	{
		_InitPool();
		_InitSetLayouts();
		_InitSets();
	}

	void VulkanBindlessSetManager::Free()
	{
		_DelLayouts();
		_DelSets();
		_DelPool();
		_DelBuffers();
	}

	VkDescriptorSetLayoutBinding _FillBindingInfo(uint32_t Binding, uint32_t Count, VkDescriptorType Type, VkShaderStageFlags Stage, VkSampler* pSampler)
	{
		VkDescriptorSetLayoutBinding BindingInfo{};
		BindingInfo.binding = Binding;
		BindingInfo.descriptorCount = Count;
		BindingInfo.descriptorType = Type;
		BindingInfo.stageFlags = Stage;
		BindingInfo.pImmutableSamplers = pSampler;
		return BindingInfo;
	}

	void VulkanBindlessSetManager::CreateMaterialSBO(size_t SizeInBytes, uint32_t MaxIndex)
	{
		_MaterialSBO = Chilli::StorageBuffer::Create(SizeInBytes);
		_MaxMaterialCount = MaxIndex;
		auto device = VulkanUtils::GetLogicalDevice();

		VkDescriptorBufferInfo BufferInfo{};
		BufferInfo.buffer = std::static_pointer_cast<VulkanStorageBuffer>(_MaterialSBO)->GetHandle();
		BufferInfo.offset = 0;
		BufferInfo.range = _MaterialSBO->GetSize();

		VkWriteDescriptorSet write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _MaterialsDataSet.Set,
			.dstBinding = 0, // Texture array binding
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = UniformTypeToVk(ShaderUniformTypes::STORAGE_BUFFER),
			.pBufferInfo = &BufferInfo
		};

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	void VulkanBindlessSetManager::CreateObjectSBO(size_t SizeInBytes, uint32_t MaxIndex)
	{
		_ObjectSBO = Chilli::StorageBuffer::Create(SizeInBytes);
		_MaxObjectCount = MaxIndex;
		auto device = VulkanUtils::GetLogicalDevice();

		VkDescriptorBufferInfo BufferInfo{};
		BufferInfo.buffer = std::static_pointer_cast<VulkanStorageBuffer>(_ObjectSBO)->GetHandle();
		BufferInfo.offset = 0;
		BufferInfo.range = _ObjectSBO->GetSize();

		VkWriteDescriptorSet write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _ObjectDataSet.Set,
			.dstBinding = 0, // Texture array binding
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = UniformTypeToVk(ShaderUniformTypes::STORAGE_BUFFER),
			.pBufferInfo = &BufferInfo
		};

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	bool VulkanBindlessSetManager::Bind(VkCommandBuffer CommandBuffer, VkPipelineLayout Layout)
	{
		return true;
	}

	void VulkanBindlessSetManager::UpdateSceneUBO(void* Data)
	{
		_SceneUniformBuffer->MapData(Data, _SceneUniformBuffer->GetSize());
	}

	void VulkanBindlessSetManager::SetSceneUBO(size_t SceneUBOSize)
	{
		_SceneUniformBuffer = Chilli::UniformBuffer::Create(SceneUBOSize);
		auto device = VulkanUtils::GetLogicalDevice();

		VkDescriptorBufferInfo BufferInfo{};
		BufferInfo.buffer = std::static_pointer_cast<VulkanUniformBuffer>(_SceneUniformBuffer)->GetHandle();
		BufferInfo.offset = 0;
		BufferInfo.range = _SceneUniformBuffer->GetSize();

		VkWriteDescriptorSet write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _SceneDataSet.Set,
			.dstBinding = 0, // Texture array binding
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = UniformTypeToVk(ShaderUniformTypes::UNIFORM_BUFFER),
			.pBufferInfo = &BufferInfo
		};

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	void VulkanBindlessSetManager::UpdateGlobalUBO(void* Data)
	{
		_GlobalUniformBuffer->MapData(Data, _GlobalUniformBuffer->GetSize());
	}

	void VulkanBindlessSetManager::SetGlobalUBO(size_t GlobalUBOSize)
	{
		_GlobalUniformBuffer = Chilli::UniformBuffer::Create(GlobalUBOSize);
		auto device = VulkanUtils::GetLogicalDevice();

		VkDescriptorBufferInfo BufferInfo{};
		BufferInfo.buffer = std::static_pointer_cast<VulkanUniformBuffer>(_GlobalUniformBuffer)->GetHandle();
		BufferInfo.offset = 0;
		BufferInfo.range = _GlobalUniformBuffer->GetSize();

		VkWriteDescriptorSet write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _GlobalUBOSet.Set,
			.dstBinding = 0, // Texture array binding
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = UniformTypeToVk(ShaderUniformTypes::UNIFORM_BUFFER),
			.pBufferInfo = &BufferInfo
		};

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	uint32_t VulkanBindlessSetManager::AddTexture(const std::shared_ptr<Texture>& Tex)
	{
		auto Index = _TextureManager.Add((Tex)->ID());
		// Update the descriptor
		auto device = VulkanUtils::GetLogicalDevice();

		VkDescriptorImageInfo ImageInfo{};
		ImageInfo.imageView = std::static_pointer_cast<VulkanTexture>(Tex)->GetHandle();
		ImageInfo.sampler = nullptr;
		ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _TexSamplersSet.Set,
			.dstBinding = 1, // Texture array binding
			.dstArrayElement = Index,
			.descriptorCount = 1,
			.descriptorType = UniformTypeToVk(ShaderUniformTypes::SAMPLED_IMAGE),
			.pImageInfo = &ImageInfo
		};

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
		return Index;
	}

	void VulkanBindlessSetManager::AddSampler(const std::shared_ptr<Sampler>& Sam)
	{
		auto Index = _SamplerManager.Add((Sam)->ID());
		// Update the descriptor
		auto device = VulkanUtils::GetLogicalDevice();

		VkDescriptorImageInfo ImageInfo{};
		ImageInfo.imageView = VK_NULL_HANDLE;
		ImageInfo.sampler = std::static_pointer_cast<VulkanSampler>(Sam)->GetHandle();
		ImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkWriteDescriptorSet write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _TexSamplersSet.Set,
			.dstBinding = 0, // Sampler array binding
			.dstArrayElement = Index,
			.descriptorCount = 1,
			.descriptorType = UniformTypeToVk(ShaderUniformTypes::SAMPLER),
			.pImageInfo = &ImageInfo
		};

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	void VulkanBindlessSetManager::UpdateMaterial(const Material& Mat, uint32_t Index)
	{
		MaterialShaderData CopyMat{};
		// Turn Texture handle to shader index
		CopyMat.Indicies.x = _TextureManager.GetIndex(Mat.AlbedoTexture);
		CopyMat.Indicies.y = _SamplerManager.GetIndex(Mat.AlbedoSampler);
		CopyMat.AlbedoColor = Mat.AlbedoColor;

		_MaterialSBO->MapData((void*)&CopyMat, sizeof(MaterialShaderData), sizeof(MaterialShaderData) * Index);
		_MaterialManager.Add(Mat.ID, Index);
	}

	void VulkanBindlessSetManager::UpdateObject(const Object& Obj, uint32_t Index)
	{
		ObjectShaderData CopyData{};
		CopyData.TransformationMat = Obj.Transform.TransformationMat;

		_ObjectSBO->MapData((void*)&CopyData, sizeof(ObjectShaderData), sizeof(ObjectShaderData) * Index);
		_ObjectManager.Add(Obj.ID, Index);
	}

	RenderCommandSpec VulkanBindlessSetManager::FromRenderCommand(const RenderCommandSpec& Spec)
	{
		RenderCommandSpec ReturnSpec{};
		ReturnSpec.MaterialIndex = _MaterialManager.GetIndex(Spec.MaterialIndex);
		ReturnSpec.ObjectIndex = _ObjectManager.GetIndex(Spec.ObjectIndex);
		return ReturnSpec;
	}

	uint32_t VulkanBindlessSetManager::GetTextureIndex(const std::shared_ptr<Texture>& Tex)
	{
		return _TextureManager.GetIndex((Tex)->ID());
	}

	void VulkanBindlessSetManager::RemoveTexture(const std::shared_ptr<Texture>& Tex)
	{
	}

	bool VulkanBindlessSetManager::IsTexPresent(const std::shared_ptr<Texture>& Tex) const
	{
		return _TextureManager.IsTexPresent(Tex->ID());
	}

	void VulkanBindlessSetManager::_InitSetLayouts()
	{
		auto device = VulkanUtils::GetLogicalDevice();
		{
			// Global Set(set = 0)
			VkDescriptorSetLayoutBinding GlobalSetBinding = _FillBindingInfo(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL, nullptr);

			VkDescriptorSetLayoutCreateInfo Info{};
			Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			Info.pBindings = &GlobalSetBinding;
			Info.bindingCount = 1;
			Info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			VULKAN_SUCCESS_ASSERT(vkCreateDescriptorSetLayout(device, &Info, nullptr, &_GlobalUBOLayout), "Global UBO Layout failed to create!");
		}
		{
			// Scene Data Set(set = 1)
			VkDescriptorSetLayoutBinding SceneSetBinding = _FillBindingInfo(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL, nullptr);

			VkDescriptorSetLayoutCreateInfo Info{};
			Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			Info.pBindings = &SceneSetBinding;
			Info.bindingCount = 1;
			Info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			VULKAN_SUCCESS_ASSERT(vkCreateDescriptorSetLayout(device, &Info, nullptr, &_SceneUBOLayout), "Scene UBO Layout failed to create!");
		}
		{
			// Texture Samplers Array Set(set = 2)
			std::array< VkDescriptorSetLayoutBinding, 2> TexSamplerSetBindings;

			TexSamplerSetBindings[0] = _FillBindingInfo(0, int(BindingPipelineFeatures::MAX_SAMPLERS), VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_ALL, nullptr);
			TexSamplerSetBindings[1] = _FillBindingInfo(1, int(BindingPipelineFeatures::MAX_TEXTURES), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_ALL, nullptr);

			VkDescriptorSetLayoutCreateInfo Info{};
			Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			Info.pBindings = TexSamplerSetBindings.data();
			Info.bindingCount = 2;
			Info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			std::vector< VkDescriptorBindingFlags> Flags = {
				// For Samplers
				VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,

				// For Samplerd Image
				VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
				VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT,
			};

			VkDescriptorSetLayoutBindingFlagsCreateInfo FlagsInfo{};
			FlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			FlagsInfo.bindingCount = 2;
			FlagsInfo.pBindingFlags = Flags.data();

			Info.pNext = &FlagsInfo;

			VULKAN_SUCCESS_ASSERT(vkCreateDescriptorSetLayout(device, &Info, nullptr, &_TexSamplerUBOLayout), "TexSampler UBO Layout failed to create!");
		}
		{
			// Materials Data Buffer Set(set = 3)
			VkDescriptorSetLayoutBinding MaterialSetBinding = _FillBindingInfo(0, 1,
				UniformTypeToVk(ShaderUniformTypes::STORAGE_BUFFER), VK_SHADER_STAGE_ALL, nullptr);

			VkDescriptorSetLayoutCreateInfo Info{};
			Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			Info.pBindings = &MaterialSetBinding;
			Info.bindingCount = 1;
			Info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			VULKAN_SUCCESS_ASSERT(vkCreateDescriptorSetLayout(device, &Info, nullptr, &_MaterialsUBOLayout), "Materials UBO Layout failed to create!");
		}
		{
			// Objects Data Buffer Set(set = 4)
			VkDescriptorSetLayoutBinding ObjectSetBinding = _FillBindingInfo(0, 1,
				UniformTypeToVk(ShaderUniformTypes::STORAGE_BUFFER), VK_SHADER_STAGE_ALL, nullptr);

			VkDescriptorSetLayoutCreateInfo Info{};
			Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			Info.pBindings = &ObjectSetBinding;
			Info.bindingCount = 1;
			Info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			VULKAN_SUCCESS_ASSERT(vkCreateDescriptorSetLayout(device, &Info, nullptr, &_ObjectsUBOLayout)
				, "Objects UBO Layout failed to create!");
		}
		VulkanPipelineLayoutInfo PipelienInfo{};
		PipelienInfo.Layouts.push_back(_GlobalUBOLayout);
		PipelienInfo.Layouts.push_back(_SceneUBOLayout);
		PipelienInfo.Layouts.push_back(_TexSamplerUBOLayout);
		PipelienInfo.Layouts.push_back(_MaterialsUBOLayout);
		PipelienInfo.Layouts.push_back(_ObjectsUBOLayout);

		VkPushConstantRange PushConstantRange{};
		PushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
		PushConstantRange.size = sizeof(RenderCommandSpec);
		PushConstantRange.offset = 0;
		PipelienInfo.PushConstant = PushConstantRange;

		s_LayoutCache.GetOrCreate(PipelienInfo, 0);

	}

	void VulkanBindlessSetManager::_InitPool()
	{
		auto device = VulkanUtils::GetLogicalDevice();
		_Pool = _CreatePool(device, 100, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT, {
				{ShaderUniformTypes::UNIFORM_BUFFER, 1000},
			{ShaderUniformTypes::SAMPLER, int(BindingPipelineFeatures::MAX_SAMPLERS)},
			{ShaderUniformTypes::SAMPLED_IMAGE, int(BindingPipelineFeatures::MAX_TEXTURES)},
			{ShaderUniformTypes::STORAGE_BUFFER, 1000},
			}, nullptr);
	}

	void VulkanBindlessSetManager::_InitSets()
	{
		auto device = VulkanUtils::GetLogicalDevice();
		_GlobalUBOSet.Set = _CreateDescSet(device, 1, _Pool, &_GlobalUBOLayout, nullptr);
		_SceneDataSet.Set = _CreateDescSet(device, 1, _Pool, &_SceneUBOLayout, nullptr);

		uint32_t variableDescCount = int(BindingPipelineFeatures::MAX_TEXTURES);
		VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{};
		countInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
		countInfo.descriptorSetCount = 1;
		countInfo.pDescriptorCounts = &variableDescCount;

		_TexSamplersSet.Set = _CreateDescSet(device, 1, _Pool, &_TexSamplerUBOLayout, &countInfo);
		_MaterialsDataSet.Set = _CreateDescSet(device, 1, _Pool, &_MaterialsUBOLayout, nullptr);
		_ObjectDataSet.Set = _CreateDescSet(device, 1, _Pool, &_ObjectsUBOLayout, nullptr);
	}

	void VulkanBindlessSetManager::_DelPool()
	{
		auto device = VulkanUtils::GetLogicalDevice();
		vkDestroyDescriptorPool(device, _Pool, nullptr);
	}

	void VulkanBindlessSetManager::_DelSets()
	{
		auto device = VulkanUtils::GetLogicalDevice();
	}

	void VulkanBindlessSetManager::_DelLayouts()
	{
		auto device = VulkanUtils::GetLogicalDevice();
		if (_GlobalUBOLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device, _GlobalUBOLayout, nullptr);
			_GlobalUBOLayout = VK_NULL_HANDLE;
		}
		if (_SceneUBOLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device, _SceneUBOLayout, nullptr);
			_SceneUBOLayout = VK_NULL_HANDLE;
		}
		if (_TexSamplerUBOLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device, _TexSamplerUBOLayout, nullptr);
			_TexSamplerUBOLayout = VK_NULL_HANDLE;
		}
		if (_MaterialsUBOLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device, _MaterialsUBOLayout, nullptr);
			_MaterialsUBOLayout = VK_NULL_HANDLE;
		}
		if (_ObjectsUBOLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device, _ObjectsUBOLayout, nullptr);
			_ObjectsUBOLayout = VK_NULL_HANDLE;
		}
	}

	void VulkanBindlessSetManager::_DelBuffers()
	{
		_ObjectSBO->Destroy();
		_MaterialSBO->Destroy();
		_SceneUniformBuffer->Destroy();
		_GlobalUniformBuffer->Destroy();
	}

	VkPipelineLayout VulkanPipelineLayoutCache::CreatePipelineLayout(const VulkanPipelineLayoutInfo& Info)
	{
		// Include it when creating your pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = (uint32_t)Info.Layouts.size();
		pipelineLayoutInfo.pSetLayouts = Info.Layouts.data(); // your descriptor layout
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &Info.PushConstant;

		VkPipelineLayout pipelineLayout;
		VULKAN_SUCCESS_ASSERT(vkCreatePipelineLayout(VulkanUtils::GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout), "Pipeline Layout Failed to create!");
		return pipelineLayout;
	}

	VkPipelineLayout VulkanPipelineLayoutCache::GetOrCreate(const VulkanPipelineLayoutInfo& Info)
	{
		size_t hash = std::hash<VulkanPipelineLayoutInfo>{}(Info);

		auto it = _LayoutsCache.find(hash);
		if (it != _LayoutsCache.end()) {
			// Hash match - verify with full comparison (handles collisions)
			if (it->second.first == Info) {
				return it->second.second; // Cache hit
			}
		}

		// Cache miss - create new pipeline layout
		VkPipelineLayout newLayout = CreatePipelineLayout(Info);
		_LayoutsCache[hash] = { Info, newLayout };
		return newLayout;
	}

	VkPipelineLayout VulkanPipelineLayoutCache::GetOrCreate(const VulkanPipelineLayoutInfo& Info, uint32_t SetIndex)
	{
		auto it = _LayoutsCache.find(SetIndex);
		if (it != _LayoutsCache.end()) {
			// Hash match - verify with full comparison (handles collisions)
			if (it->second.first == Info) {
				return it->second.second; // Cache hit
			}
		}

		// Cache miss - create new pipeline layout
		VkPipelineLayout newLayout = CreatePipelineLayout(Info);
		_LayoutsCache[SetIndex] = { Info, newLayout };
		return newLayout;
	}

	void VulkanPipelineLayoutCache::Flush()
	{
		if (Count() == 0)
			return;
		for (auto& [HashValue, PairVal] : _LayoutsCache)
			vkDestroyPipelineLayout(VulkanUtils::GetLogicalDevice(), PairVal.second, nullptr);
		_LayoutsCache.clear();
	}

	VulkanPipelineLayoutCache& VulkanPipelineLayoutCache::GetPipelineLayoutCache()
	{
		return s_LayoutCache;
	}
}
