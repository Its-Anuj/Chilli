#pragma once

#include "RenderCore.h"
#include "Pipeline.h"
#include "RenderPass.h"

namespace Chilli
{
	struct RenderCommandHeader
	{
		RenderOpCode Code;
		uint16_t Size;
	};

	struct CopyBufferCmdPayload
	{
		uint32_t SrcBuffer;
		uint32_t DstBuffer;

		uint32_t Size = 0;
		uint32_t SrcOffset = 0;
		uint32_t DstOffset = 0;
	};

	struct MaterialDataUpdateCmdPayload
	{
		uint32_t MaterialHandle = UINT32_MAX;
		char Name[SHADER_UNIFORM_BINDING_NAME_SIZE] = { 0 };
		uint32_t DstArrayIndex = UINT32_MAX;

		bool Persistent = false;

		struct {
			uint32_t Handle = UINT32_MAX;
			uint32_t Offset = 0;
			uint32_t Range = 0;
		} BufferInfo;

		struct {
			uint32_t Handle = UINT32_MAX;
			uint32_t Sampler = UINT32_MAX;
			ResourceState State = ResourceState::ShaderRead;
		} ImageInfo;
	};

	struct BindMaterialDataCmdPayload
	{
		uint32_t RawMaterialHandle = UINT32_MAX;
	};

	struct DrawPushShaderInlineUniformData
	{
		uint32_t ObjectIndex;
		uint32_t MaterialIndex;
	};

	struct PushShaderInlineUniformDataCmdPayload
	{
		uint32_t ShaderProgram = UINT32_MAX;
		uint32_t Stage = 0;
		uint32_t Offset = 0;
		uint32_t Size = 0;

		// Header -> Payload -> bytes of data of Size
	};

	struct DispatchCmdPayload
	{
		uint32_t GroupCountX = 0;
		uint32_t GroupCountY = 0;
		uint32_t GroupCountZ = 0;
	};

	struct PushUpdateGlobalShaderDataCmdPayload
	{
		GlobalShaderData Data;
	};

	struct PushUpdateSceneShaderDataCmdPayload
	{
		SceneShaderData Data;
	};

	struct RenderCommandBuffer
	{
	protected:
		std::vector<uint8_t> _Stream;

	public:
		template<typename T>
		void PushCommand(RenderOpCode op, const T& payload) {
			RenderCommandHeader header{ op, (uint16_t)sizeof(T) };
			size_t current_size = _Stream.size();
			_Stream.resize(current_size + sizeof(header) + sizeof(T));

			memcpy(&_Stream[current_size], &header, sizeof(header));
			memcpy(&_Stream[current_size + sizeof(header)], &payload, sizeof(T));
		}

		void PushCopyBuffer(uint32_t Src, uint32_t Dst, uint32_t Size, uint32_t SrcOffset = 0,
			uint32_t DstOffset = 0) {
			CopyBufferCmdPayload Payload{ Src, Dst, Size, SrcOffset, DstOffset };
			PushCommand<CopyBufferCmdPayload>(RenderOpCode::COPY_BUFFER, Payload);
		}

		void PushStreamOwnerShipTransfer(uint32_t Handle, RenderStreamTypes Src, RenderStreamTypes Dst,
			ResourceState Next_State, bool Is_Image = false)
		{

		}

		void PushInlineUniformData(uint32_t ShaderProgram, uint32_t Stage, void* Data, uint32_t Size, uint32_t Offset)
		{
			size_t current_size = _Stream.size();
			const uint16_t payload_size = sizeof(PushShaderInlineUniformDataCmdPayload) + (Size);

			// Write Header
			RenderCommandHeader header{ RenderOpCode::PUSH_SHADER_INLINE_UNIFORM_DATA, payload_size };
			_Stream.resize(current_size + sizeof(header) + payload_size);

			uint8_t* Dst = _Stream.data() + current_size;

			memcpy(Dst, &header, sizeof(header));

			Dst += sizeof(header);

			PushShaderInlineUniformDataCmdPayload Payload;
			Payload.ShaderProgram = ShaderProgram;
			Payload.Offset = Offset;
			Payload.Size = Size;
			Payload.Stage = Stage;

			memcpy(Dst, &Payload, sizeof(Payload));

			Dst += sizeof(Payload);

			memcpy(Dst, Data, Size);
		}

