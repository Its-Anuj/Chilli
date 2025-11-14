#include "ChV_PCH.h"
#include "vulkan\vulkan.h"

#include "VulkanPipeline.h"
#include "VulkanConversions.h"

#include "spirv_reflect.h"

void _ReadFile(const char* FilePath, std::vector<char>& CharData);
VkShaderModule _CreateShaderModule(VkDevice device, std::vector<char>& code);
VkPipelineShaderStageCreateInfo _CreateShaderStages(VkShaderModule module, VkShaderStageFlagBits type, const char* Name);
VkVertexInputAttributeDescription GetAttributeDescription(int Binding, int Location, int Offset, VkFormat Format);

// Helper to map SPIR-V format to VkFormat
VkFormat SPVFormatToVkFormat(SpvReflectFormat fmt);
int ShaderTypeToSize(Chilli::ShaderObjectTypes Type);
VkFormat ShaderTypeToFormat(Chilli::ShaderObjectTypes Type);
Chilli::ShaderObjectTypes FormatToShaderType(VkFormat format);

// stageFlag = VK_SHADER_STAGE_VERTEX_BIT / FRAGMENT_BIT etc.
ReflectedShaderInfo ReflectShaderModule(VkDevice device, const std::string& path, VkShaderStageFlagBits stageFlag);

namespace Chilli
{
	void VulkanGraphicsPipeline:: Init(const GraphicsPipelineCreateInfo& CreateInfo, VkDevice Device, VkFormat SwapChainFormat)
	{
		_Info = ReflectShaderModule(Device, CreateInfo.VertPath, VK_SHADER_STAGE_VERTEX_BIT);

		std::vector<char> VertShaderCode, FragShaderCode;
		_ReadFile(CreateInfo.VertPath.c_str(), VertShaderCode);
		_ReadFile(CreateInfo.FragPath.c_str(), FragShaderCode);
		// Create shader modules
		VkShaderModule vertShaderModule = _CreateShaderModule(Device, VertShaderCode);
		VkShaderModule fragShaderModule = _CreateShaderModule(Device, FragShaderCode);

		// Shader stages
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);
		shaderStages[0] = _CreateShaderStages(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT, "main");
		shaderStages[1] = _CreateShaderStages(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

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
		inputAssembly.topology = TopologyToVk(CreateInfo.TopologyMode);

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
		rasterizer.frontFace = FrontFaceToVk(CreateInfo.FrontFace);
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.cullMode = CullModeToVk(CreateInfo.ShaderCullMode);
		rasterizer.polygonMode = PolygonModeToVk(CreateInfo.ShaderFillMode);

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

		if (CreateInfo.ColorBlend)
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
;
		if (!CreateInfo.UseSwapChainColorFormat)
			SwapChainFormat = FormatToVk(CreateInfo.ColorFormat);

		renderingInfo.pColorAttachmentFormats = &SwapChainFormat;

		if (CreateInfo.EnableDepthStencil)
			renderingInfo.depthAttachmentFormat = FormatToVk(CreateInfo.DepthFormat);

		VkPipelineLayoutCreateInfo LayoutCreateInfo{};
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		vkCreatePipelineLayout(Device, &LayoutCreateInfo, nullptr, &_PipelineLayout);

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

		if (CreateInfo.EnableDepthStencil)
		{
			// Depth stencil state - THIS IS KEY!
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = CreateInfo.EnableDepthTest;
			depthStencil.depthWriteEnable = CreateInfo.EnableDepthWrite;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = CreateInfo.EnableStencilTest;
			pipelineInfo.pDepthStencilState = &depthStencil;
		}

		vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_Pipeline);

		// Cleanup shader modules
		vkDestroyShaderModule(Device, vertShaderModule, nullptr);
		vkDestroyShaderModule(Device, fragShaderModule, nullptr);
	}

	void VulkanGraphicsPipeline::Terminate(VkDevice Device)
	{
		vkDestroyPipelineLayout(Device, _PipelineLayout, nullptr);
		vkDestroyPipeline(Device, _Pipeline, nullptr);
	}
}

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

