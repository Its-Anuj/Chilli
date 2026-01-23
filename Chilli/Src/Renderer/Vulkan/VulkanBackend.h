#pragma once

#include "GraphicsBackend.h"
#include "VulkanDevice.h"
#include "VulkanSwapChainKHR.h"
#include "VulkanShader.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"

const char* VkResultToChar(VkResult Result);

#define VULKAN_SUCCESS_ASSERT(x, err) \
    {                                 \
        if (x != VK_SUCCESS)          \
        {                             \
            VULKAN_ERROR(err << " , VkResult: " << VkResultToChar(x));        \
        }                             \
    }

#define VULKAN_ASSERT(x, err) \
    {                                 \
        if (x == false)          \
        {                             \
					VULKAN_PRINTLN(err);\
					assert(false && "Vulkan Error");\
		}                             \
    }

void _TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
	VkImageLayout oldLayout, VkImageLayout newLayout,
	VkImageAspectFlags aspectFlags);

namespace Chilli
{
	struct VulkanBackendData
	{
		VkInstance Instance;
		VkDebugUtilsMessengerEXT DebugMessenger;
		VkSurfaceKHR SurfaceKHR;

		// Devices
		std::vector<VulkanPhysicalDevice> PhysicalDevices;
		std::vector<RenderDeviceInfo> PhysicalDeviceInfos;
		std::vector<RenderDeviceLimits> PhysicalDeviceLimits;
		int ActivePhysicalDeviceIndex = 0;

		VulkanDevice Device;
		// SwapChain Info
		VulkanSwapChainKHR SwapChainKHR;

		VmaAllocator Allocator;
		uint32_t MaxSets;;
	};

	class VulkanCommandManager
	{
	public:
		VulkanCommandManager() {}
		~VulkanCommandManager() {}

		void Init(const VulkanDevice& Device);
		void Free(VkDevice Device);

		CommandBufferAllocInfo AllocateCommandBuffer(VkDevice Device, CommandBufferPurpose Purpose);
		std::vector<CommandBufferAllocInfo> AllocateCommandBuffers(VkDevice Device, CommandBufferPurpose Purpose, uint32_t Count);

		VkCommandBuffer Get(uint32_t Handle) { return *_Buffers.Get(Handle); }

	private:
		std::array<VkCommandPool, int(QueueFamilies::COUNT)> _Pools;
		SparseSet<VkCommandBuffer> _Buffers;
	};

	class VulkanDataUploader
	{
	public:
		VulkanDataUploader() {}
		~VulkanDataUploader() {}

		void Init(VulkanDevice* Device, VmaAllocator Allocator, VkCommandBuffer CommandBufferHandle,
			VkCommandBuffer GraphicsCmdBuffer);
		void Destroy();

		void BeginBatching(CommandBufferPurpose Purpose = CommandBufferPurpose::TRANSFER);
		// Ends Recording and submits them
		void EndBatching();

		void CopyBufferToBuffer(VkBuffer Src, VkBuffer Dst, const BufferCopyInfo& Info,
			VkAccessFlags2 dstAccess,
			VkPipelineStageFlags2 dstStage);

		void CopyBufferToImage(VkBuffer Src, VkImage Dst, const VkBufferImageCopy& Copy,
			VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange Range,
			VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage);

		void CopyImageToBuffer(VkImage Src, VkBuffer Dst, const VkBufferImageCopy& Copy,
			VkAccessFlags2 dstAccess,
			VkPipelineStageFlags2 dstStage);

		void CopyImageToImage(VkImage Src, VkImage Dst, const VkImageCopy& Copy,
			VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange Range,
			VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage);

		bool IsBatchRecording() const { return _BatchRecording; }

		void TransitionImageLayout(VkImage image,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkImageAspectFlags aspectFlags);

		void BarrierAfterCopy_Image(VkCommandBuffer cmd, VkImage Image,
			VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange Range,
			VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage);

		void AcquireOnGraphics_Image(VkCommandBuffer cmd, VkImage Image,
			VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange Range,
			VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage);

		void BarrierAfterCopy_Buffer(
			VkCommandBuffer cmd,
			VkBuffer buffer,
			VkAccessFlags2 dstAccess,
			VkPipelineStageFlags2 dstStage);

