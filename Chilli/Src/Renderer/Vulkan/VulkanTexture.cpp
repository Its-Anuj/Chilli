#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#include "vk_mem_alloc.h"
#include "VulkanRenderer.h"
#include "VulkanTexture.h"
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
		}
	}

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

		VULKAN_SUCCESS_ASSERT(vkCreateImageView(Device, &info, nullptr, &View), "[VULKAN]: Image View failed!");
		return View;
	}

	void VulkanImage::Init(const ImageSpec& Spec)
	{
		_Spec = Spec;

		if (_Spec.Resolution.Width == 0 || _Spec.Resolution.Height == 0)
			VULKAN_ERROR("Invalid Width or Height Given!");

		CreateVulkanImageSpec Info{};

		Info.Width = _Spec.Resolution.Width;
		Info.Height = _Spec.Resolution.Height;
		Info.Tiling = TilingToVk(_Spec.Tiling);
		Info.ImageType = ImageTypeToVk(_Spec.Type);
		Info.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		Info.Format = FormatToVk(_Spec.Format);
		Info.Samples = VK_SAMPLE_COUNT_1_BIT;
		Info.SharingMode = VK_SHARING_MODE_EXCLUSIVE;
		Info.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		auto [Image, Allocation, AllocInfo] = CreateVulkanImage(VulkanUtils::GetVulkanAllocator().GetAllocator(), Info);
		_Image = Image;
		_Allocation = Allocation;
		_AllocationInfo = AllocInfo;

		if (Spec.ImageData != nullptr)
			LoadImageData(Spec.ImageData);
	}

	void VulkanImage::Destroy()
	{
		if (_Image == VK_NULL_HANDLE)
			return;
		vmaDestroyImage(VulkanUtils::GetVulkanAllocator().GetAllocator(), _Image, _Allocation); 
	}

	void VulkanImage::LoadImageData(void* ImageData)
	{
		VulkanStageBuffer StageBuffer;
		StageBuffer.Init(ImageData, _Spec.Resolution.Width * _Spec.Resolution.Height * 4);

		// Make so Image Data in buffer can be copied to Image

		BeginCommandBufferInfo Info{};
		Info.Family = QueueFamilies::GRAPHICS;
		VulkanUtils::GetPoolManager().BeginSingleTimeCommand(Info);
		_TransitionImageLayout(Info.Buffer, _Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
		VulkanUtils::GetPoolManager().EndSingleTimeCommand(Info);

		// Copy buffer to image
		Info.Family = QueueFamilies::TRANSFER;
		VulkanUtils::GetPoolManager().BeginSingleTimeCommand(Info);

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

		// Issue copy command
		vkCmdCopyBufferToImage(
			Info.Buffer,
			StageBuffer.GetHandle(),
			_Image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);
		VulkanUtils::GetPoolManager().EndSingleTimeCommand(Info);

		// Make Image able to be read by shader
		Info.Family = QueueFamilies::GRAPHICS;
		VulkanUtils::GetPoolManager().BeginSingleTimeCommand(Info);
		_TransitionImageLayout(Info.Buffer, _Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
		VulkanUtils::GetPoolManager().EndSingleTimeCommand(Info);

		StageBuffer.Delete();
	}

	void VulkanImage::LoadImageData(void* ImageData, int Width, int Height)
	{
		if (Width != _Spec.Resolution.Width || Height != _Spec.Resolution.Height)
		{
			Destroy();
			_Spec.ImageData = ImageData;
			_Spec.Resolution = { Width, Height };
			Init(_Spec);
			return;
		}
		LoadImageData(ImageData);
	}

	void VulkanTexture::Init(const TextureSpec& Spec)
	{
		void* pixels = nullptr;

		// Load image data using stb_image
		int texWidth, texHeight, texChannels;
		pixels = (void*)stbi_load(Spec.FilePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels)
			throw std::runtime_error("failed to load texture image!");

		ImageSpec ImgSpec{};
		ImgSpec.Format = ImageFormat::RGBA8;
		ImgSpec.Tiling = Spec.Tiling;
		ImgSpec.Type = Spec.Type;
		ImgSpec.Resolution = {texWidth, texHeight};
		ImgSpec.ImageData = pixels;

		_Image.Init(ImgSpec);

		if (Spec.FilePath != nullptr)
			stbi_image_free((stbi_uc*)pixels);

		_CreateImageView();
		_CreateSampler();
	}

	void VulkanTexture::_CreateImageView()
	{
		_ImageView = CreateImageView(VulkanUtils::GetLogicalDevice(), _Image.GetHandle(),
			FormatToVk(_Image.GetSpec().Format), VK_IMAGE_ASPECT_COLOR_BIT
			, ImageViewTypeToVk(_Image.GetSpec().Type));
	}

	void VulkanTexture::_CreateSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
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

	void VulkanTexture::Destroy()
	{
		vkDestroySampler(VulkanUtils::GetLogicalDevice(), _Sampler, nullptr);
		vkDestroyImageView(VulkanUtils::GetLogicalDevice(), _ImageView, nullptr);
		_Image.Destroy();
	}
}
