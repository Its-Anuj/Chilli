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

		uint32_t InFrameFlightCount = 0;
		bool VSync;
		void* GLFWWINDOW;
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
		std::vector<VkCommandBuffer >GraphicsCommandBuffers;

		std::vector<VkSemaphore >ImageAvailableSemaphores; 
		std::vector<VkSemaphore >RenderFinishedSemaphores; 
		std::vector<VkFence >InFlightFences;	

		uint32_t CurrentImageIndex;
		uint32_t CurrentFrameIndex = 0;
		uint32_t FrameInFlightCount = 0;
		bool VSync;
		bool FrameBufferReSized;
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
		virtual bool BeginFrame() override;
		virtual bool BeginRenderPass(const BeginRenderPassInfo& Info) override;
		virtual void Submit(const std::shared_ptr<GraphicsPipeline>& Pipeline
			, const std::shared_ptr<VertexBuffer>& VB, const std::shared_ptr<IndexBuffer>& IB) override;
		virtual void EndRenderPass() override;
		virtual void RenderFrame() override;
		virtual void Present() override;
		virtual void EndFrame() override;	
		virtual void FinishRendering() override;
		virtual void FrameBufferReSized(int Width, int Height) override;

		virtual std::shared_ptr<ResourceFactory> GetResourceFactory() override;

		virtual float GetFrameBufferWidth() override { return _Data.FrameBufferSize.x; }
		virtual float GetFrameBufferHeight() override{ return _Data.FrameBufferSize.y; }
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
		void _ReCreateSwapChainKHR();

		void _CreateResourceFactory();

		// Commands
		void _CreateCommandBuffers();

		void _CreateSyncObjects();
	private:
		VulkanRendererSpec _Spec;
		VulkanRendererData _Data;
		std::shared_ptr< VulkanResourceFactory> _ResourceFactory;
	};
}