		void AcquireOnGraphics_Buffer(
			VkCommandBuffer cmd,
			VkBuffer buffer,
			VkAccessFlags2 dstAccess,
			VkPipelineStageFlags2 dstStage);

		void GenerateMipmaps(VkImage image,
			VkFormat format,
			int32_t texWidth,
			int32_t texHeight,
			uint32_t mipLevels);

		void ResolveImage(VkImage srcImage,
			VkImage dstImage, VkExtent2D extent, VkImageAspectFlags aspect);


	private:
		VulkanDevice* _Device;
		VmaAllocator _Allocator;
		VkFence _TransferFence, _GraphicsFence;
		VkCommandBuffer _CmdBuffer, _GraphicsCmdBuffer;
		VkCommandBuffer _ActiveCmdBuffer;

		ChBool8 _BatchRecording = CH_NONE;

		uint32_t _GraphicsQueueFamilyIndex;
		uint32_t _TransferQueueFamilyIndex;
		bool _SameFamily = false;
	};

	class VulkanGraphicsBackend : public GraphicsBackendApi
	{
	public:
		VulkanGraphicsBackend(const GraphcisBackendCreateSpec& Spec);
		~VulkanGraphicsBackend() {}

		virtual void Init(const GraphcisBackendCreateSpec& Spec);
		virtual void Terminate();

		virtual const char* GetName() const { return "VULKAN_1_3"; }
		virtual GraphicsBackendType GetType() const { return GraphicsBackendType::VULKAN_1_3; }

		virtual void FrameBufferResize(int Width, int Height) {}
		virtual void BeginFrame(uint32_t Index) override;
		virtual void EndFrame(const RenderFramePacket& Packet) override;

		virtual uint32_t AllocateBuffer(const BufferCreateInfo& Info);
		virtual void MapBufferData(uint32_t BufferHandle, void* Data, uint32_t Size, uint32_t Offset = 0);
		virtual void FreeBuffer(uint32_t BufferHandle) override;

		virtual ShaderModule CreateShaderModule(const char* FilePath, ShaderStageType Type);
		virtual void DestroyShaderModule(const ShaderModule& Module);

		virtual uint32_t MakeShaderProgram() override { return _ShaderManager.MakeShaderProgram(); }
		virtual void AttachShader(uint32_t ProgramHandle, const ShaderModule& Shader) override {
			_ShaderManager.AttachShader(ProgramHandle, Shader);
		}
		virtual void LinkShaderProgram(uint32_t ProgramHandle) override
		{
			_ShaderManager.LinkShaderProgram(_Data.Device.GetHandle(), ProgramHandle,
				_BindlessManager.GetBindlessSetLayouts());
		}

		virtual void ClearShaderProgram(uint32_t ProgramHandle) override
		{
			_ShaderManager.ClearShaderProgram(_Data.Device.GetHandle(), ProgramHandle);
		}

		void SetActiveGraphicsPipelineState(VkCommandBuffer CmdBuffer, const PipelineStateInfo& State);
		virtual void PrepareForShutDown() override;

		void SetColorBlendState(VkCommandBuffer CmdBuffer,
			const ColorBlendAttachmentState* ColorBlendAttachments, uint32_t ColorBlendAttachmentCount);

		void SetPrimitiveTopology(VkCommandBuffer CmdBuffer, InputTopologyMode mode);
		void SetPrimitiveRestartEnable(VkCommandBuffer CmdBuffer, ChBool8 enable);

		void SetCullMode(VkCommandBuffer CmdBuffer, CullMode mode);
		void SetFrontFace(VkCommandBuffer CmdBuffer, FrontFaceMode mode);
		void SetPolygonMode(VkCommandBuffer CmdBuffer, PolygonMode mode);
		void SetLineWidth(VkCommandBuffer CmdBuffer, float width);
		void SetRasterizerDiscardEnable(VkCommandBuffer CmdBuffer, ChBool8 enable);

		void SetDepthBiasEnable(VkCommandBuffer CmdBuffer, ChBool8 enable);
		void SetDepthBias(VkCommandBuffer CmdBuffer, float constantFactor, float clamp, float slopeFactor);
		void SetDepthTestEnable(VkCommandBuffer CmdBuffer, ChBool8 enable);

