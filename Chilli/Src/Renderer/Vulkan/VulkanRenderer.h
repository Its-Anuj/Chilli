#pragma once

#include "RenderAPI.h"
#include "VulkanDevice.h"
#include "VulkanSwapChainKHR.h"

#define VULKAN_SUCCESS_ASSERT(x, err) \
    {                                 \
        if (x != VK_SUCCESS)          \
        {                             \
            VULKAN_ERROR(err);        \
        }                             \
    }
namespace Chilli
{
	struct VulkanRendererSpec
	{
		const char* Name;
		bool EnableValidationLayer = false;
		void* Win32Surface;

		struct
		{
			float x, y;
		} FrameBufferSize;
		std::vector<const char*> DeviceExtensions;
		std::vector<const char*> InstanceExtensions;
		std::vector<const char*> InstanceLayers;
	};

	struct VulkanVersion
	{
		int Major, Minor, Patch;
	};

	struct VulkanRendererData
	{
		VkInstance Instance;
		VkDebugUtilsMessengerEXT DebugMessenger;
		VkSurfaceKHR SurfaceKHR;

		struct {
			int x, y;
		} FrameBufferSize;

		VulkanSwapChainKHR SwapChainKHR;

		// Devices
		std::vector<VulkanPhysicalDevice> PhysicalDevices;
		int ActivePhysicalDeviceIndex = 0;

		VulkanDevice Device;

		// Commands
		VkCommandPool GraphicsCommandPool, TransferCommandPool;
		VkCommandBuffer GraphicsCommandBuffer;

		VkSemaphore ImageAvailableSemaphores;
		VkSemaphore RenderFinishedSemaphores;
		VkFence InFlightFences;

		uint32_t CurrentImageIndex;
	};

	struct VulkanResourceFactory;

	class VulkanRenderer : public RenderAPI
	{
	public:
		VulkanRenderer() {}
		~VulkanRenderer() {}

		inline RenderAPITypes GetType() override {
			return  RenderAPITypes::VULKAN1_3;
		}

		inline const char* GetName() override {
			return "VULKAN_1_3";
		}

		virtual void Init(void* Spec) override; 
		virtual void ShutDown() override;


		// Rendering related
		virtual void BeginFrame() override;
		virtual void BeginRenderPass() override; 
		virtual void Submit(const std::shared_ptr<GraphicsPipeline>& Pipeline) override;
		virtual void EndRenderPass() override;
		virtual void RenderFrame() override;
		virtual void Present() override;
		virtual void EndFrame() override;	
		virtual void FinishRendering() override;

		virtual std::shared_ptr<ResourceFactory> GetResourceFactory() override;
	private:
		void _CreateInstance();
		void _CreateDebugMessenger();
		void _PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void _CreateSurfaceKHR();

		// Devices
		void _CreatePhysicalDevice();
		void _FindSuitablePhysicalDevice();
		void _CreateLogicalDevice();

		// SwapChain
		void _CreateSwapChainKHR();

		void _CreateResourceFactory();

		// Commands
		void _CreateCommandPools();
		void _CreateCommandBuffers();

		void _CreateSyncObjects();
	private:
		VulkanRendererSpec _Spec;
		VulkanRendererData _Data;
		std::shared_ptr< VulkanResourceFactory> _ResourceFactory;
	};
}
