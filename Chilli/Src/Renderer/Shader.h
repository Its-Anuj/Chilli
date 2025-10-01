#pragma once

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

	struct GraphicsPipelineSpec
	{
		std::string Paths[2];
		std::vector<VertexBufferAttrib> Attribs;
	};

	class GraphicsPipeline
	{
	public:
		~GraphicsPipeline() {}

		virtual void Bind() = 0;  
		virtual void LinkUniformBuffer(std::shared_ptr<UniformBuffer>& UB) = 0;
	private:
	};
} // namespace VEngine
