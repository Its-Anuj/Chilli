#include "ChV_PCH.h"
#include "vulkan\vulkan.h"

#include "VulkanPipeline.h"
#include "UUID\UUID.h"
#include "VulkanConversions.h"
#include <unordered_set>
#include <algorithm>

#include "spirv_reflect.h"

void _ReadFile(const char* FilePath, std::vector<char>& CharData);
VkShaderModule _CreateShaderModule(VkDevice device, std::vector<char>& code);
VkPipelineShaderStageCreateInfo _CreateShaderStages(VkShaderModule module, VkShaderStageFlagBits type, const char* Name);
VkVertexInputAttributeDescription GetAttributeDescription(int Binding, int Location, int Offset, VkFormat Format);

// Helper to map SPIR-V format to VkFormat
VkFormat SPVFormatToVkFormat(SpvReflectFormat fmt);
VkFormat ShaderTypeToFormat(Chilli::ShaderObjectTypes Type);
Chilli::ShaderObjectTypes FormatToShaderType(VkFormat format);

namespace Chilli
{
	void CombineVertFragUniformReflect(std::vector<Chilli::ReflectedSetUniformInput>& UniformInputs,
		std::vector< Chilli::ReflectedSetUniformInput>& VertInfo, std::vector< Chilli::ReflectedSetUniformInput>& FragInfo);

	void CombineVertFragPushConstants(
		std::vector<VkPushConstantRange>& out,
		const std::vector<VkPushConstantRange>& vertPC,
		const std::vector<VkPushConstantRange>& fragPC);

