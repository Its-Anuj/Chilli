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
		std::string Name = "";
		uint32_t Binding, SetLayoutIndex, Count;
		ShaderStageType Stage;
		ShaderUniformTypes Type;
		VkSampler* pImmutableSamplers;

		bool operator==(const ReflectedUniformInput& other) const {
			return Binding == other.Binding &&
				Count == other.Count &&
				Stage == other.Stage &&
				Type == other.Type &&
				SetLayoutIndex == other.SetLayoutIndex &&
				pImmutableSamplers == other.pImmutableSamplers; // Pointer comparison
		}
	};

	struct ReflectedShaderInfo {
		std::vector< ReflectedVertexInput> VertexInputs;
		std::vector<VkDescriptorSetLayout> SetLayouts;
		std::vector<VkPushConstantRange> PushConstantRanges;
		std::vector<ReflectedUniformInput> Bindings;
	};

	struct PipelineLayoutCacheInfo
	{
		std::vector<VkPushConstantRange> PushConstantRanges;
		std::vector<ReflectedUniformInput> Bindings;
	};

	enum class BindingPipelineFeatures
	{
		MAX_TEXTURES = 100000,
		MAX_SAMPLERS = 16,

	};

	class VulkanGraphiscPipeline : public GraphicsPipeline
	{
	public:
		VulkanGraphiscPipeline(const GraphicsPipelineSpec& Spec) { Init(Spec); }
		~VulkanGraphiscPipeline() {}

		virtual void Init(const GraphicsPipelineSpec& Spec) override;
		virtual void ReCreate(const GraphicsPipelineSpec& Spec) override;
		virtual void Destroy() override;

		virtual void Bind() const override;

		VkPipeline GetHandle()const { return _Pipeline; }
		const ReflectedShaderInfo& GetInfo() const { return _Info; }
		size_t GetLayoutHash() const { return _PipelineLayoutHash; }

		virtual const GraphicsPipelineSpec& GetSpec() const  override { return _Spec; }
	private:
		void _FillInfo();
	private:
		GraphicsPipelineSpec _Spec;
		VkPipeline _Pipeline;
		ReflectedShaderInfo _Info;
		size_t _PipelineLayoutHash = -1;
	};

	struct SetsInfo
	{
		std::vector<ReflectedUniformInput> BindingInfos;
		VkDescriptorSet Set;
	};

	class VulkanBindlessSetManager : public BindlessSetManager
	{
	public:
		VulkanBindlessSetManager() {}
		~VulkanBindlessSetManager() {}

		void Init();
		void Free();

		virtual void CreateMaterialSBO(size_t SizeInBytes, uint32_t MaxIndex) override;
		virtual void CreateObjectSBO(size_t SizeInBytes, uint32_t MaxIndex)  override;
		bool Bind(VkCommandBuffer CommandBuffer, VkPipelineLayout Layout);

		virtual void UpdateSceneUBO(void* Data) override;
		virtual void SetSceneUBO(size_t SceneUBOSize) override;

		virtual void UpdateGlobalUBO(void* Data) override;
		virtual void SetGlobalUBO(size_t GlobalUBOSize) override;

		const SetsInfo& GetGlobalSet() { return _GlobalUBOSet; }
		const SetsInfo& GetTexSamplerSet() { return _TexSamplersSet; }
		const SetsInfo& GetSceneSet() { return _SceneDataSet; }
		const SetsInfo& GetMaterialSet() { return _MaterialsDataSet; }
		const SetsInfo& GetObjectSets() { return _ObjectDataSet; }

		virtual uint32_t AddTexture(const std::shared_ptr<Texture>& Tex) override;
		// returns -1 on not found
		virtual uint32_t GetTextureIndex(const std::shared_ptr<Texture>& Tex) override;
		virtual void RemoveTexture(const std::shared_ptr<Texture>& Tex) override;
		virtual uint32_t GetTextureCount() const  override { return _TextureManager.GetCount(); }

		virtual bool IsTexPresent(const std::shared_ptr<Texture>& Tex) const override;

		virtual void AddSampler(const std::shared_ptr<Sampler>& Sam) override;

		virtual void UpdateMaterial(const Material& Mat, uint32_t Index) override;
		virtual void UpdateObject(Object& Obj, uint32_t Index) override;

		RenderCommandPushData FromRenderCommand(const RenderCommandSpec& Spec);

		BindlessSetMaterialManager GetMaterialManager() { return _MaterialManager; }
	private:
		void _InitSetLayouts();
		void _InitPool();
		void _InitSets();

		void _DelPool();
		void _DelSets();
		void _DelLayouts();
		void _DelBuffers();
	private:
		VkDescriptorPool _Pool;
		VkDescriptorSetLayout _GlobalUBOLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout _SceneUBOLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout _TexSamplerUBOLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout _MaterialsUBOLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout _ObjectsUBOLayout = VK_NULL_HANDLE;
		SetsInfo _GlobalUBOSet; // Corresponds to set 0 
		SetsInfo _SceneDataSet;// Corresponds to set 1
		SetsInfo _TexSamplersSet; // Corresponds to set 2
		SetsInfo _MaterialsDataSet; // Corresponds to set 3
		SetsInfo _ObjectDataSet; // Corresponds to set 4

		BindlessSetTextureManager _TextureManager;
		BindlessSetSamplerManager _SamplerManager;
		BindlessSetMaterialManager _MaterialManager;
		BindlessSetObjectManager _ObjectManager;

		std::shared_ptr<Chilli::UniformBuffer> _GlobalUniformBuffer, _SceneUniformBuffer;

		std::shared_ptr<StorageBuffer> _MaterialSBO;
		std::shared_ptr<StorageBuffer> _ObjectSBO;
		uint32_t _MaxObjectCount;
		uint32_t _MaxMaterialCount;
	};

	struct VulkanPipelineLayoutInfo
	{
		std::vector<VkDescriptorSetLayout> Layouts;
		VkPushConstantRange PushConstant;

		bool operator==(const VulkanPipelineLayoutInfo& other) const {
			return Layouts == other.Layouts &&
				PushConstant.offset == other.PushConstant.offset &&
				PushConstant.size == other.PushConstant.size &&
				PushConstant.stageFlags == other.PushConstant.stageFlags;
		}
	};

	class VulkanPipelineLayoutCache
	{
	public:
		VulkanPipelineLayoutCache() {}
		~VulkanPipelineLayoutCache() {}

		static VulkanPipelineLayoutCache& GetPipelineLayoutCache();

		VkPipelineLayout GetOrCreate(const VulkanPipelineLayoutInfo& Info);
		VkPipelineLayout GetOrCreate(const VulkanPipelineLayoutInfo& Info, uint32_t SetIndex);
		VkPipelineLayout GetOrCreate(size_t Index) { return _LayoutsCache[Index].second; }
		uint32_t Count() const { return _LayoutsCache.size(); }
		void Flush();
	private:
		VkPipelineLayout CreatePipelineLayout(const VulkanPipelineLayoutInfo& Info);

	private:
		std::unordered_map< size_t, std::pair<VulkanPipelineLayoutInfo, VkPipelineLayout>> _LayoutsCache;
	};

}

