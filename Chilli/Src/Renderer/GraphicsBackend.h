#pragma once

//#include "UUID\UUID.h"
#include "Maths.h"
#include "Buffers.h"
#include "Pipeline.h"
#include "Texture.h"
#include "glm/glm.hpp"
#include "Material.h"

namespace Chilli
{
	enum class GraphicsBackendType
	{
		VULKAN_1_3,
		DIRECTX_12
	};

	struct GraphicsDeviceFeatures
	{
		bool EnableSwapChain;
		bool EnableDescriptorIndexing;
	};

	enum class CommandBufferPurpose
	{
		NONE,
		GRAPHICS,
		PRESENT,
		TRANSFER,
		COMPUTE,
		COUNT
	};

	enum CommandBufferSubmitState
	{
		COMMAND_BUFFER_SUBMIT_STATE_ONE_TIME = 1, COMMAND_BUFFER_SUBMIT_STATE_SIMULTANEOUS_USE = 2, COMMAND_BUFFER_SUBMIT_STATE_RENDER_PASS_CONTINUE = 4, COMMAND_BUFFER_SUBMIT_STATE_NONE = 8
	};

	struct CommandBufferAllocInfo
	{
		uint32_t CommandBufferHandle;
		CommandBufferPurpose Purpose = CommandBufferPurpose::NONE;
		int State = CommandBufferSubmitState::COMMAND_BUFFER_SUBMIT_STATE_NONE;
	};

	struct CommandBufferBeginInfo
	{
		void* Buffer;
	};

	struct Semaphore
	{

	};

	struct GraphcisBackendCreateSpec
	{
		GraphicsBackendType Type  = GraphicsBackendType::VULKAN_1_3;
		const char* Name = "UnDefined";
		bool EnableValidation = true;
		GraphicsDeviceFeatures DeviceFeatures{
			.EnableSwapChain = true,
			.EnableDescriptorIndexing = true,
		};

		void* WindowSurfaceHandle = nullptr;

		bool VSync = true;;
		IVec2 ViewPortSize = { 800, 600 };
		bool ViewPortResized = true;
		uint32_t MaxFrameInFlight = 3;
	};

	struct BufferCopyInfo
	{
		uint32_t SrcOffset, DstOffset, Size;
	};

	// Global Set(0) Data Binding  0
	struct GlobalShaderData
	{
		Vec4 ResolutionTime;
	};
	// Global Set(0) Data Binding  1
	struct SceneShaderData
	{
		glm::mat4 ViewProjMatrix{ 1.0f };
		Vec4 CameraPos;
	};

	struct Fence
	{
		uint32_t Handle;
	};

	const uint32_t CH_MATERIAL_SHADER_DATA_AMOUNT = 1000;

	struct MaterialShaderData
	{
		IVec4 Indicies;
		Vec4 AlbedoColor;
	};

	const uint32_t CH_OBJECT_SHADER_DATA_AMOUNT = 1000;

	struct ObjectShaderData
	{
		glm::mat4 TransformationMat;
		glm::mat4 InverseTransformationMat;
	};

	enum class AttachmentLoadOp { LOAD, CLEAR };
	enum class AttachmentStoreOp { STORE, DONT_CARE};

	struct ColorAttachment
	{
		BackBone::AssetHandle<Texture> ColorTexture;
		bool UseSwapChainImage = false;
		Vec4 ClearColor;
		AttachmentLoadOp LoadOp;
		AttachmentStoreOp StoreOp;
	};

	struct DepthAttachment
	{
		BackBone::AssetHandle<Texture> DepthTexture;
		AttachmentLoadOp LoadOp;
		AttachmentStoreOp StoreOp;
		float Near, Far;
	};

	struct RenderPassInfo
	{
		const char* DebugName = nullptr;
		std::vector< ColorAttachment> ColorAttachments;
		DepthAttachment DepthAttachment;
		IVec2 RenderArea;
	};

	class GraphicsBackendApi
	{
	public:
		virtual ~GraphicsBackendApi() {}

		virtual void Init(const GraphcisBackendCreateSpec& Spec) = 0;
		virtual void Terminate() = 0;

		virtual const char* GetName() const = 0;
		virtual GraphicsBackendType GetType() const = 0;

		static GraphicsBackendApi* Create(const GraphcisBackendCreateSpec& Spec);
		static void Terminate(GraphicsBackendApi* Api, bool Delete = false);

