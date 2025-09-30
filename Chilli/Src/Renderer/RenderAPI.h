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
		virtual void BeginFrame() = 0;
		virtual void BeginRenderPass() = 0;
		virtual void Submit(const std::shared_ptr<GraphicsPipeline>& Pipeline) = 0;
		virtual void EndRenderPass() = 0;
		virtual void RenderFrame() = 0;
		virtual void Present() = 0;
		virtual void EndFrame() = 0;
		virtual void FinishRendering() = 0;

		virtual std::shared_ptr<ResourceFactory> GetResourceFactory() = 0;
	protected:
	};
}
