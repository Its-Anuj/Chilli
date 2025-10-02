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
		virtual void LinkUniformBuffer(std::shared_ptr<UniformBuffer>& UB) override;

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

		void _CreateLayout(VkDevice Device);
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
		uint32_t GetSize() const { return _AllocatoinInfo.size; }

		virtual void StreamData(void* Data, size_t Size) override;
	private:
		VkBuffer _Buffer;
		VmaAllocation _Allocation;
		VmaAllocationInfo _AllocatoinInfo;
		size_t _Size = 0;
	};

	struct VulkanImageSpec
	{
		TextureSpec Spec;
	};

	class VulkanImage : public Image
	{
	public:
		VulkanImage() {}
		~VulkanImage() {}

		void Init(VmaAllocator Allocator, const VulkanImageSpec& Spec);
		void Destroy(VmaAllocator Allocator);

		virtual const ImageSpec& GetSpec() const override { return _Spec.Spec; }
		virtual void LoadImageData(void* ImageData) override;

		VkImage GetHandle() const { return _Image; }
	private:
		VulkanImageSpec _Spec;
		VkImage _Image;
		VmaAllocation _Allocation;
		VmaAllocationInfo _AllocationInfo;
	};

	class VulkanTexture : public Texture
	{
	public:
		VulkanTexture() {}
		~VulkanTexture() {}

		void Init(VmaAllocator Allocator, VkDevice Device, const VulkanImageSpec& Spec, float MaxAnisoTropy);
		void Destroy(VmaAllocator Allocator, VkDevice Device);
		  
		virtual const ImageSpec& GetSpec() const override { return _Image->GetSpec(); }
		virtual Ref<Image>& GetImage() override;

		VkImageView GetHandle() const;
	private:
		void _CreateImageView(VkDevice Device, VkImageAspectFlags aspectFlags);
		void _CreateSampler(VkDevice Device, float MaxAnisoTropy);
	private:
		std::shared_ptr<VulkanImage> _Image;
		VkImageView _ImageView;
		VkSampler _Sampler;
	};

	struct VulkanStageBuffer
	{
	public:
		VulkanStageBuffer() {}
		~VulkanStageBuffer() {}
	
		void Init(VmaAllocator Allocator, void* Data, uint32_t Size);
		void Destroy(VmaAllocator Allocator);
		void Load(void* Data, uint32_t Size);

		VkBuffer GetHandle() { return _Buffer; }
	private:
		VmaAllocation _Allocation;
		VkBuffer _Buffer;
		VmaAllocationInfo _AllocatoinInfo;
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

		virtual Ref<Texture> CreateTexture(const TextureSpec& Spec) override;
		virtual void DestroyTexture(Ref<Texture>& Tex) override;
	private:
		VulkanDevice* _Device;
		VulkanSwapChainKHR* _SwapChain;
	};
} // namespace VEngine
