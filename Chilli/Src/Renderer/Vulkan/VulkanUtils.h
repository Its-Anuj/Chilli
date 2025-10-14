#pragma once

#include "Shader.h"
#include "VulkanDevice.h"

namespace Chilli
{
	struct DescriptorTypeSpec
	{
		ShaderUniformTypes Type;
		uint32_t Count;
	};

	struct DescriptorPoolsSpec
	{
		std::vector < DescriptorTypeSpec> Types;
		uint32_t MaxSet;
	};

	class DescriptorPoolManager
	{
	public:
		DescriptorPoolManager() {}
		~DescriptorPoolManager() {}

		void Init(VkDevice device, const DescriptorPoolsSpec& Spec);
		void ShutDown();

		void CreateDescSets(std::vector<VkDescriptorSet>& Sets, uint32_t Count, const std::vector<VkDescriptorSetLayout>& Layouts);
		void FreeDescSets(std::vector<VkDescriptorSet>& Sets);

		VkDevice Device = VK_NULL_HANDLE;
	private:
		VkDescriptorPool _Pool;
		DescriptorPoolsSpec _Spec;
	};

	class CommandPoolManager
	{
	public:
		CommandPoolManager() {}
		~CommandPoolManager() {}

		void Init(VkDevice Device, QueueFamilyIndicies Indicies, QueueFamilies Family);
		void ShutDown(VkDevice Device);

		void CreateCommmandBuffers(std::vector<VkCommandBuffer>& Buffers, uint32_t Count);
		void FreeCommandBuffers(std::vector<VkCommandBuffer>& Buffers);

		QueueFamilies GetUsingFamily() const { return _UsingFamily; }

		VkDevice Device = VK_NULL_HANDLE;
	private:
		VkCommandPool _Pool;
		uint32_t _Count;
		QueueFamilies _UsingFamily;
	};

	struct BeginCommandBufferInfo
	{
		VkCommandBuffer Buffer;
		QueueFamilies Family;
	};

	class VulkanPoolsManager
	{
	public:
		VulkanPoolsManager() {}
		~VulkanPoolsManager() {}

		void Init(const VulkanDevice& Device);
		void ShutDown();

		void BeginRecording(BeginCommandBufferInfo& Info);
		void BeginSingleTimeCommand(BeginCommandBufferInfo& Info);
		void EndSingleTimeCommand(BeginCommandBufferInfo& Info);

		void CreateCommandBuffers(std::vector<VkCommandBuffer>& Buffers, uint32_t Coumt, QueueFamilies Family);

		void CopyBuffers(VkBuffer Src, VkBuffer Dst, VkBufferCopy Copy);

		void CreateDescSets(std::vector<VkDescriptorSet>& Sets, uint32_t Count, const  std::vector<VkDescriptorSetLayout>& Layouts) { _DescPoolManager.CreateDescSets(Sets, Count, Layouts); }
		void FreeDescSets(std::vector<VkDescriptorSet>& Sets) { _DescPoolManager.FreeDescSets(Sets); }

	private:
		VkDevice _Device;
		VkQueue _Queues[QueueFamilies::COUNT];

		CommandPoolManager _GraphicsPoolManager, _TransferPoolManager;
		DescriptorPoolManager _DescPoolManager;
	};

	class VulkanAllocator
	{
	public:
		VulkanAllocator() {}
		~VulkanAllocator() {}

		void Init(VkInstance Instance, VkPhysicalDevice PhysicalDevice, VkDevice Device);
		void ShutDown();

		VmaAllocator GetAllocator()  const { return _Allocator; }
	private:
		VmaAllocator _Allocator;
	};

	class VulkanUtils
	{
	public:
		static VulkanUtils& Get()
		{
			static VulkanUtils Instance;
			return Instance;
		}

		static VulkanPoolsManager& GetPoolManager() { return Get()._PoolManager; }
		static VulkanAllocator& GetVulkanAllocator() { return Get()._Allocator; }

		static void Init(VkInstance Instance, const VulkanDevice& Device);
		static void ShutDown();

		static const VulkanDevice& GetDevice() { return Get()._Device; }
		static VkDevice GetLogicalDevice() { return Get()._Device.GetHandle(); }

	private:
		VulkanUtils() {}
		~VulkanUtils() {}
	private:
		VulkanDevice _Device;

		VulkanAllocator _Allocator;
		VulkanPoolsManager _PoolManager;
	};
}
