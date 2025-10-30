#pragma once

#include "Texture.h"
#include "Samplers.h"

namespace Chilli
{
	class VulkanImage : public Image
	{
	public:
		VulkanImage(const ImageSpec& Spec) { Init(Spec); }
		VulkanImage() {}
		~VulkanImage() {}

		virtual void Init(const ImageSpec& Spec) override;
		virtual void Destroy() override;

		void LoadImageData(void* Data);

		VkImage GetHandle() const { return _Image; }
	private:
		ImageSpec _Spec;
		VkImage _Image;
		VmaAllocation _Allocation;
		VmaAllocationInfo _AllocationInfo;
	};

	class VulkanTexture : public Texture
	{
	public:
		VulkanTexture(const TextureSpec& Spec) { Init(Spec); }
		VulkanTexture() {}
		~VulkanTexture() {}

		virtual void Init(const TextureSpec& Spec) override;
		virtual void Destroy() override;

		virtual const TextureSpec& GetSpec() const { return _Spec; }
		virtual void* GetNativeHandle() const override { return (void*)_ImageView; }

		virtual VkImageView GetHandle() const { return _ImageView; }

	private:
		void _CreateImageView();
	private:
		VkImageView _ImageView;
		VulkanImage _Image;
		TextureSpec _Spec;
	};

	class VulkanSampler : public Sampler
	{
	public:
		VulkanSampler(const SamplerSpec& Spec) { Init(Spec); }
		VulkanSampler() {}
		~VulkanSampler() {}

		virtual void Init(const SamplerSpec& Spec) override;
		virtual void Destroy() override;

		virtual const SamplerSpec& GetSpec() const override { return _Spec; }
		virtual void* GetNativeHandle() const override { return (void*)_Sampler; }
		virtual VkSampler GetHandle() const { return _Sampler; }
	private:
		SamplerSpec _Spec;
		VkSampler _Sampler;
	};
}
