#include "ChV_PCH.h"
#include "vulkan\vulkan.h"

#include "VulkanPipeline.h"
#include "UUID\UUID.h"
#include "VulkanConversions.h"
#include <unordered_set>
#include <algorithm>

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

void CombineVertFragUniformReflect(std::vector<ReflectedSetUniformInput>& UniformInputs,
	std::vector< ReflectedSetUniformInput>& VertInfo, std::vector< ReflectedSetUniformInput>& FragInfo);

namespace Chilli
{
	void VulkanGraphicsPipeline::Init(const GraphicsPipelineCreateInfo& CreateInfo, VkDevice Device, VkFormat SwapChainFormat, VulkanPipelineLayoutManager& LayoutManager,
		const std::array<VkDescriptorSetLayout, int(BindlessSetTypes::COUNT_NON_USER)>& BindlessLayouts)
	{
		_Info = ReflectShaderModule(Device, CreateInfo.VertPath, VK_SHADER_STAGE_VERTEX_BIT);
		auto FragInfo = ReflectShaderModule(Device, CreateInfo.FragPath, VK_SHADER_STAGE_FRAGMENT_BIT);

		std::vector<ReflectedSetUniformInput> UniformInputs;
		CombineVertFragUniformReflect(UniformInputs, _Info.UniformInputs, FragInfo.UniformInputs);
		_Info.UniformInputs = UniformInputs;

		for (auto& SetInfo : _Info.UniformInputs)
			if (SetInfo.Set >= int(BindlessSetTypes::COUNT_NON_USER))
			{
				// Create Personal Set Layouts
				_PersonalSetLayoutUsed = true;
			}

		// We Discard the Vertex Input info and Push Constant from Fragment Shader
		PipelineLayoutInfo LayoutInfo = { _Info.PushConstants, _Info.UniformInputs };
		LayoutInfo.sort();

		std::array<VkDescriptorSetLayout, int(BindlessSetTypes::COUNT)> SetLayouts;
		for (auto& Layout : SetLayouts)
			Layout = VK_NULL_HANDLE;

		for (int i = 0; i < BindlessLayouts.size(); i++)
			SetLayouts[i] = BindlessLayouts[i];

		// Fow now every be null
		_LayoutIndex = LayoutManager.Create(Device, LayoutInfo, SetLayouts);

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
		pipelineInfo.renderPass = VK_NULL_HANDLE; // No render pass with dynamic rendering
		pipelineInfo.subpass = 0;

		auto Layout = LayoutManager.Get(_LayoutIndex);
		pipelineInfo.layout = Layout;

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
		vkDestroyPipeline(Device, _Pipeline, nullptr);
	}

	uint32_t VulkanPipelineLayoutManager::Create(VkDevice Device, const PipelineLayoutInfo& Info,
		const std::array<VkDescriptorSetLayout, int(BindlessSetTypes::COUNT)>& BindlessLayouts)
	{
		// If already created, return existing index
		auto it = _LayoutMap.find(Info);
		if (it != _LayoutMap.end())
			return it->second;

		uint32_t ReturnIndex = _Layouts.size();

		VkPipelineLayout Layout;

		VkPipelineLayoutCreateInfo LayoutCreateInfo{};
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		LayoutCreateInfo.pPushConstantRanges = nullptr;
		LayoutCreateInfo.pushConstantRangeCount = Info.PushConstants.size();
		if (LayoutCreateInfo.pushConstantRangeCount > 0)
			LayoutCreateInfo.pPushConstantRanges = Info.PushConstants.data();

		std::vector<VkDescriptorSetLayout> pSetlayouts;
		for (auto& Layout : BindlessLayouts)
			if (Layout != VK_NULL_HANDLE)
				pSetlayouts.push_back(Layout);

		LayoutCreateInfo.setLayoutCount = pSetlayouts.size();
		LayoutCreateInfo.pSetLayouts = pSetlayouts.data();

		auto res = vkCreatePipelineLayout(Device, &LayoutCreateInfo, nullptr, &Layout);
		assert(res == VK_SUCCESS);

		_Layouts.emplace_back(Layout);
		_LayoutMap[Info] = ReturnIndex;
		return ReturnIndex;
	}

	VkPipelineLayout VulkanPipelineLayoutManager::Get(uint32_t Index)
	{
		assert(Index < _Layouts.size());
		return _Layouts[Index];
	}

	void VulkanPipelineLayoutManager::Destroy(VkDevice Device, uint32_t Index)
	{
		vkDestroyPipelineLayout(Device, _Layouts[Index], nullptr);
		_Layouts[Index] = nullptr;
	}

	void VulkanPipelineLayoutManager::Clear(VkDevice Device)
	{
		for (auto Layout : _Layouts)
			vkDestroyPipelineLayout(Device, Layout, nullptr);
		_Layouts.clear();
	}

