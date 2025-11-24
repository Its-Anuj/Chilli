#pragma once

#include "Mesh.h"
#include "GraphicsBackend.h"
#include "DeafultExtensions.h"

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
		inline void EndCommandBuffer() { _Api.lock()->EndCommandBuffer(); }

		uint32_t AllocateBuffer(const BufferCreateInfo& Info) { return _Api.lock()->AllocateBuffer(Info); }
		void MapBufferData(uint32_t BufferHandle, void* Data, uint32_t Size, uint32_t Offset = 0) {
			_Api.lock()->MapBufferData(BufferHandle, Data, Size, Offset);
		}

		void FreeBuffer(uint32_t BufferHandle) { _Api.lock()->FreeBuffer(BufferHandle); }

		uint32_t CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& CreateInfo) { return _Api.lock()->CreateGraphicsPipeline(CreateInfo); }
		
		void DestroyGraphicsPipeline(uint32_t PipelineHandle) { _Api.lock()->DestroyGraphicsPipeline(PipelineHandle); }


		inline uint32_t CreateFence(bool signaled = false) { return _Api.lock()->CreateFence(signaled); }
		inline void DestroyFence(const Fence& fence) { _Api.lock()->DestroyFence(fence); }
		inline void ResetFence(const Fence& fence) { _Api.lock()->ResetFence(fence); }
		inline bool IsFenceSignaled(const Fence& fence) { return _Api.lock()->IsFenceSignaled(fence); }
		inline void WaitForFence(const Fence& fence, uint64_t TimeOut = UINT64_MAX) {
			_Api.lock()->WaitForFence(fence, TimeOut);
		}

		inline uint32_t AllocateImage(const ImageSpec& ImgSpec, const char* FilePath = nullptr)
		{
			return _Api.lock()->AllocateImage(ImgSpec, FilePath);
		}

		inline void LoadImageData(uint32_t TexHandle, const char* FilePath) {
			_Api.lock()->LoadImageData(TexHandle, FilePath);
		}
		inline void FreeTexture(uint32_t TexHandle)
		{
			_Api.lock()->FreeTexture(TexHandle);
		}

		inline uint32_t CreateSampler(const Sampler& sampler) {
			return _Api.lock()->CreateSampler(sampler);
		}
		inline void DestroySampler(uint32_t  sampler) {
			_Api.lock()->DestroySampler(sampler);
		}

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

		bool RenderBegin(const CommandBufferAllocInfo& Info, uint32_t FrameIndex)
		{
			return  _Api->RenderBegin(Info, FrameIndex);
		}

		void BeginRenderPass(const RenderPass& Pass)
		{
			_Api->BeginRenderPass(Pass);
		}

		void EndRenderPass()
		{
			_Api->EndRenderPass();
		}

		void RenderEnd() {
			_Api->RenderEnd();
		}

		inline void BindGraphicsPipeline(uint32_t PipelineHandle) { _Api->BindGraphicsPipeline(PipelineHandle); }
		inline void BindVertexBuffers(uint32_t* BufferHandles, uint32_t Count) { _Api->BindVertexBuffers(BufferHandles, Count); }
		inline void BindIndexBuffer(uint32_t IBHandle) { _Api->BindIndexBuffer(IBHandle); }

		inline void SetViewPortSize(int Width, int Height) { _Api->SetViewPortSize(Width, Height); }
		inline void SetScissorSize(int Width, int Height) { _Api->SetScissorSize(Width, Height); }

		inline  void SetViewPortSize(bool UseSwapChainDimensions) {
			_Api->SetViewPortSize(UseSwapChainDimensions);
		}

		inline void SetScissorSize(bool UseSwapChainDimensions) {
			_Api->SetScissorSize(UseSwapChainDimensions);
		}

		inline  void FrameBufferResize(int Width, int Height) { _Api->FrameBufferResize(Width, Height); };

		inline void DrawArrays(uint32_t Count) { _Api->DrawArrays(Count); }
		inline void DrawIndexed() { _Api->DrawIndexed(); }
		inline void SendPushConstant(ShaderStageType Stage, void* Data, uint32_t Size
			, uint32_t Offset = 0) {
			_Api->SendPushConstant(Stage, Data, Size, Offset);
		}
		inline void UpdateGlobalShaderData(const GlobalShaderData& Data)
		{
			_Api->UpdateGlobalShaderData(Data);
		}
		inline void UpdateSceneShaderData(const SceneShaderData& Data)
		{
			_Api->UpdateSceneShaderData(Data);
		}

		inline void UpdateMaterialSSBO(const BackBone::AssetHandle<Material>& Mat)
		{
			_Api->UpdateMaterialSSBO(Mat);
		}
		inline void UpdateObjectSSBO(const glm::mat4& TransformationMat, BackBone::Entity EntityID) {
			_Api->UpdateObjectSSBO(TransformationMat, EntityID);
		}

		inline void SubmitCommandBuffer(const CommandBufferAllocInfo& Info, const Fence& SubmitFence)
		{
			_Api->SubmitCommandBuffer(Info, SubmitFence);
		}

		inline SparseSet<uint32_t>& GetMap(BindlessSetTypes Type) {
			return _Api->GetMap(Type);
		}

		const char* GetName() const { return _Api->GetName(); }
		GraphicsBackendType GetType() const { return _Api->GetType(); }
	private:
		std::shared_ptr<GraphicsBackendApi> _Api;
	};

}
