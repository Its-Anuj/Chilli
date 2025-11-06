#pragma once

#include "Material.h"
#include <array>
#include <algorithm>
#include <cassert>

namespace Chilli
{
	inline VkImageType ImageTypeToVk(ImageType Type)
	{
		switch (Type)
		{
		case ImageType::IMAGE_TYPE_1D:
			return VK_IMAGE_TYPE_1D;
		case ImageType::IMAGE_TYPE_2D:
			return VK_IMAGE_TYPE_2D;
		case ImageType::IMAGE_TYPE_3D:
			return VK_IMAGE_TYPE_3D;
		};
	}

	inline VkImageViewType ImageViewTypeToVk(ImageType Type)
	{
		switch (Type)
		{
		case ImageType::IMAGE_TYPE_1D:
			return VK_IMAGE_VIEW_TYPE_1D;
		case ImageType::IMAGE_TYPE_2D:
			return VK_IMAGE_VIEW_TYPE_2D;
		case ImageType::IMAGE_TYPE_3D:
			return VK_IMAGE_VIEW_TYPE_3D;
		};
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

	inline VkSamplerAddressMode SamplerModeToVk(SamplerMode Mode)
	{
		switch (Mode)
		{
		case SamplerMode::REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case SamplerMode::CLAMP_TO_BORDER:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case SamplerMode::CLAMP_TO_EDGE:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case SamplerMode::MIRRORED_REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		}
	}

	inline VkFilter SamplerFilterToVk(SamplerFilter Filter)
	{
		switch (Filter)
		{
		case SamplerFilter::LINEAR:
			return VK_FILTER_LINEAR;
		case SamplerFilter::NEAREST:
			return VK_FILTER_NEAREST;
		}
	}

	inline VkDescriptorType UniformTypeToVk(ShaderUniformTypes Type)
	{
		switch (Type)
		{
		case ShaderUniformTypes::SAMPLED_IMAGE:return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case ShaderUniformTypes::SAMPLER:return VK_DESCRIPTOR_TYPE_SAMPLER;
		case ShaderUniformTypes::UNIFORM_BUFFER:return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case ShaderUniformTypes::STORAGE_BUFFER:return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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

	inline VkIndexType IndexTypeToVk(IndexBufferType Type)
	{
		switch (Type)
		{
		case IndexBufferType::UINT16_T: return VK_INDEX_TYPE_UINT16;
		case IndexBufferType::UINT32_T: return VK_INDEX_TYPE_UINT32;
		}
	}

	inline VkImageUsageFlags ImageUsageToVk(uint32_t Type)
	{
		VkImageUsageFlags Flags = 0;

		if (Type & ImageUsage::TRANSFER_SRC)
			Flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (Type & ImageUsage::TRANSFER_DST)
			Flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (Type & ImageUsage::SAMPLED_IMAGE)
			Flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
		if (Type & ImageUsage::STORAGE_IMAGE)
			Flags |= VK_IMAGE_USAGE_STORAGE_BIT;
		if (Type & ImageUsage::COLOR_ATTACHMENT)
			Flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (Type & ImageUsage::DEPTH_STENCIL_ATTACHMENT)
			Flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (Type & ImageUsage::INPUT_ATTACHMENT)
			Flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

		assert(Flags != 0 && "Unhandled ImageUsage value");
		return Flags;
	}
}
