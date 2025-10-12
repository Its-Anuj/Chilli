#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#include "vk_mem_alloc.h"
#include "VulkanRenderer.h"
#include "VulkanResources.h"
#include "VulkanShader.h"
#include "VulkanBuffer.h"
#include "VulkanUtils.h"
#include "VulkanTexture.h"

namespace Chilli
{
	void VulkanResourceFactory::Init(const VulkanResourceFactorySpec& Spec)
	{
		_Spec = Spec;
	}

	void VulkanResourceFactory::ShutDown()
	{

	}

	std::shared_ptr<GraphicsPipeline> VulkanResourceFactory::CreateGraphicsPipeline(const PipelineSpec& Spec)
	{
		auto VulkanShader = std::make_shared<VulkanGraphicsPipeline>(_Spec.Device.GetHandle(), _Spec.SwapChainFormat, Spec);
		return VulkanShader;
	}

	void VulkanResourceFactory::DestroyGraphicsPipeline(std::shared_ptr<GraphicsPipeline>& Pipeline)
	{
		auto VulkanShader = std::static_pointer_cast<VulkanGraphicsPipeline>(Pipeline);
		VulkanShader->Destroy(_Spec.Device.GetHandle());
	}

	void VulkanResourceFactory::ReCreateGraphicsPipeline(std::shared_ptr<GraphicsPipeline>& Pipeline, const PipelineSpec& Spec)
	{
		auto VulkanShader = std::static_pointer_cast<VulkanGraphicsPipeline>(Pipeline);
		VulkanShader->ReCreate(_Spec.Device.GetHandle(), _Spec.SwapChainFormat, Spec);
	}

	std::shared_ptr<VertexBuffer> VulkanResourceFactory::CreateVertexBuffer(const VertexBufferSpec& Spec)
	{
		auto VulkanVB = std::make_shared<VulkanVertexBuffer>(Spec);
		return VulkanVB;
	}

	void VulkanResourceFactory::DestroyVertexBuffer(std::shared_ptr<VertexBuffer>& VB)
	{
		auto VulkanVB = std::static_pointer_cast<VulkanVertexBuffer>(VB);
		VulkanVB->Delete();
	}

	std::shared_ptr<IndexBuffer> VulkanResourceFactory::CreateIndexBuffer(const IndexBufferSpec& Spec)
	{
		auto VulkanIB = std::make_shared<VulkanIndexBuffer>(Spec);
		return VulkanIB;
	}

	void VulkanResourceFactory::DestroyIndexBuffer(std::shared_ptr<IndexBuffer>& IB)
	{
		auto VulkanIB = std::static_pointer_cast<VulkanIndexBuffer>(IB);
		VulkanIB->Delete();
	}

	std::shared_ptr<UniformBuffer> VulkanResourceFactory::CreateUniformBuffer(size_t Size)
	{
		auto VulkanUB = std::make_shared<VulkanUniformBuffer>(Size);
		return VulkanUB;
	}

	void VulkanResourceFactory::DestroyUniformBuffer(std::shared_ptr<UniformBuffer>& UB)
	{
		auto VulkanUB = std::static_pointer_cast<VulkanUniformBuffer>(UB);
		VulkanUB->Delete();
	}

	std::shared_ptr<Texture> VulkanResourceFactory::CreateTexture(const TextureSpec& Spec)
	{
		auto VulkanTex = std::make_shared<VulkanTexture>(Spec);
		return VulkanTex;
	}

	void VulkanResourceFactory::DestroyTexture(std::shared_ptr<Texture>& Tex)
	{
		auto VulkanTex = std::static_pointer_cast<VulkanTexture>(Tex);
		VulkanTex->Destroy();
	}

	VulkanMaterialBackend::VulkanMaterialBackend(const std::shared_ptr<GraphicsPipeline>& Shader)
	{
		auto VulkanPipeleine = std::static_pointer_cast<VulkanGraphicsPipeline>(Shader);
		std::vector<VkDescriptorSet> Sets;
		std::vector<VkDescriptorSetLayout> Layouts = { VulkanPipeleine->GetPSetLayout() };
		VulkanUtils::GetPoolManager().CreateDescSets(Sets, 1, Layouts);
		_Set = Sets[0];
	}

	VulkanMaterialBackend::~VulkanMaterialBackend()
	{
		std::vector<VkDescriptorSet> Sets = { _Set };
		VulkanUtils::GetPoolManager().FreeDescSets(Sets);
	}

	void VulkanMaterialBackend::Update(const std::shared_ptr<GraphicsPipeline>& Shader,
		const std::unordered_map<std::string, std::shared_ptr<UniformBuffer>>& UBOs
		, const std::unordered_map<std::string, std::shared_ptr<Texture>>& Textures)
	{
		auto VulkanPipeleine = std::static_pointer_cast<VulkanGraphicsPipeline>(Shader);
		auto& Info = VulkanPipeleine->GetInfo();

		std::vector < VkWriteDescriptorSet > WriteSets;
		WriteSets.reserve(Info.Bindings.size());

		std::vector < VkDescriptorBufferInfo > BufferInfos{};
		BufferInfos.reserve(UBOs.size());
		std::vector < VkDescriptorImageInfo> ImageInfos{};
		ImageInfos.reserve(UBOs.size());

		for (auto& BindingInfo : Info.Bindings)
		{
			VkWriteDescriptorSet WriteInfo{};

			if (BindingInfo.Type == ShaderUniformTypes::UNIFORM)
			{
				VkDescriptorBufferInfo BufferInfo{};
				const auto& Buffer = UBOs.at(BindingInfo.Name);
				BufferInfo.buffer = std::static_pointer_cast<VulkanUniformBuffer>(Buffer)->GetHandle();

				BufferInfo.offset = 0;
				BufferInfo.range = Buffer->GetSize();

				WriteInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				BufferInfos.push_back(BufferInfo);
				WriteInfo.pBufferInfo = &BufferInfos[BufferInfos.size() - 1];
			}
			if (BindingInfo.Type == ShaderUniformTypes::SAMPLED_IMAGE)
			{
				if (Textures.size() > 0) {

					VkDescriptorImageInfo ImageInfo{};
					ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					const auto& Texture = Textures.at(BindingInfo.Name);
					auto VulkanTex = std::static_pointer_cast<VulkanTexture>(Texture);
					ImageInfo.imageView = VulkanTex->GetHandle();
					ImageInfo.sampler = VulkanTex->GetSampler();    // VkSampler

					WriteInfo.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					ImageInfos.push_back(ImageInfo);
					WriteInfo.pImageInfo = &ImageInfos[ImageInfos.size() - 1];
				}
			}

			WriteInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteInfo.dstSet = _Set;;
			WriteInfo.dstBinding = BindingInfo.Binding;
			WriteInfo.dstArrayElement = 0;
			WriteInfo.descriptorCount = 1;

			WriteSets.push_back(WriteInfo);
		}


		auto Device = VulkanUtils::GetLogicalDevice();
		vkUpdateDescriptorSets(Device, static_cast<uint32_t>(WriteSets.size()), WriteSets.data(), 0, nullptr);
	}
}