		void UpdateMaterialBufferData(uint32_t MaterialHandle, uint32_t Buffer,
			const char* Name, size_t Size, size_t Offset)
		{
			MaterialDataUpdateCmdPayload Info{};
			Info.MaterialHandle = MaterialHandle;
			Info.DstArrayIndex = 0;
			strncpy(Info.Name, Name, SHADER_UNIFORM_BINDING_NAME_SIZE);
			Info.Persistent = false;
			Info.BufferInfo.Handle = Buffer;
			Info.BufferInfo.Range = Size;
			Info.BufferInfo.Offset = Offset;

			PushCommand<MaterialDataUpdateCmdPayload>(RenderOpCode::UPDATE_MATERIL_DATA, Info);
		}

		void UpdateMaterialTextureData(uint32_t MaterialHandle,
			uint32_t Tex, const char* Name, ResourceState State)
		{
			MaterialDataUpdateCmdPayload Info{};
			Info.MaterialHandle = MaterialHandle;
			Info.DstArrayIndex = 0;
			strncpy(Info.Name, Name, SHADER_UNIFORM_BINDING_NAME_SIZE);
			Info.Persistent = false;
			Info.ImageInfo.Handle = Tex;
			Info.ImageInfo.State = State;

			PushCommand<MaterialDataUpdateCmdPayload>(RenderOpCode::UPDATE_MATERIL_DATA, Info);
		}

		void UpdateMaterialSamplerData(uint32_t MaterialHandle, uint32_t Sampler, const char* Name)
		{
			MaterialDataUpdateCmdPayload Info{};
			Info.MaterialHandle = MaterialHandle;
			Info.DstArrayIndex = 0;
			strncpy(Info.Name, Name, SHADER_UNIFORM_BINDING_NAME_SIZE);
			Info.Persistent = false;
			Info.ImageInfo.Sampler = Sampler;

			PushCommand<MaterialDataUpdateCmdPayload>(RenderOpCode::UPDATE_MATERIL_DATA, Info);
		}

		void BindMaterailData(uint32_t MaterialHandle)
		{
			PushCommand<BindMaterialDataCmdPayload>(RenderOpCode::BIND_MATERIAL_DATA, { MaterialHandle });
		}

		void PushCommandBuffer(const RenderCommandBuffer& other) {
			if (other._Stream.empty()) return;

			// This is a high-speed bulk memory copy
			size_t current_size = _Stream.size();
			_Stream.resize(current_size + other._Stream.size());
			memcpy(&_Stream[current_size], other._Stream.data(), other._Stream.size());
		}

		void PushImagePipelineBarrier(uint32_t TextureHandle, bool IsSwapChain, ResourceState SrcState,
			ResourceState DstState, uint32_t BaseMip = 0, uint32_t MipCount = 1, uint32_t BaseLayer = 0,
			uint32_t LayerCount = 1)
		{
			PipelineBarrier payload;

			// Image barrier info
			payload.ImageBarrier.ImageHandle = TextureHandle;
			payload.ImageBarrier.IsSwapChain = IsSwapChain;

			payload.ImageBarrier.SubresourceRange.baseMip = static_cast<uint16_t>(BaseMip);
			payload.ImageBarrier.SubresourceRange.mipCount = static_cast<uint16_t>(MipCount);
			payload.ImageBarrier.SubresourceRange.baseLayer = static_cast<uint16_t>(BaseLayer);
			payload.ImageBarrier.SubresourceRange.layerCount = static_cast<uint16_t>(LayerCount);

			// States
			payload.OldState = SrcState;
			payload.NewState = DstState;

			PushPipelineBarriers({ payload });
		}

		void PushBufferPipelineBarrier(
			uint32_t BufferHandle,
			ResourceState SrcState,
			ResourceState DstState,
			uint64_t Offset = 0,
			uint64_t Size = UINT64_MAX)
		{
			PipelineBarrier payload{};

			// Buffer barrier info
			payload.BufferBarrier.Handle = BufferHandle;
			payload.BufferBarrier.Offset = Offset;
			payload.BufferBarrier.Size = Size;

			// States
			payload.OldState = SrcState;
			payload.NewState = DstState;

			// Push command
			PushPipelineBarriers({ payload });
		}

