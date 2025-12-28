#include "RenderClasses.h"
#include "RenderClasses.h"
#include "RenderClasses.h"
#include "RenderClasses.h"
#include "Ch_PCH.h"
#include "RenderClasses.h"

#include "vulkan\vulkan.h"
#include "vk_mem_alloc.h"
#include "Vulkan/VulkanBackend.h"

namespace Chilli
{
	GraphicsBackendApi* GraphicsBackendApi::Create(const GraphcisBackendCreateSpec& Spec)
	{
		if (Spec.Type == GraphicsBackendType::VULKAN_1_3)
		{
			return new VulkanGraphicsBackend(Spec);
			//return nullptr;
		}
	}

	void GraphicsBackendApi::Terminate(GraphicsBackendApi* Api, bool Delete)
	{
		Api->Terminate();
		if (Delete)
			delete Api;
	}

	void Renderer::Init(const GraphcisBackendCreateSpec& Spec)
	{
		_Api = std::shared_ptr<GraphicsBackendApi>(GraphicsBackendApi::Create(Spec));
		_RenderPerFrameArena.Prepare(5 * 1024 * 1024);
		_InlineUniformDataAllocator.Ref(_RenderPerFrameArena, 1 * 1024 * 128);

		_MaxFramesInFlight = Spec.MaxFrameInFlight;
		_FrameIndex = 0;
	}

	void Renderer::Terminate()
	{
		GraphicsBackendApi::Terminate(_Api.get(), false);
	}

	void Renderer::BeginFrame()
	{
		_Api->BeginFrame(_FrameIndex);
	}

	void Renderer::EndFrame()
	{
		_Api->EndFrame(_CommandBuffers);
		_FrameIndex = (_FrameIndex + 1) % _MaxFramesInFlight;
		this->Clear();
	}

	void Renderer::SetActivePipelineState(const PipelineStateInfo& State)
	{
		PipelineStateInfo PushState = State;

		_ActiveCommandBuffer->_CurrentInfo.RenderState.PipelineStateID = _ActiveCommandBuffer->_PipelineOffset;

		_ActiveCommandBuffer->_PipelineStates.push_back(PushState);
		_ActiveCommandBuffer->_PipelineOffset++;

		// --- Input Assembly ---
		_ActiveCommandBuffer->_CurrentInfo.TopologyMode = State.TopologyMode;

		// --- Rasterization ---
		_ActiveCommandBuffer->_CurrentInfo.ShaderCullMode = State.ShaderCullMode;
		_ActiveCommandBuffer->_CurrentInfo.ShaderFillMode = State.ShaderFillMode;
		_ActiveCommandBuffer->_CurrentInfo.FrontFace = State.FrontFace;
		_ActiveCommandBuffer->_CurrentInfo.LineWidth = State.LineWidth;
		_ActiveCommandBuffer->_CurrentInfo.RasterizerDiscardEnable = State.RasterizerDiscardEnable;

		// Depth Bias
		_ActiveCommandBuffer->_CurrentInfo.DepthBiasEnable = State.DepthBiasEnable;
		_ActiveCommandBuffer->_CurrentInfo.DepthBiasConstantFactor = State.DepthBiasConstantFactor;
		_ActiveCommandBuffer->_CurrentInfo.DepthBiasClamp = State.DepthBiasClamp;
		_ActiveCommandBuffer->_CurrentInfo.DepthBiasSlopeFactor = State.DepthBiasSlopeFactor;
	}

	void Renderer::SetActiveCompiledRenderPass(const CompiledPass& Pass)
	{
		_ActiveCommandBuffer->_RenderPasses.push_back(Pass);
		_ActiveCommandBuffer->_CurrentInfo.RenderState.RenderPassIndex = _ActiveCommandBuffer->_RenderPasses.size() - 1;
	}

	void Renderer::Draw(uint32_t VertexCount, uint32_t InstanceCount, uint32_t FirstVertex, uint32_t FirstInstance)
	{
		CH_CORE_ASSERT(_ActiveCommandBuffer->_CurrentInfo.RenderState.RenderPassIndex != SparseSet<uint32_t>::npos,
			"Active Render Pass Is Required");
		CH_CORE_ASSERT(_ActiveCommandBuffer->_CurrentInfo.RenderState.PipelineStateID != SparseSet<uint32_t>::npos,
			"Active Pipeline State Is Required");

		_ActiveCommandBuffer->_CurrentInfo.Type = CommandType::Draw;
		_ActiveCommandBuffer->_CurrentInfo.ElementCount = VertexCount;
		_ActiveCommandBuffer->_CurrentInfo.FirstElement = FirstVertex;
		_ActiveCommandBuffer->_CurrentInfo.InstanceCount = InstanceCount;
		_ActiveCommandBuffer->_CurrentInfo.FirstInstance = FirstInstance;

		_ActiveCommandBuffer->_RenderCommandInfos.push_back(_ActiveCommandBuffer->_CurrentInfo);

		// Just Reset the Drawing elemnts stuffs since the states are same
		_ActiveCommandBuffer->_CurrentInfo.ElementCount = 0;
		_ActiveCommandBuffer->_CurrentInfo.FirstElement = 0;
		_ActiveCommandBuffer->_CurrentInfo.InstanceCount = 0;
		_ActiveCommandBuffer->_CurrentInfo.FirstInstance = 0;
	}

