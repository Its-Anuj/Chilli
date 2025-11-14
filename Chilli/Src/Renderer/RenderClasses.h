#pragma once

#include "Mesh.h"
#include "GraphicsBackend.h"

namespace Chilli
{
	class RenderCommand
	{
	public:
		RenderCommand(std::weak_ptr< GraphicsBackendApi> Api) { _Api = Api; }
		~RenderCommand() {}

		void Terminate() {}

		inline CommandBufferAllocInfo AllocateCommandBuffer(CommandBufferPurpose Purpose) { return _Api.lock()->AllocateCommandBuffer(Purpose); }
		inline void FreeCommandBuffer(CommandBufferAllocInfo& Info) { _Api.lock()->FreeCommandBuffer(Info); }

		inline void AllocateCommandBuffers(CommandBufferPurpose Purpose, std::vector<CommandBufferAllocInfo>& Infos, uint32_t Count)
		{
			_Api.lock()->AllocateCommandBuffers(Purpose, Infos, Count);
		}

		inline void FreeCommandBuffers(std::vector<CommandBufferAllocInfo>& Infos) { _Api.lock()->FreeCommandBuffers(Infos); }

		inline void ResetCommandBuffer(const CommandBufferAllocInfo& Info) { _Api.lock()->ResetCommandBuffer(Info); }
		inline void BeginCommandBuffer(const CommandBufferAllocInfo& Info) { _Api.lock()->BeginCommandBuffer(Info); }
		inline void EndCommandBuffer(const CommandBufferAllocInfo& Info) { _Api.lock()->EndCommandBuffer(Info); }

		size_t AllocateBuffer(const BufferCreateInfo& Info) { return _Api.lock()->AllocateBuffer(Info); }
		void MapBufferData(size_t BufferHandle, void* Data, size_t Size, uint32_t Offset = 0) {
			_Api.lock()->MapBufferData(BufferHandle, Data, Size, Offset);
		}

		void FreeBuffer(size_t BufferHandle) { _Api.lock()->FreeBuffer(BufferHandle); }

		size_t CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& CreateInfo) { return _Api.lock()->CreateGraphicsPipeline(CreateInfo); }
		void DestroyGraphicsPipeline(size_t PipelineHandle) { _Api.lock()->DestroyGraphicsPipeline(PipelineHandle); }
	private:
		std::weak_ptr<GraphicsBackendApi> _Api;
	};

	class Renderer
	{
	public:
		Renderer() {}
		~Renderer() {}

		void Init(const GraphcisBackendCreateSpec& Spec);
		void Terminate();
		std::shared_ptr<RenderCommand> CreateRenderCommand();

		void BeginFrame();

		void RenderBegin(const CommandBufferAllocInfo& Info, uint32_t FrameIndex) { _Api->RenderBegin(Info, FrameIndex); }
		
		void BeginRenderPass(const RenderPass& Pass)
		{
			_Api->BeginRenderPass(Pass);
		}
		
		void EndRenderPass()
		{
			_Api->EndRenderPass();
		}

		void RenderEnd(const CommandBufferAllocInfo& Info) {
			_Api->RenderEnd(Info);
		}

		inline void BindGraphicsPipeline(size_t PipelineHandle) { _Api->BindGraphicsPipeline(PipelineHandle); }
		inline void BindVertexBuffers(size_t* BufferHandles, uint32_t Count) { _Api->BindVertexBuffers(BufferHandles, Count); }
		inline void BindIndexBuffer(size_t IBHandle) { _Api->BindIndexBuffer(IBHandle); }

		inline void SetViewPortSize(int Width, int Height) { _Api->SetViewPortSize(Width, Height); }
		inline void SetScissorSize(int Width, int Height) { _Api->SetScissorSize(Width, Height); }

		inline void DrawArrays(uint32_t Count) { _Api->DrawArrays(Count); }
		inline void DrawIndexed() { _Api->DrawIndexed(); }

		const char* GetName() const { return _Api->GetName(); }
		GraphicsBackendType GetType() const { return _Api->GetType(); }
	private:
		std::shared_ptr<GraphicsBackendApi> _Api;
	};

}
