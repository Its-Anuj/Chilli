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

	enum class ShaderStageType
	{
		VERTEX, FRAGMENT, ALL_GRAPHICS, ALL
	};

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
		TEX_MAP_USAGE= GLOBAL_SCENE,
		TEX_SAMPLERS = 1,
		SAMPLER_MAP_USAGE = TEX_SAMPLERS,
		// This is due to Scene UBO also needing a Uniform Buffer but we cant access 2 BUffers from GLOBAL_SCENE thus, utilzing TEX_SAMPLERS Index to serve as a Scene Buffer In Graphics Backend Bindless Rendering System
		SCENE_BUFFER_USAGE= TEX_SAMPLERS,
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
}
