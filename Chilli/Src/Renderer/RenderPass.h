#pragma once
#include "RenderCore.h"

namespace Chilli
{
	enum class AttachmentLoadOp { LOAD, CLEAR };
	enum class AttachmentStoreOp { STORE, DONT_CARE };

	enum class PipelineStage : uint32_t {
		TOP_OF_PIPE = 1 << 0,
		VERTEX_SHADER = 1 << 1,
		FRAGMENT_SHADER = 1 << 2,
		COLOR_ATTACHMENT_OUTPUT = 1 << 3,
		DEPTH_STENCIL_OUTPUT = 1 << 4,
		COMPUTE_SHADER = 1 << 5,
		TRANSFER = 1 << 6,
		BOTTOM_OF_PIPE = 1 << 7,
		ALL_GRAPHICS = 1 << 8,
	};

	enum class AccessType : uint32_t {
		NONE = 0,
		COLOR_ATTACHMENT_WRITE = 1 << 0,
		COLOR_ATTACHMENT_READ = 1 << 1,
		DEPTH_STENCIL_WRITE = 1 << 2,
		DEPTH_STENCIL_READ = 1 << 3,
		SHADER_READ = 1 << 4,
		SHADER_WRITE = 1 << 5,
		TRANSFER_READ = 1 << 6,
		TRANSFER_WRITE = 1 << 7,
	};

	struct PipelineBarrier {
		// Shared synchronization data
		PipelineStage SrcStage;  // Wait for these stages to finish
		PipelineStage DstStage;  // Block these stages until transition is done
		AccessType    SrcAccess; // Memory access to flush
		AccessType    DstAccess; // Memory access to make visible

		ResourceState OldState;
		ResourceState NewState;
		RenderStreamTypes OldStream;
		RenderStreamTypes NewStream;

		// Resource-specific data
		struct BufferBarrierInfo {
			uint32_t Handle = UINT32_MAX;
			uint64_t Offset = 0;
			uint64_t Size = UINT64_MAX; // Use UINT64_MAX for "whole buffer"
		} Buffer;

		struct ImageBarrierInfo {
			uint32_t Handle = UINT32_MAX;
			bool IsSwapChain = false;

			struct {
				uint16_t BaseMip = 0;
				uint16_t MipCount = 1;
				uint16_t BaseLayer = 0;
				uint16_t LayerCount = 1;
			} SubresourceRange;
		} Image;

		// Helper to determine barrier type at runtime
		bool IsImageBarrier() const { return Image.Handle != UINT32_MAX || Image.IsSwapChain;
	}
		bool IsBufferBarrier() const { return Buffer.Handle != UINT32_MAX; }
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

	enum class RenderPassStage : uint8_t
	{
		BeforeDepthPrepass = 0,
		AfterDepthPrepass = 1,
		BeforeOpaque = 2,
		AfterOpaque = 3,
		BeforeSky = 4,
		AfterSky = 5,
		BeforeTransparent = 6,
		AfterTransparent = 7,
		BeforePostProcess = 8,
		AfterPostProcess = 9,
		BeforeUI = 10,
		AfterUI = 11,
		BeforePresent = 12,  // ADD — last chance before swapchain present
		Compute = 13,  // ADD — async compute, not tied to graphics timeline
	};

	struct ColorAttachment
	{
		uint32_t ColorTexture = UINT32_MAX;  // your existing handle
		uint32_t ResolveTexture = UINT32_MAX;

		AttachmentLoadOp             LoadOp = AttachmentLoadOp::CLEAR;
		AttachmentStoreOp            StoreOp = AttachmentStoreOp::STORE;

		ResolveMode ResolveOp = ResolveMode::AVERAGE;

		ResourceState                InitialState = ResourceState::Undefined;
		ResourceState                FinalState = ResourceState::ShaderRead;

		ResourceState                ResolveInitialState = ResourceState::Undefined;
		ResourceState                ResolveFinalState = ResourceState::ShaderRead;

		// Clear values — used when LoadOp == Clear
		Vec4 ClearColor = { 0,0,0, 1 };
		bool UseSwapChainImage = false;
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

	struct BufferDependency {
		uint32_t Handle = UINT32_MAX;
		ResourceState RequiredState = ResourceState::Undefined;
	};

#define CHILLI_MAX_COLOR_ATTACHMENT 8
#define CHILLI_MAX_INPUT_ATTACHMENT 8
#define CHILLI_MAX_PASS_BARRIERS 16
#define CHILLI_MAX_BUFFERS_DEPENDENCIES 16

	struct RenderPassDesc
	{
		std::string     Name;
		RenderPassStage Stage;

		std::array<ColorAttachment , CHILLI_MAX_COLOR_ATTACHMENT> ColorAttachments;
		uint32_t ColorAttachmentCount = 0;
		bool             HasDepthStencil = false;
		DepthAttachment DepthStencil;

		// Use a fixed array and a counter instead of vector
		std::array<uint32_t, CHILLI_MAX_INPUT_ATTACHMENT > InputAttachments;
		uint32_t InputAttachmentCount = 0;

		// Fixed-size barrier storage
		std::array<PipelineBarrier, CHILLI_MAX_PASS_BARRIERS> PrePassBarriers;
		uint32_t PrePassBarrierCount = 0;

		std::array<PipelineBarrier, CHILLI_MAX_PASS_BARRIERS> PostPassBarriers;
		uint32_t PostPassBarrierCount = 0;

		std::array<BufferDependency, CHILLI_MAX_BUFFERS_DEPENDENCIES> BufferDependencies;
		uint32_t BufferDependencyCount = 0;

		uint32_t    SubpassIndex = UINT32_MAX;
		SampleCount SampleCount = IMAGE_SAMPLE_COUNT_1_BIT;
		IVec2 RenderArea{ 0,0 };
	};
}