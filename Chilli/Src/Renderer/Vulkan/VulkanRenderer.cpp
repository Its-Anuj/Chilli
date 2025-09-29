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

        VULKAN_PRINTLN("Vulkan Initiated!!");
        _CreateResourceFactory();
    }

	void VulkanRenderer::ShutDown()
	{
        _ResourceFactory->Destroy();

        _Data.SwapChainKHR.Destroy(_Data.Device);
        _Data.Device.Destroy();
        vkDestroySurfaceKHR(_Data.Instance, _Data.SurfaceKHR, nullptr);
        if (_Spec.EnableValidationLayer)
            DestroyDebugUtilsMessengerEXT(_Data.Instance, _Data.DebugMessenger, nullptr);

        vkDestroyInstance(_Data.Instance, nullptr);
        VULKAN_PRINTLN("Vulkan Terminated!!");
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

        _ResourceFactory = std::make_shared<VulkanResourceFactory>(Spec);
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