		void PushPipelineBarriers(const std::vector<PipelineBarrier>& Barriers)
		{
			const uint8_t BarrierCount = Barriers.size();
			size_t current_size = _Stream.size();
			const uint16_t payload_size = sizeof(PipelineBarrierCmdPayload) + (sizeof(PipelineBarrier) * BarrierCount);

			// Write Header
			RenderCommandHeader header{ RenderOpCode::PIPELINE_BARRIER, payload_size };
			_Stream.resize(current_size + sizeof(header) + payload_size);

			uint8_t* Dst = _Stream.data() + current_size;

			memcpy(Dst, &header, sizeof(header));

			Dst += sizeof(header);

			PipelineBarrierCmdPayload BarrierPayload;
			BarrierPayload.BarriersCount = BarrierCount;
			memcpy(Dst, &BarrierPayload, sizeof(BarrierPayload));

			Dst += sizeof(BarrierPayload);

			// First Copy Data then set the inital point in BarrierPayload
			memcpy(Dst, Barriers.data(), sizeof(PipelineBarrier) * BarrierCount);
		}

		void Clear() { _Stream.clear(); }
		const uint8_t* Data() const { return _Stream.data(); }
		size_t Size() const { return _Stream.size(); }
	};

	struct BeginFrameCmdPayload
	{
		uint32_t FrameIndex;
	};

	struct BeginRenderPassCmdPayload
	{
		RenderPassInfo Pass;
	};

	struct EndRenderPassCmdPayload
	{

	};

	struct EndFrameCmdPayload
	{

	};

	struct BindShaderProgramCmdPayload
	{
		uint32_t ShaderProgram = UINT32_MAX;
	};

	struct BindVertexBuffersCmdPayload
	{
		uint32_t VertexBufferCount = 0;
		// Containts N number of uint32_t which each are a handle
	};

	struct BindIndexBufferCmdPayload
	{
		uint32_t IndexBufferHandle = UINT32_MAX;
		IndexBufferType Type;
	};

	struct DrawIndexedCmdPayload {
		uint32_t ElementCount;    // Number of indices to draw
		uint32_t InstanceCount; // Number of instances to draw (1 if not instancing)
		uint32_t FirstElement;    // First index in the index buffer
		int32_t  VertexOffset;  // Value added to index before indexing vertex buffer
		uint32_t FirstInstance; // ID of the first instance to draw
	};

	struct SetFullPipelineStateCmdPayload
	{
		PipelineStateInfo Info;
	};

	struct SetVertexLayoutBindingCmdPayload
	{
		uint32_t BindingIndex = 0;
		uint32_t Stride = 0;
		uint32_t AttribsCount = 0;
	};

	struct SetVertexLayoutCmdPayload
	{
		uint32_t BindingCount = 0;
		uint32_t AttribsCount = 0;

		// Memory Layout Payload -> Bindings Data -> Attrib Data
	};

	struct SetDynamicPipleineStateCmdPayload
	{
		inline static float UNCHANGE_FLOAT_VALUE = -1.0f;

		InputTopologyMode TopologyMode = InputTopologyMode::None;

		CullMode ShaderCullMode = CullMode::None;
		PolygonMode ShaderFillMode = PolygonMode::None;
		FrontFaceMode FrontFace = FrontFaceMode::None;
		float LineWidth = UNCHANGE_FLOAT_VALUE;
		ChBool8 RasterizerDiscardEnable = CH_NONE;

		// Depth Bias (dynamic in most modern APIs)
		ChBool8 DepthBiasEnable = CH_NONE;
		float DepthBiasConstantFactor = UNCHANGE_FLOAT_VALUE;
		float DepthBiasClamp = UNCHANGE_FLOAT_VALUE;
		float DepthBiasSlopeFactor = UNCHANGE_FLOAT_VALUE;
	};

	class GraphicsCommandBuffer : public RenderCommandBuffer
	{
	public:
		GraphicsCommandBuffer() {}
		~GraphicsCommandBuffer() {}

		void BeginFrame(uint32_t FrameIndex)
		{
			PushCommand<BeginFrameCmdPayload>(RenderOpCode::BEGIN_FRAME, { FrameIndex });
		}

		void BeginRenderPass(const RenderPassInfo& Pass)
		{
			PushCommand<BeginRenderPassCmdPayload>(RenderOpCode::BEGIN_RENDER_PASS, { Pass });
		}

		void EndRenderPass()
		{
			PushCommand<EndRenderPassCmdPayload>(RenderOpCode::END_RENDER_PASS, { });
		}

		void EndFrame()
		{
			PushCommand<EndFrameCmdPayload>(RenderOpCode::END_FRAME, EndFrameCmdPayload());
		}

