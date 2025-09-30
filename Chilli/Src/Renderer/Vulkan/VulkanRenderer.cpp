#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "C:\VulkanSDK\1.3.275.0\Include\vulkan\vulkan.h"

#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "vk_mem_alloc.h"
#include "VulkanResourceFactory.h"

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

namespace Chilli
{
	VulkanVersion GetInstanceSupportedVersion()
	{
		VulkanVersion Version{};
		uint32_t ApiVersion = 0;
		vkEnumerateInstanceVersion(&ApiVersion);

		Version = { VK_API_VERSION_MAJOR(ApiVersion),VK_API_VERSION_MINOR(ApiVersion),VK_API_VERSION_PATCH(ApiVersion) };
		return Version;
	}

	void VulkanRenderer::Init(void* Spec)
	{	
		auto InstanceVersion = GetInstanceSupportedVersion();
		std::cout << InstanceVersion.Major << "." << InstanceVersion.Minor << "." << InstanceVersion.Patch << "\n";

		_Spec = *(VulkanRendererSpec*)Spec;
		VULKAN_PRINTLN("Supports: " << InstanceVersion.Major << "." << InstanceVersion.Minor << "." << InstanceVersion.Patch)

		if (InstanceVersion.Minor != 3)
			VULKAN_ERROR("Vulkan verson is not supported!, You support: " << InstanceVersion.Major << "." << InstanceVersion.Minor << "." << InstanceVersion.Patch);
		
		_Data.FrameBufferSize.x = _Spec.FrameBufferSize.x;
		_Data.FrameBufferSize.y = _Spec.FrameBufferSize.y;
	
		// Core related
        _CreateInstance();
        _CreateDebugMessenger();
        _CreateSurfaceKHR();

        // Devices
        _CreatePhysicalDevice();
        _CreateLogicalDevice();

        // SwapChain
        _CreateSwapChainKHR();

        //  Commands
        _CreateCommandPools();
        _CreateCommandBuffers();
        _CreateSyncObjects();

        VULKAN_PRINTLN("Vulkan Initiated!!");
        _CreateResourceFactory();
    }

	void VulkanRenderer::ShutDown()
	{
        vkDeviceWaitIdle(_Data.Device.GetHandle());
        _ResourceFactory->Destroy();

        auto device = _Data.Device.GetHandle();
        vkDestroySemaphore(device, _Data.ImageAvailableSemaphores, nullptr);
        vkDestroySemaphore(device, _Data.RenderFinishedSemaphores, nullptr);
        vkDestroyFence(device, _Data.InFlightFences, nullptr);
        
        vkDestroyCommandPool(_Data.Device.GetHandle(), _Data.GraphicsCommandPool, nullptr);
        vkDestroyCommandPool(_Data.Device.GetHandle(), _Data.TransferCommandPool, nullptr);

        _Data.SwapChainKHR.Destroy(_Data.Device);
        
        _Data.Device.Destroy();
        
        // Core
        vkDestroySurfaceKHR(_Data.Instance, _Data.SurfaceKHR, nullptr);
        if (_Spec.EnableValidationLayer)
            DestroyDebugUtilsMessengerEXT(_Data.Instance, _Data.DebugMessenger, nullptr);

        vkDestroyInstance(_Data.Instance, nullptr);
        VULKAN_PRINTLN("Vulkan Terminated!!");
	}

    void VulkanRenderer::BeginFrame()
    {
    }

