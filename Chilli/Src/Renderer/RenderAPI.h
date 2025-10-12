#pragma once

#include "Resources.h"
#include "Material.h"

namespace Chilli
{
	enum class RenderAPIType
	{
		VULKAN1_3
	};

	class RenderAPI
	{
	public:
		virtual void Init(void* Spec) = 0;
		virtual void ShutDown() = 0;

		virtual RenderAPIType GetType() const = 0;
		virtual const char* GetName() const = 0;
	
		virtual ResourceFactory& GetResourceFactory() = 0;

		virtual bool BeginFrame() = 0;
		virtual void BeginRenderPass() = 0;
		virtual void Submit(const std::shared_ptr<GraphicsPipeline>& Pipeline, const std::shared_ptr<VertexBuffer>& VertexBuffer
			, const std::shared_ptr<IndexBuffer>& IndexBuffer) = 0;
		virtual void Submit(const Material& Mat, const std::shared_ptr<VertexBuffer>& VertexBuffer, const std::shared_ptr<IndexBuffer>& IndexBuffer) = 0;
		virtual void EndRenderPass() = 0;		
		virtual void Render() = 0;
		virtual void Present() = 0;
		virtual void EndFrame() = 0;
		virtual void FinishRendering() = 0;

		virtual void FrameBufferReSized(int Width, int Height) = 0;
	private:
	};
}
