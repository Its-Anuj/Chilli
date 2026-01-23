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

	VkImageView CreateImageView(VkDevice Device, VkImage Image, VkFormat Format,
		VkImageAspectFlags AspectFlag, VkImageViewType ViewType,
		const TextureSpec& Spec, uint32_t ImageFullMipCount);

	bool ShouldGenerateMips(const ImageSpec& spec) {
		// 1. Must want more than 1 level
		if (spec.MipLevel <= 1) return false;

		// 2. Must be a sampled color texture
		bool isSampled = (spec.Usage & IMAGE_USAGE_SAMPLED_IMAGE);
		bool isDepth = (spec.Format == ImageFormat::D32F ||
			spec.Format == ImageFormat::D32F_S8I ||
			spec.Format == ImageFormat::S8I);

		// 3. Must not be a storage or attachment-only image
		bool isStorage = (spec.Usage & IMAGE_USAGE_STORAGE_IMAGE);

		return isSampled && !isDepth && !isStorage;
	}

	void VulkanImage::Init(VmaAllocator Allocator, ImageSpec& Spec, VulkanDataUploader* Uploader)
	{
		if (Spec.Resolution.Width == 0 || Spec.Resolution.Height == 0)
			VULKAN_ERROR("Invalid Width or Height Given!");

		if (_Image != VK_NULL_HANDLE)
			VULKAN_ERROR("Image Should Be Destroyed!");

		if (Spec.Usage & IMAGE_USAGE_SAMPLED_IMAGE && !(Spec.Usage & IMAGE_USAGE_TRANSFER_DST))
		{
			Spec.Usage |= IMAGE_USAGE_TRANSFER_DST;
		}

		if (Spec.MipLevel > 1)
		{
			Spec.Usage |= IMAGE_USAGE_TRANSFER_DST;
			Spec.Usage |= IMAGE_USAGE_TRANSFER_SRC;
		}

		if (Spec.Usage & IMAGE_USAGE_COLOR_ATTACHMENT ||
			Spec.Usage & IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT)
		{
			Spec.MipLevel = 1;
		}
		else
		{
			Spec.Sample = IMAGE_SAMPLE_COUNT_1_BIT;
		}

		// --- TRANSIENT LOGIC ---
		if (Spec.Usage & IMAGE_USAGE_TRANSIENT_ATTACHMENT)
		{
			// Transient images cannot have mips and cannot be sampled outside of subpasses
			Spec.MipLevel = 1;

			// Ensure it has at least one attachment bit, otherwise Transient is invalid
			bool isAttachment = (Spec.Usage & IMAGE_USAGE_COLOR_ATTACHMENT) ||
				(Spec.Usage & IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT) ||
				(Spec.Usage & IMAGE_USAGE_INPUT_ATTACHMENT);

			if (!isAttachment) {
				VULKAN_ERROR("Transient images must be used as an attachment!");
			}
		}

		CreateVulkanImageSpec Info{};
		Info.Width = Spec.Resolution.Width;
		Info.Height = Spec.Resolution.Height;
		Info.Depth = Spec.Resolution.Depth;
		Info.Tiling = VK_IMAGE_TILING_OPTIMAL;
		Info.ImageType = ImageTypeToVk(Spec.Type);
		Info.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		Info.Format = FormatToVk(Spec.Format);
		Info.SharingMode = VK_SHARING_MODE_EXCLUSIVE;
		Info.MipLevel = Spec.MipLevel;
		Info.ArrayLayers = 1;
		Info.Samples = (VkSampleCountFlagBits)Spec.Sample;
		// 1. Enable format reinterpretation (Mutable)
		if (Spec.Format == ImageFormat::RGBA8 || Spec.Format == ImageFormat::SRGBA8) {
			Info.Flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		}

		Info.Usage = ImageUsageToVk(Spec.Usage);

		auto [Image, Allocation, AllocInfo] = CreateVulkanImage(Allocator, Info);
		_Image = Image;
		_Allocation = Allocation;
		_AllocationInfo = AllocInfo;

		VkImageAspectFlags Aspect = FormatToVkAspectMask(Spec.Format, Spec.Usage);

		if (Spec.State != ResourceState::Undefined)
		{
			if (ValidateImageState(Spec.Usage, Spec.State) == false)
				VULKAN_ERROR("The image must be created with usage that is allowed by state");
			// Get Vulkan mapping for requested state
			VulkanStateMapping TargetState = GetVulkanState(Spec.State);

			VkImageLayout targetLayout = TargetState.layout;

			// Only transition if layout actually changes
			if (_Layout != targetLayout)
			{
				Uploader->TransitionImageLayout(
					_Image,
					_Layout,
					targetLayout,
					Aspect
				);

				// Track new layout internally
				SetImageLayout(targetLayout);
			}
		}
		_Spec = Spec;
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

		if (SamplerInfo.MipmapMode == SamplerFilter::LINEAR)
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		if (SamplerInfo.MipmapMode == SamplerFilter::NEAREST)
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

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

	void VulkanTexture::Init(VkDevice Device, uint32_t ImageHandle, const VulkanImage* Image, TextureSpec& Spec)
	{
		auto& ImgSpec = Image->GetSpec();
		_ImageHandle = ImageHandle;

		// 1. Determine Aspect
		VkImageAspectFlags AspectFlag = 0;
		ImageFormat finalFormat = (Spec.Format != ImageFormat::NONE) ? Spec.Format : ImgSpec.Format;

		if (Spec.Aspect & IMAGE_ASPECT_ASSUME_FROM_USAGE) {
			AspectFlag = FormatToVkAspectMask(finalFormat, ImgSpec.Usage);
		}
		else {
			AspectFlag = ImageAspectsToVk(Spec.Aspect);
		}

		// 2. Determine Format
		VkFormat Format = FormatToVk(finalFormat);
		Spec.Format = VkToFormat(Format);
		Spec.Aspect = VkToImageAspects(AspectFlag);

		// 3. Create View using engine Spec
		// We pass ImgSpec.MipCount to help calculate the UINT32_MAX logic
		_ImageView = CreateImageView(Device, Image->GetHandle(), Format, AspectFlag,
			VK_IMAGE_VIEW_TYPE_2D, Spec, ImgSpec.MipLevel);
		_Spec = Spec;
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

		// --- LAZY ALLOCATION LOGIC ---
		if (Spec.Usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
		{
			// Prefer lazily allocated memory if the hardware supports it.
			// This is primarily beneficial for mobile/tiling GPUs.
			imageAllocInfo.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
		}
		else
		{
			imageAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}

		VkImage Image;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;

		vmaCreateImage(Allocator, &imageInfo, &imageAllocInfo, &Image, &Allocation, &AllocationInfo);
		return { Image, Allocation, AllocationInfo };
	}

	VkImageView CreateImageView(VkDevice Device, VkImage Image, VkFormat Format,
		VkImageAspectFlags AspectFlag, VkImageViewType ViewType,
		const TextureSpec& Spec, uint32_t ImageFullMipCount)
	{
		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = Image;
		info.viewType = ViewType;
		info.format = Format;

		// --- MAPPING SWIZZLE ---
		info.components.r = SwizzleToVk(Spec.Swizzle.r);
		info.components.g = SwizzleToVk(Spec.Swizzle.g);
		info.components.b = SwizzleToVk(Spec.Swizzle.b);
		info.components.a = SwizzleToVk(Spec.Swizzle.a);

		// --- MAPPING SUBRESOURCE RANGE ---
		info.subresourceRange.aspectMask = AspectFlag;
		info.subresourceRange.baseMipLevel = Spec.BaseMipLevel;

		// Handle the UINT32_MAX "All Mips" logic
		uint32_t levelCount = Spec.MipCount;
		if (levelCount == UINT32_MAX) {
			levelCount = ImageFullMipCount - Spec.BaseMipLevel;
		}
		info.subresourceRange.levelCount = levelCount;

		info.subresourceRange.baseArrayLayer = Spec.BaseArrayLayer;
		info.subresourceRange.layerCount = Spec.LayerCount;

		VkImageView View;
		if (vkCreateImageView(Device, &info, nullptr, &View) != VK_SUCCESS)
			throw std::runtime_error("[VULKAN]: Image View creation failed with swizzle/mip settings!");

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

	uint32_t VulkanImageDataManager::AllocateImage(VmaAllocator Allocator, ImageSpec& Spec)
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
		size_t requiredSize = (size_t)Width * Height * GetImageFormatBytesPerPixel(Image->GetSpec().Format);

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

		// Fix the range! (Crucial to stop validation errors)
		VkImageSubresourceRange Range{};
		Range.aspectMask = aspect;
		Range.baseMipLevel = 0;
		Range.levelCount = (Image->GetSpec().MipLevel > 1) ? VK_REMAINING_MIP_LEVELS : 1;
		Range.baseArrayLayer = 0;
		Range.layerCount = 1; // Or VK_REMAINING_ARRAY_LAYERS

		// Use Sync2 Types
		VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkAccessFlags2 DstAccess = VK_ACCESS_2_SHADER_READ_BIT;
		VkPipelineStageFlags2 DstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		if (Image->GetSpec().Usage & IMAGE_USAGE_STORAGE_IMAGE) {
			finalLayout = VK_IMAGE_LAYOUT_GENERAL;
			DstAccess = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			DstStage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		}
		else if (Image->GetSpec().Usage & IMAGE_USAGE_STORAGE_IMAGE) {
			finalLayout = VK_IMAGE_LAYOUT_GENERAL;
			DstAccess = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			DstStage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		}

		if (ShouldGenerateMips(Image->GetSpec()))
		{
			_Spec.Uploader->CopyBufferToImage(
				_Spec.GetBuffer(_StagingBuffer),
				Image->GetHandle(),
				region,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Stay in DST for now
				Range,
				VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT
			);

			Image->SetImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			_Spec.Uploader->GenerateMipmaps(
				Image->GetHandle(),
				FormatToVk(Image->GetSpec().Format),
				Width, Height,
				Image->GetSpec().MipLevel
			);
			Image->SetImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		else
		{
			_Spec.Uploader->CopyBufferToImage(_Spec.GetBuffer(_StagingBuffer), Image->GetHandle(), region,
				Image->GetImageLayout(), finalLayout, Range, DstAccess, DstStage);
			Image->SetImageLayout(finalLayout);
		}
	}

	uint32_t VulkanImageDataManager::CreateTexture(VkDevice Device, uint32_t ImageHandle, TextureSpec& Spec)
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
