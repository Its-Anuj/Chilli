#pragma once

#include "Buffer.h"

namespace Chilli
{
	struct VulkanBufferSpec
	{
		void* Data = nullptr;
		uint32_t Size = 0;
		BufferState State = BufferState::STATIC_DRAW;
		VkBufferUsageFlags UsageFlag;
	};

	class VulkanBuffer
	{
	public:
		VulkanBuffer() {}
		~VulkanBuffer() {}

		void Init(const VulkanBufferSpec& Spec);
		void Delete();
		void MapData(void* Data, size_t Size, uint32_t Offset = 0);

		VkBuffer GetHandle() const { return _Buffer; }
	private:
		VmaAllocation _Allocation;
		VmaAllocationInfo _AllocationInfo;
		VkBuffer _Buffer = VK_NULL_HANDLE;
		VulkanBufferSpec _Spec;
	};

	class VulkanStageBuffer
	{
	public:
		VulkanStageBuffer() {}
		~VulkanStageBuffer() {}

		void Init(void* Data, size_t Size);
		void Delete();

		VkBuffer GetHandle() const { return _Buffer.GetHandle(); }
	private:
		VulkanBuffer _Buffer;
	};

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer() {}
		VulkanVertexBuffer(const VertexBufferSpec& Spec) { Init(Spec); }
		~VulkanVertexBuffer() {}

		virtual void Init(const VertexBufferSpec& Spec) override;
		virtual void Destroy() override;
		virtual void MapData(void* Data, size_t Size) override;

		virtual const VertexBufferSpec& GetSpec() const override { return _Spec; }
		VkBuffer GetHandle() const { return _Buffer.GetHandle(); }

	private:
		VulkanBuffer _Buffer;
		VertexBufferSpec _Spec;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer() {}
		VulkanIndexBuffer(const IndexBufferSpec& Spec) { Init(Spec); }
		~VulkanIndexBuffer() {}

		virtual void Init(const IndexBufferSpec& Spec) override;
		virtual void Destroy() override;
		virtual void MapData(void* Data, size_t Size) override;   

		virtual const IndexBufferSpec& GetSpec() const override { return _Spec; }

		VkBuffer GetHandle() const { return _Buffer.GetHandle(); }

	private:
		VulkanBuffer _Buffer;
		IndexBufferSpec _Spec;
	};

	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer() {}
		VulkanUniformBuffer(size_t Size) { Init(Size); }
		~VulkanUniformBuffer() {}

		virtual void Init(size_t Size) override;
		virtual void Destroy() override;
		virtual void MapData(void* Data, size_t Size) override;

		virtual size_t GetSize() const override { return _Size; }

		VkBuffer GetHandle() const { return _Buffer.GetHandle(); }

	private:
		VulkanBuffer _Buffer;
		size_t _Size = 0;
	};

	class VulkanStorageBuffer : public StorageBuffer
	{
	public:
		VulkanStorageBuffer() {}
		VulkanStorageBuffer(size_t Size) { Init(Size); }
		~VulkanStorageBuffer() {}

		virtual void Init(size_t Size) override;
		virtual void Destroy() override;
		virtual void MapData(void* Data, size_t Size, uint32_t Offset = 0) override;

		virtual size_t GetSize() const override { return _Size; }

		VkBuffer GetHandle() const { return _Buffer.GetHandle(); }

	private:
		VulkanBuffer _Buffer;
		size_t _Size = 0;
	};
}
