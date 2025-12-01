#pragma once

#include "Image.h"

namespace Chilli
{
	enum class ShaderObjectTypes
	{
		FLOAT1,
		FLOAT2,
		FLOAT3,
		FLOAT4,
		INT1,
		INT2,
		INT3,
		INT4,
		UINT1,
		UINT2,
		UINT3,
		UINT4,
	};

	inline int ShaderTypeToSize(Chilli::ShaderObjectTypes Type)
	{
		switch (Type)
		{
		case Chilli::ShaderObjectTypes::FLOAT1:
			return sizeof(float) * 1;
		case Chilli::ShaderObjectTypes::FLOAT2:
			return sizeof(float) * 2;
		case Chilli::ShaderObjectTypes::FLOAT3:
			return sizeof(float) * 3;
		case Chilli::ShaderObjectTypes::FLOAT4:
			return sizeof(float) * 4;
		case Chilli::ShaderObjectTypes::INT1:
			return sizeof(int) * 1;
		case Chilli::ShaderObjectTypes::INT2:
			return sizeof(int) * 2;
		case Chilli::ShaderObjectTypes::INT3:
			return sizeof(int) * 3;
		case Chilli::ShaderObjectTypes::INT4:
			return sizeof(int) * 4;
		case Chilli::ShaderObjectTypes::UINT1:
			return sizeof(unsigned int) * 1;
		case Chilli::ShaderObjectTypes::UINT2:
			return sizeof(unsigned int) * 2;
		case Chilli::ShaderObjectTypes::UINT3:
			return sizeof(unsigned int) * 3;
		case Chilli::ShaderObjectTypes::UINT4:
			return sizeof(unsigned int) * 4;
		default:
			return -1;
		}
	}

	inline int ShaderTypeToElementCount(Chilli::ShaderObjectTypes Type)
	{
		switch (Type)
		{
		case Chilli::ShaderObjectTypes::FLOAT1:
			return 1;
		case Chilli::ShaderObjectTypes::FLOAT2:
			return 2;
		case Chilli::ShaderObjectTypes::FLOAT3:
			return 3;
		case Chilli::ShaderObjectTypes::FLOAT4:
			return 4;
		case Chilli::ShaderObjectTypes::INT1:
			return  1;
		case Chilli::ShaderObjectTypes::INT2:
			return  2;
		case Chilli::ShaderObjectTypes::INT3:
			return  3;
		case Chilli::ShaderObjectTypes::INT4:
			return  4;
		case Chilli::ShaderObjectTypes::UINT1:
			return  1;
		case Chilli::ShaderObjectTypes::UINT2:
			return  2;
		case Chilli::ShaderObjectTypes::UINT3:
			return  3;
		case Chilli::ShaderObjectTypes::UINT4:
			return  4;
		default:
			return -1;
		}
	}
	
	enum ShaderStageType : uint32_t
	{
		SHADER_STAGE_NONE,	
		// Rasterization pipeline
		SHADER_STAGE_VERTEX = 1 << 0,   // 0x00000001
		SHADER_STAGE_TESSELLATION_CONTROL = 1 << 1,   // 0x00000002
		SHADER_STAGE_TESSELLATION_EVALUATION = 1 << 2,   // 0x00000004
		SHADER_STAGE_GEOMETRY = 1 << 3,   // 0x00000008
		SHADER_STAGE_FRAGMENT = 1 << 4,   // 0x00000010

		// Modern Mesh Shader pipeline
		SHADER_STAGE_TASK = 1 << 5,   // 0x00000020  (Amplification shader)
		SHADER_STAGE_MESH = 1 << 6,   // 0x00000040

		// Compute
		SHADER_STAGE_COMPUTE = 1 << 7,   // 0x00000080

		// Ray Tracing pipeline
		SHADER_STAGE_RAYGEN = 1 << 8,   // 0x00000100
		SHADER_STAGE_ANY_HIT = 1 << 9,   // 0x00000200
		SHADER_STAGE_CLOSEST_HIT = 1 << 10,  // 0x00000400
		SHADER_STAGE_MISS = 1 << 11,  // 0x00000800
		SHADER_STAGE_INTERSECTION = 1 << 12,  // 0x00001000
		SHADER_STAGE_CALLABLE = 1 << 13,  // 0x00002000,
		SHADER_STAGE_LAST = SHADER_STAGE_CALLABLE,

		// Convenience masks
		SHADER_STAGE_ALL_GRAPHICS =
		SHADER_STAGE_VERTEX |
		SHADER_STAGE_TESSELLATION_CONTROL |
		SHADER_STAGE_TESSELLATION_EVALUATION |
		SHADER_STAGE_GEOMETRY |
		SHADER_STAGE_FRAGMENT |
		SHADER_STAGE_TASK |
		SHADER_STAGE_MESH,

		SHADER_STAGE_ALL_RAY_TRACING =
		SHADER_STAGE_RAYGEN |
		SHADER_STAGE_ANY_HIT |
		SHADER_STAGE_CLOSEST_HIT |
		SHADER_STAGE_MISS |
		SHADER_STAGE_INTERSECTION |
		SHADER_STAGE_CALLABLE,

