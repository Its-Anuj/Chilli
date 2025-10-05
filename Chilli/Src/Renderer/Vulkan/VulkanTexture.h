#pragma once

namespace Chilli
{
	struct VulkanImageSpec
	{
		TextureSpec Spec;
	};

	class VulkanImage : public Image
	{
	public:
		VulkanImage(VmaAllocator Allocator, VulkanImageSpec& Spec) { Init(Allocator, Spec); }
		VulkanImage() {}
		~VulkanImage() {}

		void Init(VmaAllocator Allocator, VulkanImageSpec& Spec);
		void Destroy(VmaAllocator Allocator);

		virtual const ImageSpec& GetSpec() const override { return _Spec.Spec; }
		virtual void LoadImageData(const void* ImageData) override;

		VkImage GetHandle() const { return _Image; }
	private:
		VulkanImageSpec _Spec;
		VkImage _Image = VK_NULL_HANDLE;
		VmaAllocation _Allocation = VK_NULL_HANDLE;
		VmaAllocationInfo _AllocationInfo{};
	};

	class VulkanTexture : public Texture
	{
	public:
		VulkanTexture(VmaAllocator Allocator, VkDevice Device, VulkanImageSpec& Spec, float MaxAnisoTropy) {
			Init(Allocator, Device, Spec, MaxAnisoTropy);
		}
		~VulkanTexture() {}

		void Init(VmaAllocator Allocator, VkDevice Device, VulkanImageSpec& Spec, float MaxAnisoTropy);
		void Destroy(VmaAllocator Allocator, VkDevice Device);

		virtual const ImageSpec& GetSpec() const override { return _Image->GetSpec(); }
		virtual Ref<Image>& GetImage() override;

		VkImageView GetHandle() const { return _ImageView; }
		VkSampler GetSampler() const { return _Sampler; }
	private:
		void _CreateImageView(VkDevice Device, VkImageAspectFlags aspectFlags);
		void _CreateSampler(VkDevice Device, float MaxAnisoTropy);
	private:
		std::shared_ptr<VulkanImage> _Image;
		VkImageView _ImageView = VK_NULL_HANDLE;
		VkSampler _Sampler = VK_NULL_HANDLE;
	};
}
