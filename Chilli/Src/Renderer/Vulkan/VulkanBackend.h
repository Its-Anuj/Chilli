#pragma once

#include "GraphicsBackend.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanPipeline.h"
#include "VulkanTextures.h"
#include "Core/SparseSet.h"
#include "VulkanDescriptorManager.h"

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
	struct VulkanFenceManager;

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

		std::vector<VkSemaphore >ImageAvailableSemaphores;
		std::vector<VkSemaphore >RenderFinishedSemaphores;
		std::vector<VkFence >InFlightFences;
	};

	struct VulkanBuffer
	{
		VkBuffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;
		BufferCreateInfo CreateInfo;
	};

	// Engine Fence API (non-virtual)
	class VulkanFenceManager {
	public:
		// Create a new fence, returns handle
		uint32_t CreateFence(VkDevice Device, bool signaled = false);

		// Wait for fence completion on CPU (blocks)
		void Wait(VkDevice Device, const uint32_t& handle, uint64_t timeoutNs = UINT64_MAX);

		// Check if fence is complete without blocking
		bool IsSignaled(VkDevice Device, const uint32_t& handle);

		// Reset fence
		void Reset(VkDevice Device, const uint32_t& handle);

		// Destroy fence
		void Destroy(VkDevice Device, const uint32_t& handle);

		VkFence Get(uint32_t Handle) { return _Fences.Get(Handle)->BackendFence; }
	private:
		struct FenceEntry {
			VkFence BackendFence; // e.g., VkFence, ID3D12Fence*, MTLFence*
			bool inUse;
		};

		SparseSet<FenceEntry> _Fences; // sparse set or dense vector
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
		virtual void EndCommandBuffer() override;
		virtual void SubmitCommandBuffer(const CommandBufferAllocInfo& Info, const Fence& SubmitFence) override;

		virtual bool RenderBegin(const CommandBufferAllocInfo& Info, uint32_t FrameIndex) override;
		virtual void BeginRenderPass(const RenderPassInfo& Pass) override;
		virtual void EndRenderPass() override;
		virtual void RenderEnd() override;

		virtual void BindGraphicsPipeline(uint32_t PipelineHandle) override;
		virtual void BindVertexBuffers(uint32_t* BufferHandles, uint32_t Count) override;
		virtual void BindIndexBuffer(uint32_t IBHandle, IndexBufferType Type) override;
		virtual void SetViewPortSize(int Width, int Height) override;

		inline virtual void SetViewPortSize(bool UseActiveRenderPassArea) override {
			SetViewPortSize(_ActiveRenderPass->RenderArea.x, _ActiveRenderPass->RenderArea.y);
		}
		inline virtual void SetScissorSize(bool UseActiveRenderPassArea) override {
			SetScissorSize(_ActiveRenderPass->RenderArea.x, _ActiveRenderPass->RenderArea.y);
		}
		 
		virtual void SetScissorSize(int Width, int Height) override;

		virtual void FrameBufferResize(int Width, int Height) override;
		virtual void DrawArrays(uint32_t Count) override;
		virtual void DrawIndexed(uint32_t Count) override;
		virtual void SendPushConstant(int Type, void* Data, uint32_t Size
			, uint32_t Offset = 0) override;

		virtual void Present() override {}

		virtual uint32_t AllocateBuffer(const BufferCreateInfo& Info) override;
		virtual void MapBufferData(uint32_t BufferHandle, void* Data, uint32_t Size, uint32_t Offset = 0) override;
		virtual void FreeBuffer(uint32_t BufferHandle) override;

		virtual uint32_t AllocateImage(const ImageSpec& ImgSpec, const char* FilePath = nullptr) override;
		virtual void LoadImageData(uint32_t TexHandle, const char* FilePath) override;
		virtual void LoadImageData(uint32_t TexHandle, void* Data, IVec2 Resolution) override;
		virtual void FreeTexture(uint32_t TexHandle) override;

		virtual uint32_t CreateSampler(const Sampler& sampler) override;
		virtual void DestroySampler(uint32_t  sampler) override;

		virtual uint32_t CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& CreateInfo) override;
		virtual void DestroyGraphicsPipeline(uint32_t PipelineHandle) override;

		virtual uint32_t GetActiveCommandBufferHandle() const override { return _ActiveCommandBufferHandle; }
		virtual void CopyBufferToBuffer(uint32_t SrcHandle, uint32_t DstHandle, const BufferCopyInfo& Info) override;

		void BindDescriptorSets(uint32_t* Sets, uint32_t SetCount);
		virtual void PrepareForShutDown() override;

		virtual void UpdateGlobalShaderData(const GlobalShaderData& Data) override;
		virtual void UpdateSceneShaderData(const SceneShaderData& Data) override;
		virtual void UpdateMaterialSSBO(const BackBone::AssetHandle<Material>& Mat) override;
		virtual void UpdateObjectSSBO(const glm::mat4& TransformationMat, BackBone::Entity EntityID) override;

		virtual SparseSet<uint32_t>& GetMap(BindlessSetTypes Type) override {
			return _BindlessSetManager.GetMap(Type);
		}

		virtual uint32_t CreateFence(bool signaled = false) override;
		virtual void DestroyFence(const Fence& fence) override;
		virtual void ResetFence(const Fence& fence) override;
		virtual bool IsFenceSignaled(const Fence& fence) override;
		virtual void WaitForFence(const Fence& fence, uint64_t TimeOut = UINT64_MAX) override;

		virtual ShaderModule CreateShaderModule(const char* FilePath, ShaderStageType Type) override;	
		virtual void DestroyShaderModule(const ShaderModule& Module) override;

	private:
		VulkanBackendData _Data;
		GraphcisBackendCreateSpec _Spec;
		VmaAllocator _Allocator;

		SparseSet<VkCommandBuffer> _CommandBuffers;

		uint32_t _ActiveCommandBufferHandle = 0;
		VkCommandBuffer _ActiveCommandBuffer;
		VulkanGraphicsPipeline* _ActivePipeline = nullptr;
		VkPipelineLayout _ActivePipelineLayout = nullptr;

		std::array<VkCommandPool, int(CommandBufferPurpose::COUNT)> _CommandPools;
		SparseSet<VulkanSampler> _Samplers;
		SparseSet<VulkanBuffer> _BufferSet;
		SparseSet<VulkanTexture> _TextureSet;
		SparseSet<VulkanGraphicsPipeline> _GraphicsPipelineSet;
		VulkanPipelineLayoutManager _LayoutManager;

		struct VulkanShaderModule
		{
			VkShaderModule Module;
			ReflectedShaderInfo ReflectedInfo;
		};

		SparseSet<VulkanShaderModule> _ShaderModules;
		RenderPassInfo* _ActiveRenderPass = nullptr;

		struct
		{
			uint32_t BufferID = SparseSet<uint32_t>::npos;
			CommandBufferAllocInfo StagingBufferCmd{ SparseSet<uint32_t>::npos, CommandBufferPurpose::TRANSFER,
				COMMAND_BUFFER_SUBMIT_STATE_ONE_TIME };
			Fence Fence;
			// For now 1 megabyte
			const uint32_t StagingBufferPoolSize = 1e6;
		} _StagingBufferManager;

		VulkanFenceManager _Fences;
		VulkanBindlessSetManagerCreateInfo CreateInfo;
		VulkanBindlessSetManager _BindlessSetManager;

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

		void _CreateStagingBufferManager();
		void _FreeStagingBufferManager();

		void _SetupBindlessSetManager();
		void _ShutDownBindlessSetManager();
	};
}
