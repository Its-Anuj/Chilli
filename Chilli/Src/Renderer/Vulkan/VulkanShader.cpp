#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#include "vk_mem_alloc.h"
#include "VulkanRenderer.h"
#include "VulkanResources.h"
#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VulkanUtils.h"
#include "spirv_reflect.h"

namespace Chilli
{
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
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = stageFlag;
				layoutBinding.pImmutableSamplers = nullptr;

				ReflectedUniformInput UniformInfo{};
				UniformInfo.Binding = refl.binding;

				if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
					UniformInfo.Type = ShaderUniformTypes::UNIFORM;
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

	void VulkanGraphicsPipeline::Init(VkDevice Device, VkFormat SwapChainFormat, const PipelineSpec& Spec)
	{
		std::vector<char> VertShaderCode, FragShaderCode;
		_ReadFile(Spec.Paths[0].c_str(), VertShaderCode);
		_ReadFile(Spec.Paths[1].c_str(), FragShaderCode);
		// Create shader modules
		VkShaderModule vertShaderModule = _CreateShaderModule(Device, VertShaderCode);
		VkShaderModule fragShaderModule = _CreateShaderModule(Device, FragShaderCode);

		// Shader stages
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);
		shaderStages[0] = _CreateShaderStages(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT, "main");
		shaderStages[1] = _CreateShaderStages(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

		// Calculate Stride
		size_t Stride = 0;

		_FillInfo();

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
		if (Spec.TopologyMode == InputTopologyMode::Triangle_List)
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		else if (Spec.TopologyMode == InputTopologyMode::Triangle_Strip)
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

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
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		if (Spec.ShaderCullMode == CullMode::None)
			rasterizer.cullMode = VK_CULL_MODE_NONE;
		else if (Spec.ShaderCullMode == CullMode::Back)
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		else if (Spec.ShaderCullMode == CullMode::Front)
			rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

		if (Spec.ShaderFillMode == FillMode::Fill)
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

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
		renderingInfo.pColorAttachmentFormats = &SwapChainFormat;

		if (Spec.EnableDepthStencil) {
			if (Spec.DepthFormat == ImageFormat::D24_S8)
				renderingInfo.depthAttachmentFormat = VK_FORMAT_D24_UNORM_S8_UINT;
			else if (Spec.DepthFormat == ImageFormat::D32)
				renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
			else if (Spec.DepthFormat == ImageFormat::D32_S8)
				renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pSetLayouts = _Info.SetLayouts.data();
		pipelineLayoutInfo.setLayoutCount = (uint32_t)_Info.SetLayouts.size();

		vkCreatePipelineLayout(Device, &pipelineLayoutInfo, nullptr, &_PipelineLayout);

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
		pipelineInfo.layout = _PipelineLayout;
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

	void VulkanGraphicsPipeline::Destroy(VkDevice Device)
	{
		for (auto layout : _Info.SetLayouts)
			vkDestroyDescriptorSetLayout(Device, layout, nullptr);
		vkDestroyPipelineLayout(Device, _PipelineLayout, nullptr);
		vkDestroyPipeline(Device, _Pipeline, nullptr);
	}

	void VulkanGraphicsPipeline::ReCreate(VkDevice Device, VkFormat SwapChainFormat, const PipelineSpec& Spec)
	{
		if (_Pipeline != VK_NULL_HANDLE)
			Destroy(Device);
		Init(Device, SwapChainFormat, Spec);
	}

	void VulkanGraphicsPipeline::_FillInfo()
	{
		_Info = ReflectShaderModule(VulkanUtils::GetLogicalDevice(), "vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		auto FragInfo = ReflectShaderModule(VulkanUtils::GetLogicalDevice(), "frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		_Info.Bindings.reserve(_Info.Bindings.size() + FragInfo.Bindings.size());

		for (auto& i : FragInfo.Bindings)
			_Info.Bindings.push_back(i);

		std::vector<VkDescriptorSetLayout> Layouts;

		std::vector<int> UniqueSetNums;

		for (auto& BindingInfo : _Info.Bindings)
		{
			if (UniqueSetNums.size() == 0)
				UniqueSetNums.push_back(BindingInfo.SetLayoutIndex);
			else
			{
				if (std::find(UniqueSetNums.begin(), UniqueSetNums.end(), BindingInfo.SetLayoutIndex) ==
					UniqueSetNums.end())
				{
					UniqueSetNums.push_back(BindingInfo.SetLayoutIndex);
				}
			}
		}

		for (auto SetLayoutIndex : UniqueSetNums)
		{
			std::vector< VkDescriptorSetLayoutBinding> Bindings;
			for (auto& BindingInfo : _Info.Bindings)
			{
				if (BindingInfo.SetLayoutIndex == SetLayoutIndex)
				{
					VkDescriptorSetLayoutBinding layoutBinding{};
					layoutBinding.binding = BindingInfo.Binding;
					layoutBinding.descriptorCount = BindingInfo.Count;
					layoutBinding.pImmutableSamplers = nullptr;

					if (BindingInfo.Type == ShaderUniformTypes::SAMPLED_IMAGE)
						layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					if (BindingInfo.Type == ShaderUniformTypes::UNIFORM)
						layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

					if (BindingInfo.Stage == ShaderStageType::VERTEX)
						layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
					if (BindingInfo.Stage == ShaderStageType::FRAGMENT)
						layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

					Bindings.push_back(layoutBinding);
				}
			}

			// Create descriptor set layout for this set
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = (uint32_t)Bindings.size();
			layoutInfo.pBindings = Bindings.data();

			VkDescriptorSetLayout setLayout;
			if (vkCreateDescriptorSetLayout(VulkanUtils::GetLogicalDevice(), &layoutInfo, nullptr, &setLayout) != VK_SUCCESS)
				throw std::runtime_error("Failed to create descriptor set layout!");
			_Info.SetLayouts.push_back(setLayout);
		}
	}
}
