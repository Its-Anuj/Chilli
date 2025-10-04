#pragma once

#include "Texture.h"
#include "Buffers.h"

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

	struct ShaderUnifromAttrib
	{
		ShaderUniformTypes Type;
		ShaderStageType StageType;
		std::string Name;
		uint32_t Binding = 0;
	};

	struct GraphicsPipelineSpec
	{
		std::string Paths[2];
		std::vector<VertexBufferAttrib> Attribs;
		std::vector< ShaderUnifromAttrib> UniformAttribs;
		std::vector<std::shared_ptr<UniformBuffer>> UBs;;
		std::shared_ptr<Texture> Texs;;
	};

	class GraphicsPipeline
	{
	public:
		~GraphicsPipeline() {}

		virtual void LinkUniformBuffer(const std::shared_ptr<UniformBuffer>& UB) = 0;
		virtual void Bind() = 0;
	private:
	};
} // namespace VEngine
