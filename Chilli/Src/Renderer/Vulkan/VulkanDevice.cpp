#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "C:\VulkanSDK\1.3.275.0\Include\vulkan\vulkan.h"

#include "VulkanDevice.h"
#include "VulkanRenderer.h"

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

        int i = 0;
        for (auto& props : QueueProps)
        {
            if (props.queueFlags & VK_QUEUE_COMPUTE_BIT)
                info.QueueIndicies.Queues[int(QueueFamilies::COMPUTE)] = i;
            if (props.queueFlags & VK_QUEUE_TRANSFER_BIT)
                info.QueueIndicies.Queues[int(QueueFamilies::TRANSFER)] = i;
            if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                info.QueueIndicies.Queues[int(QueueFamilies::GRAPHICS)] = i;

            VkBool32 presentSupport;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, SurfaceKHR, &presentSupport);

            if (presentSupport)
                info.QueueIndicies.Queues[QueueFamilies::PRESENT] = i;
            i++;
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
        std::set<uint32_t> uniqueQueueFamilies = { indices.Queues[QueueFamilies::GRAPHICS].value(), indices.Queues[QueueFamilies::PRESENT].value(),
                                                  indices.Queues[QueueFamilies::COMPUTE].value(), indices.Queues[QueueFamilies::TRANSFER].value() };

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

        // ALl modern req features
        VkPhysicalDeviceVulkan13Features features13{};
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features13.dynamicRendering = VK_TRUE;
        features13.synchronization2 = VK_TRUE;
        features13.maintenance4 = VK_TRUE;
        features13.pipelineCreationCacheControl = VK_TRUE;
        features13.shaderDemoteToHelperInvocation = VK_TRUE;
        features13.shaderTerminateInvocation = VK_TRUE;
        features13.pNext = nullptr; // next in chain

        VkPhysicalDeviceVulkan12Features features12{};
        features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features12.samplerFilterMinmax = VK_TRUE;
        features12.bufferDeviceAddressCaptureReplay = VK_TRUE;
        features12.pNext = &features13; // chain 1.3 features after 1.2

        VkPhysicalDeviceFeatures features{};
        features.samplerAnisotropy = VK_TRUE;

        createInfo.pEnabledFeatures = &features;

        createInfo.enabledLayerCount = 0;
        createInfo.pNext = &features12;

        VULKAN_SUCCESS_ASSERT(vkCreateDevice(PDevice->PhysicalDevice, &createInfo, nullptr, &_Device), "Logical Device Failed to create!");

        vkGetDeviceQueue(_Device, indices.Queues[QueueFamilies::GRAPHICS].value(), 0, &_Queues[QueueFamilies::GRAPHICS]);
        vkGetDeviceQueue(_Device, indices.Queues[QueueFamilies::PRESENT].value(), 0, &_Queues[QueueFamilies::PRESENT]);

        if (indices.Queues[QueueFamilies::TRANSFER].has_value())
            vkGetDeviceQueue(_Device, indices.Queues[QueueFamilies::TRANSFER].value(), 0, &_Queues[QueueFamilies::TRANSFER]);
        if (indices.Queues[QueueFamilies::COMPUTE].has_value())
            vkGetDeviceQueue(_Device, indices.Queues[QueueFamilies::COMPUTE].value(), 0, &_Queues[QueueFamilies::COMPUTE]);
    }

    void VulkanDevice::Destroy()
    {
        vkDestroyDevice(_Device, nullptr);
    }
}