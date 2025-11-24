#pragma once

#include "Buffers.h"
#include "Pipeline.h"
#include "GraphicsBackend.h"

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

	inline VkShaderStageFlags ShaderStageToVk(ShaderStageType Type)
	{
		switch (Type)
		{
		case ShaderStageType::VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStageType::FRAGMENT:return VK_SHADER_STAGE_FRAGMENT_BIT;
		};
	}

	inline VkDescriptorType UniformTypeToVkDescriptorType(ShaderUniformTypes Type)
	{
		switch (Type)
		{
		case ShaderUniformTypes::UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case ShaderUniformTypes::SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case ShaderUniformTypes::COMBINED_IMAGE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case ShaderUniformTypes::SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
		case ShaderUniformTypes::STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		};
	}

	inline ShaderStageType VkShaderStageToChilli(VkShaderStageFlags Type)
	{
		switch (Type)
		{
		case VK_SHADER_STAGE_VERTEX_BIT: return ShaderStageType::VERTEX;
		case VK_SHADER_STAGE_FRAGMENT_BIT: return ShaderStageType::FRAGMENT;
		};
	}

	inline VkCommandBufferUsageFlags CommandBufferSubmitStateToVk(int state)
	{
		VkCommandBufferUsageFlags flags = 0;
		// Assuming CommandBufferSubmitState is an enum or bitmask similar to BufferType.
		// Replace these with actual enum/bitmask values as defined in your codebase.
		if (state & COMMAND_BUFFER_SUBMIT_STATE_ONE_TIME)      flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (state & COMMAND_BUFFER_SUBMIT_STATE_RENDER_PASS_CONTINUE) flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		if (state & COMMAND_BUFFER_SUBMIT_STATE_SIMULTANEOUS_USE)     flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		return flags;
	}

	inline VkImageUsageFlags ImageUsageToVk(uint32_t Type)
	{
		VkImageUsageFlags Flags = 0;

		if (Type & IMAGE_USAGE_TRANSFER_SRC)
			Flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (Type & IMAGE_USAGE_TRANSFER_DST)
			Flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (Type & IMAGE_USAGE_SAMPLED_IMAGE)
			Flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
		if (Type & IMAGE_USAGE_STORAGE_IMAGE)
			Flags |= VK_IMAGE_USAGE_STORAGE_BIT;
		if (Type & IMAGE_USAGE_COLOR_ATTACHMENT)
			Flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (Type & IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT)
			Flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (Type & IMAGE_USAGE_INPUT_ATTACHMENT)
			Flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

		assert(Flags != 0 && "Unhandled ImageUsage value");
		return Flags;
	}

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

}