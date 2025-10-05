#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#include "vk_mem_alloc.h"
#include "VulkanUtils.h"
#include "VulkanRenderer.h"
#include "VulkanResourceFactory.h"

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

	struct CreateVulkanImageReturn
	{
		VkImage Image;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocInfo;
	};

	VkImageTiling TilingToVk(ImageTiling Tiling)
	{
		switch (Tiling)
		{
		case ImageTiling::IMAGE_TILING_OPTIONAL:
			return VK_IMAGE_TILING_OPTIMAL;
		};
	}

	VkImageType ImageTypeToVk(ImageType Type)
	{
		switch (Type)
		{
		case ImageType::IMAGE_TYPE_1D:
			return VK_IMAGE_TYPE_1D;
		case ImageType::IMAGE_TYPE_2D:
			return VK_IMAGE_TYPE_2D;
		case ImageType::IMAGE_TYPE_3D:
			return VK_IMAGE_TYPE_3D;
		};
	}

	VkImageViewType ImageViewTypeToVk(ImageType Type)
	{
		switch (Type)
		{
		case ImageType::IMAGE_TYPE_1D:
			return VK_IMAGE_VIEW_TYPE_1D;
		case ImageType::IMAGE_TYPE_2D:
			return VK_IMAGE_VIEW_TYPE_2D;
		case ImageType::IMAGE_TYPE_3D:
			return VK_IMAGE_VIEW_TYPE_3D;
		};
	}

	VkImageLayout ImageLayoutToVk(ImageLayout Layout)
	{
		switch (Layout)
		{
		case ImageLayout::COLOR:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ImageLayout::DEPTH:
			return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		}
	}

	VkFormat FormatToVk(ImageFormat Format)
	{
		switch (Format)
		{
		case ImageFormat::RGBA8:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case ImageFormat::D32_FLOAT:
			return VK_FORMAT_D32_SFLOAT;
		}
	}

	VkImageAspectFlags ImageAspectToVk(ImageAspect Aspect)
	{
		switch (Aspect)
		{
		case ImageAspect::COLOR:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		case ImageAspect::DEPTH:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		}
	}

	CreateVulkanImageReturn CreateVulkanImage(VmaAllocator Allocator, const CreateVulkanImageSpec& Spec)
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

		CreateVulkanImageReturn ReturnInfo{};

		vmaCreateImage(Allocator, &imageInfo, &imageAllocInfo, &ReturnInfo.Image, &ReturnInfo.Allocation, &ReturnInfo.AllocInfo);
		return ReturnInfo;
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

	void VulkanImage::Init(VmaAllocator Allocator, VulkanImageSpec& Spec)
	{
		_Spec = Spec;
		auto& ImgSpec = Spec.Spec;
		CreateVulkanImageSpec Info{};

		Info.Width = ImgSpec.Resolution.Width;
		Info.Height = ImgSpec.Resolution.Height;
		Info.Tiling = TilingToVk(ImgSpec.Tiling);
		Info.ImageType = ImageTypeToVk(ImgSpec.Type);
		Info.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		Info.Format = FormatToVk(ImgSpec.Format);
		Info.Samples = VK_SAMPLE_COUNT_1_BIT;
		Info.SharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (Spec.Spec.Aspect == ImageAspect::COLOR)
			Info.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		if (Spec.Spec.Aspect == ImageAspect::DEPTH)
			Info.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		auto ReturnInfo = CreateVulkanImage(Allocator, Info);
		_Image = ReturnInfo.Image;
		_Allocation = ReturnInfo.Allocation;
		_AllocationInfo = ReturnInfo.AllocInfo;

		if (Spec.Spec.ImageData != nullptr)
			LoadImageData(Spec.Spec.ImageData);
	}

	void VulkanImage::Destroy(VmaAllocator Allocator)
	{
		vmaDestroyImage(Allocator, _Image, _Allocation);
	}

	void VulkanImage::LoadImageData(const void* ImageData)
	{
		VulkanStageBufferSpec StageBufferSpec{};
		StageBufferSpec.Allocator = VulkanUtils::GetAllocator();
		StageBufferSpec.Data = ImageData;
		VkDeviceSize imageSize = _Spec.Spec.Resolution.Width * _Spec.Spec.Resolution.Height * 4;
		StageBufferSpec.Size = imageSize;

		VulkanStageBuffer StageBuffer;
		StageBuffer.Init(StageBufferSpec);

		// Make so Image Data in buffer can be copied to Image
		VkCommandBuffer SingleTime;
		VulkanUtils::BeginSingleTimeCommands(SingleTime, QueueFamilies::GRAPHICS);
		_TransitionImageLayout(SingleTime, _Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ImageAspectToVk(_Spec.Spec.Aspect));
		VulkanUtils::EndSingleTimeCommands(SingleTime, QueueFamilies::GRAPHICS);

		// Copy buffer to image
		VulkanUtils::BeginSingleTimeCommands(SingleTime, QueueFamilies::TRANSFER);

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
		region.imageExtent.width = _Spec.Spec.Resolution.Width;
		region.imageExtent.height = _Spec.Spec.Resolution.Height;
		region.imageExtent.depth = 1;

		// Issue copy command
		vkCmdCopyBufferToImage(
			SingleTime,
			StageBuffer.GetHandle(),
			_Image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);
		VulkanUtils::EndSingleTimeCommands(SingleTime, QueueFamilies::TRANSFER);

		// Make Image able to be read by shader
		VulkanUtils::BeginSingleTimeCommands(SingleTime, QueueFamilies::GRAPHICS);
		_TransitionImageLayout(SingleTime, _Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ImageAspectToVk(_Spec.Spec.Aspect));
		VulkanUtils::EndSingleTimeCommands(SingleTime, QueueFamilies::GRAPHICS);

		StageBuffer.Destroy(VulkanUtils::GetAllocator());
	}

	void VulkanTexture::Init(VmaAllocator Allocator, VkDevice Device, VulkanImageSpec& Spec, float MaxAnisoTropy)
	{
		void* pixels = nullptr;

		if (Spec.Spec.FilePath == nullptr)
		{
			pixels = Spec.Spec.ImageData;
		}
		else
		{
			// Load image data using stb_image
			int texWidth, texHeight, texChannels;
			pixels = (void*)stbi_load(Spec.Spec.FilePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			if (!pixels)
				throw std::runtime_error("failed to load texture image!");

			Spec.Spec.Resolution.Width = texWidth;
			Spec.Spec.Resolution.Height = texHeight;
		}

		_Image = std::make_shared<VulkanImage>();
		_Image->Init(Allocator, Spec);

		if (pixels != nullptr)
			_Image->LoadImageData((const void*)pixels);

		if (Spec.Spec.FilePath != nullptr)
			stbi_image_free((stbi_uc*)pixels);

		if (Spec.Spec.Aspect == ImageAspect::DEPTH)
		{
			// Transition Image Layout to Depth
			// Make so Image Data in buffer can be copied to Image
			VkCommandBuffer SingleTime;
			VulkanUtils::BeginSingleTimeCommands(SingleTime, QueueFamilies::GRAPHICS);
			_TransitionImageLayout(SingleTime, _Image->GetHandle(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, ImageAspectToVk(Spec.Spec.Aspect));
			VulkanUtils::EndSingleTimeCommands(SingleTime, QueueFamilies::GRAPHICS);
		}

		_CreateImageView(Device, ImageAspectToVk(Spec.Spec.Aspect));

		if (Spec.Spec.Aspect == ImageAspect::COLOR) {
			_CreateSampler(Device, MaxAnisoTropy);
		}
	}

	void VulkanTexture::Destroy(VmaAllocator Allocator, VkDevice Device)
	{
		_Image->Destroy(Allocator);
		vkDestroyImageView(Device, _ImageView, nullptr);
		if (_Sampler != VK_NULL_HANDLE)
			vkDestroySampler(Device, _Sampler, nullptr);
	}

	std::shared_ptr<Image>& VulkanTexture::GetImage()
	{
		auto BaseImage = std::static_pointer_cast<Image>(_Image);
		return BaseImage;
	}

	void VulkanTexture::_CreateImageView(VkDevice Device, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.format = FormatToVk(_Image->GetSpec().Format);
		info.image = _Image->GetHandle();
		info.viewType = ImageViewTypeToVk(_Image->GetSpec().Type);
		info.subresourceRange.aspectMask = aspectFlags;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;

		VULKAN_SUCCESS_ASSERT(vkCreateImageView(Device, &info, nullptr, &_ImageView), "[VULKAN]: Image View failed!");
	}

	void VulkanTexture::_CreateSampler(VkDevice Device, float MaxAnisoTropy)
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;

		samplerInfo.maxAnisotropy = MaxAnisoTropy;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(Device, &samplerInfo, nullptr, &_Sampler) != VK_SUCCESS)
			throw std::runtime_error("failed to create texture sampler!");
	}

}
