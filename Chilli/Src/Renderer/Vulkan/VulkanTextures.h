#pragma once

#include "Texture.h"

namespace Chilli
{
	class VulkanTexture
	{
	public:
		VulkanTexture() {}
		~VulkanTexture() {}

		void Init(VkDevice Device, VmaAllocator Allocator, const ImageSpec& Spec, 
			const char* FilePath = nullptr);
		void Terminate(VkDevice Device, VmaAllocator Allocator);
		void LoadImageData();

		VkImageView GetHandle() const { return _ImageView; }
		VkImage GetImageHandle() const { return _Image; }
	private:
		void _CreateImage(VmaAllocator Allocator,const ImageSpec& Spec);
		void _DestroyImage(VmaAllocator Allocator);

		void _CreateImageView(VkDevice Device, const ImageSpec& Spec);
		void _DestroyImageView(VkDevice Device);
	private:
		VkImageView _ImageView;
		VkImage _Image;
		VmaAllocation _Allocation;
		VmaAllocationInfo _AllocationInfo;

		ImageSpec _Spec;
		std::string _FilePath;
	};

	struct VulkanSampler
	{
		VkSampler Handle;

		operator VkSampler() const { return Handle; }

		static VkSampler Create(VkDevice Device, float MaxAnisotropy, const Sampler& SamplerInfo);
		static void Destroy(VkDevice Device, VkSampler Sampler);
	};

}
