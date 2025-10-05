#pragma once

#include "ResourceFactory.h"
#include "VulkanTexture.h"

namespace Chilli
{
	struct VulkanResourceFactorySpec
	{
		VulkanDevice* Device;
		VulkanSwapChainKHR* SwapChain;
		VkInstance Instance;
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

	struct VulkanUniformBufferrSpec
	{
		UniformBufferSpec Spec;
		VmaAllocator Allocator;
	};

	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(const VulkanUniformBufferrSpec& Spec);
		~VulkanUniformBuffer() {}

		void Destroy(VmaAllocator Allocator);
		void Init(const VulkanUniformBufferrSpec& Spec);

		VkBuffer GetHandle() const { return _Buffer; }
		// Returns ALlocation Size  which is in power of 16 but our may not alwys be
		virtual uint32_t GetSize() const  override { return _AllocatoinInfo.size; }
		// return actual size of the object
		uint32_t GetBufferSize() const  { return _Size; }
		virtual void StreamData(void* Data, size_t Size) override;

		// uint64_t avoids void
		virtual uint64_t GetNativeHandle() const override { return reinterpret_cast<uint64_t>(_Buffer); }

	private:
		VkBuffer _Buffer;
		VmaAllocation _Allocation;
		VmaAllocationInfo _AllocatoinInfo;
		size_t _Size = 0;
	};

	struct VulkanStageBufferSpec
	{
		VmaAllocator Allocator;
		uint32_t Size;
		const void* Data;
	};

	struct VulkanStageBuffer
	{	
	public:
		VulkanStageBuffer() {}
		~VulkanStageBuffer() {}

		void Init(const VulkanStageBufferSpec& Spec);
		void Destroy(VmaAllocator Allocator);
		void Load(void* Data, uint32_t Size);

		VkBuffer GetHandle() { return _Buffer; }
	private:
		VmaAllocation _Allocation;
		VkBuffer _Buffer;
		VmaAllocationInfo _AllocatoinInfo;
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
		virtual void LinkUniformBuffer(const std::shared_ptr<UniformBuffer>& UB) override {}
		void MakeUniformWork(const std::vector<std::shared_ptr<UniformBuffer>>& UB, const std::shared_ptr<Texture>& Texs);

		VkPipeline GetHandle() const { return _Pipeline; }
		VkPipelineLayout GetLayout() { return _PipelineLayout; }
		const std::vector<VkDescriptorSet>& GetSets() { return _Sets; }

		void TestSPIRV(const char* Path);
	private:
		VkPipeline _Pipeline;
		VkPipelineLayout _PipelineLayout;
		VkDevice _Device;

		std::vector<VkDescriptorSet> _Sets;
		VkDescriptorSetLayout _Layout;

		void _CreateLayout(VkDevice Device, std::vector< ShaderUnifromAttrib> UniformAttribs);
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

		virtual Ref<UniformBuffer> CreateUniformBuffer(const UniformBufferSpec& Spec) override;
		virtual void DestroyUniformBuffer(Ref<UniformBuffer>& IB) override;

		virtual Ref<Texture> CreateTexture(TextureSpec& Spec) override;
		virtual void DestroyTexture(Ref<Texture>& Tex) override;
	private:
		VulkanDevice* _Device;
		VulkanSwapChainKHR* _SwapChain;
	};
} // namespace VEngine
