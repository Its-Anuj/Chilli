#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_ENABLE_BETA_EXTENSIONS
#include "C:\VulkanSDK\1.3.275.0\Include\vulkan\vulkan.h"
#include "vk_mem_alloc.h"

#include "VulkanRenderer.h"
#include "VulkanTexture.h"
#include "VulkanConversions.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"
#include "VulkanBuffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

	void _TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout, VkImageAspectFlags aspectFlags)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = OldLayout;
		barrier.newLayout = NewLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = Image;
		barrier.subresourceRange.aspectMask = aspectFlags;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
		{
			if (NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			if (NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			}
		}
		else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void VulkanImage::Init(const ImageSpec& Spec)
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
		Info.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;;

		auto [Image, Allocation, AllocInfo] = CreateVulkanImage(VulkanUtils::GetVulkanAllocator().GetAllocator(), Info);
		_Image = Image;
		_Allocation = Allocation;
		_AllocationInfo = AllocInfo;

		if (Spec.ImageData != nullptr)
			LoadImageData(Spec.ImageData);
	}

	void VulkanImage::Destroy()
	{
		vmaDestroyImage(VulkanUtils::GetVulkanAllocator().GetAllocator(), _Image, _Allocation);
	}

	void VulkanImage::LoadImageData(void* Data)
	{
		VulkanStageBuffer StageBuffer;
		StageBuffer.Init(Data, _Spec.Resolution.Width * _Spec.Resolution.Height * 4);

		// Make so Image Data in buffer can be copied to Image

		BeginCommandBufferInfo Info{};
		Info.Family = QueueFamilies::GRAPHICS;
		VulkanUtils::GetPoolManager().BeginSingleTimeCommand(Info);
		_TransitionImageLayout(Info.Buffer, _Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
		VulkanUtils::GetPoolManager().EndSingleTimeCommand(Info);

		// Copy buffer to image
		// Define region to copy
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;   // Tightly packed
		region.bufferImageHeight = 0; // Tightly packed

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent.width = _Spec.Resolution.Width;
		region.imageExtent.height = _Spec.Resolution.Height;
		region.imageExtent.depth = 1;
		VulkanUtils::GetPoolManager().CopyBufferToImage(region, StageBuffer.GetHandle(), _Image);

		// Make Image able to be read by shader
		Info.Family = QueueFamilies::GRAPHICS;
		VulkanUtils::GetPoolManager().BeginSingleTimeCommand(Info);
		_TransitionImageLayout(Info.Buffer, _Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
		VulkanUtils::GetPoolManager().EndSingleTimeCommand(Info);

		StageBuffer.Delete();
	}

	void VulkanTexture::Init(const TextureSpec& Spec)
	{
		_Spec = Spec;

		void* pixels = nullptr;
		int texWidth, texHeight, texChannels;

		ImageSpec ImgSpec{};

		if (Spec.FilePath.c_str() != nullptr)
		{
			stbi_set_flip_vertically_on_load(Spec.YFlip);

			// Load image data using stb_image
			pixels = (void*)stbi_load(Spec.FilePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			if (!pixels)
				throw std::runtime_error("failed to load texture image!");
			ImgSpec.Resolution = { texWidth, texHeight };
		}
		else
		{
			ImgSpec.Resolution = { Spec.Resolution.Width, Spec.Resolution.Height };
		}

		ImgSpec.Format = Spec.Format;
		ImgSpec.Type = Spec.Type;
		ImgSpec.ImageData = pixels;

		_Image.Init(ImgSpec);

		if (Spec.FilePath.empty() != false)	
			stbi_image_free((stbi_uc*)pixels);

		_CreateImageView();
	}

	void VulkanTexture::_CreateImageView()
	{
		VkImageAspectFlags AspectFlag;
		AspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;

		_ImageView = CreateImageView(VulkanUtils::GetLogicalDevice(), _Image.GetHandle(),
			FormatToVk(_Spec.Format), AspectFlag
			, VK_IMAGE_VIEW_TYPE_2D);
	}


	void VulkanTexture::Destroy()
	{
		vkDestroyImageView(VulkanUtils::GetLogicalDevice(), _ImageView, nullptr);
		_Image.Destroy();
	}

	void VulkanSampler::Init(const SamplerSpec& Spec)
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = SamplerFilterToVk(Spec.Filter);
		samplerInfo.minFilter = SamplerFilterToVk(Spec.Filter);

		samplerInfo.addressModeU = SamplerModeToVk(Spec.Mode);
		samplerInfo.addressModeV = SamplerModeToVk(Spec.Mode);
		samplerInfo.addressModeW = SamplerModeToVk(Spec.Mode);

		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = VulkanUtils::GetDevice().GetPhysicalDevice()->Info.features.samplerAnisotropy;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(VulkanUtils::GetLogicalDevice(), &samplerInfo, nullptr, &_Sampler) != VK_SUCCESS)
			throw std::runtime_error("failed to create texture sampler!");
	}

	void VulkanSampler::Destroy()
	{
		vkDestroySampler(VulkanUtils::GetLogicalDevice(), _Sampler, nullptr);
	}
}
