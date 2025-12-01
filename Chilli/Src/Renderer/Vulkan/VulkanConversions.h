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

	inline VkIndexType IndexTypeToVK(IndexBufferType Type)
	{
		switch (Type)
		{
		case IndexBufferType::UINT16_T:
			return VK_INDEX_TYPE_UINT16;
		case IndexBufferType::UINT32_T:
			return VK_INDEX_TYPE_UINT32;
		}
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
	
	inline VkShaderStageFlags ShaderStageTypeToVk(int type)
	{
		VkShaderStageFlags flags = 0;

		if (type & SHADER_STAGE_VERTEX)                   flags |= VK_SHADER_STAGE_VERTEX_BIT;
		if (type & SHADER_STAGE_TESSELLATION_CONTROL)    flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		if (type & SHADER_STAGE_TESSELLATION_EVALUATION) flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		if (type & SHADER_STAGE_GEOMETRY)                flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
		if (type & SHADER_STAGE_FRAGMENT)                flags |= VK_SHADER_STAGE_FRAGMENT_BIT;

		if (type & SHADER_STAGE_TASK)                    flags |= VK_SHADER_STAGE_TASK_BIT_EXT;
		if (type & SHADER_STAGE_MESH)                    flags |= VK_SHADER_STAGE_MESH_BIT_EXT;

		if (type & SHADER_STAGE_COMPUTE)                 flags |= VK_SHADER_STAGE_COMPUTE_BIT;

		if (type & SHADER_STAGE_RAYGEN)                  flags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		if (type & SHADER_STAGE_ANY_HIT)                 flags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		if (type & SHADER_STAGE_CLOSEST_HIT)             flags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		if (type & SHADER_STAGE_MISS)                    flags |= VK_SHADER_STAGE_MISS_BIT_KHR;
		if (type & SHADER_STAGE_INTERSECTION)            flags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		if (type & SHADER_STAGE_CALLABLE)                flags |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;

		return flags;
	}

	inline VkShaderStageFlags ShaderStageTypeToVk(ShaderStageType type)
	{
		VkShaderStageFlags flags = 0;
		if (type & SHADER_STAGE_VERTEX)                  flags |= VK_SHADER_STAGE_VERTEX_BIT;
		if (type & SHADER_STAGE_TESSELLATION_CONTROL)    flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		if (type & SHADER_STAGE_TESSELLATION_EVALUATION) flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		if (type & SHADER_STAGE_GEOMETRY)                flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
		if (type & SHADER_STAGE_FRAGMENT)                flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		if (type & SHADER_STAGE_TASK)                    flags |= VK_SHADER_STAGE_TASK_BIT_EXT;
		if (type & SHADER_STAGE_MESH)                    flags |= VK_SHADER_STAGE_MESH_BIT_EXT;
		if (type & SHADER_STAGE_COMPUTE)                 flags |= VK_SHADER_STAGE_COMPUTE_BIT;
		if (type & SHADER_STAGE_RAYGEN)                  flags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		if (type & SHADER_STAGE_ANY_HIT)                 flags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		if (type & SHADER_STAGE_CLOSEST_HIT)             flags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		if (type & SHADER_STAGE_MISS)                    flags |= VK_SHADER_STAGE_MISS_BIT_KHR;
		if (type & SHADER_STAGE_INTERSECTION)            flags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		if (type & SHADER_STAGE_CALLABLE)                flags |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;
		return flags;
	}

	inline VkShaderStageFlagBits ShaderStageTypeToVkFlagBit(ShaderStageType type)
	{
		switch (type)
		{
		case SHADER_STAGE_VERTEX:                  return VK_SHADER_STAGE_VERTEX_BIT;
		case SHADER_STAGE_TESSELLATION_CONTROL:    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case SHADER_STAGE_TESSELLATION_EVALUATION: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case SHADER_STAGE_GEOMETRY:                return VK_SHADER_STAGE_GEOMETRY_BIT;
		case SHADER_STAGE_FRAGMENT:                return VK_SHADER_STAGE_FRAGMENT_BIT;
		case SHADER_STAGE_TASK:                    return VK_SHADER_STAGE_TASK_BIT_EXT;
		case SHADER_STAGE_MESH:                    return VK_SHADER_STAGE_MESH_BIT_EXT;
		case SHADER_STAGE_COMPUTE:                 return VK_SHADER_STAGE_COMPUTE_BIT;
		case SHADER_STAGE_RAYGEN:                  return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		case SHADER_STAGE_ANY_HIT:                 return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		case SHADER_STAGE_CLOSEST_HIT:             return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		case SHADER_STAGE_MISS:                    return VK_SHADER_STAGE_MISS_BIT_KHR;
		case SHADER_STAGE_INTERSECTION:            return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		case SHADER_STAGE_CALLABLE:                return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
		default:                                   return static_cast<VkShaderStageFlagBits>(0);
		}
	}

	inline ShaderStageType VkFlagBitToShaderStageType(VkShaderStageFlagBits flag)
	{
		switch (flag)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:                  return SHADER_STAGE_VERTEX;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:   return SHADER_STAGE_TESSELLATION_CONTROL;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:return SHADER_STAGE_TESSELLATION_EVALUATION;
		case VK_SHADER_STAGE_GEOMETRY_BIT:               return SHADER_STAGE_GEOMETRY;
		case VK_SHADER_STAGE_FRAGMENT_BIT:               return SHADER_STAGE_FRAGMENT;
		case VK_SHADER_STAGE_TASK_BIT_EXT:               return SHADER_STAGE_TASK;
		case VK_SHADER_STAGE_MESH_BIT_EXT:               return SHADER_STAGE_MESH;
		case VK_SHADER_STAGE_COMPUTE_BIT:                return SHADER_STAGE_COMPUTE;
		case VK_SHADER_STAGE_RAYGEN_BIT_KHR:             return SHADER_STAGE_RAYGEN;
		case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:            return SHADER_STAGE_ANY_HIT;
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:        return SHADER_STAGE_CLOSEST_HIT;
		case VK_SHADER_STAGE_MISS_BIT_KHR:               return SHADER_STAGE_MISS;
		case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:       return SHADER_STAGE_INTERSECTION;
		case VK_SHADER_STAGE_CALLABLE_BIT_KHR:           return SHADER_STAGE_CALLABLE;
		default:                                         return SHADER_STAGE_NONE;
		}
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
		case VK_SHADER_STAGE_VERTEX_BIT: return ShaderStageType::SHADER_STAGE_VERTEX;
		case VK_SHADER_STAGE_FRAGMENT_BIT: return ShaderStageType::SHADER_STAGE_FRAGMENT;
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

	inline VkAttachmentLoadOp LoadOpToVk(AttachmentLoadOp Mode)
	{
		switch (Mode)
		{
		case AttachmentLoadOp::LOAD:
			return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		case AttachmentLoadOp::CLEAR:
			return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		}
	}

	inline VkAttachmentStoreOp StoreOpToVk(AttachmentStoreOp Mode)
	{
		switch (Mode)
		{
		case AttachmentStoreOp::DONT_CARE:
			return VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		case AttachmentStoreOp::STORE:
			return VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		}
	}

	inline VkFormat ShaderObjectTypeToVkFormat(ShaderObjectTypes type)
	{
		switch (type)
		{
		case ShaderObjectTypes::FLOAT1:  return VK_FORMAT_R32_SFLOAT;
		case ShaderObjectTypes::FLOAT2:  return VK_FORMAT_R32G32_SFLOAT;
		case ShaderObjectTypes::FLOAT3:  return VK_FORMAT_R32G32B32_SFLOAT;
		case ShaderObjectTypes::FLOAT4:  return VK_FORMAT_R32G32B32A32_SFLOAT;

		case ShaderObjectTypes::INT1:    return VK_FORMAT_R32_SINT;
		case ShaderObjectTypes::INT2:    return VK_FORMAT_R32G32_SINT;
		case ShaderObjectTypes::INT3:    return VK_FORMAT_R32G32B32_SINT;
		case ShaderObjectTypes::INT4:    return VK_FORMAT_R32G32B32A32_SINT;

		case ShaderObjectTypes::UINT1:   return VK_FORMAT_R32_UINT;
		case ShaderObjectTypes::UINT2:   return VK_FORMAT_R32G32_UINT;
		case ShaderObjectTypes::UINT3:   return VK_FORMAT_R32G32B32_UINT;
		case ShaderObjectTypes::UINT4:   return VK_FORMAT_R32G32B32A32_UINT;

		default: return VK_FORMAT_UNDEFINED;
		}
	}
	inline ShaderObjectTypes VkFormatToShaderObjectType(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R32_SFLOAT:           return ShaderObjectTypes::FLOAT1;
		case VK_FORMAT_R32G32_SFLOAT:        return ShaderObjectTypes::FLOAT2;
		case VK_FORMAT_R32G32B32_SFLOAT:     return ShaderObjectTypes::FLOAT3;
		case VK_FORMAT_R32G32B32A32_SFLOAT:  return ShaderObjectTypes::FLOAT4;

		case VK_FORMAT_R32_SINT:             return ShaderObjectTypes::INT1;
		case VK_FORMAT_R32G32_SINT:          return ShaderObjectTypes::INT2;
		case VK_FORMAT_R32G32B32_SINT:       return ShaderObjectTypes::INT3;
		case VK_FORMAT_R32G32B32A32_SINT:    return ShaderObjectTypes::INT4;

		case VK_FORMAT_R32_UINT:             return ShaderObjectTypes::UINT1;
		case VK_FORMAT_R32G32_UINT:          return ShaderObjectTypes::UINT2;
		case VK_FORMAT_R32G32B32_UINT:       return ShaderObjectTypes::UINT3;
		case VK_FORMAT_R32G32B32A32_UINT:    return ShaderObjectTypes::UINT4;

		default: return ShaderObjectTypes::FLOAT1; // fallback
		}
	}

}