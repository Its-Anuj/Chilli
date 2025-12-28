#pragma once

namespace Chilli
{
	static constexpr uint32_t SWAPCHAIN_IMAGE_HANDLE = UINT32_MAX;

	enum class AttachmentLoadOp { LOAD, CLEAR };
	enum class AttachmentStoreOp { STORE, DONT_CARE };
	enum class ResourceState { Undefined, RenderTarget, DepthWrite, ShaderRead, ComputeRead, ComputeWrite, Present };

	struct PipelineBarrier {
		uint32_t RawTextureHandle = UINT32_MAX;
		bool IsSwapchain = false; // Backend: if true, use current swapchain image
		ResourceState OldState;
		ResourceState NewState;
	};

	struct ColorAttachment {
		uint32_t ColorTexture = UINT32_MAX;
		bool UseSwapChainImage = false;
		Vec4 ClearColor = { 0, 0, 0, 1 };
		AttachmentLoadOp LoadOp = AttachmentLoadOp::CLEAR;
		AttachmentStoreOp StoreOp = AttachmentStoreOp::STORE;
	};

	struct DepthAttachment {
		uint32_t DepthTexture = UINT32_MAX;
		AttachmentLoadOp LoadOp = AttachmentLoadOp::CLEAR;
		AttachmentStoreOp StoreOp = AttachmentStoreOp::STORE;
	};

	struct RenderPassInfo {
		const char* DebugName = "UnnamedPass";
		IVec2 RenderArea = { 0, 0 };
		PipelineStateInfo InitialPipelineState;
		std::vector<ColorAttachment> ColorAttachments;
		DepthAttachment DepthAttachment;
	};

	struct CompiledPass {
		RenderPassInfo Info;
		std::vector<PipelineBarrier> PrePassBarriers;
	};

	class RenderPassBuilder {
	public:
		RenderPassBuilder(const char* name) { _info.DebugName = name; }

		RenderPassBuilder& SetArea(int w, int h) { _info.RenderArea = { w, h }; return *this; }

		RenderPassBuilder& AddColor(ColorAttachment col) { _info.ColorAttachments.push_back(col); return *this; }

		RenderPassBuilder& AddSwapChain(Vec4 clear = { 0,0,0,1 }) {
			ColorAttachment swap; swap.UseSwapChainImage = true; 
			swap.ClearColor = clear;
			_info.ColorAttachments.push_back(swap);
			return *this;
		}

		RenderPassBuilder& SetPipeline(const PipelineStateInfo& pipe) { _info.InitialPipelineState = pipe; return *this; }

		RenderPassBuilder& SetDepth(uint32_t texHandle, AttachmentLoadOp load = AttachmentLoadOp::CLEAR, AttachmentStoreOp store = AttachmentStoreOp::STORE) {
			_info.DepthAttachment.DepthTexture = texHandle;
			_info.DepthAttachment.LoadOp = load;
			_info.DepthAttachment.StoreOp = store;
			return *this;
		}

		const CompiledPass& Build() { return { _info, _PipelineBarriers }; }

		RenderPassBuilder& AddPipelineBarrier(uint32_t RawTextureHandle, ResourceState SrcState, ResourceState DstState) {
			_PipelineBarriers.push_back({ RawTextureHandle , false , SrcState, DstState });
		}

		RenderPassBuilder& SetSwapChainPipelineBarrier(ResourceState DstState) {
			_PipelineBarriers.push_back({ UINT32_MAX , false , _PipelineState, DstState });
		}

	private:
		RenderPassInfo _info;
		ResourceState _PipelineState;
		std::vector< PipelineBarrier> _PipelineBarriers;
	};

	// --- 5. The Compiler (Automation Logic) ---
	class RenderPassCompiler {
	private:
		// Internal key used only for tracking the swapchain in the map
		static constexpr uint32_t TRACKER_SWAPCHAIN_KEY = 0xFFFFFFFE;

	public:
		int PushPass(const CompiledPass& pass) { _queue.push_back(pass); return _queue.size() - 1; }

		std::vector<CompiledPass> Compile() {
			return _queue;
		}

	private:
		std::vector<CompiledPass> _queue;
	}
}
