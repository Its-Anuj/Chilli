#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan\vulkan.h"

#include "vk_mem_alloc.h"
#include "VulkanDevice.h"

namespace Chilli
{
#pragma region PhsyiclalDevice
	void _FillVulkanPhysicalDevice(VulkanPhysicalDevice& Physicaldevice, VkSurfaceKHR SurfaceKHR)
	{
		auto device = Physicaldevice.PhysicalDevice;
		auto& info = Physicaldevice.Info;

		vkGetPhysicalDeviceFeatures(device, &info.features);
		vkGetPhysicalDeviceProperties(device, &info.properties);
		vkGetPhysicalDeviceMemoryProperties(device, &info.memoryProperties);

		info.features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		info.features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		info.features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

		VkPhysicalDeviceFeatures2 features2{};
		features2.pNext = &info.features11;
		info.features11.pNext = &info.features12;
		info.features12.pNext = &info.features13;
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

		vkGetPhysicalDeviceFeatures2(device, &features2);

		// Queue families
		uint32_t queufamiliescount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queufamiliescount, nullptr);

		std::vector<VkQueueFamilyProperties> QueueProps;
		QueueProps.resize(queufamiliescount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queufamiliescount, QueueProps.data());

		QueueFamilyIndicies indices{};

		auto has = [&](uint32_t i, VkQueueFlags f) {
			return (QueueProps[i].queueFlags & f) == f;
			};

		const uint32_t familyCount = static_cast<uint32_t>(QueueProps.size());

		for (uint32_t i = 0; i < familyCount; ++i)
		{
			const auto& props = QueueProps[i];

			// --- Graphics ---
			if (has(i, VK_QUEUE_GRAPHICS_BIT))
			{
				if (!indices.Queues[int(QueueFamilies::GRAPHICS)])
					indices.Queues[int(QueueFamilies::GRAPHICS)] = i;
			}

			// --- Compute (prefer no graphics) ---
			if (has(i, VK_QUEUE_COMPUTE_BIT))
			{
				auto& cur = indices.Queues[int(QueueFamilies::COMPUTE)];
				if (!cur ||
					!has(*cur, VK_QUEUE_COMPUTE_BIT) ||
					(has(*cur, VK_QUEUE_GRAPHICS_BIT) && !has(i, VK_QUEUE_GRAPHICS_BIT)))
				{
					cur = i;
				}
			}

			// --- Transfer (prefer no graphics & no compute) ---
			if (has(i, VK_QUEUE_TRANSFER_BIT))
			{
				auto& cur = indices.Queues[int(QueueFamilies::TRANSFER)];
				if (!cur ||
					!has(*cur, VK_QUEUE_TRANSFER_BIT) ||
					((has(*cur, VK_QUEUE_GRAPHICS_BIT) || has(*cur, VK_QUEUE_COMPUTE_BIT)) &&
						(!has(i, VK_QUEUE_GRAPHICS_BIT) && !has(i, VK_QUEUE_COMPUTE_BIT))))
				{
					cur = i;
				}
			}

			// --- Present ---
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, SurfaceKHR, &presentSupport);
			if (presentSupport)
			{
				if (!indices.Queues[int(QueueFamilies::PRESENT)])
					indices.Queues[int(QueueFamilies::PRESENT)] = i;
			}
		}

		auto& gfx = indices.Queues[int(QueueFamilies::GRAPHICS)];

		if (!indices.Queues[int(QueueFamilies::COMPUTE)])
			indices.Queues[int(QueueFamilies::COMPUTE)] = gfx;

		if (!indices.Queues[int(QueueFamilies::TRANSFER)])
			indices.Queues[int(QueueFamilies::TRANSFER)] = gfx;

		if (!indices.Queues[int(QueueFamilies::PRESENT)])
			indices.Queues[int(QueueFamilies::PRESENT)] = gfx;

		info.QueueIndicies = indices;

		for (uint32_t i = 0; i < QueueProps.size(); ++i) {
			auto f = QueueProps[i].queueFlags;
			std::cout << "Family " << i << ": "
				<< ((f & VK_QUEUE_GRAPHICS_BIT) ? "G " : "")
				<< ((f & VK_QUEUE_COMPUTE_BIT) ? "C " : "")
				<< ((f & VK_QUEUE_TRANSFER_BIT) ? "T " : "")
				<< std::endl;
		}
	}

	bool CheckVulkanPhysicalDeviceExtensions(VulkanPhysicalDevice& Physicaldevice, std::vector<const char*>& ReExts)
	{
		auto& deviceprops = Physicaldevice.Info;
		auto device = Physicaldevice.PhysicalDevice;

		uint32_t count = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

		std::vector<VkExtensionProperties> ExtProps;
		ExtProps.resize(count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, ExtProps.data());

		for (auto name : ReExts)
		{
			bool found = false;
			for (auto& props : ExtProps)
			{
				if (strcmp(props.extensionName, name))
					found = true;
			}

			if (!found)
			{
				deviceprops.SupportsExtensions = false;
				return false;
			}
		}
		return true;
	}
