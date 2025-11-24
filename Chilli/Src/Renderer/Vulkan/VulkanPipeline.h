#pragma once

#include "Pipeline.h"
#include "SparseSet.h"

#define SHADER_UNIFORM_BINDING_NAME_SIZE 32

struct ReflectedVertexInput {
	std::string Name;
	uint32_t Location;
	VkFormat Format; // VkFormat
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

struct ReflectedShaderInfo
{
	std::vector< ReflectedVertexInput> VertexInputs;
	std::vector< ReflectedSetUniformInput> UniformInputs;
	std::vector< VkPushConstantRange> PushConstants;
};

namespace Chilli
{
	struct PipelineLayoutInfo
	{
		std::vector<VkPushConstantRange> PushConstants;
		std::vector<ReflectedSetUniformInput> UniformInputs;

		// Ensure vectors are sorted for fast comparison
		void sort();

		// Ultra-fast equality using memcmp for simple structs
		bool operator==(const PipelineLayoutInfo& other) const noexcept
		{
			// Quick size checks first
			if (PushConstants.size() != other.PushConstants.size() ||
				UniformInputs.size() != other.UniformInputs.size())
				return false;

			// Compare PushConstants using memcmp (they're POD structs)
			if (!PushConstants.empty() &&
				memcmp(PushConstants.data(), other.PushConstants.data(),
					PushConstants.size() * sizeof(VkPushConstantRange)) != 0)
				return false;

			// Compare UniformInputs
			for (size_t i = 0; i < UniformInputs.size(); ++i)
			{
				const auto& a = UniformInputs[i];
				const auto& b = other.UniformInputs[i];

				if (a.Set != b.Set ||
					a.Bindings.size() != b.Bindings.size())
					return false;

				// Compare bindings using memcmp (assuming ReflectedBindingUniformInput is POD)
				if (!a.Bindings.empty() &&
					memcmp(a.Bindings.data(), b.Bindings.data(),
						a.Bindings.size() * sizeof(ReflectedBindingUniformInput)) != 0)
					return false;
			}

			return true;
		}
	};

	struct PipelineLayoutInfoHash
	{
		std::size_t operator()(const PipelineLayoutInfo& info) const noexcept
		{
			// For sorted data, we can use std::hash on vectors
			std::size_t seed = std::hash<std::size_t>{}(info.PushConstants.size());

			// Use std::hash for vector of simple structs (much faster)
			for (const auto& pc : info.PushConstants) {
				seed ^= std::hash<uint32_t>{}(pc.stageFlags) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= std::hash<uint32_t>{}(pc.offset) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= std::hash<uint32_t>{}(pc.size) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}

			// Hash UniformInputs
			seed ^= std::hash<std::size_t>{}(info.UniformInputs.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for (const auto& set : info.UniformInputs) {
				seed ^= std::hash<uint32_t>{}(set.Set) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= std::hash<std::size_t>{}(set.Bindings.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				for (const auto& binding : set.Bindings) {
					seed ^= std::hash<uint32_t>{}(binding.Binding) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
					seed ^= std::hash<uint32_t>{}(static_cast<uint32_t>(binding.Type)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
					seed ^= std::hash<uint32_t>{}(static_cast<uint32_t>(binding.Stage)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
					seed ^= std::hash<std::string>{}(binding.Name) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				}
			}

			return seed;
		}
	};

	class VulkanPipelineLayoutManager
	{
	public:
		VulkanPipelineLayoutManager() {}
		~VulkanPipelineLayoutManager() {}

		uint32_t Create(VkDevice Device, const PipelineLayoutInfo& Info,
			const std::array<VkDescriptorSetLayout, int(BindlessSetTypes::COUNT)>& BindlessLayouts);
		void Destroy(VkDevice Device, uint32_t Index);
		void Clear(VkDevice Device);

		VkPipelineLayout Get(uint32_t Index);
		VkPipelineLayout Get(const PipelineLayoutInfo& Info) { return _Layouts[_LayoutMap[Info]]; }
	private:
		std::unordered_map<PipelineLayoutInfo, uint32_t, PipelineLayoutInfoHash> _LayoutMap;
		std::vector<VkPipelineLayout> _Layouts;
	};

	class VulkanGraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline() {}
		~VulkanGraphicsPipeline() {}

		void Init(const GraphicsPipelineCreateInfo& CreateInfo, VkDevice Device, VkFormat SwapChainFormat, VulkanPipelineLayoutManager& LayoutManager,
			const std::array<VkDescriptorSetLayout, int(BindlessSetTypes::COUNT_NON_USER)>& BindlessLayouts);
		void Terminate(VkDevice Device);

		VkPipeline GetHandle() const { return _Pipeline; }
		const PipelineLayoutInfo& GetLayoutInfo() const {
			return { _Info.PushConstants,_Info.UniformInputs };
		}
		VkPipelineLayout GetLayoutFromInfo(VulkanPipelineLayoutManager& Manager) const
		{
			return Manager.Get({ _Info.PushConstants, _Info.UniformInputs });
		}

		bool IsPersonalSetLayoutUsed() const { return _PersonalSetLayoutUsed; }
	private:
		VkPipeline _Pipeline;
		ReflectedShaderInfo _Info;
		uint32_t _LayoutIndex;
		std::array<VkDescriptorSetLayout, int(int(BindlessSetTypes::COUNT) - int(BindlessSetTypes::COUNT_NON_USER))> _PersonalSetLayouts;
		bool _PersonalSetLayoutUsed = false;
	};
}

