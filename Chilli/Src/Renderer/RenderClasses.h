#pragma once

#include "Mesh.h"
#include "GraphicsBackend.h"
#include "DeafultExtensions.h"
#include "MemoryArena.h"
#include "FrameAllocator.h"

namespace Chilli
{
	class RenderCommand
	{
	public:
		RenderCommand(std::weak_ptr< GraphicsBackendApi> Api) { _Api = Api; }
		~RenderCommand() {}

		void Terminate() {}

		uint32_t AllocateBuffer(const BufferCreateInfo& Info) { return _Api.lock()->AllocateBuffer(Info); }
		void MapBufferData(uint32_t BufferHandle, void* Data, uint32_t Size, uint32_t Offset = 0) {
			_Api.lock()->MapBufferData(BufferHandle, Data, Size, Offset);
		}

		void FreeBuffer(uint32_t BufferHandle) { _Api.lock()->FreeBuffer(BufferHandle); }

		inline uint32_t CreateSampler(const SamplerSpec& Spec) {
			return _Api.lock()->CreateSampler(Spec);
		}
		inline void DestroySampler(uint32_t  sampler) {
			_Api.lock()->DestroySampler(sampler);
		}

		inline void PrepareForShutDown() {
			_Api.lock()->PrepareForShutDown();
		}

		inline ShaderModule CreateShaderModule(const char* FilePath, ShaderStageType Type) {
			return _Api.lock()->CreateShaderModule(FilePath, Type);
		}
		inline void DestroyShaderModule(const ShaderModule& Module) {
			_Api.lock()->DestroyShaderModule(Module);
		}

		inline uint32_t MakeShaderProgram() {
			return _Api.lock()->MakeShaderProgram();
		}
		inline void AttachShader(uint32_t ProgramHandle, const ShaderModule& Shader) {
			_Api.lock()->AttachShader(ProgramHandle, Shader);
		}

		inline void ClearShaderProgram(uint32_t ProgramHandle) {
			_Api.lock()->ClearShaderProgram(ProgramHandle);
		}
		inline void LinkShaderProgram(uint32_t ProgramHandle) {
			_Api.lock()->LinkShaderProgram(ProgramHandle);
		}

		inline uint32_t PrepareMaterialData(uint32_t ShaderProgramHandle)
		{
			return _Api.lock()->PrepareMaterialData(ShaderProgramHandle);
		}

		inline uint32_t PrepareMaterialData(const BackBone::AssetHandle<Material>& Mat)
		{
			return PrepareMaterialData(Mat.ValPtr->ShaderProgramId.ValPtr->RawProgramHandle);
		}

		void ClearMaterialData(uint32_t RawMaterialHandle)
		{
			_Api.lock()->ClearMaterialData(RawMaterialHandle);
		}

		void ClearMaterialData(const BackBone::AssetHandle<Material>& Mat)
		{
			_Api.lock()->ClearMaterialData(Mat.ValPtr->RawMaterialHandle);
		}

		const GraphcisBackendCreateSpec& GetSpec() const
		{
			return _Api.lock()->GetSpec();
		}

		inline uint32_t AllocateImage(const ImageSpec& Spec) {
			return _Api.lock()->AllocateImage(Spec);
		}

		inline void DestroyImage(uint32_t ImageHandle) {
			_Api.lock()->DestroyImage(ImageHandle);
		}
		inline void MapImageData(uint32_t ImageHandle, void* Data, int Width, int Height) {
			_Api.lock()->MapImageData(ImageHandle, Data, Width, Height);
		}

		inline uint32_t CreateTexture(uint32_t ImageHandle, const TextureSpec& Spec) {
			return _Api.lock()->CreateTexture(ImageHandle, Spec);
		}
		inline void DestroyTexture(uint32_t TextureHandle)
		{
			_Api.lock()->DestroyTexture(TextureHandle);
		}

	private:
		std::weak_ptr<GraphicsBackendApi> _Api;
	};

	class Renderer
	{
	public:
		Renderer() :_RenderPerFrameArena() {
		}

		~Renderer() {}

		void Init(const GraphcisBackendCreateSpec& Spec);
		void Terminate();
		std::shared_ptr<RenderCommand> CreateRenderCommand();

		inline  void FrameBufferResize(int Width, int Height) { _Api->FrameBufferResize(Width, Height); };

		void Clear();
		void BeginFrame();
		// Pushes all Graphics CommandBuffers to be executed and clears all graphics Command Buffer
		void EndFrame();

		const char* GetName() const { return _Api->GetName(); }
		GraphicsBackendType GetType() const { return _Api->GetType(); }

		void PushGraphicsCommandBuffer(const GraphicsCommandBuffer& Buffer) {
			_FramePackets[_FrameIndex].Graphics_Stream.PushCommandBuffer(Buffer);
		}

		void BeginRenderPass(const RenderPassInfo& Pass)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.BeginRenderPass(Pass);
		}