#pragma endregion

	void VulkanDevice::Init(VulkanPhysicalDevice* PDevice, std::vector<const char*>& ReqExts)
	{
		_UsedPhysicalDevice = PDevice;
		auto indices = PDevice->Info.QueueIndicies;

		float queuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.Queues[int(QueueFamilies::GRAPHICS)].value(), indices.Queues[int(QueueFamilies::PRESENT)].value(),
												  indices.Queues[int(QueueFamilies::COMPUTE)].value(), indices.Queues[int(QueueFamilies::TRANSFER)].value() };

		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.enabledExtensionCount = ReqExts.size();
		createInfo.ppEnabledExtensionNames = ReqExts.data();

		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = queueCreateInfos.size();

		VkPhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures{};
		shaderObjectFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
		shaderObjectFeatures.shaderObject = VK_TRUE; // **ENABLE THE FEATURE**

		VkPhysicalDeviceColorWriteEnableFeaturesEXT ColorWriteExt{};
		ColorWriteExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT;
		ColorWriteExt.colorWriteEnable = true;

		VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dyn3{};
		dyn3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
		dyn3.extendedDynamicState3ColorBlendEnable = VK_TRUE;
		dyn3.extendedDynamicState3PolygonMode = VK_TRUE;

		VkPhysicalDeviceExtendedDynamicState2FeaturesEXT dynState2{};
		dynState2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
		dynState2.extendedDynamicState2 = true;

		VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynState{};
		dynState.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

		// Always set features as last
		shaderObjectFeatures.pNext = nullptr; // Temporarily set pNext to nullptr
		ColorWriteExt.pNext = &shaderObjectFeatures;
		dyn3.pNext = &ColorWriteExt;
		dynState2.pNext = &dyn3;
		dynState.pNext = &dynState2;

		// ALl modern req features
		VkPhysicalDeviceVulkan13Features features13{};
		features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		features13.dynamicRendering = VK_TRUE;
		features13.synchronization2 = VK_TRUE;
		features13.maintenance4 = VK_TRUE;
		features13.pipelineCreationCacheControl = VK_TRUE;
		features13.shaderDemoteToHelperInvocation = VK_TRUE;
		features13.shaderTerminateInvocation = VK_TRUE;
		features13.pNext = &dynState; // next in chain

		VkPhysicalDeviceVulkan12Features features12{};
		features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		features12.samplerFilterMinmax = VK_TRUE;
		features12.bufferDeviceAddressCaptureReplay = VK_TRUE;
		features12.descriptorBindingPartiallyBound = VK_TRUE;
		features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
		features12.runtimeDescriptorArray = VK_TRUE;
		features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		features12.descriptorIndexing = VK_TRUE;
		features12.pNext = &features13; // chain 1.3 features after 1.
		features12.descriptorBindingUniformBufferUpdateAfterBind = PDevice->Info.features12.descriptorBindingUniformBufferUpdateAfterBind ? VK_TRUE : VK_FALSE;
		features12.descriptorBindingSampledImageUpdateAfterBind = PDevice->Info.features12.descriptorBindingSampledImageUpdateAfterBind ? VK_TRUE : VK_FALSE;

		VkPhysicalDeviceFeatures EnabledFeatures{};
		EnabledFeatures.fillModeNonSolid = VK_TRUE;
		EnabledFeatures.samplerAnisotropy = VK_TRUE;

		VkPhysicalDeviceFeatures2 features2{};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.features.samplerAnisotropy = VK_TRUE;
		features2.features = EnabledFeatures;
		features2.pNext = &features12;

		createInfo.enabledLayerCount = 0;
		createInfo.pNext = &features2;

		if (vkCreateDevice(PDevice->PhysicalDevice, &createInfo, nullptr, &_Device) != VK_SUCCESS)
			assert(false && "Logical Device Failed to create!");

		vkGetDeviceQueue(_Device, indices.Queues[int(QueueFamilies::GRAPHICS)].value(), 0, &_Queues[int(QueueFamilies::GRAPHICS)]);
		vkGetDeviceQueue(_Device, indices.Queues[int(QueueFamilies::PRESENT)].value(), 0, &_Queues[int(QueueFamilies::PRESENT)]);

		if (indices.Queues[int(QueueFamilies::TRANSFER)].has_value())
			vkGetDeviceQueue(_Device, indices.Queues[int(QueueFamilies::TRANSFER)].value(), 0, &_Queues[int(QueueFamilies::TRANSFER)]);
		if (indices.Queues[int(QueueFamilies::COMPUTE)].has_value())
			vkGetDeviceQueue(_Device, indices.Queues[int(QueueFamilies::COMPUTE)].value(), 0, &_Queues[int(QueueFamilies::COMPUTE)]);
	}

	void VulkanDevice::Destroy()
	{
		vkDestroyDevice(_Device, nullptr);
	}
}