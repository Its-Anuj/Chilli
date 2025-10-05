#include "ChV_PCH.h"

#include "vk_mem_alloc.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"

namespace Chilli
{
	VkDescriptorType ShaderUTypesToVk(ShaderUniformTypes Type)
	{
		switch (Type)
		{
		case ShaderUniformTypes::UNIFORM: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case ShaderUniformTypes::SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}
	}

	void DescriptorPoolsManager::Init(VkDevice device, const std::vector< ShaderDescAttribs>& Attribs, uint32_t MaxSets)
	{
		std::vector<VkDescriptorPoolSize> Sizes;
		Sizes.reserve(Attribs.size());

		for (auto& attrib : Attribs)
		{
			VkDescriptorPoolSize size;
			size.descriptorCount = attrib.MaxCount;
			size.type = ShaderUTypesToVk(attrib.Type);
			Sizes.push_back(size);
		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = Sizes.size();
		poolInfo.pPoolSizes = Sizes.data();
		poolInfo.maxSets = MaxSets;

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &_Pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}

		Device = device;
	}

	void DescriptorPoolsManager::Destroy()
	{
		vkDestroyDescriptorPool(Device, _Pool, nullptr);
		Device = VK_NULL_HANDLE;
	}

	void DescriptorPoolsManager::CreateDescSets(std::vector<VkDescriptorSet>& Sets, uint32_t Count,std::vector<VkDescriptorSetLayout>& Layouts)
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

	void DescriptorPoolsManager::FreeDescSets(std::vector<VkDescriptorSet>& Sets)
	{
		vkFreeDescriptorSets(Device, _Pool, Sets.size(), Sets.data());
	}

	VkCommandPool CreateCommandPool(VkDevice device, uint32_t QueueIndex, VkCommandPoolCreateFlags Flags)
	{
		VkCommandPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.queueFamilyIndex = QueueIndex;
		info.flags = Flags;

		VkCommandPool CommandPool;
		VULKAN_SUCCESS_ASSERT(vkCreateCommandPool(device, &info, nullptr, &CommandPool), "Command Pool Createion Failed");
		return CommandPool;
	}

