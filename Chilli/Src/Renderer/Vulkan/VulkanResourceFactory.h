#pragma once

#include "ResourceFactory.h"

namespace Chilli
{
	struct VulkanResourceFactorySpec
	{
		VulkanDevice* Device;
		VulkanSwapChainKHR* SwapChain;
		VkInstance Instance;
	};

	struct VulkanGraphicsPipelineSpec
	{
		GraphicsPipelineSpec Spec;
		VkDevice Device;
		VkFormat SwapChainFormat;
	};

	class VulkanGraphicsPipeline : public GraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline(const VulkanGraphicsPipelineSpec& Spec);
		~VulkanGraphicsPipeline() {}

		void Destroy(VkDevice Device);
		void Init(const VulkanGraphicsPipelineSpec& Spec);

		virtual void Bind() override {}

		VkPipeline GetHandle() const { return _Pipeline; }
	private:
		VkPipeline _Pipeline;
		VkPipelineLayout _PipelineLayout;
	};

	struct VulkanVertexBufferSpec
	{
		VertexBufferSpec Spec;
		VmaAllocator Allocator;
	};

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(const VulkanVertexBufferSpec& Spec);
		~VulkanVertexBuffer() {}

		void Destroy(VmaAllocator Allocator);
		void Init(const VulkanVertexBufferSpec& Spec);

		VkBuffer GetHandle() const { return _Buffer; }

		virtual size_t GetCount() const override { return _Count; }
		virtual size_t GetSize() const override { return _AllocatoinInfo.size; }
	private:
		VkBuffer _Buffer;
		VmaAllocation _Allocation;
		VmaAllocationInfo _AllocatoinInfo;
		size_t _Count = 0;
	};

	struct VulkanIndexBufferSpec
	{
		IndexBufferSpec Spec;
		VmaAllocator Allocator;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(const VulkanIndexBufferSpec& Spec);
		~VulkanIndexBuffer() {}

		void Destroy(VmaAllocator Allocator);
		void Init(const VulkanIndexBufferSpec& Spec);

		VkBuffer GetHandle() const { return _Buffer; }

		virtual size_t GetCount() const override { return _Count; }
		virtual size_t GetSize() const override { return _AllocatoinInfo.size; }
	private:
		VkBuffer _Buffer;
		VmaAllocation _Allocation;
		VmaAllocationInfo _AllocatoinInfo;
		size_t _Count = 0;
	};

	class VulkanResourceFactory : public ResourceFactory
	{
	public:
		VulkanResourceFactory(const VulkanResourceFactorySpec& Spec);
		virtual ~VulkanResourceFactory() {}

		void Destroy();

		virtual Ref<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineSpec& Spec) override;
		virtual void DestroyGraphicsPipeline(Ref<GraphicsPipeline>& Pipeline) override;

		virtual Ref<VertexBuffer> CreateVertexBuffer(const VertexBufferSpec& Spec) override;
		virtual void DestroyVertexBuffer(Ref<VertexBuffer>& VB) override;

		virtual Ref<IndexBuffer> CreateIndexBuffer(const IndexBufferSpec& Spec) override;
		virtual void DestroyIndexBuffer(Ref<IndexBuffer>& IB) override;
	private:
		VmaAllocator _Allocator;
		VulkanDevice* _Device;
		VulkanSwapChainKHR* _SwapChain;
	};
} // namespace VEngine
