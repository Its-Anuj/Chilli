#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "VulkanRenderer.h"
#include "VulkanResourceFactory.h"

namespace Chilli
{
	VulkanResourceFactory::VulkanResourceFactory(const VulkanResourceFactorySpec& Spec)
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = Spec.Device->GetPhysicalDevice()->PhysicalDevice;
		allocatorInfo.device = Spec.Device->GetHandle();
		allocatorInfo.instance = Spec.Instance;
		_Device = Spec.Device;
		_SwapChain = Spec.SwapChain;

		VULKAN_SUCCESS_ASSERT(vmaCreateAllocator(&allocatorInfo, &_Allocator), "VMA Allocator failed to initialize!");
		VULKAN_PRINTLN("VMA Created!!");
		VULKAN_PRINTLN("Resource Factory Created!")
	}

	void VulkanResourceFactory::Destroy()
	{
		vmaDestroyAllocator(_Allocator);
		VULKAN_PRINT("Resource Factory Desytoyed!")
	}

	Ref<GraphicsPipeline> VulkanResourceFactory::CreateGraphicsPipeline(const GraphicsPipelineSpec& Spec)
	{
		VulkanGraphicsPipelineSpec VulkanSpec{};
		VulkanSpec.Spec = Spec;
		VulkanSpec.Device = this->_Device->GetHandle();
		VulkanSpec.SwapChainFormat = _SwapChain->GetFormat();

		return std::make_shared<VulkanGraphicsPipeline>(VulkanSpec);
	}

	void VulkanResourceFactory::DestroyGraphicsPipeline(Ref<GraphicsPipeline>& Pipeline)
	{
		auto VulkanPipeline = std::static_pointer_cast<VulkanGraphicsPipeline>(Pipeline);
		VulkanPipeline->Destroy(_Device->GetHandle());
	}

	Ref<VertexBuffer> VulkanResourceFactory::CreateVertexBuffer(const VertexBufferSpec& Spec)
	{
		VulkanVertexBufferSpec VulkanSpec{};
		VulkanSpec.Spec = Spec;
		VulkanSpec.Allocator = _Allocator;

		return std::make_shared<VulkanVertexBuffer>(VulkanSpec);
	}

	void VulkanResourceFactory::DestroyVertexBuffer(Ref<VertexBuffer>& VB)
	{
		auto VulkanVB = std::static_pointer_cast<VulkanVertexBuffer>(VB);
		VulkanVB->Destroy(_Allocator);
	}

	Ref<IndexBuffer> VulkanResourceFactory::CreateIndexBuffer(const IndexBufferSpec& Spec)
	{
		VulkanIndexBufferSpec VulkanSpec{};
		VulkanSpec.Spec = Spec;
		VulkanSpec.Allocator = _Allocator;

		return std::make_shared<VulkanIndexBuffer>(VulkanSpec);
	}

	void VulkanResourceFactory::DestroyIndexBuffer(Ref<IndexBuffer>& IB)
	{
		auto VulkanIB = std::static_pointer_cast<VulkanIndexBuffer>(IB);
		VulkanIB->Destroy(_Allocator);
	}

	VulkanVertexBuffer::VulkanVertexBuffer(const VulkanVertexBufferSpec& Spec)
	{
		Init(Spec);
	}

	void VulkanVertexBuffer::Init(const VulkanVertexBufferSpec& Spec)
	{
		_Count = Spec.Spec.Count;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = Spec.Spec.Size;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto Allocator = Spec.Allocator;
		vmaCreateBuffer(Allocator, &bufferInfo, &allocInfo, &_Buffer, &_Allocation, &_AllocatoinInfo);

		// Copy pixel data to staging buffer
		void* data;
		vmaMapMemory(Allocator, _Allocation, &data);
		memcpy(data, Spec.Spec.Data, static_cast<size_t>(Spec.Spec.Size));
		vmaUnmapMemory(Allocator, _Allocation);
	}

	void VulkanVertexBuffer::Destroy(VmaAllocator Allocator)
	{
		vmaDestroyBuffer(Allocator, _Buffer, _Allocation);
	}

	VulkanIndexBuffer::VulkanIndexBuffer(const VulkanIndexBufferSpec& Spec)
	{
		Init(Spec);
	}

	void VulkanIndexBuffer::Destroy(VmaAllocator Allocator)
	{
		vmaDestroyBuffer(Allocator, _Buffer, _Allocation);
	}

	void VulkanIndexBuffer::Init(const VulkanIndexBufferSpec& Spec)
	{
		_Count = Spec.Spec.Count;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = Spec.Spec.Size;
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto Allocator = Spec.Allocator;
		vmaCreateBuffer(Allocator, &bufferInfo, &allocInfo, &_Buffer, &_Allocation, &_AllocatoinInfo);

		// Copy pixel data to staging buffer
		void* data;
		vmaMapMemory(Allocator, _Allocation, &data);
		memcpy(data, Spec.Spec.Data, static_cast<size_t>(Spec.Spec.Size));
		vmaUnmapMemory(Allocator, _Allocation);
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
			break;
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
		}
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

	VulkanGraphicsPipeline::VulkanGraphicsPipeline(const VulkanGraphicsPipelineSpec& Spec)
	{
		Init(Spec);
	}

	void VulkanGraphicsPipeline::Destroy(VkDevice Device)
	{
		vkDestroyPipelineLayout(Device, _PipelineLayout, nullptr);
		vkDestroyPipeline(Device, _Pipeline, nullptr);
	}

	void VulkanGraphicsPipeline::Init(const VulkanGraphicsPipelineSpec& Spec)
	{
		std::vector<char> VertShaderCode, FragShaderCode;
		_ReadFile(Spec.Spec.Paths[0].c_str(), VertShaderCode);
		_ReadFile(Spec.Spec.Paths[1].c_str(), FragShaderCode);
		// Create shader modules
		VkShaderModule vertShaderModule = _CreateShaderModule(Spec.Device, VertShaderCode);
		VkShaderModule fragShaderModule = _CreateShaderModule(Spec.Device, FragShaderCode);

		// Shader stages
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);
		shaderStages[0] = _CreateShaderStages(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT, "main");
		shaderStages[1] = _CreateShaderStages(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

		struct BindingDataSpec
		{
			int Stride, Binding;

			BindingDataSpec(int pStride, int pBinding) : Stride(pStride), Binding(pBinding) {}
		};

		std::vector<BindingDataSpec> BindingData;
		std::vector<VkVertexInputAttributeDescription> AttributeDescriptions;
		AttributeDescriptions.reserve(Spec.Spec.Attribs.size());

		// Calculate Stride
		size_t Stride = 0;
		for (auto& attrib : Spec.Spec.Attribs)
			Stride += ShaderTypeToSize(attrib.Type);

		int Offset = 0;
		for (auto& attrib : Spec.Spec.Attribs)
		{
			AttributeDescriptions.push_back(GetAttributeDescription(attrib.Binding, attrib.Location, Offset, ShaderTypeToFormat(attrib.Type)));
			Offset += ShaderTypeToSize(attrib.Type);

			if (BindingData.size() == 0)
				BindingData.push_back({ ShaderTypeToSize(attrib.Type), attrib.Binding });
			else
			{
				bool FoundBinding = false;
				for (auto& BindData : BindingData)
					if (BindData.Binding == attrib.Binding)
					{
						BindData.Stride += ShaderTypeToSize(attrib.Type);
						FoundBinding = true;
					}

				if (!FoundBinding)
					BindingData.push_back({ ShaderTypeToSize(attrib.Type), attrib.Binding });
			}
		}

		// 🆕 UPDATED: Vertex input descriptions for the new shader
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		bindingDescriptions.reserve(BindingData.size());

		for (auto& BindData : BindingData)
		{
			VkVertexInputBindingDescription bindingdesc{};
			bindingdesc.binding = BindData.Binding;
			bindingdesc.stride = BindData.Stride;
			bindingdesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			bindingDescriptions.push_back(bindingdesc);
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = AttributeDescriptions.size();
		vertexInputInfo.pVertexAttributeDescriptions = AttributeDescriptions.data();

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

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
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

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
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

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
		renderingInfo.pColorAttachmentFormats = &Spec.SwapChainFormat;

		// Create pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		vkCreatePipelineLayout(Spec.Device, &pipelineLayoutInfo, nullptr, &_PipelineLayout);

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

		vkCreateGraphicsPipelines(Spec.Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_Pipeline);

		// Cleanup shader modules
		vkDestroyShaderModule(Spec.Device, vertShaderModule, nullptr);
		vkDestroyShaderModule(Spec.Device, fragShaderModule, nullptr);
	}
}
