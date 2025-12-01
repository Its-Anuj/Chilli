#pragma once

#include "UUID\UUID.h"

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
		UINT16_T,
		UINT32_T,
		NONE
	};

	struct VertexBufferSpec
	{
		void* Data = nullptr;
		size_t Size = 0;
		uint32_t Count = 0;
		BufferState State;
	};

	enum BufferType
	{
		BUFFER_TYPE_NONE = 0,
		BUFFER_TYPE_VERTEX = 1 << 0,
		BUFFER_TYPE_INDEX = 1 << 1,
		BUFFER_TYPE_STORAGE = 1 << 2,
		BUFFER_TYPE_UNIFORM = 1 << 3,
		BUFFER_TYPE_TRANSFER_DST = 1 << 4,
		BUFFER_TYPE_TRANSFER_SRC = 1 << 5
	};

	struct BufferCreateInfo
	{
		uint32_t Type;
		uint32_t SizeInBytes = 0;
		void* Data = nullptr;
		BufferState State;
		uint32_t Count;
	};
}