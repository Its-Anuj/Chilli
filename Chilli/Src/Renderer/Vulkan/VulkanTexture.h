#pragma once

#include "Texture.h"

namespace Chilli
{
	class VulkanImage : public Image
	{
	public:
		VulkanImage() {}
		~VulkanImage() {}

		void Init(const ImageSpec& Spec);
		void Destroy();

		virtual const ImageSpec& GetSpec() const override { return _Spec; }
		virtual void LoadImageData(void* ImageData) override;
		virtual void LoadImageData(void* ImageData, int Width, int Height) override;

		VkImage GetHandle() const { return _Image; }
	private:
		ImageSpec _Spec;
		VkImage _Image = VK_NULL_HANDLE;
		VmaAllocation _Allocation = VK_NULL_HANDLE;
		VmaAllocationInfo _AllocationInfo;
	};

	class VulkanTexture : public Texture
	{
	public:
		VulkanTexture(const TextureSpec& Spec) { Init(Spec); }
		VulkanTexture() {}
		~VulkanTexture() {}

		void Init(const TextureSpec& Spec);
		void Destroy();

		const TextureSpec& GetSpec() const override { return _Spec; }
		VkImageView GetHandle() const { return _ImageView; }
		VkImage GetImageHandle() const { return _Image.GetHandle(); }
		VkSampler GetSampler() const { return _Sampler; }
	private:
		void _CreateImageView();
		void _CreateSampler(const TextureSpec& Spec);
	private:
		TextureSpec _Spec;
		VulkanImage _Image;
		VkImageView _ImageView = VK_NULL_HANDLE;
		VkSampler _Sampler = VK_NULL_HANDLE;
	};
}
