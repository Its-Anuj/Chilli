#include "ChV_PCH.h"
#include "vulkan\vulkan.h"

#include "vk_mem_alloc.h"
#include "VulkanBackend.h"
#include "VulkanConversions.h"

namespace Chilli
{
	struct CreateVulkanImageSpec
	{
		int Width;
		int Height;
		int Depth;
		VkImageType ImageType;
		int MipLevel;
		VkFormat Format;
		VkImageTiling Tiling;
		VkImageLayout Layout;
		VkImageUsageFlags Usage;
		VkSharingMode SharingMode;
		VkSampleCountFlagBits Samples;
		VkImageCreateFlags Flags = 0;
		int ArrayLayers = 1;
	};

	std::tuple<VkImage, VmaAllocation, VmaAllocationInfo> CreateVulkanImage(VmaAllocator Allocator, const CreateVulkanImageSpec& Spec);

	VkImageView CreateImageView(VkDevice Device, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlag,
		VkImageViewType ViewType);

	void VulkanImage::Init(VmaAllocator Allocator, const ImageSpec& Spec, VulkanDataUploader* Uploader)
	{
		_Spec = Spec;

		if (_Spec.Resolution.Width == 0 || _Spec.Resolution.Height == 0)
			VULKAN_ERROR("Invalid Width or Height Given!");

		if (_Image != VK_NULL_HANDLE)
			VULKAN_ERROR("Image Should Be Destroyed!");

		CreateVulkanImageSpec Info{};
		Info.Width = _Spec.Resolution.Width;
		Info.Height = _Spec.Resolution.Height;
		Info.Depth = _Spec.Resolution.Depth;
		Info.Tiling = VK_IMAGE_TILING_OPTIMAL;
		Info.ImageType = ImageTypeToVk(_Spec.Type);
		Info.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		Info.Format = FormatToVk(_Spec.Format);
		Info.Samples = VK_SAMPLE_COUNT_1_BIT;
		Info.SharingMode = VK_SHARING_MODE_EXCLUSIVE;
		Info.MipLevel = Spec.MipLevel;
		Info.ArrayLayers = 1;

		Info.Usage = ImageUsageToVk(Spec.Usage);
		Info.Usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if (Spec.MipLevel > 1)
			Info.Usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		// 1. Enable format reinterpretation (Mutable)
		if (Spec.Format == ImageFormat::RGBA8 || Spec.Format == ImageFormat::SRGBA8) {
			Info.Flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		}

		auto [Image, Allocation, AllocInfo] = CreateVulkanImage(Allocator, Info);
		_Image = Image;
		_Allocation = Allocation;
		_AllocationInfo = AllocInfo;

		VkImageAspectFlags Aspect = FormatToVkAspectMask(Spec.Format);
		// Resolve the correct initial transition target
		VkImageLayout targetLayout = VK_IMAGE_LAYOUT_GENERAL; // Safe fallback
		if (Spec.Usage & IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT)
			targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		else if (Spec.Usage & IMAGE_USAGE_COLOR_ATTACHMENT)
			targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		else if (Spec.Usage & IMAGE_USAGE_SAMPLED_IMAGE)
			targetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		// Transition Image Layout
		Uploader->TransitionImageLayout(_Image, VK_IMAGE_LAYOUT_UNDEFINED, targetLayout, Aspect);
		this->SetImageLayout(targetLayout);
	}

	void VulkanImage::Destroy(VmaAllocator Allocator)
	{
		vmaDestroyImage(Allocator, _Image, _Allocation);
		_Image = VK_NULL_HANDLE;
	}

	void VulkanSampler::Init(VkDevice Device, float MaxAnisotropy, const SamplerSpec& SamplerInfo)
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
		samplerInfo.compareEnable = SamplerInfo.bEnableCompare;
		samplerInfo.compareOp = CompareOpToVk(SamplerInfo.bCompareOp);
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = SamplerInfo.MinLod;
		samplerInfo.maxLod = SamplerInfo.MaxLod;

		if (vkCreateSampler(Device, &samplerInfo, nullptr, &_Sampler) != VK_SUCCESS)
			throw std::runtime_error("failed to create texture sampler!");

