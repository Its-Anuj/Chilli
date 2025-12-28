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
			_CommandBuffers.reserve(10);
			_CommandBuffers.push_back(GraphicsCommandBuffer());
			SetActiveCommandBuffer(0);
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

		uint32_t CreateCommandBuffer() {
			_CommandBuffers.push_back(GraphicsCommandBuffer());
			return _CommandBuffers.size();
		}

		void SetActiveCommandBuffer(uint32_t CmdBufferID) {
			_ActiveCommandBufferIndex = CmdBufferID;
			_ActiveCommandBuffer = &_CommandBuffers[_ActiveCommandBufferIndex];
		}

		void SetActivePipelineState(const PipelineStateInfo& State);
		void SetActiveCompiledRenderPass(const CompiledPass& Pass);

		inline void BindVertexBuffer(uint32_t VertexBufferID) {
			_ActiveCommandBuffer->_CurrentInfo.RenderState.VertexBufferID = VertexBufferID; 
		}
		
		inline void UnBindVertexBuffer() { 
			_ActiveCommandBuffer->_CurrentInfo.RenderState.VertexBufferID = UINT32_MAX; 
		}
		
		inline void BindIndexBuffer(uint32_t IndexBufferID) { 
			_ActiveCommandBuffer->_CurrentInfo.RenderState.IndexBufferID = IndexBufferID;
		}
		
		inline void UnBindIndexBuffer() {
			_ActiveCommandBuffer->_CurrentInfo.RenderState.IndexBufferID = UINT32_MAX;
		}

		inline void UseShaderProgram(uint32_t ProgramID) {
			_ActiveCommandBuffer->_CurrentInfo.RenderState.ShaderProgramID = ProgramID;
		}

		inline void UseMaterial(uint32_t RawMaterialID) { 
			_ActiveCommandBuffer->_CurrentInfo.RenderState.MaterialID = RawMaterialID;
		}

		void Draw(uint32_t VertexCount, uint32_t InstanceCount, uint32_t FirstVertex, uint32_t  FirstInstance);

		void PushInlineUniformData(uint64_t Stage, void* Data, size_t Size, size_t Offset = 0);

		void PushEmptyInlineUniformData() { 
			_ActiveCommandBuffer->_CurrentInfo.InlineUniformData.Block = nullptr;
		}

		void UpdateGlobalShaderData(const GlobalShaderData& Data) { _Api->UpdateGlobalShaderData(Data); }
		void UpdateSceneShaderData(const SceneShaderData& Data) { _Api->UpdateSceneShaderData(Data); }

		const char* GetName() const { return _Api->GetName(); }
		GraphicsBackendType GetType() const { return _Api->GetType(); }

		// --- Rasterization ---
		void SetCullMode(CullMode value)
		{
			_ActiveCommandBuffer->_CurrentInfo.ShaderCullMode = value;
		}

		void SetPolygonMode(PolygonMode value)
		{
			_ActiveCommandBuffer->_CurrentInfo.ShaderFillMode = value;
		}

		void SetFrontFace(FrontFaceMode value)
		{
			_ActiveCommandBuffer->_CurrentInfo.FrontFace = value;
		}

		void SetLineWidth(float value)
		{
			_ActiveCommandBuffer->_CurrentInfo.LineWidth = value;
		}

		void SetRasterizerDiscard(bool value)
		{
			_ActiveCommandBuffer->_CurrentInfo.RasterizerDiscardEnable = value;
		}

		// --- Depth Bias ---
		void SetDepthBiasEnable(bool value)
		{
			_ActiveCommandBuffer->_CurrentInfo.DepthBiasEnable = value;
		}

		void SetDepthBiasConstantFactor(float value)
		{
			_ActiveCommandBuffer->_CurrentInfo.DepthBiasConstantFactor = value;
		}

		void SetDepthBiasClamp(float value)
		{
			_ActiveCommandBuffer->_CurrentInfo.DepthBiasClamp = value;
		}

		void SetDepthBiasSlopeFactor(float value)
		{
			_ActiveCommandBuffer->_CurrentInfo.DepthBiasSlopeFactor = value;
		}

		void PushShaderBuffer(const BackBone::AssetHandle<Material>& Mat,
			const Chilli::BackBone::AssetHandle<Chilli::Buffer>& Buffer,
			const char* Name, size_t Size, size_t Offset);

		void PushShaderTexture(const BackBone::AssetHandle<Material>& Mat,
			const Chilli::BackBone::AssetHandle<Chilli::Texture>& Tex,
			const char* Name);

		void PushShaderSampler(const BackBone::AssetHandle<Material>& Mat,
			const Chilli::BackBone::AssetHandle<Chilli::Sampler>& Tex,
			const char* Name);

		template<typename T>
		void PushShaderBuffer(const BackBone::AssetHandle<Material>& Mat,
			const Chilli::BackBone::AssetHandle<Chilli::Buffer>& Buffer,
			const char* Name, size_t Offset)
		{
			PushShaderBuffer(Mat, Buffer, Name, sizeof(T), Offset);
		}

		uint32_t GetCurrentFrameIndex() { return _Api->GetCurrentFrameIndex(); }

		uint32_t GetTextureShaderIndex(uint32_t RawTextureHandle) { return _Api->GetTextureShaderIndex(RawTextureHandle); }
		uint32_t GetSamplerShaderIndex(uint32_t RawSamplerHandle) { return _Api->GetSamplerShaderIndex(RawSamplerHandle); }

		void UpdateMaterialShaderData(uint32_t MaterialHandle, const MaterialShaderData& Data) {
			_Api->UpdateMaterialShaderData(MaterialHandle, Data);
		}

		void UpdateMaterialShaderData(BackBone::AssetHandle<Material> MaterialHandle, const MaterialShaderData& Data) {
			_Api->UpdateMaterialShaderData(MaterialHandle.ValPtr->RawMaterialHandle, Data);
		}

		uint32_t GetMaterialShaderIndex(uint32_t RawMaterialHandle) {
			return _Api->GetMaterialShaderIndex(RawMaterialHandle);
		}

	private:
		std::shared_ptr<GraphicsBackendApi> _Api;
		
		std::vector< GraphicsCommandBuffer> _CommandBuffers;
		uint32_t _ActiveCommandBufferIndex = UINT32_MAX;
		GraphicsCommandBuffer* _ActiveCommandBuffer;

		// Separate data storage (also contiguous)
		FrameAllocator _InlineUniformDataAllocator;

		MemoryArena _RenderPerFrameArena;
		uint32_t _FrameIndex = 0;
		uint32_t _MaxFramesInFlight = 0;
	};
}