	void PipelineLayoutInfo::sort()
	{
		std::sort(PushConstants.begin(), PushConstants.end(),
			[](const VkPushConstantRange& a, const VkPushConstantRange& b) {
				return std::tie(a.stageFlags, a.offset, a.size) <
					std::tie(b.stageFlags, b.offset, b.size);
			});

		std::sort(UniformInputs.begin(), UniformInputs.end(),
			[](const ReflectedSetUniformInput& a, const ReflectedSetUniformInput& b) {
				return a.Set < b.Set;
			});

		for (auto& set : UniformInputs) {
			std::sort(set.Bindings.begin(), set.Bindings.end(),
				[](const ReflectedBindingUniformInput& a, const ReflectedBindingUniformInput& b) {
					return a.Binding < b.Binding;
				});
		}
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

Chilli::ShaderUniformTypes SpirvTypeToChilli(SpvReflectDescriptorType Type)
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
			rb.Type = SpirvTypeToChilli(b->descriptor_type);
			rb.Stage = Chilli::VkShaderStageToChilli(stageFlag);

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
		VkPushConstantRange range{};
		range.offset = 0;
		range.size = pc->size;
		range.stageFlags = stageFlag;
		info.PushConstants.push_back(range);
	}

	spvReflectDestroyShaderModule(&module);
	return info;
}

void CombineVertFragUniformReflect(std::vector<ReflectedSetUniformInput>& UniformInputs, std::vector<ReflectedSetUniformInput>& VertInfo, std::vector<ReflectedSetUniformInput>& FragInfo)
{
	// Combine sets from both vertex and fragment shaders
	std::unordered_set<uint32_t> allSets;
	for (auto& vertSet : VertInfo) allSets.insert(vertSet.Set);
	for (auto& fragSet : FragInfo) allSets.insert(fragSet.Set);

	// Process each set
	for (uint32_t setIndex : allSets)
	{
		ReflectedSetUniformInput combinedSet;
		combinedSet.Set = setIndex;

		// Find this set in vertex and fragment shaders
		auto vertSetIt = std::find_if(VertInfo.begin(), VertInfo.end(),
			[setIndex](const ReflectedSetUniformInput& set) { return set.Set == setIndex; });

		auto fragSetIt = std::find_if(FragInfo.begin(), FragInfo.end(),
			[setIndex](const ReflectedSetUniformInput& set) { return set.Set == setIndex; });

		// Combine bindings from both shaders
		std::unordered_set<uint32_t> allBindings;

		if (vertSetIt != VertInfo.end())
		{
			for (auto& binding : vertSetIt->Bindings) allBindings.insert(binding.Binding);
		}

		if (fragSetIt != FragInfo.end())
		{
			for (auto& binding : fragSetIt->Bindings) allBindings.insert(binding.Binding);
		}

		// Process each binding in this set
		for (uint32_t bindingIndex : allBindings)
		{
			ReflectedBindingUniformInput combinedBinding;
			combinedBinding.Binding = bindingIndex;
			combinedBinding.Set = setIndex;

			// Look for this binding in vertex shader
			ReflectedBindingUniformInput* vertBinding = nullptr;
			if (vertSetIt != VertInfo.end())
			{
				auto it = std::find_if(vertSetIt->Bindings.begin(), vertSetIt->Bindings.end(),
					[bindingIndex](const ReflectedBindingUniformInput& binding) {
						return binding.Binding == bindingIndex;
					});
				if (it != vertSetIt->Bindings.end()) vertBinding = &(*it);
			}

			// Look for this binding in fragment shader
			ReflectedBindingUniformInput* fragBinding = nullptr;
			if (fragSetIt != FragInfo.end())
			{
				auto it = std::find_if(fragSetIt->Bindings.begin(), fragSetIt->Bindings.end(),
					[bindingIndex](const ReflectedBindingUniformInput& binding) {
						return binding.Binding == bindingIndex;
					});
				if (it != fragSetIt->Bindings.end()) fragBinding = &(*it);
			}

			// Combine the binding information
			if (vertBinding && fragBinding)
			{
				// Binding exists in both shaders - combine stage flags
				// Prefer vertex shader name, or could check if they match
				strncpy(combinedBinding.Name, vertBinding->Name, SHADER_UNIFORM_BINDING_NAME_SIZE - 1);
				combinedBinding.Name[SHADER_UNIFORM_BINDING_NAME_SIZE - 1] = '\0';

				combinedBinding.Type = vertBinding->Type;
				combinedBinding.Count = vertBinding->Count;
				combinedBinding.Stage = Chilli::ShaderStageType::ALL_GRAPHICS; // Combined stage
			}
			else if (vertBinding)
			{
				// Binding only in vertex shader
				combinedBinding = *vertBinding;
				combinedBinding.Stage = Chilli::ShaderStageType::VERTEX;
			}
			else if (fragBinding)
			{
				// Binding only in fragment shader
				combinedBinding = *fragBinding; // ok for POD
				// but ensure Name is copied safely:
				char tempName[SHADER_UNIFORM_BINDING_NAME_SIZE];
				strncpy(tempName, fragBinding->Name, SHADER_UNIFORM_BINDING_NAME_SIZE);
				memcpy(combinedBinding.Name, tempName, SHADER_UNIFORM_BINDING_NAME_SIZE);

				combinedBinding.Stage = Chilli::ShaderStageType::FRAGMENT;
			}

			combinedSet.Bindings.push_back(combinedBinding);
		}

		// Sort bindings by binding index for consistency
		std::sort(combinedSet.Bindings.begin(), combinedSet.Bindings.end(),
			[](const ReflectedBindingUniformInput& a, const ReflectedBindingUniformInput& b) {
				return a.Binding < b.Binding;
			});

		UniformInputs.push_back(combinedSet);
	}

	// Sort sets by set index for consistency
	std::sort(UniformInputs.begin(), UniformInputs.end(),
		[](const ReflectedSetUniformInput& a, const ReflectedSetUniformInput& b) {
			return a.Set < b.Set;
		});

}