// Helper function for combining hashes
inline void hash_combine(size_t& seed, size_t hash) {
	hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
	seed ^= hash;
}

namespace std {
	template<>
	struct hash<Chilli::ReflectedUniformInput> {
		size_t operator()(const Chilli::ReflectedUniformInput& input) const {
			size_t seed = 0;

			// Combine all members using boost-style hash combine
			auto hash_combine = [&seed](size_t hash) {
				hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= hash;
				};

			hash_combine(hash<uint32_t>()(input.Binding));
			hash_combine(hash<uint32_t>()(input.Count));
			hash_combine(hash<uint32_t>()(input.SetLayoutIndex));
			hash_combine(hash<Chilli::ShaderStageType>()(input.Stage));
			hash_combine(hash<Chilli::ShaderUniformTypes>()(input.Type));

			// Hash the pointer value, not the sampler contents
			hash_combine(hash<uintptr_t>()(reinterpret_cast<uintptr_t>(input.pImmutableSamplers)));

			return seed;
		}
	};

	template<>
	struct hash<Chilli::VulkanPipelineLayoutInfo> {
		size_t operator()(const Chilli::VulkanPipelineLayoutInfo& info) const {
			size_t seed = 0;

			// Hash descriptor set layouts
			for (const auto& layout : info.Layouts) {
				hash_combine(seed, hash<VkDescriptorSetLayout>()(layout));
			}

			// Hash push constant range
			hash_combine(seed, hash<uint32_t>()(info.PushConstant.offset));
			hash_combine(seed, hash<uint32_t>()(info.PushConstant.size));
			hash_combine(seed, hash<VkShaderStageFlags>()(info.PushConstant.stageFlags));

			return seed;
		}
	};
}