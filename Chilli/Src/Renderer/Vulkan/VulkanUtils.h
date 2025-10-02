#pragma once

namespace Chilli
{
	enum class ShaderUniformTypes
	{
		UNIFORM,
		SAMPLED_IMAGE
	};

	struct ShaderDescAttribs
	{
		ShaderUniformTypes Type;
		uint32_t MaxCount;
	};

	struct DescriptorPoolsManager
	{
	public:
		DescriptorPoolsManager() {}
		~DescriptorPoolsManager() {}

		void Init(VkDevice device, const std::vector< ShaderDescAttribs>& Attribs, uint32_t MaxSets);
		void Destroy();

		void CreateDescSets(std::vector<VkDescriptorSet>& Sets, uint32_t Count, std::vector<VkDescriptorSetLayout>& Layouts);
		void FreeDescSets(std::vector<VkDescriptorSet>& Sets);

		VkDevice Device = VK_NULL_HANDLE;
	private:
		VkDescriptorPool _Pool;
		uint32_t _SetsCount;
	};

	struct CommandPoolsManager
	{
	public:
		CommandPoolsManager() {}
		~CommandPoolsManager() {}

		void Init(VkDevice device, QueueFamilyIndicies Indicies, QueueFamilies ChosenFamily);
		void Destroy();

		void CreateCommandBuffers(std::vector<VkCommandBuffer>& Buffers, uint32_t Count,
			VkCommandBufferLevel Level);
		void FreeCommandBuffer(std::vector<VkCommandBuffer>& Buffers);

		VkDevice Device = VK_NULL_HANDLE;

		uint32_t GetBuffersCount() { return _BuffersCount; }
		uint32_t GetPoolsCount() { return 0; }
	private:
		VkCommandPool _Pool;
		uint32_t _BuffersCount;
	};

	struct VulkanUtilsSpec
	{
		VkInstance Instance;
		VulkanDevice* Device;
	};

	class VulkanUtils
	{
	public:
		static VulkanUtils& Get()
		{
			static VulkanUtils Instance;
			return Instance;
		}

		static void Init(const VulkanUtilsSpec& Spec);
		static void ShutDown();

		static void CreateCommandBuffers(QueueFamilies Family, std::vector<VkCommandBuffer>& Buffers, uint32_t Count);
		static void FreeCommandBuffers(QueueFamilies Family, std::vector<VkCommandBuffer>& Buffers);

		static void CreateDescSets(std::vector<VkDescriptorSet>& Sets, uint32_t Count, std::vector<VkDescriptorSetLayout>& Layouts);
		static void FreeDescSets(std::vector<VkDescriptorSet>& Sets);

		static void BeginSingleTimeCommands(VkCommandBuffer& SingleTimeBuffer, QueueFamilies Family);
		static void EndSingleTimeCommands(VkCommandBuffer& SingleTimeBuffer, QueueFamilies Family);
		static VkResult SubmitQueue(const VkSubmitInfo& SubmitInfo, QueueFamilies Family, VkFence& Fence);

		static void CreateAllocator(VkInstance Instance, VkPhysicalDevice PhysicalDevice, VkDevice Device);
		static void DestroyAllocator();

		static  VmaAllocator GetAllocator();
	private:
		void _CreateCommandPools();
		void _CreateDescPools();
	private:
		VulkanUtilsSpec _Spec;
		DescriptorPoolsManager _DescManager;
		CommandPoolsManager _GraphicsCmdManager, _TransferCmdManager;
		VmaAllocator _Allocator;
	};
}
