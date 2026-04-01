#pragma once

#include "RenderCore.h"

namespace Chilli
{
	enum class BufferState
	{
		NONE,
		STATIC_DRAW,
		DYNAMIC_DRAW,
		STREAM_DRAW,   // Set every frame, used once

		// Readback (GPU -> User)
		STATIC_READ,   // GPU writes once, user reads
		DYNAMIC_READ,  // GPU writes often, user reads

		// Internal Copy (GPU -> GPU)
		STATIC_COPY,   // GPU writes once, GPU reads
		DYNAMIC_COPY   // GPU writes often, GPU reads
	};

	enum class IndexBufferType
	{
		UINT16_T,
		UINT32_T,
		NONE
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
	};

#define CH_BUFFER_MAX_DEBUG_NAME_SIZE 32
#define CH_BUFFER_WHOLE_SIZE UINT32_MAX

	struct Buffer
	{
		uint32_t RawBufferHandle = UINT32_MAX;
		BufferCreateInfo CreateInfo;
		char DebugName[CH_BUFFER_MAX_DEBUG_NAME_SIZE];
		ResourceState ResourceState = Chilli::ResourceState::VertexRead;
	};
}