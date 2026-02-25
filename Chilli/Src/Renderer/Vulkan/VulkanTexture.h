#pragma once

#include "Texture.h"

namespace Chilli
{
	class VulkanImage
	{
	public:
		VulkanImage() {}
		~VulkanImage() {}

		void Init(VmaAllocator Allocator, ImageSpec& Spec, VulkanDataUploader* Uploader);
		void Destroy(VmaAllocator Allocator);

		VkImage GetHandle() const { return _Image; }
		const ImageSpec& GetSpec() const { return _Spec; }

		VkImageLayout GetImageLayout() const { return _Layout; }
		void SetImageLayout(VkImageLayout Layout) { _Layout = Layout; }
		void SetResourceState(ResourceState State) { _Spec.State = State; }
	private:
		VkImage _Image = VK_NULL_HANDLE;
		VmaAllocation _Allocation;
		VmaAllocationInfo _AllocationInfo;
		ImageSpec _Spec;
		VkImageLayout _Layout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	class VulkanSampler
	{
	public:
		VulkanSampler() {}
		~VulkanSampler() {}

		void Init(VkDevice Device, float MaxAnisotropy, const SamplerSpec& SamplerInfo);
		void Destroy(VkDevice Device);

		VkSampler GetHandle() const { return _Sampler; }
		const SamplerSpec& GetInfo() const { return _SamplerInfo; }

	private:
		VkSampler _Sampler = VK_NULL_HANDLE;
		SamplerSpec _SamplerInfo;
	};

	class VulkanTexture
	{
	public:
		VulkanTexture() {}
		~VulkanTexture() {}

		void Init(VkDevice Device, uint32_t ImageHandle, const VulkanImage* Image, TextureSpec& Spec);
		void Destroy(VkDevice Device);

		VkImageView GetHandle() const { return _ImageView; }
		const TextureSpec& GetSpec() const { return _Spec; }
		uint32_t GetImageHandle() { return _ImageHandle; }

	private:
		VkImageView _ImageView = VK_NULL_HANDLE;
		TextureSpec _Spec;
		uint32_t _ImageHandle = UINT32_MAX;
	};

	struct VulkanImageDataManagerSpec
	{
		VulkanDataUploader* Uploader;
		VulkanDevice* Device;
		std::function<VkBuffer(uint32_t)> GetBuffer;
		std::function<uint32_t(const BufferCreateInfo&)> AllocateBuffer;
		std::function<void(uint32_t)> FreeBuffer;
		std::function<void(uint32_t, void*, uint32_t, uint32_t)>MapBufferData;
	};

	struct VulkanImageDataManager
	{
	public:
		VulkanImageDataManager() {}
		~VulkanImageDataManager() {}

		void Init(const VulkanImageDataManagerSpec& Spec);
		void Destroy();

		uint32_t AllocateImage(VmaAllocator Allocator, ImageSpec& Spec);
		void DestroyImage(VmaAllocator Allocator, uint32_t ImageHandle);
		void MapImageData(uint32_t ImageHandle, void* Data, int Width, int Height);
		VulkanImage* GetImage(uint32_t Handle) { return _ImageSet.Get(Handle); }

		uint32_t CreateTexture(VkDevice Device, uint32_t ImageHandle, TextureSpec& Spec);
		void DestroyTexture(VkDevice Device, uint32_t TextureHandle);
		VulkanTexture* GetTexture(uint32_t TextureHandle) { return _TextureSet.Get(TextureHandle); }

		uint32_t GetImageAllocatedCount() { return _ImageSet.GetActiveCount(); }
		uint32_t GetTextureAllocatedCount() { return _TextureSet.GetActiveCount(); }
	private:
		VulkanImageDataManagerSpec _Spec;
		SparseSet<VulkanImage> _ImageSet;
		SparseSet<VulkanTexture> _TextureSet;
		uint32_t _StagingBuffer = UINT32_MAX;
		uint32_t _StagingBufferRange = 1e6;
	};
}