    void VulkanRenderer::BeginRenderPass()
    {
        vkWaitForFences(_Data.Device.GetHandle(), 1, &_Data.InFlightFences, VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(_Data.Device.GetHandle(), _Data.SwapChainKHR.GetHandle(), UINT64_MAX,
            _Data.ImageAvailableSemaphores, VK_NULL_HANDLE,
            &_Data.CurrentImageIndex);

        if (result != VK_SUCCESS)
        {
            std::cout << "ERROR: AcquireNextImage failed: " << result << "\n";
        }
        vkResetFences(_Data.Device.GetHandle(), 1, &_Data.InFlightFences);
        vkResetCommandBuffer(_Data.GraphicsCommandBuffer, 0);

        auto commandBuffer = _Data.GraphicsCommandBuffer;

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        // 🆕 TRANSITION: undefined → color attachment optimal (for rendering)
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _Data.SwapChainKHR.GetImages()[_Data.CurrentImageIndex]; // 🆕 Use the actual image
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,             // Before any operations
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Before color attachment output
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        // Continue with dynamic rendering...
        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = { {0, 0}, _Data.SwapChainKHR.GetExtent() };
        renderingInfo.layerCount = 1;

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = _Data.SwapChainKHR.GetImageViews()[_Data.CurrentImageIndex];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // ✅ Now correct
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = { {0.5f, 0.5f, 0.0f, 1.0f} };
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
    }

    void VulkanRenderer::Submit(const std::shared_ptr<GraphicsPipeline>& Pipeline)
    {
        auto VulkanPipeline = std::static_pointer_cast<VulkanGraphicsPipeline>(Pipeline);
        auto commandBuffer = _Data.GraphicsCommandBuffer;
        // Bind pipeline (created without render pass)
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VulkanPipeline->GetHandle());

        // Set dynamic viewport (matches pipeline dynamic state)
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(_Data.SwapChainKHR.GetExtent().width);
        viewport.height = static_cast<float>(_Data.SwapChainKHR.GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        // Set dynamic scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = _Data.SwapChainKHR.GetExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // 🎨 Draw your geometry
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }

    void VulkanRenderer::EndRenderPass()
    {
        auto commandBuffer = _Data.GraphicsCommandBuffer;

        vkCmdEndRendering(commandBuffer);

        // 🆕 TRANSITION: color attachment optimal → present source (for presentation)
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // ✅ Required for presentation
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _Data.SwapChainKHR.GetImages()[_Data.CurrentImageIndex]; // 🆕 Use the actual image
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // After color attachment writing
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // Before presentation
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        vkEndCommandBuffer(commandBuffer);
    }

    void VulkanRenderer::RenderFrame()
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { _Data.ImageAvailableSemaphores };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_Data.GraphicsCommandBuffer;

        VkSemaphore signalSemaphores[] = { _Data.RenderFinishedSemaphores};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(_Data.Device.GetQueue(QueueFamilies::GRAPHICS), 1, &submitInfo, _Data.InFlightFences) != VK_SUCCESS)
            throw std::runtime_error("failed to submit draw command buffer!");
    }

    void VulkanRenderer::Present()
    {
        VkSemaphore signalSemaphores[] = { _Data.RenderFinishedSemaphores};

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { _Data.SwapChainKHR.GetHandle()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &_Data.CurrentImageIndex;
        presentInfo.pResults = nullptr; // Optional

        auto result = vkQueuePresentKHR(_Data.Device.GetQueue(QueueFamilies::PRESENT), &presentInfo);
    }

    void VulkanRenderer::EndFrame()
    {
    }

    void VulkanRenderer::FinishRendering()
    {
        vkDeviceWaitIdle(_Data.Device.GetHandle());
    }

    std::shared_ptr<ResourceFactory> VulkanRenderer::GetResourceFactory()
    {
        return _ResourceFactory;
    }

    void VulkanRenderer::_CreateInstance()
    {
        std::vector<const char*> RequiredExts, RequiredLayers;
        RequiredExts.push_back("VK_KHR_win32_surface");
        RequiredExts.push_back("VK_KHR_surface");

        if (_Spec.EnableValidationLayer)
        {
            // Check if validation layer exists
            RequiredLayers.push_back("VK_LAYER_KHRONOS_validation");
            RequiredExts.push_back("VK_EXT_debug_utils");
        }

        _Spec.DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                  "VK_KHR_dynamic_rendering",
                                  "VK_KHR_synchronization2",
                                  "VK_KHR_maintenance4",
                                  "VK_EXT_pipeline_creation_cache_control",
                                  "VK_EXT_shader_demote_to_helper_invocation",
                                  "VK_KHR_shader_terminate_invocation",
                                  "VK_EXT_sampler_filter_minmax",
                                  "VK_KHR_buffer_device_address" };
        VkApplicationInfo appinfo{};
        appinfo.apiVersion = VK_API_VERSION_1_3;
        appinfo.pEngineName = _Spec.Name;
        appinfo.pApplicationName = _Spec.Name;
        appinfo.applicationVersion = VK_API_VERSION_1_3;
        appinfo.engineVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appinfo;

        createInfo.enabledLayerCount = 0;
        createInfo.enabledExtensionCount = RequiredExts.size();
        createInfo.ppEnabledExtensionNames = RequiredExts.data();
        createInfo.enabledLayerCount = RequiredLayers.size();
        createInfo.pNext = nullptr;

        if (RequiredLayers.size() == 0)
            createInfo.ppEnabledLayerNames = nullptr;
        else
            createInfo.ppEnabledLayerNames = RequiredLayers.data();

        if (_Spec.EnableValidationLayer)
        {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

            _PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }

        if (vkCreateInstance(&createInfo, nullptr, &_Data.Instance) != VK_SUCCESS)
            throw std::runtime_error("Adsa");
    }

    void VulkanRenderer::_CreateDebugMessenger()
    {
        if (!_Spec.EnableValidationLayer)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        _PopulateDebugMessengerCreateInfo(createInfo);

        VULKAN_SUCCESS_ASSERT(CreateDebugUtilsMessengerEXT(_Data.Instance, &createInfo, nullptr, &_Data.DebugMessenger), "Debug Messenger not loaded!");
        VULKAN_PRINTLN("Debug Messenger Loaded!");
    }

    void VulkanRenderer::_PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional
        createInfo.pNext = nullptr;     // Optional
        createInfo.flags = 0;           // Must be 0
    }

    void VulkanRenderer::_CreateSurfaceKHR()
    {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = (HWND)_Spec.Win32Surface;
        createInfo.hinstance = GetModuleHandle(nullptr);

        if (vkCreateWin32SurfaceKHR(_Data.Instance, &createInfo, nullptr, &_Data.SurfaceKHR) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface!");
        VULKAN_PRINTLN("[VULKAN]: Window Surface KHR Created!");
    }

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    void VulkanRenderer::_CreatePhysicalDevice()
    {
        uint32_t PDeviceCount = 0;
        vkEnumeratePhysicalDevices(_Data.Instance, &PDeviceCount, nullptr);

        std::vector<VkPhysicalDevice> PDevices;
        PDevices.resize(PDeviceCount);
        vkEnumeratePhysicalDevices(_Data.Instance, &PDeviceCount, PDevices.data());

        if (PDevices.size() == 0)
            throw std::runtime_error("No Vulkan Supporting device found!");

        VULKAN_PRINTLN("Vulkan Supported devices: " << PDevices.size())
            _Data.PhysicalDevices.resize(PDevices.size());

        for (int i = 0; i < PDevices.size(); i++)
            _Data.PhysicalDevices[i].PhysicalDevice = PDevices[i];

        // Check how good each phsyical device is in features
        for (auto& device : _Data.PhysicalDevices)
        {
            _FillVulkanPhysicalDevice(device, _Data.SurfaceKHR);

            device.Info.SwapChainDetails = QuerySwapChainSupport(device.PhysicalDevice, _Data.SurfaceKHR);

            VULKAN_PRINTLN("Device Found: " << device.Info.properties.deviceName << "\n");
        }

        for (auto& device : _Data.PhysicalDevices)
            CheckVulkanPhysicalDeviceExtensions(device, _Spec.DeviceExtensions);

        _FindSuitablePhysicalDevice();
    }

    void VulkanRenderer::_FindSuitablePhysicalDevice()
    {
        std::vector<int> Scores(_Data.PhysicalDevices.size());

        int i = 0;
        for (auto& device : _Data.PhysicalDevices)
        {
            auto& deviceinfo = device.Info;
            int CurrentScore = 0;

            if (deviceinfo.features13.dynamicRendering == true)
                CurrentScore += 1000;
            if (deviceinfo.features13.synchronization2 == true)
                CurrentScore += 1000;
            if (deviceinfo.features13.maintenance4 == true)
                CurrentScore += 1000;
            if (deviceinfo.features13.pipelineCreationCacheControl)
                CurrentScore += 1000;
            if (deviceinfo.features13.shaderDemoteToHelperInvocation)
                CurrentScore += 1000;
            if (deviceinfo.features13.shaderTerminateInvocation)
                CurrentScore += 1000;
            if (deviceinfo.features12.samplerFilterMinmax)
                CurrentScore += 1000;
            if (deviceinfo.features12.bufferDeviceAddressCaptureReplay)
                CurrentScore += 1000;

            if (deviceinfo.features.geometryShader)
                CurrentScore += 1000;
            if (deviceinfo.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                CurrentScore += 10000;
            if (deviceinfo.QueueIndicies.Complete())
                CurrentScore += 10000;

            if ((!deviceinfo.SwapChainDetails.formats.empty() && !deviceinfo.SwapChainDetails.presentModes.empty()) == true)
                CurrentScore += 10000;
            VULKAN_PRINTLN(deviceinfo.properties.deviceName << " scored: " << CurrentScore);
            Scores[i++] = CurrentScore;
        }

        int Bestid = 0;
        for (int i = 0; i < _Data.PhysicalDevices.size(); i++)
        {
            if (Scores[Bestid] < Scores[i])
                Bestid = i;
        }
        VULKAN_PRINTLN("Chosen Device: " << _Data.PhysicalDevices[Bestid].Info.properties.deviceName << "\n")
            _Data.ActivePhysicalDeviceIndex = Bestid;
    }

    void VulkanRenderer::_CreateLogicalDevice()
    {
        _Data.Device.Init(&_Data.PhysicalDevices[_Data.ActivePhysicalDeviceIndex], _Spec.DeviceExtensions);
    }

    void VulkanRenderer::_CreateSwapChainKHR()
    {
        _Data.SwapChainKHR.Init(_Data.Device, _Data.SurfaceKHR, _Data.FrameBufferSize.x, _Data.FrameBufferSize.y);
    }

    void VulkanRenderer::_CreateResourceFactory()
    {
        VulkanResourceFactorySpec Spec{};
        Spec.Device = &_Data.Device;
        Spec.Instance = _Data.Instance;
        Spec.SwapChain = &_Data.SwapChainKHR;

        _ResourceFactory = std::make_shared<VulkanResourceFactory>(Spec);
    }
    
    VkCommandPool __CreateCommandPool(VkDevice device, uint32_t QueueIndex, VkCommandPoolCreateFlags Flags)
    {
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = QueueIndex;
        info.flags = Flags;

        VkCommandPool CommandPool;
        VULKAN_SUCCESS_ASSERT(vkCreateCommandPool(device, &info, nullptr, &CommandPool), "Command Pool Createion Failed");
        return CommandPool;
    }

    void VulkanRenderer::_CreateCommandPools()
    {
        _Data.GraphicsCommandPool = __CreateCommandPool(_Data.Device.GetHandle()
            , _Data.Device.GetPhysicalDevice()->Info.QueueIndicies.Queues[QueueFamilies::GRAPHICS].value(),
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        _Data.TransferCommandPool = __CreateCommandPool(_Data.Device.GetHandle()
            , _Data.Device.GetPhysicalDevice()->Info.QueueIndicies.Queues[QueueFamilies::TRANSFER].value(),
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    }

    void __CreateCommandBuffer(VkDevice Device, VkCommandPool Pool, VkCommandBuffer* Buffers, uint32_t Count
        , VkCommandBufferLevel Level)
    {
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = Pool;
        info.commandBufferCount = Count;
        info.level = Level;

        VULKAN_SUCCESS_ASSERT(vkAllocateCommandBuffers(Device, &info, Buffers), "Command Buffer Failed!");
    }

    void VulkanRenderer::_CreateCommandBuffers()
    {
        __CreateCommandBuffer(_Data.Device.GetHandle(), _Data.GraphicsCommandPool, &_Data.GraphicsCommandBuffer, 1,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        VULKAN_PRINTLN("[VULKAN]: Graphics Command Buffers Created !!");
    }

    void VulkanRenderer::_CreateSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first frame doesn't wait forever
        auto device = _Data.Device.GetHandle();

        VULKAN_SUCCESS_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_Data.ImageAvailableSemaphores), "Image Available Sempahore Failed!");
        VULKAN_SUCCESS_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_Data.RenderFinishedSemaphores), "Render Finished Semaphore Failed!");
        VULKAN_SUCCESS_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &_Data.InFlightFences), "Fences Failed To create!");
    }
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}