	// stageFlag = VK_SHADER_STAGE_VERTEX_BIT / FRAGMENT_BIT etc.
	ReflectedShaderInfo ReflectShaderModule(VkDevice device, const std::string& path, VkShaderStageFlagBits stageFlag)
	{
		ReflectedShaderInfo info;
		std::vector<char> code;
		_ReadFile(path.c_str(), code);

		SpvReflectShaderModule module;
		SpvReflectResult result = spvReflectCreateShaderModule(code.size(), code.data(), &module);
		assert(result == SPV_REFLECT_RESULT_SUCCESS);

		// ---------- Reflect Vertex Input Layout -----
		uint32_t VertexInputCount = 0;
		spvReflectEnumerateInputVariables(&module, &VertexInputCount, nullptr);
		std::vector<SpvReflectInterfaceVariable*> VertexInputs(VertexInputCount);
		spvReflectEnumerateInputVariables(&module, &VertexInputCount, VertexInputs.data());

		uint32_t VertexInputOffset = 0;

		for (auto* VertexInputInfo : VertexInputs)
		{
			// Skip built-ins like gl_VertexIndex, gl_InstanceIndex
			if (VertexInputInfo->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
				continue;

			ReflectedVertexInput input{};
			input.Name = VertexInputInfo->name ? VertexInputInfo->name : "";
			input.Location = VertexInputInfo->location;
			input.Format = VkFormatToShaderObjectType(SPVFormatToVkFormat(VertexInputInfo->format));
			input.Offset = VertexInputOffset; // You'll compute offsets later
			VertexInputOffset += ShaderTypeToSize(FormatToShaderType(SPVFormatToVkFormat(VertexInputInfo->format)));

			info.VertexInputs.push_back(input);
		}

		// -------- Descriptor Sets --------
		uint32_t set_count = 0;
		spvReflectEnumerateDescriptorSets(&module, &set_count, nullptr);

		std::vector<SpvReflectDescriptorSet*> sets(set_count);
		spvReflectEnumerateDescriptorSets(&module, &set_count, sets.data());
		info.UniformInputs.reserve(set_count);

		for (uint32_t si = 0; si < set_count; ++si) {
			SpvReflectDescriptorSet* s = sets[si];
			ReflectedSetUniformInput mySet;
			mySet.Set = s->set;
			mySet.Bindings.reserve(s->binding_count);

			for (uint32_t bi = 0; bi < s->binding_count; ++bi) {
				SpvReflectDescriptorBinding* b = s->bindings[bi];

				ReflectedBindingUniformInput rb;
				rb.Binding = b->binding;
				rb.Set = b->set;
				rb.Count = b->count;
				rb.Type = SpirvTypeToChilli(b->descriptor_type);
				rb.Stage = Chilli::VkShaderStageToChilli(stageFlag);

				// FIX: Copy name into char[32]
				if (b->name) {
					strncpy(rb.Name, b->name, SHADER_UNIFORM_BINDING_NAME_SIZE - 1);
					rb.Name[SHADER_UNIFORM_BINDING_NAME_SIZE - 1] = '\0';
				}
				else
					rb.Name[0] = '\0';

				mySet.Bindings.push_back(rb);
			}

			info.UniformInputs.push_back(mySet);
		}

		// ---------- Reflect Push Constants ----------
		uint32_t pcCount = 0;
		spvReflectEnumeratePushConstantBlocks(&module, &pcCount, nullptr);
		std::vector<SpvReflectBlockVariable*> pcs(pcCount);
		spvReflectEnumeratePushConstantBlocks(&module, &pcCount, pcs.data());

		for (auto* pc : pcs)
		{
			ReflectedPushConstantRange range{};
			range.Offset = 0;
			range.Size = pc->size;
			range.Stage = VkFlagBitToShaderStageType(stageFlag);
			info.PushConstants.push_back(range);
		}

		spvReflectDestroyShaderModule(&module);
		return info;
	}

	void CombineVertFragUniformReflect(std::vector<ReflectedSetUniformInput>& UniformInputs, std::vector<ReflectedSetUniformInput>& VertInfo, std::vector<ReflectedSetUniformInput>& FragInfo)
	{
		// Combine sets from both vertex and fragment shaders
		std::unordered_set<uint32_t> allSets;
		for (auto& vertSet : VertInfo) allSets.insert(vertSet.Set);
		for (auto& fragSet : FragInfo) allSets.insert(fragSet.Set);

		// Process each set
		for (uint32_t setIndex : allSets)
		{
			ReflectedSetUniformInput combinedSet;
			combinedSet.Set = setIndex;

			// Find this set in vertex and fragment shaders
			auto vertSetIt = std::find_if(VertInfo.begin(), VertInfo.end(),
				[setIndex](const ReflectedSetUniformInput& set) { return set.Set == setIndex; });

			auto fragSetIt = std::find_if(FragInfo.begin(), FragInfo.end(),
				[setIndex](const ReflectedSetUniformInput& set) { return set.Set == setIndex; });

			// Combine bindings from both shaders
			std::unordered_set<uint32_t> allBindings;

			if (vertSetIt != VertInfo.end())
			{
				for (auto& binding : vertSetIt->Bindings) allBindings.insert(binding.Binding);
			}

			if (fragSetIt != FragInfo.end())
			{
				for (auto& binding : fragSetIt->Bindings) allBindings.insert(binding.Binding);
			}

			// Process each binding in this set
			for (uint32_t bindingIndex : allBindings)
			{
				ReflectedBindingUniformInput combinedBinding;
				combinedBinding.Binding = bindingIndex;
				combinedBinding.Set = setIndex;

				// Look for this binding in vertex shader
				ReflectedBindingUniformInput* vertBinding = nullptr;
				if (vertSetIt != VertInfo.end())
				{
					auto it = std::find_if(vertSetIt->Bindings.begin(), vertSetIt->Bindings.end(),
						[bindingIndex](const ReflectedBindingUniformInput& binding) {
							return binding.Binding == bindingIndex;
						});
					if (it != vertSetIt->Bindings.end()) vertBinding = &(*it);
				}

				// Look for this binding in fragment shader
				ReflectedBindingUniformInput* fragBinding = nullptr;
				if (fragSetIt != FragInfo.end())
				{
					auto it = std::find_if(fragSetIt->Bindings.begin(), fragSetIt->Bindings.end(),
						[bindingIndex](const ReflectedBindingUniformInput& binding) {
							return binding.Binding == bindingIndex;
						});
					if (it != fragSetIt->Bindings.end()) fragBinding = &(*it);
				}

				// Combine the binding information
				if (vertBinding && fragBinding)
				{
					// Binding exists in both shaders - combine stage flags
					// Prefer vertex shader name, or could check if they match
					strncpy(combinedBinding.Name, vertBinding->Name, SHADER_UNIFORM_BINDING_NAME_SIZE - 1);
					combinedBinding.Name[SHADER_UNIFORM_BINDING_NAME_SIZE - 1] = '\0';

					combinedBinding.Type = vertBinding->Type;
					combinedBinding.Count = vertBinding->Count;
					combinedBinding.Stage = Chilli::ShaderStageType::SHADER_STAGE_ALL_GRAPHICS; // Combined stage
				}
				else if (vertBinding)
				{
					// Binding only in vertex shader
					combinedBinding = *vertBinding;
					combinedBinding.Stage = Chilli::ShaderStageType::SHADER_STAGE_VERTEX;
				}
				else if (fragBinding)
				{
					// Binding only in fragment shader
					combinedBinding = *fragBinding; // ok for POD
					// but ensure Name is copied safely:
					char tempName[SHADER_UNIFORM_BINDING_NAME_SIZE];
					strncpy(tempName, fragBinding->Name, SHADER_UNIFORM_BINDING_NAME_SIZE);
					memcpy(combinedBinding.Name, tempName, SHADER_UNIFORM_BINDING_NAME_SIZE);

					combinedBinding.Stage = Chilli::ShaderStageType::SHADER_STAGE_FRAGMENT;
				}

				combinedSet.Bindings.push_back(combinedBinding);
			}

			// Sort bindings by binding index for consistency
			std::sort(combinedSet.Bindings.begin(), combinedSet.Bindings.end(),
				[](const Chilli::ReflectedBindingUniformInput& a, const Chilli::ReflectedBindingUniformInput& b) {
					return a.Binding < b.Binding;
				});

			UniformInputs.push_back(combinedSet);
		}

		// Sort sets by set index for consistency
		std::sort(UniformInputs.begin(), UniformInputs.end(),
			[](const Chilli::ReflectedSetUniformInput& a, const Chilli::ReflectedSetUniformInput& b) {
				return a.Set < b.Set;
			});

	}
	void CombineVertFragPushConstants(
		std::vector<ReflectedPushConstantRange>& out,
		const std::vector<Chilli::ReflectedPushConstantRange>& vertPC,
		const std::vector<Chilli::ReflectedPushConstantRange>& fragPC)
	{
		// Map keyed by (offset, size)
		// Because push constants are uniquely identified by these two fields.
		struct Key {
			uint32_t offset;
			uint32_t size;
			bool operator==(const Key& other) const {
				return offset == other.offset && size == other.size;
			}
		};

		struct KeyHash {
			size_t operator()(const Key& k) const noexcept {
				return (size_t(k.offset) << 32) ^ size_t(k.size);
			}
		};

		std::unordered_map<Key, ReflectedPushConstantRange, KeyHash> merged;

		// Insert vertex PC ranges
		for (const auto& pc : vertPC)
		{
			Key key{ pc.Offset, pc.Size };
			merged[key] = pc;
			merged[key].Stage = SHADER_STAGE_VERTEX;
		}

		// Insert fragment PC ranges
		for (const auto& pc : fragPC)
		{
			Key key{ pc.Offset, pc.Size };

			auto it = merged.find(key);
			if (it == merged.end())
			{
				// Only exists in fragment shader
				ReflectedPushConstantRange newPC = pc;
				newPC.Stage = SHADER_STAGE_FRAGMENT;
				merged[key] = newPC;
			}
			else
			{
				// Exists in both: merge stageFlags
				it->second.Stage |= SHADER_STAGE_FRAGMENT;
			}
		}

		// Convert map → vector
		out.clear();
		out.reserve(merged.size());

		for (auto& kv : merged)
			out.push_back(kv.second);

		// Sort by offset
		std::sort(out.begin(), out.end(),
			[](const VkPushConstantRange& a, const VkPushConstantRange& b)
			{
				return a.offset < b.offset;
			});
	}

}

namespace Chilli
{
	void VulkanGraphicsPipeline::Init(const GraphicsPipelineFlags& Flags, const ShaderProgram& Program)
	{

	}
}

VkPipelineShaderStageCreateInfo _CreateShaderStages(VkShaderModule module, VkShaderStageFlagBits type, const char* Name)
{
	VkPipelineShaderStageCreateInfo stage{};
	stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage.module = module;
	stage.pName = Name;
	stage.stage = type;

	return stage;
}

VkVertexInputAttributeDescription GetAttributeDescription(int Binding, int Location, int Offset, VkFormat Format)

{
	VkVertexInputAttributeDescription attributeDescriptions{};
	attributeDescriptions.binding = Binding;
	attributeDescriptions.location = Location;
	attributeDescriptions.format = Format; // vec3
	attributeDescriptions.offset = Offset;
	return attributeDescriptions;
}

// Helper to map SPIR-V format to VkFormat
VkFormat SPVFormatToVkFormat(SpvReflectFormat fmt)
{
	switch (fmt) {
	case SPV_REFLECT_FORMAT_R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
	case SPV_REFLECT_FORMAT_R32G32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
	case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
	case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
	default: return VK_FORMAT_UNDEFINED;
	}
}

VkFormat ShaderTypeToFormat(Chilli::ShaderObjectTypes Type)
{
	switch (Type)
	{
	case Chilli::ShaderObjectTypes::FLOAT1:
		return VkFormat::VK_FORMAT_R32_SFLOAT;
	case Chilli::ShaderObjectTypes::FLOAT2:
		return VkFormat::VK_FORMAT_R32G32_SFLOAT;
	case Chilli::ShaderObjectTypes::FLOAT3:
		return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	case Chilli::ShaderObjectTypes::FLOAT4:
		return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
	case Chilli::ShaderObjectTypes::INT1:
		return VkFormat::VK_FORMAT_R32_SINT;
	case Chilli::ShaderObjectTypes::INT2:
		return VkFormat::VK_FORMAT_R32G32_SINT;
	case Chilli::ShaderObjectTypes::INT3:
		return VkFormat::VK_FORMAT_R32G32B32_SINT;
	case Chilli::ShaderObjectTypes::INT4:
		return VkFormat::VK_FORMAT_R32G32B32A32_SINT;
	case Chilli::ShaderObjectTypes::UINT1:
		return VkFormat::VK_FORMAT_R32_UINT;
	case Chilli::ShaderObjectTypes::UINT2:
		return VkFormat::VK_FORMAT_R32G32_UINT;
	case Chilli::ShaderObjectTypes::UINT3:
		return VkFormat::VK_FORMAT_R32G32B32_UINT;
	case Chilli::ShaderObjectTypes::UINT4:
		return VkFormat::VK_FORMAT_R32G32B32A32_UINT;
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

Chilli::ShaderObjectTypes FormatToShaderType(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_R32_SFLOAT:
		return Chilli::ShaderObjectTypes::FLOAT1;
	case VK_FORMAT_R32G32_SFLOAT:
		return Chilli::ShaderObjectTypes::FLOAT2;
	case VK_FORMAT_R32G32B32_SFLOAT:
		return Chilli::ShaderObjectTypes::FLOAT3;
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return Chilli::ShaderObjectTypes::FLOAT4;
	case VK_FORMAT_R32_SINT:
		return Chilli::ShaderObjectTypes::INT1;
	case VK_FORMAT_R32G32_SINT:
		return Chilli::ShaderObjectTypes::INT2;
	case VK_FORMAT_R32G32B32_SINT:
		return Chilli::ShaderObjectTypes::INT3;
	case VK_FORMAT_R32G32B32A32_SINT:
		return Chilli::ShaderObjectTypes::INT4;
	case VK_FORMAT_R32_UINT:
		return Chilli::ShaderObjectTypes::UINT1;
	case VK_FORMAT_R32G32_UINT:
		return Chilli::ShaderObjectTypes::UINT2;
	case VK_FORMAT_R32G32B32_UINT:
		return Chilli::ShaderObjectTypes::UINT3;
	case VK_FORMAT_R32G32B32A32_UINT:
		return Chilli::ShaderObjectTypes::UINT4;
	}
}

Chilli::ShaderUniformTypes SpirvTypeToChilli(SpvReflectDescriptorType Type)
{
	switch (Type)
	{
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
		return Chilli::ShaderUniformTypes::SAMPLER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		return Chilli::ShaderUniformTypes::COMBINED_IMAGE_SAMPLER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		return Chilli::ShaderUniformTypes::SAMPLED_IMAGE;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		return Chilli::ShaderUniformTypes::UNIFORM_BUFFER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		return Chilli::ShaderUniformTypes::STORAGE_BUFFER;
	default:
		return Chilli::ShaderUniformTypes::UNKNOWN;
	}
}
