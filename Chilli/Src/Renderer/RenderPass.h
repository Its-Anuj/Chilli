#pragma once

#include "RenderCore.h"

namespace Chilli
{
	static constexpr uint32_t SWAPCHAIN_IMAGE_HANDLE = UINT32_MAX;

	enum class AttachmentLoadOp { LOAD, CLEAR };
	enum class AttachmentStoreOp { STORE, DONT_CARE };

	struct PipelineBarrier {
		struct {
			uint32_t Handle = UINT32_MAX;
			uint64_t Offset = 0;
			uint64_t Size = UINT64_MAX;
		} BufferBarrier;

		struct {
			uint32_t ImageHandle = UINT32_MAX;
			bool IsSwapChain = false;

			struct {
				uint16_t baseMip = 0;
				uint16_t mipCount = 1;
				uint16_t baseLayer = 0;
				uint16_t layerCount = 1;
			} SubresourceRange;
		} ImageBarrier;

		ResourceState OldState;
		ResourceState NewState;

		RenderStreamTypes OldStream;
		RenderStreamTypes NewStream;
	};

	struct PipelineBarrierCmdPayload {
		uint32_t BarriersCount = 0;
	};

	enum class ResolveMode {
		NONE = 0,
		AVERAGE = 1 << 0, // Standard for Color (averages samples)
		MIN = 1 << 1,     // Keeps the smallest value
		MAX = 1 << 2,     // Keeps the largest value
	};

	struct ColorAttachment {
		uint32_t ColorTexture = UINT32_MAX;
		uint32_t ResolveTexture = UINT32_MAX;  // This is Image B (Sampled/Texture)

		bool UseSwapChainImage = false;

		Vec4 ClearColor = { 0, 0, 0, 1 };
		AttachmentLoadOp LoadOp = AttachmentLoadOp::CLEAR;
		AttachmentStoreOp StoreOp = AttachmentStoreOp::STORE;

		ResolveMode ResolveOp = ResolveMode::AVERAGE;
	};

	struct DepthAttachment {
		uint32_t DepthTexture = UINT32_MAX;
		AttachmentLoadOp LoadOp = AttachmentLoadOp::CLEAR;
		AttachmentStoreOp StoreOp = AttachmentStoreOp::STORE;
		AttachmentLoadOp StencilLoadOp = AttachmentLoadOp::CLEAR;
		AttachmentStoreOp StencilStoreOp = AttachmentStoreOp::STORE;

		float Depth = 1.0f;
		float Stencil = 0.0f;
	};

	struct RenderPassInfo {
		const char* DebugName;

		IVec2 RenderArea;

		uint32_t ColorAttachmentCount = 0;
		ColorAttachment ColorAttachments[CH_MAX_COLOR_ATTACHMENT_COUNT];

		DepthAttachment DepthAttachment;
	};

	struct CompiledPass {
		RenderPassInfo Info;
		std::vector<PipelineBarrier> PrePassBarriers;
		std::vector<PipelineBarrier> PostPassBarriers;
	};

	class RenderPassBuilder {
	public:
		RenderPassBuilder(const char* name) { _info.DebugName = name; }

		RenderPassBuilder& SetArea(int w, int h) { _info.RenderArea = { w, h }; return *this; }

		RenderPassBuilder& AddColor(ColorAttachment col) {
			_info.ColorAttachments[_info.ColorAttachmentCount] = col;
			_info.ColorAttachmentCount++;
			return *this;
		}

		RenderPassBuilder& AddSwapChain(Vec4 clear = { 0,0,0,1 }) {
			ColorAttachment swap; swap.UseSwapChainImage = true;
			swap.ClearColor = clear;
			_info.ColorAttachments[_info.ColorAttachmentCount] = swap;
			_info.ColorAttachmentCount++;
			return *this;
		}

		RenderPassBuilder& SetDepth(uint32_t texHandle, AttachmentLoadOp load = AttachmentLoadOp::CLEAR, AttachmentStoreOp store = AttachmentStoreOp::STORE) {
			_info.DepthAttachment.DepthTexture = texHandle;
			_info.DepthAttachment.LoadOp = load;
			_info.DepthAttachment.StoreOp = store;
			return *this;
		}

		RenderPassBuilder& SetDepth(const DepthAttachment& Depth) {
			_info.DepthAttachment = Depth;
			return *this;
		}

		RenderPassBuilder& AddImageBarrier(bool prePass, uint32_t handle, ResourceState oldState, ResourceState newState, bool isSwapChain = false) {
			PipelineBarrier barrier{};
			barrier.ImageBarrier.ImageHandle = handle;
			barrier.ImageBarrier.IsSwapChain = isSwapChain;
			barrier.ImageBarrier.SubresourceRange = { 0, 1, 0, 1 }; // Default to 1 mip/layer
			barrier.OldState = oldState;
			barrier.NewState = newState;

			if (prePass) _PrePipelineBarriers.push_back(barrier);
			else _PostPipelineBarriers.push_back(barrier);
			return *this;
		}

		// Custom Buffer Barrier
		RenderPassBuilder& AddBufferBarrier(bool prePass, uint32_t handle, ResourceState oldState, ResourceState newState, uint64_t offset = 0, uint64_t size = (size_t)CH_BUFFER_WHOLE_SIZE) {
			PipelineBarrier b{};
			b.BufferBarrier.Handle = handle;
			b.BufferBarrier.Offset = offset;
			b.BufferBarrier.Size = size;
			b.OldState = oldState;
			b.NewState = newState;

			if (prePass) _PrePipelineBarriers.push_back(b);
			else _PostPipelineBarriers.push_back(b);
			return *this;
		}

		// Sugar for the specific methods you asked for
		RenderPassBuilder& AddPrePipelineBarrier(uint32_t handle, ResourceState src, ResourceState dst, bool isBuffer = true, bool isSwapChainImage = false) {
			if (isBuffer) return AddBufferBarrier(true, handle, src, dst);
			return AddImageBarrier(true, handle, src, dst, isSwapChainImage);
		}

		// Sugar for the specific methods you asked for
		RenderPassBuilder& AddPrePipelineBarrier(const PipelineBarrier& Barrier) {
			_PrePipelineBarriers.push_back(Barrier);
			return *this;
		}

		// Sugar for the specific methods you asked for
		RenderPassBuilder& AddPostPipelineBarrier(const PipelineBarrier& Barrier) {
			_PrePipelineBarriers.push_back(Barrier);
			return *this;
		}

		RenderPassBuilder& AddPostPipelineBarrier(uint32_t handle, ResourceState src, ResourceState dst, bool isBuffer = true, bool isSwapChainImage = false) {
			if (isBuffer) return AddBufferBarrier(false, handle, src, dst
			);
			return AddImageBarrier(false, handle, src, dst, isSwapChainImage);
		}

		CompiledPass Build() {
			return { _info, _PrePipelineBarriers, _PostPipelineBarriers };
		}

	private:
		RenderPassInfo _info;
		std::vector<PipelineBarrier> _PrePipelineBarriers;
		std::vector<PipelineBarrier> _PostPipelineBarriers;
	};

	// --- 5. The Compiler (Automation Logic) ---
	class RenderPassCompiler {
	private:

	public:
		int PushPass(const CompiledPass& pass) { _queue.push_back(pass); return _queue.size() - 1; }

		std::vector<CompiledPass> Compile() {
			return _queue;
		}

	private:
		std::vector<CompiledPass> _queue;
	};
}
