#pragma once

#include "RenderAPI.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "VulkanSwapChain.h"

const char* VkResultToChar(VkResult Result);

#define VULKAN_SUCCESS_ASSERT(x, err) \
    {                                 \
        if (x != VK_SUCCESS)          \
        {                             \
            VULKAN_ERROR(err << " , VkResult: " << VkResultToChar(x));        \
        }                             \
    }
namespace Chilli
{
	struct VulkanRenderInitSpec
	{
		void* GlfwWindow;
		uint16_t InFrameFlightCount = 0;
		bool VSync = false;

		struct {
			int x, y;
		} InitialWindowSize;
		struct {
			int x, y;
		} InitialFrameBufferSize;

		bool EnableValidationLayer = false;
		std::vector<const char*> DeviceExtensions;
		std::vector<const char*> InstanceExtensions;
		std::vector<const char*> InstanceLayers;
		const char* Name;

	};

	struct VulkanBackendData
	{
		// Core
		VkInstance Instance;
		VkDebugUtilsMessengerEXT DebugMessenger;
		VkSurfaceKHR SurfaceKHR;

		// Devices
		std::vector<VulkanPhysicalDevice> PhysicalDevices;
		int ActivePhysicalDeviceIndex = 0;

		VulkanDevice Device;

		struct {
			int Width, Height;
		} FrameBufferSize;

		uint16_t InFrameFlightCount = 0;
		uint16_t CurrentFrameIndex = 0;
		uint32_t CurrentImageIndex = 0;

		VulkanSwapChainKHR SwapChainKHR;

		std::vector<VkCommandBuffer> GraphicsCommandBuffers;

		std::vector<VkSemaphore> ImageAvailableSemaphores;
		std::vector<VkSemaphore> RenderFinishedSemaphores;
		std::vector<VkFence> InFlightFences;
		bool FrameBufferReSized;
	};

	class VulkanRenderer : public RenderAPI
	{
	public:
		virtual void Init(void* Spec) override;
		virtual void ShutDown() override;

		virtual RenderAPIType GetType() const override { return RenderAPIType::VULKAN1_3; }
		virtual const char* GetName() const override { return "VULKAN1_3"; }
		
		virtual ResourceFactory& GetResourceFactory() override;

		virtual bool BeginFrame() override;
		virtual void BeginRenderPass(const RenderPass& Pass) override;
		virtual void Submit(const std::shared_ptr<GraphicsPipeline>& Pipeline, const std::shared_ptr<VertexBuffer>& VertexBuffer
			, const std::shared_ptr<IndexBuffer>& IndexBuffer) override;
		virtual void Submit(const Material& Mat, const std::shared_ptr<VertexBuffer>& VertexBuffer, const std::shared_ptr<IndexBuffer>& IndexBuffer)  override;
		virtual void EndRenderPass() override;		
		virtual void Render() override;
		virtual void Present() override;
		virtual void EndFrame() override;
		virtual void FinishRendering() override;

		virtual void FrameBufferReSized(int Width, int Height) override;
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

		void _CreateCommandBuffers();

		void _CreateSyncObjects();
	private:
		VulkanBackendData _Data;
		VulkanRenderInitSpec _Spec;
		VulkanResourceFactory _ResourceFactory;
	};
}
