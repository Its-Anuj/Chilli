#pragma once

#include "Buffers.h"
#include "Pipeline.h"

namespace Chilli
{
	inline VkBufferUsageFlags BufferTypesToVk(int Type)
	{
		VkBufferUsageFlags usage = 0;
		if (Type & BufferType::BUFFER_TYPE_VERTEX)       usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if (Type & BufferType::BUFFER_TYPE_INDEX)        usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		if (Type & BufferType::BUFFER_TYPE_STORAGE)      usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if (Type & BufferType::BUFFER_TYPE_UNIFORM)      usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		if (Type & BufferType::BUFFER_TYPE_TRANSFER_DST) usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		if (Type & BufferType::BUFFER_TYPE_TRANSFER_SRC) usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		return usage;
	}

	inline VkFormat FormatToVk(ImageFormat Format)
	{
		switch (Format)
		{
		case ImageFormat::RGBA8:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case ImageFormat::D32:
			return VK_FORMAT_D32_SFLOAT;
		case ImageFormat::D24_S8:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case ImageFormat::D32_S8:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
		}
	}

	inline VkPrimitiveTopology TopologyToVk(InputTopologyMode Mode)
	{
		switch (Mode)
		{
		case InputTopologyMode::Triangle_List: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case InputTopologyMode::Triangle_Strip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		}
	}

	inline VkPolygonMode PolygonModeToVk(PolygonMode  Mode)
	{
		switch (Mode)
		{
		case PolygonMode::Fill: return VK_POLYGON_MODE_FILL;
		case PolygonMode::Wireframe: return VK_POLYGON_MODE_LINE;
		}
	}

	inline VkCullModeFlags CullModeToVk(CullMode Mode)
	{
		switch (Mode)
		{
		case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
		case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
		case CullMode::None: return VK_CULL_MODE_NONE;
		}
	}

	inline VkFrontFace FrontFaceToVk(FrontFaceMode Mode)
	{
		switch (Mode)
		{
		case FrontFaceMode::Clock_Wise: return VK_FRONT_FACE_CLOCKWISE;
		case FrontFaceMode::Counter_Clock_Wise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}
	}

}