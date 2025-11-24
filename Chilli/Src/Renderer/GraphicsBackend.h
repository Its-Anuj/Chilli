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
		GraphicsBackendType Type;
		const char* Name;
		bool EnableValidation;
		GraphicsDeviceFeatures DeviceFeatures;
		void* WindowSurfaceHandle;

		bool VSync;
		IVec2 ViewPortSize;
		bool ViewPortResized;
		uint32_t MaxFrameInFlight;
	};

	struct ColorAttachment
	{
		bool UseSwapChainImage = true;
		Vec4 ClearColor;
	};

	struct RenderPass
	{
		ColorAttachment* pColorAttachments = nullptr;
		uint32_t ColorAttachmentCount = 0;
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
		virtual void BeginRenderPass(const RenderPass& Pass) = 0;
		virtual void EndRenderPass() = 0;
		virtual void RenderEnd() = 0;

		virtual uint32_t AllocateBuffer(const BufferCreateInfo& Info) = 0;
		virtual void MapBufferData(uint32_t BufferHandle, void* Data, uint32_t Size, uint32_t Offset = 0) = 0;
		virtual void FreeBuffer(uint32_t BufferHandle) = 0;

		virtual uint32_t AllocateImage(const ImageSpec& ImgSpec, const char* FilePath = nullptr) = 0;
		virtual void LoadImageData(uint32_t TexHandle, const char* FilePath) = 0;
		virtual void FreeTexture(uint32_t TexHandle) = 0;

		virtual uint32_t CreateSampler(const Sampler& sampler) = 0;
		virtual void DestroySampler(uint32_t  sampler) = 0;

		virtual uint32_t CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& CreateInfo) = 0;
		virtual void DestroyGraphicsPipeline(uint32_t PipelineHandle) = 0;

		virtual void BindGraphicsPipeline(uint32_t PipelineHandle) = 0;
		virtual void BindVertexBuffers(uint32_t* BufferHandles, uint32_t Count) = 0;
		virtual void BindIndexBuffer(uint32_t IBHandle) = 0;

		virtual void SetViewPortSize(int Width, int Height) = 0;
		virtual void SetViewPortSize(bool UseSwapChainDimensions) = 0;
		virtual void SetScissorSize(int Width, int Height) = 0;
		virtual void SetScissorSize(bool UseSwapChainDimensions) = 0;

		virtual void DrawArrays(uint32_t Count) = 0;
		virtual void DrawIndexed() = 0;
		virtual void SendPushConstant(ShaderStageType Stage, void* Data, uint32_t Size
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
	private:
	};
}
