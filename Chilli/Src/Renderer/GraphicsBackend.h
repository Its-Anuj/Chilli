#pragma once

#include "UUID\UUID.h"
#include "Maths.h"
#include "Buffers.h"
#include "Pipeline.h"

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

	struct CommandBufferAllocInfo
	{
		size_t CommandBufferHandle;
		CommandBufferPurpose Purpose = CommandBufferPurpose::NONE;
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
		virtual void EndCommandBuffer(const CommandBufferAllocInfo& Info) = 0;

		virtual void SubmitCommandBuffer() = 0;
		virtual void Present() = 0;

		virtual void RenderBegin(const CommandBufferAllocInfo& Info, uint32_t FrameIndex) = 0;
		virtual void BeginRenderPass(const RenderPass& Pass) = 0;
		virtual void EndRenderPass() = 0;
		virtual void RenderEnd(const CommandBufferAllocInfo& Info) = 0;

		virtual size_t AllocateBuffer(const BufferCreateInfo& Info) = 0;
		virtual void MapBufferData(size_t BufferHandle, void* Data, size_t Size, uint32_t Offset = 0) = 0;
		virtual void FreeBuffer(size_t BufferHandle) = 0;

		virtual size_t CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& CreateInfo) = 0;
		virtual void DestroyGraphicsPipeline(size_t PipelineHandle) = 0;

		virtual void BindGraphicsPipeline(size_t PipelineHandle) = 0;
		virtual void BindVertexBuffers(size_t* BufferHandles, uint32_t Count) = 0;
		virtual void BindIndexBuffer(size_t IBHandle) = 0;

		virtual void SetViewPortSize(int Width, int Height) = 0;
		virtual void SetScissorSize(int Width, int Height) = 0;

		virtual void DrawArrays(uint32_t Count) = 0;
		virtual void DrawIndexed() = 0;

		virtual size_t GetActiveCommandBufferHandle() const = 0;
	private:
	};
}