int ShaderTypeToSize(Chilli::ShaderObjectTypes Type)
{
	switch (Type)
	{
	case Chilli::ShaderObjectTypes::FLOAT1:
		return sizeof(float) * 1;
	case Chilli::ShaderObjectTypes::FLOAT2:
		return sizeof(float) * 2;
	case Chilli::ShaderObjectTypes::FLOAT3:
		return sizeof(float) * 3;
	case Chilli::ShaderObjectTypes::FLOAT4:
		return sizeof(float) * 4;
	case Chilli::ShaderObjectTypes::INT1:
		return sizeof(int) * 1;
	case Chilli::ShaderObjectTypes::INT2:
		return sizeof(int) * 2;
	case Chilli::ShaderObjectTypes::INT3:
		return sizeof(int) * 3;
	case Chilli::ShaderObjectTypes::INT4:
		return sizeof(int) * 4;
	case Chilli::ShaderObjectTypes::UINT1:
		return sizeof(unsigned int) * 1;
	case Chilli::ShaderObjectTypes::UINT2:
		return sizeof(unsigned int) * 2;
	case Chilli::ShaderObjectTypes::UINT3:
		return sizeof(unsigned int) * 3;
	case Chilli::ShaderObjectTypes::UINT4:
		return sizeof(unsigned int) * 4;
	default:
		return -1;
	}
}

VkFormat ShaderTypeToFormat(Chilli::ShaderObjectTypes Type)
{
	switch (Type)
	{
	case Chilli::ShaderObjectTypes::FLOAT1:
		return VkFormat::VK_FORMAT_R32_SFLOAT;
	case Chilli::ShaderObjectTypes::FLOAT2:
		return VkFormat::VK_FORMAT_R32G32_SFLOAT;
	case Chilli::ShaderObjectTypes::FLOAT3:
		return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	case Chilli::ShaderObjectTypes::FLOAT4:
		return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
	case Chilli::ShaderObjectTypes::INT1:
		return VkFormat::VK_FORMAT_R32_SINT;
	case Chilli::ShaderObjectTypes::INT2:
		return VkFormat::VK_FORMAT_R32G32_SINT;
	case Chilli::ShaderObjectTypes::INT3:
		return VkFormat::VK_FORMAT_R32G32B32_SINT;
	case Chilli::ShaderObjectTypes::INT4:
		return VkFormat::VK_FORMAT_R32G32B32A32_SINT;
	case Chilli::ShaderObjectTypes::UINT1:
		return VkFormat::VK_FORMAT_R32_UINT;
	case Chilli::ShaderObjectTypes::UINT2:
		return VkFormat::VK_FORMAT_R32G32_UINT;
	case Chilli::ShaderObjectTypes::UINT3:
		return VkFormat::VK_FORMAT_R32G32B32_UINT;
	case Chilli::ShaderObjectTypes::UINT4:
		return VkFormat::VK_FORMAT_R32G32B32A32_UINT;
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

Chilli::ShaderObjectTypes FormatToShaderType(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_R32_SFLOAT:
		return Chilli::ShaderObjectTypes::FLOAT1;
	case VK_FORMAT_R32G32_SFLOAT:
		return Chilli::ShaderObjectTypes::FLOAT2;
	case VK_FORMAT_R32G32B32_SFLOAT:
		return Chilli::ShaderObjectTypes::FLOAT3;
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return Chilli::ShaderObjectTypes::FLOAT4;
	case VK_FORMAT_R32_SINT:
		return Chilli::ShaderObjectTypes::INT1;
	case VK_FORMAT_R32G32_SINT:
		return Chilli::ShaderObjectTypes::INT2;
	case VK_FORMAT_R32G32B32_SINT:
		return Chilli::ShaderObjectTypes::INT3;
	case VK_FORMAT_R32G32B32A32_SINT:
		return Chilli::ShaderObjectTypes::INT4;
	case VK_FORMAT_R32_UINT:
		return Chilli::ShaderObjectTypes::UINT1;
	case VK_FORMAT_R32G32_UINT:
		return Chilli::ShaderObjectTypes::UINT2;
	case VK_FORMAT_R32G32B32_UINT:
		return Chilli::ShaderObjectTypes::UINT3;
	case VK_FORMAT_R32G32B32A32_UINT:
		return Chilli::ShaderObjectTypes::UINT4;
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

	spvReflectDestroyShaderModule(&module);
	return info;
}