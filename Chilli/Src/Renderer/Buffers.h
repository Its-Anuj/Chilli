#pragma once

namespace Chilli
{
	enum class BufferType
	{
		STATIC_DRAW,
		DYNAMIC_DRAW,
		DYNAMIC_STREAM
	};

	struct VertexBufferSpec
	{
		void* Data;
		BufferType Type;
		size_t Size = 0;
		size_t Count = 0;
	};

	class VertexBuffer
	{
	public:
		virtual size_t GetCount() const = 0;
		virtual size_t GetSize() const = 0 ;
	private:
	};

	struct IndexBufferSpec
	{
		void* Data;
		BufferType Type;
		size_t Size = 0;
		size_t Count = 0;
	};

	class IndexBuffer
	{
	public:
		virtual size_t GetCount() const = 0;
		virtual size_t GetSize() const = 0;
	private:
	};
}
