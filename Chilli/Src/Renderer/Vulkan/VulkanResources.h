#pragma once

#include "Resources.h"

namespace Chilli
{
	struct VulkanMaterialBackend : public MaterialBackend
	{
	public:
		VulkanMaterialBackend(const std::shared_ptr<GraphicsPipeline>& Shader);
		~VulkanMaterialBackend();

		virtual void Update(const std::shared_ptr<GraphicsPipeline>& Shader,
			const std::unordered_map<std::string, std::shared_ptr<UniformBuffer>>& UBOs,
			const std::unordered_map<std::string, std::shared_ptr<Texture>>& Textures) override;

		VkDescriptorSet GetGlobalSet() const { return _GlobalSet; }
		VkDescriptorSet GetSet() const { return _Set; }
	private:
		VkDescriptorSet _GlobalSet ;// Having Set Index 0
		VkDescriptorSet _Set; // Having Set Index 1
	};

	struct VulkanResourceFactorySpec
	{
		VulkanDevice Device;
		VkFormat SwapChainFormat;
	};

	class VulkanResourceFactory : public ResourceFactory
	{
	public:
		VulkanResourceFactory() {}
		~VulkanResourceFactory(){}

		void Init(const  VulkanResourceFactorySpec& Spec);
		void ShutDown();

		virtual std::shared_ptr <GraphicsPipeline> CreateGraphicsPipeline(const PipelineSpec& Spec) override; 
		virtual void DestroyGraphicsPipeline(std::shared_ptr<GraphicsPipeline>& Pipeline) override;
		virtual void ReCreateGraphicsPipeline(std::shared_ptr <GraphicsPipeline>& Pipeline, const PipelineSpec& Spec) override;

		virtual std::shared_ptr <VertexBuffer> CreateVertexBuffer(const VertexBufferSpec& Spec) override;	
		virtual void DestroyVertexBuffer(std::shared_ptr<VertexBuffer>& VB) override;

		virtual std::shared_ptr <IndexBuffer> CreateIndexBuffer(const IndexBufferSpec& Spec) override; 
		virtual void DestroyIndexBuffer(std::shared_ptr<IndexBuffer>& IB) override;

		virtual std::shared_ptr <UniformBuffer> CreateUniformBuffer(size_t Size) override;
		virtual void DestroyUniformBuffer(std::shared_ptr<UniformBuffer>& UB) override;

		virtual std::shared_ptr <Texture> CreateTexture(const TextureSpec& Spec) override; 
		virtual void DestroyTexture(std::shared_ptr<Texture>& Tex) override;
	private:
		VulkanResourceFactorySpec _Spec;
	};
}