		virtual CommandBufferAllocInfo AllocateCommandBuffer(CommandBufferPurpose Purpose) = 0;
		virtual void FreeCommandBuffer(CommandBufferAllocInfo& Info) = 0;

		virtual void AllocateCommandBuffers(CommandBufferPurpose Purpose, std::vector<CommandBufferAllocInfo>& Infos, uint32_t Count) = 0;
		virtual void FreeCommandBuffers(std::vector<CommandBufferAllocInfo>& Infos) = 0;

		virtual void ResetCommandBuffer(const CommandBufferAllocInfo& Info) = 0;
		virtual void BeginCommandBuffer(const CommandBufferAllocInfo& Info) = 0;
		virtual void EndCommandBuffer() = 0;

		virtual void SubmitCommandBuffer(const CommandBufferAllocInfo& Info, const Fence& SubmitFence) = 0;
		virtual void Present() = 0;
		virtual void FrameBufferResize(int Width, int Height) = 0;

		virtual bool RenderBegin(const CommandBufferAllocInfo& Info, uint32_t FrameIndex) = 0;
		virtual void BeginRenderPass(const RenderPassInfo& Pass) = 0;
		virtual void EndRenderPass() = 0;
		virtual void RenderEnd() = 0;

		virtual uint32_t AllocateBuffer(const BufferCreateInfo& Info) = 0;
		virtual void MapBufferData(uint32_t BufferHandle, void* Data, uint32_t Size, uint32_t Offset = 0) = 0;
		virtual void FreeBuffer(uint32_t BufferHandle) = 0;

		virtual uint32_t AllocateImage(const ImageSpec& ImgSpec, const char* FilePath = nullptr) = 0;
		virtual void LoadImageData(uint32_t TexHandle, const char* FilePath) = 0;
		virtual void LoadImageData(uint32_t TexHandle, void* Data, IVec2 Resolution) = 0;
		virtual void FreeTexture(uint32_t TexHandle) = 0;

		virtual uint32_t CreateSampler(const Sampler& sampler) = 0;
		virtual void DestroySampler(uint32_t  sampler) = 0;

		virtual uint32_t CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& CreateInfo) = 0;
		virtual void DestroyGraphicsPipeline(uint32_t PipelineHandle) = 0;

		virtual void BindGraphicsPipeline(uint32_t PipelineHandle) = 0;
		virtual void BindVertexBuffers(uint32_t* BufferHandles, uint32_t Count) = 0;
		virtual void BindIndexBuffer(uint32_t IBHandle, IndexBufferType Type) = 0;

		virtual void SetViewPortSize(int Width, int Height) = 0;
		virtual void SetViewPortSize(bool UseActiveRenderPassArea) = 0;
		virtual void SetScissorSize(int Width, int Height) = 0;
		virtual void SetScissorSize(bool UseActiveRenderPassArea) = 0;

		virtual void DrawArrays(uint32_t Count) = 0;
		virtual void DrawIndexed(uint32_t Count) = 0;
		virtual void SendPushConstant(int Type, void* Data, uint32_t Size
			, uint32_t Offset = 0) = 0;

		virtual uint32_t GetActiveCommandBufferHandle() const = 0;
		virtual void CopyBufferToBuffer(uint32_t SrcHandle, uint32_t DstHandle, const BufferCopyInfo& Info) = 0;

		// Should be Inside of BeginFrame
		virtual void UpdateGlobalShaderData(const GlobalShaderData& Data) = 0;
		virtual void UpdateSceneShaderData(const SceneShaderData& Data) = 0;
		virtual void UpdateMaterialSSBO(const BackBone::AssetHandle<Material>& Mat) = 0;
		virtual void UpdateObjectSSBO(const glm::mat4& TransformationMat, BackBone::Entity EntityID) = 0;

		virtual uint32_t CreateFence(bool signaled = false) = 0;
		virtual void DestroyFence(const Fence& fence) = 0;
		virtual void ResetFence(const Fence& fence) = 0;
		virtual bool IsFenceSignaled(const Fence& fence) = 0;
		virtual void WaitForFence(const Fence& fence, uint64_t TimeOut = UINT64_MAX) = 0;

		virtual SparseSet<uint32_t>& GetMap(BindlessSetTypes Type) = 0;
		virtual void PrepareForShutDown() = 0;

		virtual ShaderModule CreateShaderModule(const char* FilePath, ShaderStageType Type) = 0;
		virtual void DestroyShaderModule(const ShaderModule& Module) = 0;
	private:
	};
}
