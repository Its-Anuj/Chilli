#include "ChV_PCH.h"

#define VK_ENABLE_BETA_EXTENSIONS
#define VK_USE_PLATFORM_WIN32_KHR
#include "C:\VulkanSDK\1.3.275.0\Include\vulkan\vulkan.h"

#include "vk_mem_alloc.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "VulkanBuffer.h"

namespace Chilli
{
	void VulkanBuffer::Init(const VulkanBufferSpec& Spec)
	{
		_Spec = Spec;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = Spec.Size;
		bufferInfo.usage = Spec.UsageFlag;;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};

		if (Spec.State == BufferState::STATIC_DRAW)
		{
			// Static GPU-only buffer: You should use a staging upload.
			bufferInfo.usage += VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			// No MAPPED flag — GPU_ONLY means it’s not directly mappable
			allocInfo.flags = 0;
		}
		else if (Spec.State == BufferState::DYNAMIC_DRAW)
		{
			// Frequent updates, can be directly mapped.
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			allocInfo.flags =
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}
		else if (Spec.State == BufferState::DYNAMIC_STREAM)
		{
			// Extremely frequent updates, maybe even per draw call.
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			// STREAM can also use sequential write + mapped, but could also benefit
			// from `HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT` if you double-buffer updates.
			allocInfo.flags =
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}

		auto Allocator = VulkanUtils::GetVulkanAllocator().GetAllocator();
		vmaCreateBuffer(Allocator, &bufferInfo, &allocInfo, &_Buffer, &_Allocation, &_AllocationInfo);
	}

	void VulkanBuffer::Delete()
	{
		auto Allocator = VulkanUtils::GetVulkanAllocator().GetAllocator();
		vmaDestroyBuffer(Allocator, _Buffer, _Allocation);
	}

	void VulkanBuffer::MapData(void* Data, size_t Size, uint32_t Offset)
	{
		auto Allocator = VulkanUtils::GetVulkanAllocator().GetAllocator();
		// Copy pixel data to staging buffer
		void* data;
		vmaMapMemory(Allocator, _Allocation, &data);
		memcpy((uint8_t*)data + Offset, Data, static_cast<size_t>(Size));
		vmaUnmapMemory(Allocator, _Allocation);
	}

	void VulkanStageBuffer::Init(void* Data, size_t Size)
	{
		VulkanBufferSpec Spec{};
		Spec.UsageFlag = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		Spec.Data = Data;
		Spec.Size = Size;
		Spec.State = BufferState::DYNAMIC_DRAW;

		_Buffer.Init(Spec);
		_Buffer.MapData(Data, Size);
	}

	void VulkanStageBuffer::Delete()
	{
		_Buffer.Delete();
	}

	void VulkanVertexBuffer::Init(const VertexBufferSpec& Spec)
	{
		VulkanBufferSpec BufferSpec{};
		BufferSpec.UsageFlag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		BufferSpec.Data = Spec.Data;
		BufferSpec.Size = Spec.Size;
		BufferSpec.State = Spec.State;

		_Spec = Spec;
		_Buffer.Init(BufferSpec);

		if (Spec.State == BufferState::STATIC_DRAW)
		{
			// Use Staging Buffer
			VulkanStageBuffer StagingBuffer;
			StagingBuffer.Init(Spec.Data, Spec.Size);
			VkBufferCopy Copy{};
			Copy.dstOffset = 0;
			Copy.srcOffset = 0;
			Copy.size = Spec.Size;
			VulkanUtils::GetPoolManager().CopyBufferToBuffer(Copy, StagingBuffer.GetHandle(), _Buffer.GetHandle());
			StagingBuffer.Delete();
		}
		else
			_Buffer.MapData(Spec.Data, Spec.Size);
	}

	void VulkanVertexBuffer::Destroy()
	{
		_Buffer.Delete();
	}

	void VulkanVertexBuffer::MapData(void* Data, size_t Size)
	{
		if (_Spec.State == BufferState::STATIC_DRAW)
			return;
		_Buffer.MapData(Data, Size);
	}

	void VulkanIndexBuffer::Init(const IndexBufferSpec& Spec)
	{
		VulkanBufferSpec BufferSpec{};
		BufferSpec.UsageFlag = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		BufferSpec.Data = Spec.Data;
		BufferSpec.Size = Spec.Size;
		BufferSpec.State = Spec.State;

		_Spec = Spec;
		_Buffer.Init(BufferSpec);

		if (Spec.State == BufferState::STATIC_DRAW)
		{
			// Use Staging Buffer
			VulkanStageBuffer StagingBuffer;
			StagingBuffer.Init(Spec.Data, Spec.Size);;
			VkBufferCopy Copy{};
			Copy.dstOffset = 0;
			Copy.srcOffset = 0;
			Copy.size = Spec.Size;
			VulkanUtils::GetPoolManager().CopyBufferToBuffer(Copy, StagingBuffer.GetHandle(), _Buffer.GetHandle());

			StagingBuffer.Delete();
		}
		else
			_Buffer.MapData(Spec.Data, Spec.Size);
	}

	void VulkanIndexBuffer::Destroy()
	{
		_Buffer.Delete();
	}

	void VulkanIndexBuffer::MapData(void* Data, size_t Size)
	{
		if (_Spec.State == BufferState::STATIC_DRAW)
			return;
		_Buffer.MapData(Data, Size);
	}

	void VulkanUniformBuffer::Init(size_t Size)
	{
		VulkanBufferSpec BufferSpec{};
		BufferSpec.UsageFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		BufferSpec.Data = nullptr;
		BufferSpec.Size = Size;
		BufferSpec.State = BufferState::DYNAMIC_STREAM;

		_Size = Size;
		_Buffer.Init(BufferSpec);
	}

	void VulkanUniformBuffer::Destroy()
	{
		_Buffer.Delete();
	}

	void VulkanUniformBuffer::MapData(void* Data, size_t Size)
	{
		_Buffer.MapData(Data, Size);
	}

	void VulkanStorageBuffer::Init(size_t Size)
	{
		VulkanBufferSpec BufferSpec{};
		BufferSpec.UsageFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		BufferSpec.Data = nullptr;
		BufferSpec.Size = Size;
		BufferSpec.State = BufferState::DYNAMIC_STREAM;

		_Size = Size;
		_Buffer.Init(BufferSpec);
	}

	void VulkanStorageBuffer::Destroy()
	{
		_Buffer.Delete();
	}

	void VulkanStorageBuffer::MapData(void* Data, size_t Size, uint32_t Offset)
	{
		_Buffer.MapData(Data, Size, Offset);
	}

}
