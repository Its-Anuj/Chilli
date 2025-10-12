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
		uint32_t Size = 0;
		uint32_t Count = 0;
		BufferState State = BufferState::STATIC_DRAW;
	};

	class VertexBuffer
	{
	public:
		// Returns size in bytes
		virtual size_t GetSize() const = 0;
		virtual uint32_t GetCount() const = 0;

		virtual void MapData(void* Data, size_t Size) = 0;
	private:
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
		// Returns size in bytes
		virtual size_t GetSize() const = 0;
		virtual uint32_t GetCount() const = 0;
		virtual IndexBufferType GetType() const = 0;
		
		virtual void MapData(void* Data, size_t Size) = 0;
	private:
	};

	class UniformBuffer
	{
	public:
		virtual void MapData(void* Data, size_t Size) = 0;
		virtual size_t GetSize() const = 0;
	private:
	};
}