	void CommandPoolsManager::Init(VkDevice device, QueueFamilyIndicies Indicies, QueueFamilies ChosenFamily)
	{
		_Pool = CreateCommandPool(device, Indicies.Queues[ChosenFamily].value(),
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		Device = device;
	}

	void CommandPoolsManager::Destroy()
	{
		vkDestroyCommandPool(Device, _Pool, nullptr);
		Device = VK_NULL_HANDLE;
	}

	void CommandPoolsManager::CreateCommandBuffers(std::vector<VkCommandBuffer>& Buffers, uint32_t Count, VkCommandBufferLevel Level)
	{
		//if (_BuffersCount + 1 > COMMAND_POOL_LIMIT)
		//{
		//	// Crete a new command pool and use that
		//}

		VkCommandBufferAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandPool = _Pool;;
		info.commandBufferCount = Count;
		info.level = Level;
		Buffers.resize(Count);

		VULKAN_SUCCESS_ASSERT(vkAllocateCommandBuffers(Device, &info, Buffers.data()), "Command Buffer Failed!");
		this->_BuffersCount++;
	}

	void CommandPoolsManager::FreeCommandBuffer(std::vector<VkCommandBuffer>& Buffers)
	{
		_BuffersCount -= Buffers.size();
		vkFreeCommandBuffers(Device, _Pool, Buffers.size(), Buffers.data());
	}

	void VulkanUtils::Init(const VulkanUtilsSpec& Spec)
	{
		Get()._Spec = Spec;
		Get()._CreateCommandPools();
		Get()._CreateDescPools();
		CreateAllocator(Spec.Instance, Spec.Device->GetPhysicalDevice()->PhysicalDevice, Spec.Device->GetHandle());
	}

	void VulkanUtils::ShutDown()
	{
		Get()._GraphicsCmdManager.Destroy();
		Get()._TransferCmdManager.Destroy();
		Get()._DescManager.Destroy();
		DestroyAllocator();
	}

	void VulkanUtils::CreateCommandBuffers(QueueFamilies Family, std::vector<VkCommandBuffer>& Buffers, uint32_t Count)
	{
		auto Manager = &Get()._TransferCmdManager;
		if(Family == QueueFamilies::GRAPHICS)
			Manager = &Get()._GraphicsCmdManager;

		Manager->CreateCommandBuffers(Buffers, Count, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	}

	void VulkanUtils::FreeCommandBuffers(QueueFamilies Family, std::vector<VkCommandBuffer>& Buffers)
	{
		auto Manager = &Get()._TransferCmdManager;
		if (Family == QueueFamilies::GRAPHICS)
			Manager = &Get()._GraphicsCmdManager;

		Manager->FreeCommandBuffer(Buffers);
	}

	void VulkanUtils::BeginSingleTimeCommands(VkCommandBuffer& SingleTimeBuffer, QueueFamilies Family)
	{
		std::vector<VkCommandBuffer> Buffers;
		Get().CreateCommandBuffers(Family, Buffers, 1);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		SingleTimeBuffer = Buffers[0];
		vkBeginCommandBuffer(SingleTimeBuffer, &beginInfo);
	}

	void VulkanUtils::EndSingleTimeCommands(VkCommandBuffer& SingleTimeBuffer, QueueFamilies Family)
	{
		vkEndCommandBuffer(SingleTimeBuffer);

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &SingleTimeBuffer;
		
		SubmitQueue(submit, Family);

		vkQueueWaitIdle(Get()._Spec.Device->GetQueue(Family));

		std::vector<VkCommandBuffer> Buffers = { SingleTimeBuffer };
		if (Family == QueueFamilies::GRAPHICS)
			Get()._GraphicsCmdManager.FreeCommandBuffer(Buffers);
		if (Family == QueueFamilies::TRANSFER)
			Get()._TransferCmdManager.FreeCommandBuffer(Buffers);
		SingleTimeBuffer = VK_NULL_HANDLE;
	}

	VkResult VulkanUtils::SubmitQueue(const VkSubmitInfo& SubmitInfo, QueueFamilies Family, VkFence& Fence)
	{
		return vkQueueSubmit(Get()._Spec.Device->GetQueue(Family), 1, &SubmitInfo, Fence);
	}

	VkResult VulkanUtils::SubmitQueue(const VkSubmitInfo& SubmitInfo, QueueFamilies Family)
	{
		return vkQueueSubmit(Get()._Spec.Device->GetQueue(Family), 1, &SubmitInfo, VK_NULL_HANDLE);
	}

	void VulkanUtils::CreateAllocator(VkInstance Instance, VkPhysicalDevice PhysicalDevice, VkDevice Device)
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = PhysicalDevice;
		allocatorInfo.device = Device;
		allocatorInfo.instance = Instance;
		VULKAN_SUCCESS_ASSERT(vmaCreateAllocator(&allocatorInfo, &Get()._Allocator), "VMA Allocator failed to initialize!");
		VULKAN_PRINTLN("VMA Created!!");
	}

	void VulkanUtils::DestroyAllocator()
	{
		vmaDestroyAllocator(Get()._Allocator);
	}

	VmaAllocator VulkanUtils::GetAllocator()
	{
		return Get()._Allocator;
	}

	void VulkanUtils::_CreateCommandPools()
	{
		_GraphicsCmdManager.Init(_Spec.Device->GetHandle(), _Spec.Device->GetPhysicalDevice()->Info.QueueIndicies
			, QueueFamilies::GRAPHICS);
		_TransferCmdManager.Init(_Spec.Device->GetHandle(), _Spec.Device->GetPhysicalDevice()->Info.QueueIndicies
			, QueueFamilies::TRANSFER);
	}

	void VulkanUtils::_CreateDescPools()
	{
		std::vector<ShaderDescAttribs> Attribs = { {ShaderUniformTypes::UNIFORM, 1000} };
		_DescManager.Init(_Spec.Device->GetHandle(), Attribs, 500);
	}

	void VulkanUtils::CreateDescSets(std::vector<VkDescriptorSet>& Sets, uint32_t Count, std::vector<VkDescriptorSetLayout>& Layouts)
	{
		auto Manager = &Get()._DescManager;
		Manager->CreateDescSets(Sets, Count, Layouts);
	}
	
	void VulkanUtils::FreeDescSets(std::vector<VkDescriptorSet>& Sets)
	{
		Get()._DescManager.FreeDescSets(Sets);
	}
}
