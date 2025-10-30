#pragma once

#include "VulkanDevice.h"

namespace Chilli
{
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

		void CopyBufferToBuffer(const VkBufferCopy& Copy, VkBuffer Src, VkBuffer Dst);
		void CopyBufferToImage(const VkBufferImageCopy& Copy, VkBuffer SrcBuffer, VkImage DstImage);

	private:
		VkDevice _Device;
		VkQueue _Queues[QueueFamilies::COUNT];

		CommandPoolManager _GraphicsPoolManager, _TransferPoolManager;
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

		static void Init(VkInstance Instance, const VulkanDevice& Device, const VulkanSwapChainKHR& SwapChain);
		static void ShutDown();

		static const VulkanDevice& GetDevice() { return Get()._Device; }
		static VkDevice GetLogicalDevice() { return Get()._Device.GetHandle(); }

		static VulkanSwapChainKHR GetSwapChainKHR() { return Get()._SwapChain; }

		static VkCommandBuffer GetActiveCommandBuffer() { return Get()._ActiveCommandBuffer; }
		static void SetActiveCommandBuffer(VkCommandBuffer Cmd) { Get()._ActiveCommandBuffer = Cmd; }

	private:
		VulkanUtils() {}
		~VulkanUtils() {}
	private:
		VulkanDevice _Device;
		VulkanSwapChainKHR _SwapChain;
		VkCommandBuffer _ActiveCommandBuffer = VK_NULL_HANDLE;

		VulkanAllocator _Allocator;
		VulkanPoolsManager _PoolManager;
	};
}
