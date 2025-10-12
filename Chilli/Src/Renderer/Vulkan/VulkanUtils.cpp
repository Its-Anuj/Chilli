#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "C:\VulkanSDK\1.3.275.0\Include\vulkan\vulkan.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "VulkanRenderer.h"
#include "VulkanUtils.h"

namespace Chilli
{
	void VulkanUtils::Init(VkInstance Instance, const VulkanDevice& Device)
	{
		Get()._Device = Device;
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

		DescriptorPoolsSpec DescSpec{};
		DescSpec.Types = {
			{ShaderUniformTypes::UNIFORM, 1000},
			{ShaderUniformTypes::SAMPLED_IMAGE, 500}
		};
		DescSpec.MaxSet = 500;

		_DescPoolManager.Init(Device.GetHandle(), DescSpec);
	}

	void VulkanPoolsManager::ShutDown()
	{
		_GraphicsPoolManager.ShutDown(_Device);
		_TransferPoolManager.ShutDown(_Device);
		_DescPoolManager.ShutDown();
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

	void VulkanPoolsManager::CopyBuffers(VkBuffer Src, VkBuffer Dst, VkBufferCopy Copy)
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

	VkDescriptorType ShaderUTypesToVk(ShaderUniformTypes Type)
	{
		switch (Type)
		{
		case ShaderUniformTypes::UNIFORM: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case ShaderUniformTypes::SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}
	}

	void DescriptorPoolManager::Init(VkDevice device, const DescriptorPoolsSpec& Spec)
	{
		_Spec = Spec;

		std::vector<VkDescriptorPoolSize> Sizes;
		Sizes.reserve(Spec.Types.size());

		for (auto& attrib : Spec.Types)
		{
			VkDescriptorPoolSize size;
			size.descriptorCount = attrib.Count;
			size.type = ShaderUTypesToVk(attrib.Type);
			Sizes.push_back(size);
		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = Sizes.size();
		poolInfo.pPoolSizes = Sizes.data();
		poolInfo.maxSets = Spec.MaxSet;

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &_Pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}

		this->Device = device;
	}

	void DescriptorPoolManager::ShutDown()
	{
		vkDestroyDescriptorPool(Device, _Pool, nullptr);
	}

	void DescriptorPoolManager::CreateDescSets(std::vector<VkDescriptorSet>& Sets, uint32_t Count, std::vector<VkDescriptorSetLayout>& Layouts)
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = _Pool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(Count);
		allocInfo.pSetLayouts = Layouts.data();

		Sets.resize(Count);
		if (vkAllocateDescriptorSets(Device, &allocInfo, Sets.data()) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate descriptor sets!");
	}

	void DescriptorPoolManager::FreeDescSets(std::vector<VkDescriptorSet>& Sets)
	{
		vkFreeDescriptorSets(Device, _Pool, Sets.size(), Sets.data());
	}

}