		void EndRenderPass()
		{
			_FramePackets[_FrameIndex].Graphics_Stream.EndRenderPass();
		}

		void PushPipelienBarrier(const PipelineBarrier& Barrier, RenderStreamTypes Stream)
		{
			if (Stream == RenderStreamTypes::TRANSFER)
				return;
			if (Stream == RenderStreamTypes::GRAPHICS)
				_FramePackets[_FrameIndex].Graphics_Stream.PushPipelineBarriers({ Barrier });
			if (Stream == RenderStreamTypes::COMPUTE)
				_FramePackets[_FrameIndex].Compute_Stream.PushPipelineBarriers({ Barrier });
		}

		void BindVertexBuffer(const std::vector<uint32_t>& Buffers)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.BindVertexBuffer(Buffers);
		}

		void BindIndexBuffer(uint32_t Handle, IndexBufferType Type)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.BindIndexBuffer(Handle, Type);
		}

		void DrawIndexed(uint32_t ElementCount, uint32_t InstanceCount, uint32_t FirstElement, uint32_t VertexOffset, uint32_t FirstInstance)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.DrawIndexed(ElementCount, InstanceCount, FirstElement,
				VertexOffset, FirstInstance);
		}

		void BindShaderProgram(uint32_t ShaderProgramHandle)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.BindShaderPrgoram(ShaderProgramHandle);
		}

		void SetFullPipelineState(const PipelineStateInfo& Info)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.SetFullPipelineState(Info);
		}

		void SetVertexInputLayout(const VertexInputShaderLayout& Info)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.SetVertexInputLayout(Info);
		}
		void SetTopologyMode(InputTopologyMode Mode)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.SetTopologyMode(Mode);
		}

		void SetCullMode(CullMode Mode)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.SetCullMode(Mode);
		}

		void SetFillMode(PolygonMode Mode)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.SetFillMode(Mode);
		}

		void SetFrontFace(FrontFaceMode Mode)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.SetFrontFace(Mode);
		}

		void SetLineWidth(float Width)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.SetLineWidth(Width);
		}

		void SetRasterizerDiscard(bool Enable)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.SetRasterizerDiscard(Enable);
		}

		void SetDepthBiasEnable(bool Enable)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.SetDepthBiasEnable(Enable);
		}

		void SetDepthBias(float ConstantFactor, float Clamp, float SlopeFactor)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.SetDepthBias(
				ConstantFactor,
				Clamp,
				SlopeFactor
			);
		}

		void DisableDepthBias()
		{
			_FramePackets[_FrameIndex].Graphics_Stream.DisableDepthBias();
		}

		uint32_t GetTextureShaderIndex(uint32_t RawTextureHandle) {
			return _Api->GetTextureShaderIndex(RawTextureHandle);
		}

		uint32_t GetSamplerShaderIndex(uint32_t RawSamplerHandle) {
			return _Api->GetSamplerShaderIndex(RawSamplerHandle);
		}

		uint32_t GetMaterialShaderIndex(uint32_t RawMaterialHandle) {
			return _Api->GetMaterialShaderIndex(RawMaterialHandle);
		}

		void UpdateMaterialBufferData(uint32_t MaterialHandle, uint32_t Buffer,
			const char* Name, size_t Size, size_t Offset)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.UpdateMaterialBufferData(MaterialHandle, Buffer,
				Name, Size, Offset);
		}

		void UpdateMaterialTextureData(uint32_t MaterialHandle,
			uint32_t Tex, const char* Name, ResourceState State)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.UpdateMaterialTextureData(MaterialHandle,
				Tex, Name, State);
		}

		void UpdateMaterialSamplerData(uint32_t MaterialHandle, uint32_t Sampler, const char* Name)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.UpdateMaterialSamplerData(MaterialHandle, Sampler, Name);
		}

		void BindMaterailData(uint32_t MaterialHandle)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.BindMaterailData(MaterialHandle);
		}

		void PushInlineUniformData(uint32_t ShaderProgram, uint32_t Stage, void* Data, uint32_t Size, uint32_t Offset)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.PushInlineUniformData(ShaderProgram, Stage, Data, Size, Offset);
		}

		void PushUpdateGlobalShaderData(const GlobalShaderData& Data)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.PushUpdateGlobalShaderData(Data);
		}

		void PushUpdateSceneShaderData(const SceneShaderData& Data)
		{
			_FramePackets[_FrameIndex].Graphics_Stream.PushUpdateSceneShaderData(Data);
		}


	private:
		std::shared_ptr<GraphicsBackendApi> _Api;
		std::vector<RenderFramePacket> _FramePackets;

		// Separate data storage (also contiguous)
		FrameAllocator _InlineUniformDataAllocator;

		MemoryArena _RenderPerFrameArena;
		uint32_t _FrameIndex = 0;
		uint32_t _MaxFramesInFlight = 0;
	};
}
