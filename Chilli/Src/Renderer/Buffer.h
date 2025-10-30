#pragma once

namespace Chilli
{
	enum class BufferState
	{
		STATIC_DRAW,
		DYNAMIC_DRAW,
		DYNAMIC_STREAM
	};

	enum class IndexBufferType
	{
		UINT8_T,
		UINT16_T,
		UINT32_T,
		UINT64_T,
	};

	struct VertexBufferSpec
	{
		void* Data = nullptr;
		size_t Size = 0;
		uint32_t Count = 0;
		BufferState State;
	};

	class VertexBuffer
	{
	public:
		virtual void Init(const VertexBufferSpec& Spec) = 0;
		virtual void Destroy() = 0;
		virtual void MapData(void* Data, size_t Size) = 0;

		virtual const VertexBufferSpec& GetSpec() const = 0;

		static std::shared_ptr<VertexBuffer> Create(const VertexBufferSpec& Spec);
	};

	struct IndexBufferSpec
	{
		void* Data = nullptr;
		uint32_t Size = 0;
		uint32_t Count = 0;
		BufferState State = BufferState::STATIC_DRAW;
		IndexBufferType Type = IndexBufferType::UINT16_T;
	};

	class IndexBuffer
	{
	public:
		virtual void Init(const IndexBufferSpec& Spec) = 0;
		virtual void Destroy() = 0;
		virtual void MapData(void* Data, size_t Size) = 0;

		virtual const IndexBufferSpec& GetSpec() const = 0;

		static std::shared_ptr<IndexBuffer> Create(const IndexBufferSpec& Spec);
	};

	class UniformBuffer
	{
	public:
		virtual void Init(size_t Size) = 0;
		virtual void Destroy() = 0;
		virtual void MapData(void* Data, size_t Size) = 0;

		virtual size_t GetSize() const = 0;

		static std::shared_ptr<UniformBuffer> Create(size_t Size);
	};

	class StorageBuffer
	{
	public:
		virtual void Init(size_t Size) = 0;
		virtual void Destroy() = 0;
		virtual void MapData(void* Data, size_t Size, uint32_t Offset = 0) = 0;

		virtual size_t GetSize() const = 0;

		static std::shared_ptr<StorageBuffer> Create(size_t Size);
	};
}
