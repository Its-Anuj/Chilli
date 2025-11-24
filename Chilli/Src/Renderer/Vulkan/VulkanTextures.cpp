#include "ChV_PCH.h"
#include "vulkan\vulkan.h"

#include "VulkanConversions.h"

#include "vk_mem_alloc.h"
#include "VulkanDevice.h"
#include "VulkanTextures.h"

namespace Chilli
{
	struct CreateVulkanImageSpec
	{
		int Width;
		int Height;
		VkImageType ImageType;
		VkFormat Format;
		VkImageTiling Tiling;
		VkImageLayout Layout;
		VkImageUsageFlags Usage;
		VkSharingMode SharingMode;
		VkSampleCountFlagBits Samples;
	};

	std::tuple<VkImage, VmaAllocation, VmaAllocationInfo> CreateVulkanImage(VmaAllocator Allocator, const CreateVulkanImageSpec& Spec)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.extent.width = Spec.Width;
		imageInfo.extent.height = Spec.Height;
		imageInfo.extent.depth = 1;
		imageInfo.imageType = Spec.ImageType;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = Spec.Format;
		imageInfo.tiling = Spec.Tiling;
		imageInfo.initialLayout = Spec.Layout;
		imageInfo.usage = Spec.Usage;
		imageInfo.sharingMode = Spec.SharingMode;
		imageInfo.samples = Spec.Samples;

		VmaAllocationCreateInfo imageAllocInfo{};
		imageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		imageAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		VkImage Image;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;

		vmaCreateImage(Allocator, &imageInfo, &imageAllocInfo, &Image, &Allocation, &AllocationInfo);
		return { Image, Allocation, AllocationInfo };
	}

	VkImageView CreateImageView(VkDevice Device, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlag,
		VkImageViewType ViewType)
	{
		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.format = Format;
		info.image = Image;
		info.viewType = ViewType;
		info.subresourceRange.aspectMask = AspectFlag;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;

		VkImageView View;

		if (vkCreateImageView(Device, &info, nullptr, &View) != VK_SUCCESS)
			throw std::runtime_error("[VULKAN]: Image View failed!");;
		return View;
	}

	void VulkanTexture::Init(VkDevice Device, VmaAllocator Allocator, const ImageSpec& Spec, const char* FilePath)
	{
		_Spec = Spec;
		_FilePath = FilePath;

		_CreateImage(Allocator, Spec);
		_CreateImageView(Device, Spec);
	}

	void VulkanTexture::Terminate(VkDevice Device, VmaAllocator Allocator)
	{
		_DestroyImageView(Device);
		_DestroyImage(Allocator);
	}

	void VulkanTexture::_CreateImage(VmaAllocator Allocator,const ImageSpec& Spec)
	{
		_Spec = Spec;

		if (_Spec.Resolution.Width == 0 || _Spec.Resolution.Height == 0)
			VULKAN_ERROR("Invalid Width or Height Given!");

		CreateVulkanImageSpec Info{};
		Info.Width = _Spec.Resolution.Width;
		Info.Height = _Spec.Resolution.Height;
		Info.Tiling = VK_IMAGE_TILING_OPTIMAL;
		Info.ImageType = ImageTypeToVk(_Spec.Type);
		Info.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		Info.Format = FormatToVk(_Spec.Format);
		Info.Samples = VK_SAMPLE_COUNT_1_BIT;
		Info.SharingMode = VK_SHARING_MODE_EXCLUSIVE;

		Info.Usage = ImageUsageToVk(Spec.Usage);

		auto [Image, Allocation, AllocInfo] = CreateVulkanImage(Allocator, Info);
		_Image = Image;
		_Allocation = Allocation;
		_AllocationInfo = AllocInfo;
	}

	void VulkanTexture::_DestroyImage(VmaAllocator Allocator)
	{
		vmaDestroyImage(Allocator, _Image, _Allocation);
	}

	void VulkanTexture::_CreateImageView(VkDevice Device, const ImageSpec& Spec)
	{
		VkImageAspectFlags AspectFlag;
		if (_Spec.Usage & IMAGE_USAGE_SAMPLED_IMAGE || _Spec.Usage & IMAGE_USAGE_COLOR_ATTACHMENT)
			AspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
		if (_Spec.Usage & IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT)
			AspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

		_ImageView = CreateImageView(Device, _Image,FormatToVk(_Spec.Format), AspectFlag
			, VK_IMAGE_VIEW_TYPE_2D);
	}

	void VulkanTexture::_DestroyImageView(VkDevice Device)
	{
		vkDestroyImageView(Device, _ImageView, nullptr);
	}

	VkSampler VulkanSampler::Create(VkDevice Device, float MaxAnisotropy, const Sampler & SamplerInfo)
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = SamplerFilterToVk(SamplerInfo.Filter);
		samplerInfo.minFilter = SamplerFilterToVk(SamplerInfo.Filter);

		samplerInfo.addressModeU = SamplerModeToVk(SamplerInfo.Mode);
		samplerInfo.addressModeV = SamplerModeToVk(SamplerInfo.Mode);
		samplerInfo.addressModeW = SamplerModeToVk(SamplerInfo.Mode);

		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = MaxAnisotropy;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		VkSampler Sampler;

		if (vkCreateSampler(Device, &samplerInfo, nullptr, &Sampler) != VK_SUCCESS)
			throw std::runtime_error("failed to create texture sampler!");
		return Sampler;
	}

	void VulkanSampler::Destroy(VkDevice Device, VkSampler Sampler)
	{
		vkDestroySampler(Device, Sampler, nullptr);
	}
}