		SHADER_STAGE_COUNT = std::countr_zero((uint32_t)SHADER_STAGE_LAST) + 1,
	};

	inline uint32_t GetShaderStageTypeToIndex(uint32_t Stage) {
		if (Stage == SHADER_STAGE_NONE)
			return UINT32_MAX; // or any sentinel you prefer

		return std::countr_zero((uint32_t)Stage); 
	}

	enum class ShaderUniformTypes
	{
		UNIFORM_BUFFER,
		STORAGE_BUFFER,
		SAMPLED_IMAGE,
		SAMPLER,
		COMBINED_IMAGE_SAMPLER,
		UNKNOWN
	};

	enum class BindlessSetTypes
	{
		GLOBAL_SCENE = 0,
		TEX_MAP_USAGE = GLOBAL_SCENE,
		TEX_SAMPLERS = 1,
		SAMPLER_MAP_USAGE = TEX_SAMPLERS,
		// This is due to Scene UBO also needing a Uniform Buffer but we cant access 2 BUffers from GLOBAL_SCENE thus, utilzing TEX_SAMPLERS Index to serve as a Scene Buffer In Graphics Backend Bindless Rendering System
		SCENE_BUFFER_USAGE = TEX_SAMPLERS,
		MATERIAl = 2,
		PER_OBJECT = 3,
		COUNT_NON_USER,
		USER_0 = COUNT_NON_USER,
		USER_1 = 5,
		USER_2 = 6,
		USER_3 = 7,
		COUNT,
	};

	enum BindlessRenderingLimits
	{
		MAX_TEXTURES = 100000,
		MAX_SAMPLERS = 16,
		MAX_UNIFORM_BUFFERS = 500,
		MAX_STORAGE_BUFFERS = 500,
	};

	enum class CullMode { None, Front, Back };
	enum class PolygonMode { Fill, Wireframe };
	enum class InputTopologyMode { Triangle_List, Triangle_Strip };
	enum class FrontFaceMode { Clock_Wise, Counter_Clock_Wise };

	struct GraphicsPipelineCreateInfo
	{
		std::string VertPath;
		std::string FragPath;

		bool ColorBlend = true;
		InputTopologyMode TopologyMode = InputTopologyMode::Triangle_List;
		CullMode ShaderCullMode = CullMode::Back;
		PolygonMode  ShaderFillMode = PolygonMode::Fill;
		FrontFaceMode FrontFace = FrontFaceMode::Clock_Wise;

		bool EnableDepthStencil = false;
		bool EnableDepthTest = false;
		bool EnableDepthWrite = false;
		bool EnableStencilTest = false;
		ImageFormat DepthFormat;

		bool UseSwapChainColorFormat = true;
		ImageFormat ColorFormat;
	};

	struct GraphicsPipeline
	{
		uint32_t PipelineHandle;
	};

#define SHADER_UNIFORM_BINDING_NAME_SIZE 32

	struct ReflectedVertexInput {
		std::string Name;
		uint32_t Location;
		ShaderObjectTypes Format; // VkFormat
		uint32_t Offset;
	};

	struct ReflectedBindingUniformInput
	{
		uint32_t Binding = 0;
		uint32_t Set = 0;
		uint32_t Count = 0;
		Chilli::ShaderUniformTypes Type;
		char Name[SHADER_UNIFORM_BINDING_NAME_SIZE] = { 0 };
		Chilli::ShaderStageType Stage;
	};

	struct ReflectedSetUniformInput
	{
		uint32_t Set = 0;
		std::vector< ReflectedBindingUniformInput> Bindings;
	};

	struct ReflectedPushConstantRange
	{
		int Stage;
		uint32_t Offset = 0;
		uint32_t Size = 0;
	};

	struct ReflectedShaderInfo
	{
		std::vector< ReflectedVertexInput> VertexInputs;
		std::vector< ReflectedSetUniformInput> UniformInputs;
		std::vector< ReflectedPushConstantRange> PushConstants;
	};

	struct ShaderModule
	{
		uint32_t RawModuleHandle = BackBone::npos;
		const ReflectedShaderInfo* ReflectedInfo;
		std::string Path;
		std::string Name;
		ShaderStageType Stage;
	};

	struct ShaderProgram
	{
		ShaderModule Modules[int(SHADER_STAGE_COUNT)];

		ShaderProgram() {
			for (int i = 0; i < SHADER_STAGE_COUNT; i++)
			{
				Modules[i].RawModuleHandle = BackBone::npos;
			}
		}
	};

	struct GraphicsPipelineFlags
	{
		bool ColorBlend = true;
		InputTopologyMode TopologyMode = InputTopologyMode::Triangle_List;
		CullMode ShaderCullMode = CullMode::Back;
		PolygonMode  ShaderFillMode = PolygonMode::Fill;
		FrontFaceMode FrontFace = FrontFaceMode::Clock_Wise;

		bool EnableDepthStencil = false;
		bool EnableDepthTest = false;
		bool EnableDepthWrite = false;
		bool EnableStencilTest = false;
		ImageFormat DepthFormat;

		bool UseSwapChainColorFormat = true;
		ImageFormat ColorFormat;
	};

}
