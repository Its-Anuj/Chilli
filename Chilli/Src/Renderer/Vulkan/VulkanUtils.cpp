#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_ENABLE_BETA_EXTENSIONS
#include "C:\VulkanSDK\1.3.275.0\Include\vulkan\vulkan.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "VulkanRenderer.h"
#include "VulkanUtils.h"

namespace Chilli
{
	void VulkanUtils::Init(VkInstance Instance, const VulkanDevice& Device, const VulkanSwapChainKHR& SwapChain)
	{
		Get()._Device = Device;
		Get()._SwapChain = SwapChain;
		Get()._PoolManager.Init(Device);
		Get()._Allocator.Init(Instance, Device.GetPhysicalDevice()->PhysicalDevice, Device.GetHandle());
	}

	void VulkanUtils::ShutDown()
	{
		Get()._PoolManager.ShutDown();
		Get()._Allocator.ShutDown();
	}

	void VulkanPoolsManager::Init(const VulkanDevice& Device)
	{
		_GraphicsPoolManager.Init(Device.GetHandle(), Device.GetPhysicalDevice()->Info.QueueIndicies,
			QueueFamilies::GRAPHICS);
		_TransferPoolManager.Init(Device.GetHandle(), Device.GetPhysicalDevice()->Info.QueueIndicies,
			QueueFamilies::TRANSFER);

		_Device = Device.GetHandle();
		_Queues[QueueFamilies::GRAPHICS] = Device.GetQueue(QueueFamilies::GRAPHICS);
		_Queues[QueueFamilies::TRANSFER] = Device.GetQueue(QueueFamilies::TRANSFER);
	}

	void VulkanPoolsManager::ShutDown()
	{
		_GraphicsPoolManager.ShutDown(_Device);
		_TransferPoolManager.ShutDown(_Device);
	}

	void VulkanPoolsManager::BeginRecording(BeginCommandBufferInfo& Info)
	{

	}

	void VulkanPoolsManager::BeginSingleTimeCommand(BeginCommandBufferInfo& Info)
	{
		std::vector<VkCommandBuffer> Buffers;
		CreateCommandBuffers(Buffers, 1, Info.Family);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		Info.Buffer = Buffers[0];
		vkBeginCommandBuffer(Info.Buffer, &beginInfo);
	}

	void VulkanPoolsManager::EndSingleTimeCommand(BeginCommandBufferInfo& Info)
	{
		vkEndCommandBuffer(Info.Buffer);

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &Info.Buffer;

		vkQueueSubmit(_Queues[Info.Family], 1, &submit, VK_NULL_HANDLE);

		vkQueueWaitIdle(_Queues[Info.Family]);

		std::vector<VkCommandBuffer> Buffers = { Info.Buffer };
		if (Info.Family == QueueFamilies::GRAPHICS)
			_GraphicsPoolManager.FreeCommandBuffers(Buffers);
		if (Info.Family == QueueFamilies::TRANSFER)
			_TransferPoolManager.FreeCommandBuffers(Buffers);
		Info.Buffer = VK_NULL_HANDLE;
	}

	void VulkanPoolsManager::CreateCommandBuffers(std::vector<VkCommandBuffer>& Buffers, uint32_t Count, QueueFamilies Family)
	{
		auto Manager = &_TransferPoolManager;
		if (Family == QueueFamilies::GRAPHICS)
			Manager = &_GraphicsPoolManager;

		Manager->CreateCommmandBuffers(Buffers, Count);
	}

	void VulkanPoolsManager::CopyBufferToBuffer(const VkBufferCopy& Copy, VkBuffer Src, VkBuffer Dst)
	{
		BeginCommandBufferInfo Info{};
		Info.Family = QueueFamilies::TRANSFER;

		BeginSingleTimeCommand(Info);
		vkCmdCopyBuffer(Info.Buffer, Src, Dst, 1, &Copy);
		EndSingleTimeCommand(Info);
	}

	void CommandPoolManager::Init(VkDevice Device, QueueFamilyIndicies Indicies, QueueFamilies Family)
	{
		VkCommandPoolCreateInfo Info{};
		Info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		Info.queueFamilyIndex = Indicies.Queues[Family].value();
		Info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VULKAN_SUCCESS_ASSERT(vkCreateCommandPool(Device, &Info, nullptr, &_Pool), "Command Pool Creation Failed!");
		this->Device = Device;
		_UsingFamily = Family;
	}

	void CommandPoolManager::ShutDown(VkDevice Device)
	{
		vkDestroyCommandPool(Device, _Pool, nullptr);
	}

	void CommandPoolManager::CreateCommmandBuffers(std::vector<VkCommandBuffer>& Buffers, uint32_t Count)
	{
		Buffers.resize(Count);

		VkCommandBufferAllocateInfo Info{};
		Info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		Info.commandBufferCount = (uint32_t)Count;
		Info.commandPool = _Pool;
		Info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VULKAN_SUCCESS_ASSERT(vkAllocateCommandBuffers(Device, &Info, Buffers.data()), "Command Buffer Failed to Allocate!")
	}

	void CommandPoolManager::FreeCommandBuffers(std::vector<VkCommandBuffer>& Buffers)
	{
		vkFreeCommandBuffers(Device, _Pool, Buffers.size(), Buffers.data());
	}

	void VulkanPoolsManager::CopyBufferToImage(const VkBufferImageCopy& Copy, VkBuffer SrcBuffer, VkImage DstImage)
	{
		BeginCommandBufferInfo Info{};
		Info.Buffer = VK_NULL_HANDLE;
		Info.Family = QueueFamilies::TRANSFER;

		VulkanUtils::GetPoolManager().BeginSingleTimeCommand(Info);

		// Issue copy command
		vkCmdCopyBufferToImage(
			Info.Buffer,
			SrcBuffer,
			DstImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&Copy);
		VulkanUtils::GetPoolManager().EndSingleTimeCommand(Info);

	}

	void VulkanAllocator::Init(VkInstance Instance, VkPhysicalDevice PhysicalDevice, VkDevice Device)
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = PhysicalDevice;
		allocatorInfo.device = Device;
		allocatorInfo.instance = Instance;
		VULKAN_SUCCESS_ASSERT(vmaCreateAllocator(&allocatorInfo, &_Allocator), "VMA Allocator failed to initialize!");
		VULKAN_PRINTLN("VMA Created!!");
	}

	void VulkanAllocator::ShutDown()
	{
		vmaDestroyAllocator(_Allocator);
	}
}
