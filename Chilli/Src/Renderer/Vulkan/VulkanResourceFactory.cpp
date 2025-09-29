#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "VulkanRenderer.h"
#include "VulkanResourceFactory.h"

namespace Chilli
{
    VulkanResourceFactory::VulkanResourceFactory(const VulkanResourceFactorySpec& Spec)
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = Spec.Device->GetPhysicalDevice()->PhysicalDevice;
        allocatorInfo.device = Spec.Device->GetHandle();
        allocatorInfo.instance = Spec.Instance;

        VULKAN_SUCCESS_ASSERT(vmaCreateAllocator(&allocatorInfo, &_Allocator), "VMA Allocator failed to initialize!");
        VULKAN_PRINTLN("VMA Created!!");
        VULKAN_PRINTLN("Resource Factory Created!")
    }

    void VulkanResourceFactory::Destroy()
    {
        vmaDestroyAllocator(_Allocator);
        VULKAN_PRINT("Resource Factory Desytoyed!")
    }

    Ref<GraphicsPipeline> VulkanResourceFactory::CreateGraphicsPipeline()
    {
        VulkanGraphicsPipelineSpec Spec{}; 
        return std::make_shared<VulkanGraphicsPipeline>(Spec);
    }

    void VulkanResourceFactory::DestroyGraphicsPipeline(Ref<GraphicsPipeline>& Pipeline)
    {
    }

    VulkanGraphicsPipeline::VulkanGraphicsPipeline(const VulkanGraphicsPipelineSpec& Spec)
    {
    }

    void VulkanGraphicsPipeline::Destroy()
    {
    }

    void VulkanGraphicsPipeline::Init(const VulkanGraphicsPipelineSpec& Spec)
    {
    }
}
