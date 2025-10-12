#pragma once

#include "Buffer.h"

namespace Chilli
{
	struct VulkanBufferSpec
	{
		void* Data = nullptr;
		uint32_t Size = 0;
		BufferState State = BufferState::STATIC_DRAW;
		VkBufferUsageFlagBits UsageFlag;
	};

	class VulkanBuffer
	{
	public:
		VulkanBuffer(){} 
		~VulkanBuffer(){}

		void Init(const VulkanBufferSpec& Spec);
		void Delete ();
		void MapData(void* Data, size_t Size);

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
		~VulkanVertexBuffer(){}

		void Init(const VertexBufferSpec& Spec);
		void Delete();

		VkBuffer GetHandle() const { return _Buffer.GetHandle(); }
		virtual void MapData(void* Data, size_t Size) override;

		// Returns size in bytes
		virtual size_t GetSize() const override { return _Spec.Size; }
		virtual uint32_t GetCount() const override { return _Spec.Count; }

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

		void Init(const IndexBufferSpec& Spec);
		void Delete();
		virtual void MapData(void* Data, size_t Size) override;

		VkBuffer GetHandle() const { return _Buffer.GetHandle(); }

		// Returns size in bytes
		virtual size_t GetSize() const override { return _Spec.Size; }
		virtual uint32_t GetCount() const override { return _Spec.Count; }
		virtual IndexBufferType GetType() const override { return _Spec.Type; }

	private:
		VulkanBuffer _Buffer;
		IndexBufferSpec _Spec;
	};

	class VulkanUniformBuffer: public UniformBuffer
	{
	public:
		VulkanUniformBuffer() {}
		VulkanUniformBuffer(size_t Size){ Init(Size); }
		~VulkanUniformBuffer() {}

		void Init(size_t Size);
		void Delete();
		virtual void MapData(void* Data, size_t Size) override;

		virtual size_t GetSize() const override { return _Size; }
		VkBuffer GetHandle() const { return _Buffer.GetHandle(); }

	private:
		VulkanBuffer _Buffer;
		size_t _Size = 0;
	};
}
