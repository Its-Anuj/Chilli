#pragma once

#include "GraphicsBackend.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanPipeline.h"
#include "Core/SparseSet.h"

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
	struct VulkanBackendData
	{
		VkInstance Instance;
		VkDebugUtilsMessengerEXT DebugMessenger;
		VkSurfaceKHR SurfaceKHR;

		// Devices
		std::vector<VulkanPhysicalDevice> PhysicalDevices;
		int ActivePhysicalDeviceIndex = 0;

		VulkanDevice Device;
		uint16_t CurrentFrameIndex = 0;
		uint32_t CurrentImageIndex = 0;

		// SwapChain Info
		VulkanSwapChainKHR SwapChainKHR;

		VkSemaphore ImageAvailableSemaphore;
		VkSemaphore RenderFinishedSemaphore;
		VkFence InFlightFence;
	}; 

	struct VulkanBuffer
	{
		VkBuffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;
	};

	class VulkanGraphicsBackendApi : public GraphicsBackendApi
	{
	public:
		VulkanGraphicsBackendApi(const GraphcisBackendCreateSpec& Spec) { Init(Spec); }
		~VulkanGraphicsBackendApi() {}

		virtual void Init(const GraphcisBackendCreateSpec& Spec) override;
		virtual void Terminate() override;

		virtual const char* GetName() const override { return "Vulkan_1_3"; }
		virtual GraphicsBackendType GetType() const override { return GraphicsBackendType::VULKAN_1_3; }

		virtual CommandBufferAllocInfo AllocateCommandBuffer(CommandBufferPurpose Purpose) override;
		virtual void FreeCommandBuffer(CommandBufferAllocInfo& Info) override;

		virtual void AllocateCommandBuffers(CommandBufferPurpose Purpose, std::vector<CommandBufferAllocInfo>& Infos, uint32_t Count) override;
		virtual void FreeCommandBuffers(std::vector<CommandBufferAllocInfo>& Infos) override;

		virtual void* AcquireSwapChainImage() { return nullptr; }
		virtual void ResetCommandBuffer(const CommandBufferAllocInfo& Info) override;
		virtual void BeginCommandBuffer(const CommandBufferAllocInfo& Info) override;
		virtual void EndCommandBuffer(const CommandBufferAllocInfo& Info) override;
		virtual void SubmitCommandBuffer() override {}

		virtual void RenderBegin(const CommandBufferAllocInfo& Info, uint32_t FrameIndex) override;
		virtual void BeginRenderPass(const RenderPass& Pass) override;	
		virtual void EndRenderPass() override;
		virtual void RenderEnd(const CommandBufferAllocInfo& Info) override;

		virtual void BindGraphicsPipeline(size_t PipelineHandle) override;
		virtual void BindVertexBuffers(size_t* BufferHandles, uint32_t Count) override;
		virtual void BindIndexBuffer(size_t IBHandle) override;
		virtual void SetViewPortSize(int Width, int Height) override;
		virtual void SetScissorSize(int Width, int Height) override;
		virtual void DrawArrays(uint32_t Count) override;
		virtual void DrawIndexed() override{}																						

		virtual void Present() override {}

		virtual size_t AllocateBuffer(const BufferCreateInfo& Info) override;
		virtual void MapBufferData(size_t BufferHandle, void* Data, size_t Size, uint32_t Offset = 0) override;
		virtual void FreeBuffer(size_t BufferHandle) override;

		virtual size_t CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& CreateInfo) override;
		virtual void DestroyGraphicsPipeline(size_t PipelineHandle) override;

		virtual size_t GetActiveCommandBufferHandle() const override { return _ActiveCommandBufferHandle; }
	private:
		VulkanBackendData _Data;
		GraphcisBackendCreateSpec _Spec;
		VmaAllocator _Allocator;

		SparseSet<VkCommandBuffer> _CommandBuffers;
		size_t _ActiveCommandBufferHandle = 0;
		VkCommandBuffer _ActiveCommandBuffer;

		std::array<VkCommandPool, int(CommandBufferPurpose::COUNT)> _CommandPools;
		SparseSet<VulkanBuffer> _BufferSet;
		SparseSet<VulkanGraphicsPipeline> _GraphicsPipelineSet;;
	private:
		// Core Initials
		void _CreateInstance();
		void _CreateDebugMessenger();
		void _PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void _CreateSurfaceKHR();

		// Devices
		void _CreatePhysicalDevice(std::vector<const char*>& DeviceExtensions);
		void _FindSuitablePhysicalDevice();
		void _CreateLogicalDevice(std::vector<const char*>& DeviceExtensions);

		// SwapChain
		void _CreateSwapChainKHR();
		void _ReCreateSwapChainKHR();

		void _CreateCommandPools();
		void _DeleteCommandPools();

		void _CreateAllocator();
		void _DestroyAllocator();

		void _CreateSynchornization();
		void _DestroySynchornization();
	};
}
