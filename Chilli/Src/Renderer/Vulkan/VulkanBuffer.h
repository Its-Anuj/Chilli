#pragma once

#include "SparseSet.h"
#include "Buffers.h"

namespace Chilli
{
	class VulkanDataUploader;

	struct VulkanBuffer
	{
		VkBuffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;
		BufferCreateInfo CreateInfo;
	};

	struct StagingBufferManager
	{
		uint32_t Buffer = BackBone::npos;
		uint32_t AllocatedSize = 1e6;
	};

	class VulkanBufferManager
	{
	public:
		VulkanBufferManager() {}
		~VulkanBufferManager() {}

		void Init(const VulkanDevice& Device, VmaAllocator Allocator, VulkanDataUploader* Uploader, uint32_t StagingBufferAllocatedSize);
		void Free(const VulkanDevice& Device, VmaAllocator Allocator);

		uint32_t Create(const VulkanDevice& Device, VmaAllocator Allocator, const BufferCreateInfo& CreateInfo);
		void Destroy(VmaAllocator Allocator, uint32_t Handle);
		void MapBufferData(const VulkanDevice& Device, uint32_t Handle, void* Data, size_t Size, size_t Offset);

		void CopyBufferToBuffer(uint32_t SrcBuffer, uint32_t DstBuffer, BufferCopyInfo Copy);

		VulkanBuffer* Get(uint32_t Handle) { return _Buffers.Get(Handle); }

		uint32_t GetActiveCount() { return _Buffers.GetActiveCount(); }
	private:
		void _SetupStagingBuffer(const VulkanDevice& Device, VmaAllocator Allocator);
		void _FreeStagingBuffer(const VulkanDevice& Device, VmaAllocator Allocator);
	private:
		SparseSet<VulkanBuffer> _Buffers;
		StagingBufferManager _StagingBuffer;
		VulkanDataUploader* _Uploader;
	};
}
