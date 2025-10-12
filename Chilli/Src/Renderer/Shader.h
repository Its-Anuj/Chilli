#pragma once

#include "Buffer.h"

namespace Chilli
{
	enum class CullMode { None, Front, Back };
	enum class FillMode { Fill, Wireframe };
	enum class InputTopologyMode { Triangle_List, Triangle_Strip};

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

	struct VertexBufferAttrib
	{
		const char* Name;
		ShaderVertexTypes Type;
		int Binding = 0;
		int Location = 0;
	};

	enum class ShaderUniformTypes
	{
		UNIFORM,
		SAMPLED_IMAGE
	};

	enum class ShaderStageType
	{
		VERTEX, FRAGMENT
	};
	
	enum class ShaderUniformUse
	{
		PUSH_CONSTANT,
		DESCRIPTOR
	};

	struct ShaderUnifromAttrib
	{
		ShaderUniformTypes Type;
		ShaderStageType StageType;
		ShaderUniformUse Usage;
		std::string Name;
		uint32_t Binding = 0;
	};

	struct PipelineSpec
	{
		std::string Paths[2];
		
		bool ColorBlend = true;
		InputTopologyMode TopologyMode = InputTopologyMode::Triangle_List;
		CullMode ShaderCullMode = CullMode::Back;
		FillMode ShaderFillMode = FillMode::Fill;
	};

	class GraphicsPipeline
	{
	public:
		~GraphicsPipeline() {}

		virtual void Bind() = 0;
	private:
	};
}
