#pragma once

#include "Image.h"

namespace Chilli
{
	enum class ShaderVertexTypes
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
		VERTEX, FRAGMENT, ALL
	};

	enum class ShaderUniformTypes
	{
		UNIFORM_BUFFER,
		STORAGE_BUFFER,
		SAMPLED_IMAGE,
		SAMPLER
	};

	enum class ShaderUniformSetsBindings
	{
		GLOBAL = 0,
		SCENE = 1,
		PER_OBJECT = 2,
		USER = 3,
		COUNT,
	};

	enum class CullMode { None, Front, Back };
	enum class PolygonMode { Fill, Wireframe };
	enum class InputTopologyMode { Triangle_List, Triangle_Strip };
	enum class FrontFaceMode { Clock_Wise, Counter_Clock_Wise };

	struct GraphicsPipelineSpec
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
	};

	class GraphicsPipeline
	{
	public:
		virtual void Init(const GraphicsPipelineSpec& Spec) = 0;
		virtual void ReCreate(const GraphicsPipelineSpec& Spec) = 0;
		virtual void Destroy() = 0;

		virtual void Bind() const = 0;

		virtual const GraphicsPipelineSpec& GetSpec() const = 0;

		static std::shared_ptr<GraphicsPipeline> Create(const GraphicsPipelineSpec& Spec);
	};
}
