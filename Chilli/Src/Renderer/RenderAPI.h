#pragma once

#include "ResourceFactory.h"

namespace Chilli
{
	enum class RenderAPITypes
	{
		VULKAN1_3
	};

	class RenderAPI
	{
	public:
		virtual ~RenderAPI(){}

		virtual void Init(void* Spec) = 0;
		virtual void ShutDown() = 0;

		virtual RenderAPITypes GetType() = 0;
		virtual const char* GetName() = 0;

		// Rendering related
		virtual bool BeginFrame() = 0;
		virtual bool BeginRenderPass(const BeginRenderPassInfo& Info) = 0;
		virtual void Submit(const std::shared_ptr<GraphicsPipeline>& Pipeline, const std::shared_ptr<VertexBuffer>& VB
		, const std::shared_ptr<IndexBuffer>& IB) = 0;
		virtual void EndRenderPass() = 0;
		virtual void RenderFrame() = 0;
		virtual void Present() = 0;
		virtual void EndFrame() = 0;
		virtual void FinishRendering() = 0;

		virtual void FrameBufferReSized(int Width, int Height) = 0;

		virtual std::shared_ptr<ResourceFactory> GetResourceFactory() = 0;
		
		virtual float GetFrameBufferWidth() = 0;
		virtual float GetFrameBufferHeight() = 0;
	protected:
	};
}
