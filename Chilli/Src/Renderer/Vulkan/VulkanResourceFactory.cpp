#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "VulkanUtils.h"
#include "VulkanRenderer.h"
#include "VulkanResourceFactory.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Chilli
{
	VulkanResourceFactory::VulkanResourceFactory(const VulkanResourceFactorySpec& Spec)
	{
		this->_Device = Spec.Device;
		this->_SwapChain = Spec.SwapChain;
		VULKAN_PRINTLN("Resource Factory Created!")
	}

	void VulkanResourceFactory::Destroy()
	{
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
		VulkanSpec.Allocator = VulkanUtils::GetAllocator();

		return std::make_shared<VulkanVertexBuffer>(VulkanSpec);
	}

	void VulkanResourceFactory::DestroyVertexBuffer(Ref<VertexBuffer>& VB)
	{
		auto VulkanVB = std::static_pointer_cast<VulkanVertexBuffer>(VB);
		VulkanVB->Destroy(VulkanUtils::GetAllocator());
	}

	Ref<IndexBuffer> VulkanResourceFactory::CreateIndexBuffer(const IndexBufferSpec& Spec)
	{
		VulkanIndexBufferSpec VulkanSpec{};
		VulkanSpec.Spec = Spec;
		VulkanSpec.Allocator = VulkanUtils::GetAllocator();

		return std::make_shared<VulkanIndexBuffer>(VulkanSpec);
	}

	void VulkanResourceFactory::DestroyIndexBuffer(Ref<IndexBuffer>& IB)
	{
		auto VulkanIB = std::static_pointer_cast<VulkanIndexBuffer>(IB);
		VulkanIB->Destroy(VulkanUtils::GetAllocator());
	}

	Ref<UniformBuffer> VulkanResourceFactory::CreateUniformBuffer(const UniformBufferSpec& Spec)
	{
		VulkanUniformBufferrSpec VulkanSpec{};
		VulkanSpec.Spec = Spec;
		VulkanSpec.Allocator = VulkanUtils::GetAllocator();

		return std::make_shared<VulkanUniformBuffer>(VulkanSpec);
	}

	void VulkanResourceFactory::DestroyUniformBuffer(Ref<UniformBuffer>& IB)
	{
		auto VulkanIB = std::static_pointer_cast<VulkanUniformBuffer>(IB);
		VulkanIB->Destroy(VulkanUtils::GetAllocator());
	}

	std::shared_ptr<Texture> VulkanResourceFactory::CreateTexture(TextureSpec& Spec)
	{
		VulkanImageSpec VulkanSpec{};
		VulkanSpec.Spec = Spec;

		return std::make_shared<VulkanTexture>(VulkanUtils::GetAllocator(), _Device->GetHandle(), VulkanSpec,
			_Device->GetPhysicalDevice()->Info.properties.limits.maxSamplerAnisotropy);
	}

	void VulkanResourceFactory::DestroyTexture(Ref<Texture>& Tex)
	{
		auto VulkanIB = std::static_pointer_cast<VulkanTexture>(Tex);
		VulkanIB->Destroy(VulkanUtils::GetAllocator(), this->_Device->GetHandle());
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
		_Device = Spec.Device;
		Init(Spec);
	}

	void VulkanGraphicsPipeline::Destroy(VkDevice Device)
	{
		vkDestroyDescriptorSetLayout(Device, _Layout, nullptr);
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
		_CreateLayout(Spec.Device, Spec.Spec.UniformAttribs);;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &_Layout;
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

		std::vector<VkDescriptorSetLayout> layouts(1, _Layout);
		VulkanUtils::CreateDescSets(_Sets, 1, layouts);

		MakeUniformWork(Spec.Spec.UBs, Spec.Spec.Texs);
	}

	void VulkanGraphicsPipeline::MakeUniformWork(const std::vector<std::shared_ptr<UniformBuffer>>& UB, const std::shared_ptr<Texture>& Texs)
	{
		std::vector< VkDescriptorBufferInfo> BufferInfos;
		BufferInfos.reserve(UB.size());

		for (int i = 0; i < UB.size(); i++)
		{
			auto VulkanIB = std::static_pointer_cast<VulkanUniformBuffer>(UB[i]);
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = VulkanIB->GetHandle();
			bufferInfo.offset = 0;
			bufferInfo.range = VulkanIB->GetSize();
			BufferInfos.push_back(bufferInfo);
		}

		std::vector< VkDescriptorImageInfo > ImageInfos;
		ImageInfos.reserve(1);

		auto VulkanTex = std::static_pointer_cast<VulkanTexture>(Texs);
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = VulkanTex->GetHandle();  // VkImageView
		imageInfo.sampler = VulkanTex->GetSampler();    // VkSampler
		ImageInfos.push_back(imageInfo);

		std::vector< VkWriteDescriptorSet> Sets;

		VkWriteDescriptorSet BufferDescriptorWrite{};
		BufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		BufferDescriptorWrite.dstSet = _Sets[0];
		BufferDescriptorWrite.dstBinding = 0;
		BufferDescriptorWrite.dstArrayElement = 0;
		BufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		BufferDescriptorWrite.descriptorCount = BufferInfos.size();
		BufferDescriptorWrite.pBufferInfo = BufferInfos.data();
		Sets.push_back(BufferDescriptorWrite);

		VkWriteDescriptorSet ImageDescriptorWrite{};
		ImageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		ImageDescriptorWrite.dstSet = _Sets[0];
		ImageDescriptorWrite.dstBinding = 1;
		ImageDescriptorWrite.dstArrayElement = 0;
		ImageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		ImageDescriptorWrite.descriptorCount = ImageInfos.size();
		ImageDescriptorWrite.pImageInfo = ImageInfos.data();
		Sets.push_back(ImageDescriptorWrite);

		vkUpdateDescriptorSets(_Device, static_cast<uint32_t>(Sets.size()), Sets.data(), 0, nullptr);
	}

	void VulkanGraphicsPipeline::TestSPIRV(const char* Path)
	{

	}

	VkDescriptorType UniformTypeToVk(ShaderUniformTypes Type)
	{
		switch (Type)
		{
		case ShaderUniformTypes::UNIFORM:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		};
	}

	VkShaderStageFlags ShaderStageTypeToVk(ShaderStageType Type)
	{
		switch (Type)
		{
		case ShaderStageType::VERTEX:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStageType::FRAGMENT:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		};
	}

	void VulkanGraphicsPipeline::_CreateLayout(VkDevice Device, std::vector< ShaderUnifromAttrib> UniformAttribs)
	{
		std::vector< VkDescriptorSetLayoutBinding> Bindings;
		Bindings.reserve(UniformAttribs.size());

		for (auto& Attrib : UniformAttribs)
		{
			VkDescriptorSetLayoutBinding Binding{};
			Binding.binding = Attrib.Binding;
			Binding.descriptorType = UniformTypeToVk(Attrib.Type);
			Binding.descriptorCount = 1;
			Binding.stageFlags = ShaderStageTypeToVk(Attrib.StageType);
			Binding.pImmutableSamplers = nullptr; // Optional
			Bindings.push_back(Binding);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = Bindings.size();
		layoutInfo.pBindings = Bindings.data();

		if (vkCreateDescriptorSetLayout(Device, &layoutInfo, nullptr, &_Layout) != VK_SUCCESS)
			throw std::runtime_error("failed to create descriptor set layout!");
	}

	VulkanUniformBuffer::VulkanUniformBuffer(const VulkanUniformBufferrSpec& Spec)
	{
		Init(Spec);
	}

	void VulkanUniformBuffer::Destroy(VmaAllocator Allocator)
	{
		vmaDestroyBuffer(Allocator, _Buffer, _Allocation);
	}

	void VulkanUniformBuffer::Init(const VulkanUniformBufferrSpec& Spec)
	{
		_Size = Spec.Spec.Size;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = Spec.Spec.Size;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;
		allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		
		auto Allocator = Spec.Allocator;
		vmaCreateBuffer(Allocator, &bufferInfo, &allocInfo, &_Buffer, &_Allocation, &_AllocatoinInfo);
	}

	void VulkanUniformBuffer::StreamData(void* Data, size_t Size)
	{
		memcpy(_AllocatoinInfo.pMappedData, Data, Size);
	}

	struct CreateVulkanImageSpec
	{
		int Width;
		int Height;
		VkImageType ImageType;
		VkFormat Format;
		VkImageTiling Tiling;
		VkImageLayout Layout;
		VkImageUsageFlags Usage;
		VkSharingMode SharingMode;
		VkSampleCountFlagBits Samples;
	};

	struct CreateVulkanImageReturn
	{
		VkImage Image;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocInfo;
	};

	CreateVulkanImageReturn CreateVulkanImage(VmaAllocator Allocator, const CreateVulkanImageSpec& Spec)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.extent.width = Spec.Width;
		imageInfo.extent.height = Spec.Height;
		imageInfo.extent.depth = 1;
		imageInfo.imageType = Spec.ImageType;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = Spec.Format;
		imageInfo.tiling = Spec.Tiling;
		imageInfo.initialLayout = Spec.Layout;
		imageInfo.usage = Spec.Usage;
		imageInfo.sharingMode = Spec.SharingMode;
		imageInfo.samples = Spec.Samples;

		VmaAllocationCreateInfo imageAllocInfo{};
		imageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		imageAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		
		CreateVulkanImageReturn ReturnInfo{};

		vmaCreateImage(Allocator, &imageInfo, &imageAllocInfo, &ReturnInfo.Image, &ReturnInfo.Allocation, &ReturnInfo.AllocInfo);
		return ReturnInfo;
	}

	void _TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = OldLayout;
		barrier.newLayout = NewLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = Image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
		{
			if (NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
		}
		else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	VkImageTiling TilingToVk(ImageTiling Tiling)
	{
		switch (Tiling)
		{
		case ImageTiling::IMAGE_TILING_OPTIONAL:
			return VK_IMAGE_TILING_OPTIMAL;
		};
	}

	VkImageType ImageTypeToVk(ImageType Type)
	{
		switch (Type)
		{
		case ImageType::IMAGE_TYPE_1D:
			return VK_IMAGE_TYPE_1D;
		case ImageType::IMAGE_TYPE_2D:
			return VK_IMAGE_TYPE_2D;
		case ImageType::IMAGE_TYPE_3D:
			return VK_IMAGE_TYPE_3D;
		};
	}

	VkImageViewType ImageViewTypeToVk(ImageType Type)
	{
		switch (Type)
		{
		case ImageType::IMAGE_TYPE_1D:
			return VK_IMAGE_VIEW_TYPE_1D;
		case ImageType::IMAGE_TYPE_2D:
			return VK_IMAGE_VIEW_TYPE_2D;
		case ImageType::IMAGE_TYPE_3D:
			return VK_IMAGE_VIEW_TYPE_3D;
		};
	}

	VkImageLayout ImageLayoutToVk(ImageLayout Layout)
	{
		switch (Layout)
		{
		case ImageLayout::COLOR:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ImageLayout::DEPTH:
			return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		}
	}

	VkFormat FormatToVk(ImageFormat Format)
	{
		switch (Format)
		{
		case ImageFormat::RGBA8:
			return VK_FORMAT_R8G8B8A8_SRGB;
		}
	}

	void VulkanImage::Init(VmaAllocator Allocator, VulkanImageSpec& Spec)
	{
		_Spec = Spec;
		auto& ImgSpec = Spec.Spec;
		CreateVulkanImageSpec Info{};

		Info.Width = ImgSpec.Resolution.Width;
		Info.Height = ImgSpec.Resolution.Height;
		Info.Tiling = TilingToVk(ImgSpec.Tiling);
		Info.ImageType = ImageTypeToVk(ImgSpec.Type);
		Info.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		Info.Format = FormatToVk(ImgSpec.Format);
		Info.Samples = VK_SAMPLE_COUNT_1_BIT;
		Info.SharingMode = VK_SHARING_MODE_EXCLUSIVE;
		Info.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		auto ReturnInfo = CreateVulkanImage(Allocator, Info);
		_Image = ReturnInfo.Image;
		_Allocation = ReturnInfo.Allocation;
		_AllocationInfo = ReturnInfo.AllocInfo;

		if (Spec.Spec.ImageData != nullptr)
			LoadImageData(Spec.Spec.ImageData);
	}

	void VulkanImage::Destroy(VmaAllocator Allocator)
	{
		vmaDestroyImage(Allocator, _Image, _Allocation);
	}

	void VulkanImage::LoadImageData(const void* ImageData)
	{
		VulkanStageBufferSpec StageBufferSpec{};
		StageBufferSpec.Allocator = VulkanUtils::GetAllocator();
		StageBufferSpec.Data = ImageData;
		VkDeviceSize imageSize = _Spec.Spec.Resolution.Width * _Spec.Spec.Resolution.Height * 4;
		StageBufferSpec.Size = imageSize;

		VulkanStageBuffer StageBuffer;
		StageBuffer.Init(StageBufferSpec);

		// Make so Image Data in buffer can be copied to Image
		VkCommandBuffer SingleTime;
		VulkanUtils::BeginSingleTimeCommands(SingleTime, QueueFamilies::TRANSFER);
		_TransitionImageLayout(SingleTime, _Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VulkanUtils::EndSingleTimeCommands(SingleTime, QueueFamilies::TRANSFER);

		// Copy buffer to image
		VulkanUtils::BeginSingleTimeCommands(SingleTime, QueueFamilies::TRANSFER);

		// Define region to copy
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;   // Tightly packed
		region.bufferImageHeight = 0; // Tightly packed

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent.width = _Spec.Spec.Resolution.Width;
		region.imageExtent.height = _Spec.Spec.Resolution.Height;
		region.imageExtent.depth = 1;

		// Issue copy command
		vkCmdCopyBufferToImage(
			SingleTime,
			StageBuffer.GetHandle(),
			_Image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);
		VulkanUtils::EndSingleTimeCommands(SingleTime, QueueFamilies::TRANSFER);

		// Make Image able to be read by shader
		VulkanUtils::BeginSingleTimeCommands(SingleTime, QueueFamilies::TRANSFER);
		_TransitionImageLayout(SingleTime, _Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VulkanUtils::EndSingleTimeCommands(SingleTime, QueueFamilies::TRANSFER);

		StageBuffer.Destroy(VulkanUtils::GetAllocator());
	}

	void VulkanStageBuffer::Init(const VulkanStageBufferSpec& Spec)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = Spec.Size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		vmaCreateBuffer(Spec.Allocator, &bufferInfo, &allocInfo, &_Buffer, &_Allocation, &_AllocatoinInfo);

		// Copy pixel data to staging buffer
		void* data;
		vmaMapMemory(Spec.Allocator, _Allocation, &data);
		memcpy(data, Spec.Data, static_cast<size_t>(Spec.Size));
		vmaUnmapMemory(Spec.Allocator, _Allocation);
	}

	void VulkanStageBuffer::Destroy(VmaAllocator Allocator)
	{
		vmaDestroyBuffer(Allocator, _Buffer, _Allocation);
	}

	void VulkanStageBuffer::Load(void* Data, uint32_t Size)
	{
	}

	void VulkanTexture::Init(VmaAllocator Allocator, VkDevice Device, VulkanImageSpec& Spec, float MaxAnisoTropy)
	{
		// Load image data using stb_image
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(Spec.Spec.FilePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels)
			throw std::runtime_error("failed to load texture image!");

		Spec.Spec.Resolution.Width = texWidth;
		Spec.Spec.Resolution.Height = texHeight;

		_Image = std::make_shared<VulkanImage>();
		_Image->Init(Allocator, Spec);
		_Image->LoadImageData((const void*)pixels);

		stbi_image_free(pixels);

		_CreateImageView(Device, VK_IMAGE_ASPECT_COLOR_BIT);
		_CreateSampler(Device, MaxAnisoTropy);
	}

	void VulkanTexture::Destroy(VmaAllocator Allocator, VkDevice Device)
	{
		_Image->Destroy(Allocator);
		vkDestroyImageView(Device, _ImageView, nullptr);
		vkDestroySampler(Device, _Sampler, nullptr);
	}

	std::shared_ptr<Image>& VulkanTexture::GetImage()
	{
		auto BaseImage = std::static_pointer_cast<Image>(_Image);
		return BaseImage;
	}

	void VulkanTexture::_CreateImageView(VkDevice Device, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.format = FormatToVk(_Image->GetSpec().Format);
		info.image = _Image->GetHandle();
		info.viewType = ImageViewTypeToVk(_Image->GetSpec().Type);
		info.subresourceRange.aspectMask = aspectFlags;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;

		VULKAN_SUCCESS_ASSERT(vkCreateImageView(Device, &info, nullptr, &_ImageView), "[VULKAN]: Image View failed!");
	}

	void VulkanTexture::_CreateSampler(VkDevice Device, float MaxAnisoTropy)
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;

		samplerInfo.maxAnisotropy = MaxAnisoTropy;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(Device, &samplerInfo, nullptr, &_Sampler) != VK_SUCCESS)
			throw std::runtime_error("failed to create texture sampler!");
	}
}