		_SamplerInfo = SamplerInfo;
	}

	void VulkanSampler::Destroy(VkDevice Device)
	{
		vkDestroySampler(Device, _Sampler, nullptr);
	}

	void VulkanTexture::Init(VkDevice Device, uint32_t ImageHandle, const VulkanImage* Image, const TextureSpec& Spec)
	{
		auto& ImgSpec = Image->GetSpec();
		_ImageHandle = ImageHandle;

		// 1. Determine Aspect based on the format (Primary source of truth)
		VkImageAspectFlags AspectFlag = 0;
		ImageFormat finalFormat = (Spec.Format != ImageFormat::NONE) ? Spec.Format : ImgSpec.Format;
		AspectFlag = FormatToVkAspectMask(finalFormat, ImgSpec.Usage);

		VkFormat Format = FormatToVk(ImgSpec.Format);
		if (Spec.Format != ImageFormat::NONE && Spec.Format != ImgSpec.Format)
			Format = FormatToVk(Spec.Format);

		_ImageView = CreateImageView(Device, Image->GetHandle(), Format, AspectFlag
			, VK_IMAGE_VIEW_TYPE_2D);
	}

	void VulkanTexture::Destroy(VkDevice Device)
	{
		vkDestroyImageView(Device, _ImageView, nullptr);	
	}

	std::tuple<VkImage, VmaAllocation, VmaAllocationInfo> CreateVulkanImage(VmaAllocator Allocator, const CreateVulkanImageSpec& Spec)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.extent.width = Spec.Width;
		imageInfo.extent.height = Spec.Height;
		imageInfo.extent.depth = Spec.Depth;
		imageInfo.imageType = Spec.ImageType;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = Spec.MipLevel;
		imageInfo.arrayLayers = Spec.ArrayLayers;
		imageInfo.format = Spec.Format;
		imageInfo.tiling = Spec.Tiling;
		imageInfo.initialLayout = Spec.Layout;
		imageInfo.usage = Spec.Usage;
		imageInfo.sharingMode = Spec.SharingMode;
		imageInfo.samples = Spec.Samples;
		imageInfo.flags = Spec.Flags;

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

	void VulkanImageDataManager::Init(const VulkanImageDataManagerSpec& Spec)
	{
		_Spec = Spec;

		BufferCreateInfo CreateInfo{};
		CreateInfo.SizeInBytes = _StagingBufferRange;
		CreateInfo.State = BufferState::DYNAMIC_DRAW;
		CreateInfo.Type = BUFFER_TYPE_TRANSFER_SRC;
		_StagingBuffer = _Spec.AllocateBuffer(CreateInfo);
	}

	void VulkanImageDataManager::Destroy()
	{
		VULKAN_ASSERT(_ImageSet.GetActiveCount() == 0, "All Image must be Freed!");
		VULKAN_ASSERT(_TextureSet.GetActiveCount() == 0, "All Texture must be Freed!");
		_Spec.FreeBuffer(_StagingBuffer);
	}

	uint32_t VulkanImageDataManager::AllocateImage(VmaAllocator Allocator, const ImageSpec& Spec)
	{
		VulkanImage Image;
		Image.Init(Allocator, Spec, _Spec.Uploader);
		auto ReturnHandle = _ImageSet.Create(Image);
		return ReturnHandle;
	}

	void VulkanImageDataManager::DestroyImage(VmaAllocator Allocator, uint32_t ImageHandle)
	{
		auto Image = _ImageSet.Get(ImageHandle);
		if (Image == nullptr)VULKAN_ERROR("Image Not Found!");
		Image->Destroy(Allocator);
		_ImageSet.Destroy(ImageHandle);
	}

	void VulkanImageDataManager::MapImageData(uint32_t ImageHandle, void* Data, int Width, int Height)
	{
		auto Image = _ImageSet.Get(ImageHandle);
		VkImageAspectFlags aspect = FormatToVkAspectMask(Image->GetSpec().Format, Image->GetSpec().Usage);

		_Spec.Uploader->TransitionImageLayout(Image->GetHandle(), Image->GetImageLayout(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, aspect);
		Image->SetImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// 4 bytes per pixel for RGBA8. Adjust if using HDR/16-bit formats!
		size_t requiredSize = (size_t)Width * Height * 4;

		// --- GROWTH CHECK ---
		if (requiredSize > _StagingBufferRange) {
			// Log the growth so you can monitor memory usage
			VULKAN_PRINTLN("Growing Staging Buffer from %llu to %llu bytes", _StagingBufferRange, requiredSize);

			// 1. Destroy old buffer
			_Spec.FreeBuffer(_StagingBuffer);

			// 2. Reallocate new, larger buffer
			_StagingBufferRange = requiredSize;

			BufferCreateInfo CreateInfo{};
			CreateInfo.SizeInBytes = _StagingBufferRange;
			CreateInfo.State = BufferState::DYNAMIC_DRAW;
			CreateInfo.Type = BUFFER_TYPE_TRANSFER_SRC;
			_StagingBuffer = _Spec.AllocateBuffer(CreateInfo);
		}
		// Map the whole thing into the (now large enough) staging buffer
		_Spec.MapBufferData(_StagingBuffer, Data, requiredSize, 0);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.imageSubresource = { aspect, 0, 0, 1 }; // mip 0, layer 0, 1 layer
		region.imageExtent = { (uint32_t)Width, (uint32_t)Height, 1 };

		_Spec.Uploader->CopyBufferToImage(_Spec.GetBuffer(_StagingBuffer), Image->GetHandle(), region);

		// Instead of transitioning back to OldLayout (which might be UNDEFINED), 
		// transition to the "Ideal" usable state.
		VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// If it's a Depth Buffer or Storage Image, pick the right final state
		if (Image->GetSpec().Usage & IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT)
			finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		else if (Image->GetSpec().Usage & IMAGE_USAGE_STORAGE_IMAGE)
			finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		_Spec.Uploader->TransitionImageLayout(Image->GetHandle(), Image->GetImageLayout(),
			finalLayout, aspect);
		Image->SetImageLayout(finalLayout);
	}

	uint32_t VulkanImageDataManager::CreateTexture(VkDevice Device, uint32_t ImageHandle, const TextureSpec& Spec)
	{
		VulkanTexture Texture;
		Texture.Init(Device, ImageHandle, _ImageSet.Get(ImageHandle), Spec);
		return _TextureSet.Create(Texture);
	}

	void VulkanImageDataManager::DestroyTexture(VkDevice Device, uint32_t TextureHandle)
	{
		_TextureSet.Get(TextureHandle)->Destroy(Device);
		_TextureSet.Destroy(TextureHandle);
	}

}
