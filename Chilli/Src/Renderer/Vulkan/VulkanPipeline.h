#pragma once

#include "Pipeline.h"
#include "SparseSet.h"

namespace Chilli
{
	// stageFlag = VK_SHADER_STAGE_VERTEX_BIT / FRAGMENT_BIT etc.
	Chilli::ReflectedShaderInfo ReflectShaderModule(VkDevice device, const std::string& path, VkShaderStageFlagBits stageFlag);



	class VulkanGraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline() {}
		~VulkanGraphicsPipeline() {}

		void Init(const GraphicsPipelineFlags & Flags, const ShaderProgram& Program);
	private:
		VkPipeline _Pipeline;
	};
}

