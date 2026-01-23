#pragma once

#include "Pipeline.h"
#include "RenderPass.h"

namespace Chilli
{
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

	inline ShaderObjectTypes FormatToShaderType(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R32_SFLOAT:
			return Chilli::ShaderObjectTypes::FLOAT1;
		case VK_FORMAT_R32G32_SFLOAT:
			return Chilli::ShaderObjectTypes::FLOAT2;
		case VK_FORMAT_R32G32B32_SFLOAT:
			return Chilli::ShaderObjectTypes::FLOAT3;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return Chilli::ShaderObjectTypes::FLOAT4;
		case VK_FORMAT_R32_SINT:
			return Chilli::ShaderObjectTypes::INT1;
		case VK_FORMAT_R32G32_SINT:
			return Chilli::ShaderObjectTypes::INT2;
		case VK_FORMAT_R32G32B32_SINT:
			return Chilli::ShaderObjectTypes::INT3;
		case VK_FORMAT_R32G32B32A32_SINT:
			return Chilli::ShaderObjectTypes::INT4;
		case VK_FORMAT_R32_UINT:
			return Chilli::ShaderObjectTypes::UINT1;
		case VK_FORMAT_R32G32_UINT:
			return Chilli::ShaderObjectTypes::UINT2;
		case VK_FORMAT_R32G32B32_UINT:
			return Chilli::ShaderObjectTypes::UINT3;
		case VK_FORMAT_R32G32B32A32_UINT:
			return Chilli::ShaderObjectTypes::UINT4;
		}
	}

	inline VkDescriptorType ShaderUniformTypeToVk(ShaderUniformTypes Type)
	{
		switch (Type)
		{
		case ShaderUniformTypes::UNIFORM_BUFFER:  return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case ShaderUniformTypes::STORAGE_BUFFER:  return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case ShaderUniformTypes::SAMPLER:  return VK_DESCRIPTOR_TYPE_SAMPLER;
		case ShaderUniformTypes::COMBINED_IMAGE_SAMPLER:  return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case ShaderUniformTypes::SAMPLED_IMAGE:  return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		};
	}

	inline VkFormat FormatToVk(ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::NONE:
			return VK_FORMAT_UNDEFINED;

		case ImageFormat::RGBA8:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case ImageFormat::R8:
			return VK_FORMAT_R8_UNORM;
		case ImageFormat::RG8:
			return VK_FORMAT_R8G8_UNORM;
		case ImageFormat::RGB8:
			return VK_FORMAT_R8G8B8_UNORM;

		case ImageFormat::SRGBA8:
			return VK_FORMAT_R8G8B8A8_SRGB;

		case ImageFormat::R16F:
			return VK_FORMAT_R16_SFLOAT;

		case ImageFormat::RG16F:
			return VK_FORMAT_R16G16_SFLOAT;

		case ImageFormat::RGBA16F:
			return VK_FORMAT_R16G16B16A16_SFLOAT;

		case ImageFormat::RGBA32F:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

		case ImageFormat::BGRA8:
			return VK_FORMAT_B8G8R8A8_SRGB;

		case ImageFormat::S8I:
			return VK_FORMAT_S8_UINT;

		case ImageFormat::D32F:
			return VK_FORMAT_D32_SFLOAT;

		case ImageFormat::D32F_S8I:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
		}

		// Should never happen
		return VK_FORMAT_UNDEFINED;
	}


	inline VkPrimitiveTopology TopologyToVk(InputTopologyMode Mode)
	{
		switch (Mode)
		{
		case InputTopologyMode::Triangle_List:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case InputTopologyMode::Triangle_Strip:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		default:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}

	inline VkDynamicState DynamicStateToVk(ShaderDynamicStates State)
	{
		switch (State)
		{
		case SHADER_DYNAMIC_STATE_VIEWPORT:               return VK_DYNAMIC_STATE_VIEWPORT;
		case SHADER_DYNAMIC_STATE_SCISSOR:               return VK_DYNAMIC_STATE_SCISSOR;
		case SHADER_DYNAMIC_STATE_CULL_MODE:             return VK_DYNAMIC_STATE_CULL_MODE;
		case SHADER_DYNAMIC_STATE_FRONT_FACE:            return VK_DYNAMIC_STATE_FRONT_FACE;
		case SHADER_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY:    return VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY;
		case SHADER_DYNAMIC_STATE_DEPTH_TEST_ENABLE:     return VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE;
		case SHADER_DYNAMIC_STATE_DEPTH_WRITE_ENABLE:    return VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE;
		case SHADER_DYNAMIC_STATE_DEPTH_COMPARE_OP:      return VK_DYNAMIC_STATE_DEPTH_COMPARE_OP;
		case SHADER_DYNAMIC_STATE_STENCIL_TEST_ENABLE:   return VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE;
		case SHADER_DYNAMIC_STATE_POLYGON_MODE:          return VK_DYNAMIC_STATE_POLYGON_MODE_EXT;
		case SHADER_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE: return VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE;
		case SHADER_DYNAMIC_STATE_COLOR_WRITE_ENABLE:    return VK_DYNAMIC_STATE_COLOR_WRITE_ENABLE_EXT;
		case SHADER_DYNAMIC_STATE_COLOR_BLEND_ENABLE:    return VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT;
		case SHADER_DYNAMIC_STATE_LINE_WIDTH:    return VK_DYNAMIC_STATE_LINE_WIDTH;
		default:                                         return VK_DYNAMIC_STATE_MAX_ENUM; // invalid
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

	inline VkCompareOp CompareOpToVk(CompareOp op)
	{
		switch (op)
		{
		case CompareOp::NEVER:             return VK_COMPARE_OP_NEVER;
		case CompareOp::LESS:              return VK_COMPARE_OP_LESS;
		case CompareOp::EQUAL:             return VK_COMPARE_OP_EQUAL;
		case CompareOp::LESS_OR_EQUAL:     return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CompareOp::GREATER:           return VK_COMPARE_OP_GREATER;
		case CompareOp::NOT_EQUAL:         return VK_COMPARE_OP_NOT_EQUAL;
		case CompareOp::GREATER_OR_EQUAL:  return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case CompareOp::ALWAYS:            return VK_COMPARE_OP_ALWAYS;
		default:                                 return VK_COMPARE_OP_ALWAYS; // safe fallback
		}
	}
	
	// Ensure this function looks exactly like this:
	inline VkBufferUsageFlags BufferTypesToVk(uint32_t type) {
		VkBufferUsageFlags flags = 0;
		if (type & BUFFER_TYPE_VERTEX)   flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if (type & BUFFER_TYPE_INDEX)    flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		if (type & BUFFER_TYPE_UNIFORM)  flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		if (type & BUFFER_TYPE_STORAGE)  flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if (type & BUFFER_TYPE_TRANSFER_SRC) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		if (type & BUFFER_TYPE_TRANSFER_DST) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		return flags;
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

	inline VkStencilOp StencilOpToVulkan(StencilOp op)
	{
		switch (op)
		{
		case StencilOp::KEEP:                   return VK_STENCIL_OP_KEEP;
		case StencilOp::ZERO:                   return VK_STENCIL_OP_ZERO;
		case StencilOp::REPLACE:                return VK_STENCIL_OP_REPLACE;
		case StencilOp::INCREMENT_AND_CLAMP:    return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case StencilOp::DECREMENT_AND_CLAMP:    return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case StencilOp::INVERT:                 return VK_STENCIL_OP_INVERT;
		case StencilOp::INCREMENT_AND_WRAP:     return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case StencilOp::DECREMENT_AND_WRAP:     return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default:
			// Handle error or return a safe default
			return VK_STENCIL_OP_KEEP;
		}
	}

	inline VkBlendFactor BlendFactorToVk(BlendFactor factor)
	{
		switch (factor)
		{
		case BlendFactor::ZERO:               return VK_BLEND_FACTOR_ZERO;
		case BlendFactor::ONE:                return VK_BLEND_FACTOR_ONE;
		case BlendFactor::SRC_COLOR:          return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendFactor::ONE_MINUS_SRC_COLOR:return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendFactor::DST_COLOR:          return VK_BLEND_FACTOR_DST_COLOR;
		case BlendFactor::ONE_MINUS_DST_COLOR:return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendFactor::SRC_ALPHA:          return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendFactor::ONE_MINUS_SRC_ALPHA:return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DST_ALPHA:          return VK_BLEND_FACTOR_DST_ALPHA;
		case BlendFactor::ONE_MINUS_DST_ALPHA:return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			// Add more factors if you expand your enum
		default:                              return VK_BLEND_FACTOR_ONE; // Fallback
		}
	}

	inline VkBlendOp BlendOpToVk(BlendOp op)
	{
		switch (op)
		{
		case BlendOp::ADD:               return VK_BLEND_OP_ADD;
		case BlendOp::SUBTRACT:          return VK_BLEND_OP_SUBTRACT;
		case BlendOp::REVERSE_SUBTRACT:  return VK_BLEND_OP_REVERSE_SUBTRACT;
		case BlendOp::MIN:               return VK_BLEND_OP_MIN;
		case BlendOp::MAX:               return VK_BLEND_OP_MAX;
		default:                         return VK_BLEND_OP_ADD; // Fallback
		}
	}

	inline VkSampleCountFlagBits SampleCountToVk(uint32_t Count)
	{
		switch (Count)
		{
		case 1:   return VK_SAMPLE_COUNT_1_BIT;
		case 2:   return VK_SAMPLE_COUNT_2_BIT;
		case 4:   return VK_SAMPLE_COUNT_4_BIT;
		case 8:   return VK_SAMPLE_COUNT_8_BIT;
		case 16:  return VK_SAMPLE_COUNT_16_BIT;
		case 32:  return VK_SAMPLE_COUNT_32_BIT;
		case 64:  return VK_SAMPLE_COUNT_64_BIT;
		default:  return VK_SAMPLE_COUNT_1_BIT; // Fallback to 1 sample
		}
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
		if (Type & IMAGE_USAGE_TRANSIENT_ATTACHMENT)
			Flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;

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

	inline VkImageAspectFlags FormatToVkAspectMask(ImageFormat finalFormat, uint32_t Usage = 0) {
		VkImageAspectFlags AspectFlag = 0;

		if (finalFormat == ImageFormat::D32F) {
			AspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (finalFormat == ImageFormat::S8I) {
			AspectFlag = VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else if (finalFormat == ImageFormat::D32F_S8I) {
			// CRITICAL: If this is for a Descriptor/Sampler, you usually only want DEPTH.
			// If it's for a Framebuffer attachment, you want BOTH.
			if (Usage & IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT) {
				AspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			else {
				AspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
			}
		}
		else {
			AspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		return AspectFlag;
	}
	
	struct VulkanStateMapping {
		VkPipelineStageFlags2 stage;
		VkAccessFlags2 access;
		VkImageLayout layout;
	};

	inline VulkanStateMapping GetVulkanState(Chilli::ResourceState state) {
		switch (state) {
		case Chilli::ResourceState::Undefined:
			return { VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
					 0,
					 VK_IMAGE_LAYOUT_UNDEFINED };

		case Chilli::ResourceState::RenderTarget:
			return { VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					 VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		case Chilli::ResourceState::DepthWrite:
			return { VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
					 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		case Chilli::ResourceState::ShaderRead:
			return { VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					 VK_ACCESS_2_SHADER_READ_BIT,
					 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		case Chilli::ResourceState::ComputeRead:
			return { VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					 VK_ACCESS_2_SHADER_READ_BIT,
					 VK_IMAGE_LAYOUT_GENERAL };

		case Chilli::ResourceState::ComputeWrite:
			return { VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					 VK_ACCESS_2_SHADER_WRITE_BIT,
					 VK_IMAGE_LAYOUT_GENERAL };

		case Chilli::ResourceState::Present:
			return { VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					 0,
					 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

		default:
			// Fallback to Undefined
			return { VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
					 0,
					 VK_IMAGE_LAYOUT_UNDEFINED };
		}
	}

	inline VkImageAspectFlags ImageAspectsToVk(uint32_t aspects)
	{
		VkImageAspectFlags vkAspects = 0;

		if (aspects & IMAGE_ASPECT_COLOR)
			vkAspects |= VK_IMAGE_ASPECT_COLOR_BIT;

		if (aspects & IMAGE_ASPECT_DEPTH)
			vkAspects |= VK_IMAGE_ASPECT_DEPTH_BIT;

		if (aspects & IMAGE_ASPECT_STENCIL)
			vkAspects |= VK_IMAGE_ASPECT_STENCIL_BIT;

		return vkAspects;
	}

	inline uint32_t VkToImageAspects(VkImageAspectFlags vkAspects)
	{
		uint32_t aspects = 0;

		if (vkAspects & VK_IMAGE_ASPECT_COLOR_BIT)
			aspects |= IMAGE_ASPECT_COLOR;

		if (vkAspects & VK_IMAGE_ASPECT_DEPTH_BIT)
			aspects |= IMAGE_ASPECT_DEPTH;

		if (vkAspects & VK_IMAGE_ASPECT_STENCIL_BIT)
			aspects |= IMAGE_ASPECT_STENCIL;

		return aspects;
	}

	inline VkComponentSwizzle SwizzleToVk(ComponentSwizzle swizzle) {
		switch (swizzle) {
		case ComponentSwizzle::R:        return VK_COMPONENT_SWIZZLE_R;
		case ComponentSwizzle::G:        return VK_COMPONENT_SWIZZLE_G;
		case ComponentSwizzle::B:        return VK_COMPONENT_SWIZZLE_B;
		case ComponentSwizzle::A:        return VK_COMPONENT_SWIZZLE_A;
		case ComponentSwizzle::ZERO:     return VK_COMPONENT_SWIZZLE_ZERO;
		case ComponentSwizzle::ONE:      return VK_COMPONENT_SWIZZLE_ONE;
		case ComponentSwizzle::IDENTITY:
		default:                         return VK_COMPONENT_SWIZZLE_IDENTITY;
		}
	}

	inline uint32_t GetImageFormatBytesPerPixel(ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::R8:       return 1;
		case ImageFormat::RG8:      return 2;
		case ImageFormat::RGB8:     return 3;

		case ImageFormat::RGBA8:    return 4;
		case ImageFormat::SRGBA8:   return 4;
		case ImageFormat::BGRA8:    return 4;

		case ImageFormat::R16F:     return 2;  // 16-bit float (2 bytes)
		case ImageFormat::RG16F:    return 4;  // 2x 16-bit floats
		case ImageFormat::RGBA16F:  return 8;  // 4x 16-bit floats

		case ImageFormat::RGBA32F:  return 16; // 4x 32-bit floats

		case ImageFormat::S8I:      return 1;  // Stencil only
		case ImageFormat::D32F:     return 4;  // 32-bit depth float
		case ImageFormat::D32F_S8I: return 5;  // 4 bytes depth + 1 byte stencil

		case ImageFormat::NONE:
		default:
			return 0;
		}
	}

	inline ImageFormat VkToFormat(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_UNDEFINED:
			return ImageFormat::NONE;

		case VK_FORMAT_R8G8B8A8_UNORM:
			return ImageFormat::RGBA8;

		case VK_FORMAT_R8_UNORM:
			return ImageFormat::R8;

		case VK_FORMAT_R8G8_UNORM:
			return ImageFormat::RG8;

		case VK_FORMAT_R8G8B8_UNORM:
			return ImageFormat::RGB8;

		case VK_FORMAT_R8G8B8A8_SRGB:
			return ImageFormat::SRGBA8;

		case VK_FORMAT_R16_SFLOAT:
			return ImageFormat::R16F;

		case VK_FORMAT_R16G16_SFLOAT:
			return ImageFormat::RG16F;

		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return ImageFormat::RGBA16F;

		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return ImageFormat::RGBA32F;

		case VK_FORMAT_B8G8R8A8_SRGB:
			return ImageFormat::BGRA8;

		case VK_FORMAT_S8_UINT:
			return ImageFormat::S8I;

		case VK_FORMAT_D32_SFLOAT:
			return ImageFormat::D32F;

		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return ImageFormat::D32F_S8I;
		}

		// Unknown / unsupported VkFormat
		return ImageFormat::NONE;
	}

}
