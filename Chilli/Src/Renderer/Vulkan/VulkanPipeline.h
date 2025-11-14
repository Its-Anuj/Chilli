#pragma once

#include "Pipeline.h"

struct ReflectedVertexInput {
	std::string Name;
	uint32_t Location;
	VkFormat Format; // VkFormat
	uint32_t Offset;
};

struct ReflectedShaderInfo
{
	std::vector< ReflectedVertexInput> VertexInputs;
};

namespace Chilli
{
	class VulkanGraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline() {}
		~VulkanGraphicsPipeline(){}
	
		void Init(const GraphicsPipelineCreateInfo& CreateInfo, VkDevice Device, VkFormat SwapChainFormat);
		void Terminate(VkDevice Device);

		VkPipeline GetHandle() const { return _Pipeline; }
		const ReflectedShaderInfo& GetShaderInfo() const { return _Info; }
	private:
		VkPipeline _Pipeline;
		VkPipelineLayout _PipelineLayout;
		ReflectedShaderInfo _Info;
	};
}
