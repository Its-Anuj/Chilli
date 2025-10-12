#pragma once

#include "Material.h"

namespace Chilli
{
	struct ReflectedVertexInput {
		std::string Name;
		uint32_t Location;
		VkFormat Format; // VkFormat
		uint32_t Offset;
	};

	struct ReflectedUniformInput {
		std::string Name;
		uint32_t Binding, SetLayoutIndex, Count;
		ShaderStageType Stage;
		ShaderUniformTypes Type;
		VkSampler* pImmutableSamplers;
	};

	struct ReflectedShaderInfo {
		std::vector< ReflectedVertexInput> VertexInputs;
		std::vector<VkDescriptorSetLayout> SetLayouts;
		std::vector<VkPushConstantRange> PushConstantRanges;
		std::vector<ReflectedUniformInput> Bindings;
	};

	class VulkanGraphicsPipeline : public GraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline() {}
		VulkanGraphicsPipeline(VkDevice Device, VkFormat SwapChainFormat, const PipelineSpec& Spec) { Init(Device, SwapChainFormat, Spec); }
		~VulkanGraphicsPipeline() {}

		void Init(VkDevice Device, VkFormat SwapChainFormat, const PipelineSpec& Spec);
		void Destroy(VkDevice Device);

		void ReCreate(VkDevice Device, VkFormat SwapChainFormat, const PipelineSpec& Spec);

		VkPipeline GetHandle() const { return _Pipeline; }

		virtual void Bind() override {}

		VkPipelineLayout GetPipelineLayout() const { return _PipelineLayout; }
		VkDescriptorSetLayout GetPSetLayout() const { return _Layout; }

		const ReflectedShaderInfo& GetInfo() const { return _Info; }
	private:
		void _FillInfo();
	private:
		VkPipeline _Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout _PipelineLayout = VK_NULL_HANDLE;

		VkDescriptorSetLayout _Layout;
		ReflectedShaderInfo _Info;

	};
}
