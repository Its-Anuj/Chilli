#include "ChV_PCH.h"
#include "vulkan\vulkan.h"

#include "vk_mem_alloc.h"
#include "VulkanBackend.h"
#include "VulkanConversions.h"

namespace Chilli
{
	void VulkanBufferManager::Init(const VulkanDevice& Device, VmaAllocator Allocator, VulkanDataUploader* Uploader, uint32_t StagingBufferAllocatedSize)
	{
		_StagingBuffer.AllocatedSize = StagingBufferAllocatedSize;
		_Uploader = Uploader;
		_SetupStagingBuffer(Device, Allocator);
	}

	void VulkanBufferManager::Free(const VulkanDevice& Device, VmaAllocator Allocator)
	{
		_FreeStagingBuffer(Device, Allocator);

		if (GetActiveCount() > 0)
			VULKAN_ERROR("All Buffer Must be Fred!");
	}

	uint32_t VulkanBufferManager::Create(const VulkanDevice& Device, VmaAllocator Allocator, const BufferCreateInfo& CreateInfo)
	{
		VULKAN_ASSERT(CreateInfo.SizeInBytes != 0, "Creating a Buffer Requires SizeIN")

			VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = CreateInfo.SizeInBytes;
		bufferInfo.usage = BufferTypesToVk(CreateInfo.Type);
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		if (CreateInfo.State == BufferState::STATIC_DRAW)
		{
			// Best for: Meshes, Textures (Loaded once).
			// Lives in VRAM. Requires Staging Buffer for upload.
			bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocInfo.flags = 0;
		}
		else if (CreateInfo.State == BufferState::DYNAMIC_DRAW)
		{
			// Best for: Per-object data that changes often but not every frame.
			// Lives in Host-Visible memory (RAM or ReBAR).
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}
		else if (CreateInfo.State == BufferState::STREAM_DRAW)
		{
			// Best for: Global Frame Data (Matrices, Timers, Frame Index).
			// Always Host-Visible. Optimized for "Write once, Read once, Discard".
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT |
				VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
		}
		else if (CreateInfo.State == BufferState::STATIC_READ)
		{
			// Best for: Screenshots or one-time Compute result downloads.
			// Optimized for GPU -> CPU flow.
			bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
			allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
				VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		}
		else if (CreateInfo.State == BufferState::DYNAMIC_READ)
		{
			// Best for: Occlusion queries, GPU-driven feedback loops read by CPU.
			bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
			allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
				VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		}
		else if (CreateInfo.State == BufferState::STATIC_COPY)
		{
			// Best for: Intermediate data that stays on GPU and never changes.
			// GPU only. No CPU access.
			bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocInfo.flags = 0;
		}
		else if (CreateInfo.State == BufferState::DYNAMIC_COPY)
		{
			// Best for: GPU-driven rendering (Indirect Draw buffers filled by Compute).
			// Must be GPU_ONLY for max bandwidth during GPU-to-GPU copies.
			bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocInfo.flags = 0;
		}

		VulkanBuffer Buffer;
		Buffer.CreateInfo = CreateInfo;
		VULKAN_SUCCESS_ASSERT(vmaCreateBuffer(Allocator, &bufferInfo, &allocInfo, &Buffer.Buffer, &Buffer.Allocation, &Buffer.AllocationInfo), "Vulkan Buffer Creation Failed!");

		auto BufferHandle = _Buffers.Create(Buffer);

		if (CreateInfo.State == BufferState::STATIC_DRAW ||
			CreateInfo.State == BufferState::STATIC_COPY ||
			CreateInfo.State == BufferState::DYNAMIC_COPY)
		{
			MapBufferData(Device, BufferHandle, CreateInfo.Data, CreateInfo.SizeInBytes, 0);
		}

		return BufferHandle;
	}

	void VulkanBufferManager::Destroy(VmaAllocator Allocator, uint32_t Handle)
	{
		auto Buffer = _Buffers.Get(Handle);
		VULKAN_ASSERT(Buffer != nullptr, "Given Buffer Handle Gives a null Buffer");

		vmaDestroyBuffer(Allocator, Buffer->Buffer, Buffer->Allocation);
		_Buffers.Destroy(Handle);
	}

	void VulkanBufferManager::MapBufferData(const VulkanDevice& Device, uint32_t Handle, void* Data, size_t Size, size_t Offset)
	{
		auto Buffer = _Buffers.Get(Handle);
		if (Buffer == nullptr)
			return;
		// --- CATEGORY 1: DIRECT ACCESS (Host Visible) ---
		// These buffers are either CPU-to-GPU or GPU-to-CPU. 
		// VMA has already mapped these, so we just memcpy.
		if (Buffer->CreateInfo.State == BufferState::DYNAMIC_DRAW ||
			Buffer->CreateInfo.State == BufferState::STREAM_DRAW ||
			Buffer->CreateInfo.State == BufferState::STATIC_READ ||
			Buffer->CreateInfo.State == BufferState::DYNAMIC_READ)
		{
			if (Buffer->AllocationInfo.pMappedData)
			{
				memcpy((uint8_t*)Buffer->AllocationInfo.pMappedData + Offset, Data, Size);

				// If the memory isn't "Host Coherent", we would need to flush here.
				// But VMA_MEMORY_USAGE_CPU_TO_GPU usually handles this.
			}
			return;
		}

		// --- CATEGORY 2: STAGING REQUIRED (Device Local / GPU Private) ---
		// Includes: STATIC_DRAW, STATIC_COPY, DYNAMIC_COPY.
		// The CPU cannot see this memory directly.

		size_t remaining = Size;
		size_t srcOffset = 0;

		while (remaining > 0)
		{
			size_t chunk = std::min(remaining, (size_t)_StagingBuffer.AllocatedSize);

			// 1. Fill the staging buffer (Staging is always DYNAMIC/Mapped)
			uint32_t stagingHandle = _StagingBuffer.Buffer;
			MapBufferData(Device, stagingHandle, (uint8_t*)Data + srcOffset, chunk, 0);

			// 2. GPU-to-GPU Copy (Staging VRAM -> Destination VRAM)
			BufferCopyInfo copy{};
			copy.SrcOffset = 0;
			copy.DstOffset = Offset + srcOffset;
			copy.Size = chunk;

			auto StagingBuffer = _Buffers.Get(stagingHandle)->Buffer;
			_Uploader->CopyBufferToBuffer(StagingBuffer, Buffer->Buffer, copy);

				srcOffset += chunk;
			remaining -= chunk;
		}
	}

	void VulkanBufferManager::_FreeStagingBuffer(const VulkanDevice& Device, VmaAllocator Allocator)
	{
		Destroy(Allocator, _StagingBuffer.Buffer);
	}

	void VulkanBufferManager::_SetupStagingBuffer(const VulkanDevice& Device, VmaAllocator Allocator)
	{
		BufferCreateInfo CreateInfo{};
		CreateInfo.Data = nullptr;
		CreateInfo.Type = BUFFER_TYPE_TRANSFER_SRC;
		CreateInfo.State = BufferState::DYNAMIC_DRAW;
		CreateInfo.SizeInBytes = _StagingBuffer.AllocatedSize;

		_StagingBuffer.Buffer = Create(Device, Allocator, CreateInfo);
	}
}