	void Renderer::PushInlineUniformData(uint64_t Stage, void* Data, size_t Size, size_t Offset)
	{
		// 1. Validation Checks
		if (Offset + Size > MAX_INLINE_UNIFORM_DATA_SIZE)
		{
			CH_CORE_ERROR("This command exceeds the allowed {}-byte limit!", MAX_INLINE_UNIFORM_DATA_SIZE);
			return;
		}

		// 2. Allocate the full 128-byte block if this is the first push constant for the command.
		if (_ActiveCommandBuffer->_CurrentInfo.InlineUniformData.Block == nullptr)
		{
			// Allocate the full 128 bytes from the frame arena
			_ActiveCommandBuffer->_CurrentInfo.InlineUniformData.Block = _InlineUniformDataAllocator.Alloc(MAX_INLINE_UNIFORM_DATA_SIZE);

			// Zero out the block for safety, as only part of it might be written
			memset(_ActiveCommandBuffer->_CurrentInfo.InlineUniformData.Block, 0, MAX_INLINE_UNIFORM_DATA_SIZE);
		}

		// 3. Copy the new data into the correct offset within the contiguous block.
		uint8_t* destination = static_cast<uint8_t*>(_ActiveCommandBuffer->_CurrentInfo.InlineUniformData.Block) + Offset;
		memcpy(destination, Data, Size);

		// You might also want to track the stage flags for this command
		_ActiveCommandBuffer->_CurrentInfo.InlineUniformData.Stages |= Stage;
		_ActiveCommandBuffer->_CurrentInfo.InlineUniformData.TotalSize += Offset + Size;

		// We no longer need _InlineUniformDataOffset or _InlineUniformData list, 
		// as the entire block is now stored directly in _CurrentInfo.
	}

	void Renderer::PushShaderBuffer(const BackBone::AssetHandle<Material>& Mat,
		const Chilli::BackBone::AssetHandle<Chilli::Buffer>& Buffer,
		const char* Name, size_t Size, size_t Offset)
	{
		RenderShaderDataUpdateInfo Info{};
		Info.MaterialHandle = Mat.ValPtr->RawMaterialHandle;
		Info.DstArrayIndex = 0;
		strncpy(Info.Name, Name, SHADER_UNIFORM_BINDING_NAME_SIZE);
		Info.Persistent = false;
		Info.BufferInfo.Handle = Buffer.ValPtr->RawBufferHandle;
		Info.BufferInfo.Range = Size;
		Info.BufferInfo.Offset = Offset;

		_ActiveCommandBuffer->_ShaderUpdateInfos.push_back(Info);
	}

	void Renderer::PushShaderTexture(const BackBone::AssetHandle<Material>& Mat,
		const Chilli::BackBone::AssetHandle<Chilli::Texture>& Tex, const char* Name)
	{
		RenderShaderDataUpdateInfo Info{};
		Info.MaterialHandle = Mat.ValPtr->RawMaterialHandle;
		Info.DstArrayIndex = 0;
		strncpy(Info.Name, Name, SHADER_UNIFORM_BINDING_NAME_SIZE);
		Info.Persistent = false;
		Info.ImageInfo.Handle = Tex.ValPtr->RawTextureHandle;

		_ActiveCommandBuffer->_ShaderUpdateInfos.push_back(Info);
	}

	void Renderer::PushShaderSampler(const BackBone::AssetHandle<Material>& Mat, const Chilli::BackBone::AssetHandle<Chilli::Sampler>& Tex, const char* Name)
	{
		RenderShaderDataUpdateInfo Info{};
		Info.MaterialHandle = Mat.ValPtr->RawMaterialHandle;
		Info.DstArrayIndex = 0;
		strncpy(Info.Name, Name, SHADER_UNIFORM_BINDING_NAME_SIZE);
		Info.Persistent = false;
		Info.ImageInfo.Sampler = Tex.ValPtr->SamplerHandle;

		_ActiveCommandBuffer->_ShaderUpdateInfos.push_back(Info);
	}

	void Renderer::Clear()
	{
		_InlineUniformDataAllocator.ResetSize();

		for (auto& CmdBuffer : _CommandBuffers)
			CmdBuffer.Clear();
		_ActiveCommandBuffer = &_CommandBuffers[_ActiveCommandBufferIndex];
		_ActiveCommandBufferIndex = 0;
	}

	std::shared_ptr<RenderCommand> Renderer::CreateRenderCommand()
	{
		return std::make_shared<RenderCommand>(_Api);
	}

	static inline void HashCombine(uint64_t& seed, uint64_t value)
	{
		const uint64_t fnv_offset = 1469598103934665603ull;
		const uint64_t fnv_prime = 1099511628211ull;

		seed ^= value;
		seed *= fnv_prime;
	}

	uint64_t ShaderDescriptorBindingFlags::HashFlag(const ShaderDescriptorBindingFlags& Flag)
	{
		uint64_t hash = 1469598103934665603ull; // FNV64 Offset Basis

		// ---- Simple booleans ----
		HashCombine(hash, Flag.Set);
		HashCombine(hash, Flag.Binding);

		HashCombine(hash, Flag.EnableBindless);
		HashCombine(hash, Flag.EnableStreaming);
		HashCombine(hash, Flag.EnablePartiallyBound);
		HashCombine(hash, Flag.EnableVariableSize);
		HashCombine(hash, Flag.VariableSize);

		HashCombine(hash, Flag.StageFlags);
		return hash;
	}
}