		void SetFullPipelineState(const PipelineStateInfo& Info)
		{
			PushCommand<SetFullPipelineStateCmdPayload>(RenderOpCode::SET_FULL_PIPELINE_STATE, { Info });
		}

		void BindShaderPrgoram(uint32_t Program)
		{
			PushCommand<BindShaderProgramCmdPayload>(RenderOpCode::BIND_SHADER_PROGRAM, { Program });
		}

		void SetVertexInputLayout(const VertexInputShaderLayout& Info)
		{
			/*
				Memory Layout->
				[HEADER]
				[PAYLOAD]
				[BINDING DATA] // First will be of index 0
				[ATTRIB DATA] // All aatribs associated with the previous binding data
				[BINDING DATA] // First will be of index 1
				[ATTRIB DATA] // All aatribs associated with the previous binding data
				---repeat---
			*/

			const uint8_t BindingCount = Info.Bindings.size();

			auto LayoutBindingSize = 0;
			uint32_t AttribsCount = 0;
			for (auto& Binding : Info.Bindings) {
				LayoutBindingSize += Binding.Attribs.size() * sizeof(VertexInputShaderAttribute) + sizeof(SetVertexLayoutBindingCmdPayload);
				AttribsCount += Binding.Attribs.size();
			}

			size_t current_size = _Stream.size();
			const uint16_t payload_size = sizeof(SetVertexLayoutCmdPayload) + LayoutBindingSize;

			// Write Header
			RenderCommandHeader header{ RenderOpCode::SET_VERTEX_LAYOUT, payload_size };
			_Stream.resize(current_size + sizeof(header) + payload_size);

			uint8_t* Dst = _Stream.data() + current_size;

			memcpy(Dst, &header, sizeof(header));

			Dst += sizeof(header);

			SetVertexLayoutCmdPayload Payload;
			Payload.AttribsCount = AttribsCount;
			Payload.BindingCount = Info.Bindings.size();

			memcpy(Dst, &Payload, sizeof(Payload));

			Dst += sizeof(Payload);

			for (auto& Binding : Info.Bindings)
			{
				SetVertexLayoutBindingCmdPayload BindingInfo;
				BindingInfo.BindingIndex = Binding.BindingIndex;
				BindingInfo.Stride = Binding.Stride;
				BindingInfo.AttribsCount = Binding.Attribs.size();

				memcpy(Dst, &BindingInfo, sizeof(BindingInfo));

				Dst += sizeof(BindingInfo);

				for (auto& AttribData : Binding.Attribs)
				{
					memcpy(Dst, &AttribData, sizeof(AttribData));
					Dst += sizeof(AttribData);
				}
			}
		}

		void SetDynamicPipelineState(const SetDynamicPipleineStateCmdPayload& Payload)
		{
			PushCommand(RenderOpCode::PATCH_DYNAMIC_STATE, Payload);
		}

		void SetTopologyMode(InputTopologyMode Mode)
		{
			SetDynamicPipleineStateCmdPayload Payload;
			Payload.TopologyMode = Mode;

			PushCommand(RenderOpCode::PATCH_DYNAMIC_STATE, Payload);
		}

		void SetCullMode(CullMode Mode)
		{
			SetDynamicPipleineStateCmdPayload Payload{};
			Payload.ShaderCullMode = Mode;

			PushCommand(RenderOpCode::PATCH_DYNAMIC_STATE, Payload);
		}

		void SetFillMode(PolygonMode Mode)
		{
			SetDynamicPipleineStateCmdPayload Payload{};
			Payload.ShaderFillMode = Mode;

			PushCommand(RenderOpCode::PATCH_DYNAMIC_STATE, Payload);
		}

		void SetFrontFace(FrontFaceMode Mode)
		{
			SetDynamicPipleineStateCmdPayload Payload{};
			Payload.FrontFace = Mode;

			PushCommand(RenderOpCode::PATCH_DYNAMIC_STATE, Payload);
		}

		void SetLineWidth(float Width)
		{
			SetDynamicPipleineStateCmdPayload Payload{};
			Payload.LineWidth = Width;

			PushCommand(RenderOpCode::PATCH_DYNAMIC_STATE, Payload);
		}

		void SetRasterizerDiscard(bool Enable)
		{
			SetDynamicPipleineStateCmdPayload Payload{};
			Payload.RasterizerDiscardEnable = Enable ? CH_TRUE : CH_FALSE;

			PushCommand(RenderOpCode::PATCH_DYNAMIC_STATE, Payload);
		}