		void SetDepthWriteEnable(VkCommandBuffer CmdBuffer, ChBool8 enable);
		void SetDepthCompareOp(VkCommandBuffer CmdBuffer, CompareOp op);

		void SetStencilTestEnable(VkCommandBuffer CmdBuffer, ChBool8 enable);
		void SetStencilFrontOp(VkCommandBuffer CmdBuffer, const StencilOpState& state);
		void SetStencilBackOp(VkCommandBuffer CmdBuffer, const StencilOpState& state);
		void SetAlphaToCoverageEnable(VkCommandBuffer CmdBuffer, ChBool8 enable);
		void SetVertexInputState(VkCommandBuffer CmdBuffer, const VertexInputShaderLayout& Layout);

		void SetColorWriteEnable(VkCommandBuffer CmdBuffer, size_t Count, ChBool8* States);
		void SetSampleMask(VkCommandBuffer CmdBuffer, uint32_t sampleCount,
			uint32_t sampleMask);
		void SetRasterizationSamples(VkCommandBuffer CmdBuffer, uint32_t sampleCount);

		void UpdateGlobalShaderData(const GlobalShaderData& Data) override {
			_BindlessManager.UpdateGlobalShaderData(Data);
		}

		void UpdateSceneShaderData(uint32_t Index, const SceneData& Data) override
		{
			_BindlessManager.UpdateSceneShaderData(Index, Data);
		}

		uint32_t PrepareMaterialData(uint32_t ShaderProgramHandle) override;
		void ClearMaterialData(uint32_t RawMaterialHandle) override;

		virtual const GraphcisBackendCreateSpec& GetSpec() const override { return _Spec; }
		virtual uint32_t GetCurrentFrameIndex() override { return _FrameResource.CurrentFrameIndex; }

		virtual uint32_t AllocateImage(ImageSpec& Spec) override {
			return _ImageDataManager.AllocateImage(_Data.Allocator, Spec);
		}
		virtual void DestroyImage(uint32_t ImageHandle) override {
			_ImageDataManager.DestroyImage(_Data.Allocator, ImageHandle);
		}
		virtual void MapImageData(uint32_t ImageHandle, void* Data, int Width, int Height) override {
			_ImageDataManager.MapImageData(ImageHandle, Data, Width, Height);
		}

		virtual uint32_t CreateTexture(uint32_t ImageHandle, TextureSpec& Spec) override {
			auto Handle = _ImageDataManager.CreateTexture(_Data.Device.GetHandle(), ImageHandle, Spec);
			_BindlessManager.PrepareForTexture(Handle);
			return Handle;
		}
		virtual void DestroyTexture(uint32_t TextureHandle) override {
			_ImageDataManager.DestroyTexture(_Data.Device.GetHandle(), TextureHandle);
		}

		virtual uint32_t CreateSampler(const SamplerSpec& Spec);
		virtual void DestroySampler(uint32_t SamplerHandle) override;

		virtual uint32_t GetTextureShaderIndex(uint32_t RawTextureHandle) override;
		virtual uint32_t GetSamplerShaderIndex(uint32_t RawSamplerHandle) override;
		virtual uint32_t GetMaterialShaderIndex(uint32_t RawMaterialHandle) override;

		virtual void UpdateMaterialShaderData(uint32_t MaterialHandle, const MaterialShaderData& Data) {
			_BindlessManager.UpdateMaterialShaderData(MaterialHandle, Data);
		}

		virtual void UpdateObjectShaderData(BackBone::Entity Entity, const ObjectShaderData& Data) override {
			_BindlessManager.UpdateObjectShaderData(Entity, Data);
		}

		virtual uint32_t GetObjectShaderIndex(uint32_t Entity) override {
			return _BindlessManager.GetObjectShaderIndex(Entity);
		}

		virtual const std::vector<RenderDeviceInfo>& GetRenderDevices() {
			return _Data.PhysicalDeviceInfos;
		}
		// Takes in the raw render device handle that is inside the info
		virtual const RenderDeviceLimits& GetRenderDeviceLimit(uint32_t Handle) {
			return _Data.PhysicalDeviceLimits[Handle];
		}
		virtual const RenderDeviceLimits& GetActiveRenderDeviceLimit() {
			return _Data.PhysicalDeviceLimits[_Data.ActivePhysicalDeviceIndex];
		}
		virtual const RenderDeviceStats& GetRenderDeviceStats() { return RenderDeviceStats(); }
	private:
		GraphcisBackendCreateSpec _Spec;
		VulkanBackendData _Data;

