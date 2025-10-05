#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "C:\VulkanSDK\1.3.275.0\Include\vulkan\vulkan.h"

#include "vk_mem_alloc.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"
#include "VulkanRenderer.h"
#include "VulkanResourceFactory.h"

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

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
		_Data.FrameInFlightCount = _Spec.InFrameFlightCount;
		_Data.VSync = _Spec.VSync;

		// Core related
		_CreateInstance();
		_CreateDebugMessenger();
		_CreateSurfaceKHR();

		// Devices
		_CreatePhysicalDevice();
		_CreateLogicalDevice();

		// SwapChain
		_CreateSwapChainKHR();

		// Utils
		VulkanUtilsSpec UtilsSpec{};
		UtilsSpec.Device = &_Data.Device;
		UtilsSpec.Instance = _Data.Instance;
		VulkanUtils::Init(UtilsSpec);

		//  Commands
		_CreateCommandBuffers();
		_CreateSyncObjects();
		VULKAN_PRINTLN("Vulkan Initiated!!");
		_CreateResourceFactory();
	}

	void VulkanRenderer::ShutDown()
	{
		vkDeviceWaitIdle(_Data.Device.GetHandle());
		_ResourceFactory->Destroy();
		VulkanUtils::ShutDown();

		auto device = _Data.Device.GetHandle();
		for (int i = 0; i < _Data.FrameInFlightCount; i++)
		{
			vkDestroySemaphore(device, _Data.ImageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, _Data.RenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, _Data.InFlightFences[i], nullptr);
		}
		_Data.SwapChainKHR.Destroy(_Data.Device);

		_Data.Device.Destroy();

		// Core
		vkDestroySurfaceKHR(_Data.Instance, _Data.SurfaceKHR, nullptr);
		if (_Spec.EnableValidationLayer)
			DestroyDebugUtilsMessengerEXT(_Data.Instance, _Data.DebugMessenger, nullptr);

		vkDestroyInstance(_Data.Instance, nullptr);
		VULKAN_PRINTLN("Vulkan Terminated!!");
	}

	bool VulkanRenderer::BeginFrame()
	{
		vkWaitForFences(_Data.Device.GetHandle(), 1, &_Data.InFlightFences[_Data.CurrentFrameIndex], VK_TRUE, UINT64_MAX);

		VkResult result = vkAcquireNextImageKHR(_Data.Device.GetHandle(), _Data.SwapChainKHR.GetHandle(), UINT64_MAX,
			_Data.ImageAvailableSemaphores[_Data.CurrentFrameIndex], VK_NULL_HANDLE,
			&_Data.CurrentImageIndex);

		if (result != VK_SUCCESS)
		{
			std::cout << "ERROR: AcquireNextImage failed: " << result << "\n";
		}

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			_ReCreateSwapChainKHR();
			return false;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		vkResetFences(_Data.Device.GetHandle(), 1, &_Data.InFlightFences[_Data.CurrentFrameIndex]);
		vkResetCommandBuffer(_Data.GraphicsCommandBuffers[_Data.CurrentFrameIndex], 0);

		auto commandBuffer = _Data.GraphicsCommandBuffers[_Data.CurrentFrameIndex];

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return true;
	}

	bool VulkanRenderer::BeginRenderPass(const BeginRenderPassInfo& Info)
	{
		auto commandBuffer = _Data.GraphicsCommandBuffers[_Data.CurrentFrameIndex];

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

		std::vector< VkRenderingAttachmentInfo> ColorAttachments;
		ColorAttachments.reserve(Info.ColorAttachmentCount);

		for (int i = 0; i < Info.ColorAttachmentCount; i++)
		{
			auto& ActiveColorAttachment = Info.ColorAttachments[i];
			VkRenderingAttachmentInfo colorAttachment{};
			colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // ✅ Now correct
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			if(ActiveColorAttachment.UseSwapChainTexture)
				colorAttachment.imageView = _Data.SwapChainKHR.GetImageViews()[_Data.CurrentImageIndex];
			else
			{
				auto VulkanTex = std::static_pointer_cast<VulkanTexture>(ActiveColorAttachment.ColorTexture);
				colorAttachment.imageView = VulkanTex->GetHandle();
			}
			
			colorAttachment.clearValue = { {ActiveColorAttachment.ClearColor.r,ActiveColorAttachment.ClearColor.g,
				ActiveColorAttachment.ClearColor.b, ActiveColorAttachment.ClearColor.w} };
			ColorAttachments.push_back(colorAttachment);
		}

		VkRenderingAttachmentInfo depthAttachment{};
		if (Info.DepthAttachment.DepthTexture != VK_NULL_HANDLE)
		{
			auto VulkanTex = std::static_pointer_cast<VulkanTexture>(Info.DepthAttachment.DepthTexture);
			depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachment.imageView = VulkanTex->GetHandle();
			depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.clearValue.depthStencil = { Info.DepthAttachment.Planes.Far,  Info.DepthAttachment.Planes.Near }; // Clear to far plane
			renderingInfo.pDepthAttachment = &depthAttachment; 
		}

		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(ColorAttachments.size());
		renderingInfo.pColorAttachments = ColorAttachments.data();
		
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		return true;
	}

	void VulkanRenderer::Submit(const std::shared_ptr<GraphicsPipeline>& Pipeline
		, const std::shared_ptr<VertexBuffer>& VB, const std::shared_ptr<IndexBuffer>& IB)
	{
		auto VulkanVB = std::static_pointer_cast<VulkanVertexBuffer>(VB);
		auto VulkanIB = std::static_pointer_cast<VulkanIndexBuffer>(IB);
		auto VulkanPipeline = std::static_pointer_cast<VulkanGraphicsPipeline>(Pipeline);
		auto commandBuffer = _Data.GraphicsCommandBuffers[_Data.CurrentFrameIndex];
		// Bind pipeline (created without render pass)
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VulkanPipeline->GetHandle());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VulkanPipeline->GetLayout(),
			0, 1, &VulkanPipeline->GetSets()[0], 0, nullptr);

		// Set dynamic viewport (matches pipeline dynamic state)
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(_Data.FrameBufferSize.x);
		viewport.height = static_cast<float>(_Data.FrameBufferSize.y);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		// Set dynamic scissor
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent.width = _Data.FrameBufferSize.x;
		scissor.extent.height = _Data.FrameBufferSize.y;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		// 🆕 Bind vertex buffer
		VkBuffer vertexBuffers[] = { VulkanVB->GetHandle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, VulkanIB->GetHandle(), 0, VK_INDEX_TYPE_UINT16);
		// 🎨 Draw your geometry

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(VulkanIB->GetCount()), 1, 0, 0, 0);
	}

	void VulkanRenderer::EndRenderPass()
	{
		auto commandBuffer = _Data.GraphicsCommandBuffers[_Data.CurrentFrameIndex];

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

		VkSemaphore waitSemaphores[] = { _Data.ImageAvailableSemaphores[_Data.CurrentFrameIndex] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_Data.GraphicsCommandBuffers[_Data.CurrentFrameIndex];

		VkSemaphore signalSemaphores[] = { _Data.RenderFinishedSemaphores[_Data.CurrentFrameIndex] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(_Data.Device.GetQueue(QueueFamilies::GRAPHICS), 1, &submitInfo, _Data.InFlightFences[_Data.CurrentFrameIndex]) != VK_SUCCESS)
			throw std::runtime_error("failed to submit draw command buffer!");
	}

	void VulkanRenderer::Present()
	{
		VkSemaphore signalSemaphores[] = { _Data.RenderFinishedSemaphores[_Data.CurrentFrameIndex] };

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { _Data.SwapChainKHR.GetHandle() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &_Data.CurrentImageIndex;
		presentInfo.pResults = nullptr; // Optional

		auto result = vkQueuePresentKHR(_Data.Device.GetQueue(QueueFamilies::PRESENT), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _Data.FrameBufferReSized) {
			_Data.FrameBufferReSized = false;
			_ReCreateSwapChainKHR();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		_Data.CurrentFrameIndex = (_Data.CurrentFrameIndex + 1) % _Data.FrameInFlightCount;
	}

	void VulkanRenderer::EndFrame()
	{
	}

	void VulkanRenderer::FinishRendering()
	{
		vkDeviceWaitIdle(_Data.Device.GetHandle());
	}

	void VulkanRenderer::FrameBufferReSized(int Width, int Height)
	{
		_Data.FrameBufferSize = { Width, Height };
		_Data.FrameBufferReSized = true;
	}

	std::shared_ptr<ResourceFactory> VulkanRenderer::GetResourceFactory()
	{
		return _ResourceFactory;
	}

	void VulkanRenderer::_CreateInstance()
	{
		std::vector<const char*> RequiredExts, RequiredLayers;

		uint32_t glfwExtCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);

		RequiredExts.push_back(glfwExtensions[0]);
		RequiredExts.push_back(glfwExtensions[1]);

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
		glfwCreateWindowSurface(_Data.Instance, (GLFWwindow*)_Spec.GLFWWINDOW, nullptr, &_Data.SurfaceKHR);
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
		_Data.SwapChainKHR.Init(_Data.Device, _Data.SurfaceKHR, _Data.FrameBufferSize.x, _Data.FrameBufferSize.y, _Data.VSync);
	}

	void VulkanRenderer::_ReCreateSwapChainKHR()
	{
		_Data.SwapChainKHR.Recreate(_Data.Device, _Data.SurfaceKHR, _Data.FrameBufferSize.x, _Data.FrameBufferSize.y, _Data.VSync);
	}

	void VulkanRenderer::_CreateResourceFactory()
	{
		VulkanResourceFactorySpec Spec{};
		Spec.Device = &_Data.Device;
		Spec.Instance = _Data.Instance;
		Spec.SwapChain = &_Data.SwapChainKHR;

		_ResourceFactory = std::make_shared<VulkanResourceFactory>(Spec);
	}

	void VulkanRenderer::_CreateCommandBuffers()
	{
		VulkanUtils::CreateCommandBuffers(QueueFamilies::GRAPHICS, _Data.GraphicsCommandBuffers, _Data.FrameInFlightCount);
		VULKAN_PRINTLN("[VULKAN]: Graphics Command Buffers Created !!");
	}

	void VulkanRenderer::_CreateSyncObjects()
	{
		_Data.InFlightFences.resize(_Data.FrameInFlightCount);
		_Data.RenderFinishedSemaphores.resize(_Data.FrameInFlightCount);
		_Data.ImageAvailableSemaphores.resize(_Data.FrameInFlightCount);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first frame doesn't wait forever
		auto device = _Data.Device.GetHandle();

		for (int i = 0; i < _Data.FrameInFlightCount; i++)
		{
			VULKAN_SUCCESS_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_Data.ImageAvailableSemaphores[i]), "Image Available Sempahore Failed!");
			VULKAN_SUCCESS_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_Data.RenderFinishedSemaphores[i]), "Render Finished Semaphore Failed!");
			VULKAN_SUCCESS_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &_Data.InFlightFences[i]), "Fences Failed To create!");
		}
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