		void SetDepthBiasEnable(bool Enable)
		{
			SetDynamicPipleineStateCmdPayload Payload{};
			Payload.DepthBiasEnable = Enable ? CH_TRUE : CH_FALSE;

			PushCommand(RenderOpCode::PATCH_DYNAMIC_STATE, Payload);
		}

		void SetDepthBias(float ConstantFactor, float Clamp, float SlopeFactor)
		{
			SetDynamicPipleineStateCmdPayload Payload{};
			Payload.DepthBiasEnable = CH_TRUE;
			Payload.DepthBiasConstantFactor = ConstantFactor;
			Payload.DepthBiasClamp = Clamp;
			Payload.DepthBiasSlopeFactor = SlopeFactor;

			PushCommand(RenderOpCode::PATCH_DYNAMIC_STATE, Payload);
		}

		void DisableDepthBias()
		{
			SetDynamicPipleineStateCmdPayload Payload{};
			Payload.DepthBiasEnable = CH_FALSE;

			PushCommand(RenderOpCode::PATCH_DYNAMIC_STATE, Payload);
		}

		void BindVertexBuffer(const std::vector<uint32_t>& Buffers)
		{
			const uint8_t Count = Buffers.size();
			size_t current_size = _Stream.size();
			const uint16_t payload_size = sizeof(BindVertexBuffersCmdPayload) + (sizeof(uint32_t) * Count);

			// Write Header
			RenderCommandHeader header{ RenderOpCode::BIND_VERTEX_BUFFERS, payload_size };
			_Stream.resize(current_size + sizeof(header) + payload_size);

			uint8_t* Dst = _Stream.data() + current_size;

			memcpy(Dst, &header, sizeof(header));

			Dst += sizeof(header);

			BindVertexBuffersCmdPayload BufferPayload;
			BufferPayload.VertexBufferCount = Buffers.size();
			memcpy(Dst, &BufferPayload, sizeof(BufferPayload));

			Dst += sizeof(BufferPayload);

			// First Copy Data then set the inital point in BarrierPayload
			memcpy(Dst, Buffers.data(), sizeof(uint32_t) * Count);
		}

		void BindIndexBuffer(uint32_t Handle, IndexBufferType Type)
		{
			PushCommand<BindIndexBufferCmdPayload>(RenderOpCode::BIND_INDEX_BUFFER, { Handle, Type });
		}

		void DrawIndexed(uint32_t ElementCount, uint32_t InstanceCount, uint32_t FirstElement, uint32_t VertexOffset, uint32_t FirstInstance)
		{
			DrawIndexedCmdPayload Payload;
			Payload.ElementCount = ElementCount;
			Payload.FirstElement = FirstElement;
			Payload.InstanceCount = InstanceCount;
			Payload.VertexOffset = VertexOffset;
			Payload.FirstInstance = FirstInstance;
			PushCommand<DrawIndexedCmdPayload>(RenderOpCode::DRAW_INDEXED, Payload);
		}

		void Dispatch(uint32_t GroupCountX, uint32_t GroupCountY, uint32_t GroupCountZ)
		{
			PushCommand<DispatchCmdPayload>(RenderOpCode::DISPATCH, { GroupCountX, GroupCountY, GroupCountZ });
		}

		void PushUpdateGlobalShaderData(const GlobalShaderData& Data)
		{
			PushCommand<PushUpdateGlobalShaderDataCmdPayload>(RenderOpCode::UPDATE_GLOBAL_SHADER_DATA,
				{ Data });
		}

		void PushUpdateSceneShaderData(const SceneShaderData& Data)
		{
			PushCommand<PushUpdateSceneShaderDataCmdPayload>(RenderOpCode::UPDATE_SCENE_SHADER_DATA,
				{ Data });
		}

	private:
	};

	class ComputeCommandBuffer : public RenderCommandBuffer
	{
	public:
		ComputeCommandBuffer() {}
		~ComputeCommandBuffer() {}

		void BindShaderPrgoram(uint32_t Program)
		{
			PushCommand<BindShaderProgramCmdPayload>(RenderOpCode::BIND_SHADER_PROGRAM, { Program });
		}

		void Dispatch(uint32_t GroupCountX, uint32_t GroupCountY, uint32_t GroupCountZ)
		{
			PushCommand<DispatchCmdPayload>(RenderOpCode::DISPATCH, { GroupCountX, GroupCountY, GroupCountZ });
		}

	};
}