		VulkanShaderDataManager _ShaderManager;
		VulkanBufferManager _BufferManager;
		VulkanCommandManager _CommandManager;

		struct {
			std::vector<CommandBufferAllocInfo> CommandBuffers;
			uint16_t CurrentFrameIndex = 0;
			uint32_t CurrentImageIndex = 0;

			std::vector<VkSemaphore >ImageAvailableSemaphores;
			std::vector<VkSemaphore >ComputeFinishedSemaphores;
			std::vector<VkSemaphore >RenderFinishedSemaphores;
			std::vector<VkFence >InFlightFences;

			std::vector<VkDescriptorImageInfo> WritingImageInfos;
			std::vector<VkDescriptorBufferInfo> WritingBufferInfos;
			std::vector< VkWriteDescriptorSet> WritingSets;
			std::vector< MaterialDataUpdateCmdPayload> MaterailWritingDatas;
		} _FrameResource;

		PipelineStateInfo _ActivePipelineState;
		VulkanBindlessRenderingManager _BindlessManager;
		VulkanBindlessSetManagerCreateInfo  _BindlessCreateInfo;

		struct VulkanBackendMaterialInfo
		{
			std::vector<std::array<VkDescriptorSet, int(BindlessSetTypes::COUNT_USER)>> Sets;
			uint32_t ProgramID = UINT32_MAX;
			std::array<std::vector<uint64_t>, int(BindlessSetTypes::COUNT_USER)> SetBindingsStates;
		};

		SparseSet<VulkanBackendMaterialInfo> _MaterialManager;

		VkDescriptorPool _GeneralDescriptorPool;
		VulkanDataUploader _Uploader;
		VulkanImageDataManager _ImageDataManager;
		SparseSet<VulkanSampler> _SamplerSet;
		SceneShaderData _ActiveSceneShaderData;

	private:
		// Core Initials
		void _CreateInstance();
		void _DestroyInstance();

		void _CreateDebugMessenger();
		void _PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void _DestroyDebugMessenger();

		void _CreateSurfaceKHR();
		void _DestroySurfaceKHR();

		// Devices
		void _CreatePhysicalDevice(std::vector<const char*>& DeviceExtensions);
		void _FindSuitablePhysicalDevice();
		void _CreateLogicalDevice(std::vector<const char*>& DeviceExtensions);
		void _DestroyLogicalDevice();

		// SwapChain
		void _CreateSwapChainKHR();
		void _ReCreateSwapChainKHR();
		void _DestroySwapChainKHR();

		void _CreateVMAAllocator();
		void _DestroyVMAAllocator();

		void _CreateFrameResources();
		void _DestroyFrameResources();

		void _SetViewPortSize(VkCommandBuffer CmdBuffer, int Width, int Height);
		void _SetScissorSize(VkCommandBuffer CmdBuffer, int Width, int Height);

		void _CreateGeneralDescriptorPool();
		void _DestroyGeneralDescriptorPool();

		void _CreateVulkanDataUploader();
		void _DestroyVulkanDataUploader();

		void _CreateVulkanImageDataManager();
		void _DestroyVulkanImageDataManager();

		uint32_t _GetQueueFamilyIndex(RenderStreamTypes stream);

		VkCommandBuffer _BeginFrame(const BeginFrameCmdPayload& Payload);
		void _EndFrame(const EndFrameCmdPayload& Payload, VkCommandBuffer CmdBuffer);
		void _BeginRenderPass(const BeginRenderPassCmdPayload& Payload, VkCommandBuffer CmdBuffer);
		void _EndRenderPass(const EndRenderPassCmdPayload& Payload, VkCommandBuffer CmdBuffer);
		void _ExecutePipelineBarriers(const PipelineBarrier* barriers, uint32_t count, VkCommandBuffer cmdBuffer);
		void _AppendMaterialUpdateData(const MaterialDataUpdateCmdPayload& Payload);
		void _UpdateAllMaterialUpdateData();

		VkCommandBuffer _TranslateGraphicsCommandBuffer(const GraphicsCommandBuffer& CmdBuffer);
	};
}
