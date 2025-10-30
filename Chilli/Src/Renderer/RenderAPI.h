#pragma once

#include "Shader.h"
#include "Buffer.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"
#include "Object.h"

namespace Chilli
{
	enum class RenderAPIType
	{
		VULKAN1_3
	};

	struct RenderCommandSpec
	{
		UUID ObjectIndex;
		UUID MaterialIndex;
	};

	class RenderAPI
	{
	public:
		virtual void Init(void* Spec) = 0;
		virtual void ShutDown() = 0;

		virtual RenderAPIType GetType() const = 0;
		virtual const char* GetName() const = 0;
	
		virtual bool BeginFrame() = 0;
		virtual void BeginScene() = 0;
		virtual void BeginRenderPass() = 0;
		virtual void Submit(const std::shared_ptr<GraphicsPipeline>& Shader, const std::shared_ptr<VertexBuffer>& VB) = 0;
		virtual void Submit(const std::shared_ptr<GraphicsPipeline>& Shader, const std::shared_ptr<VertexBuffer>& VB
			, const std::shared_ptr<IndexBuffer>& IB) = 0;
		virtual void Submit(const std::shared_ptr<GraphicsPipeline>& Shader, const std::shared_ptr<VertexBuffer>& VB
			, const std::shared_ptr<IndexBuffer>& IB, const RenderCommandSpec& CommandSpec) = 0;

		virtual void EndRenderPass() = 0;		
		virtual void EndScene() = 0;
		virtual void Render() = 0;
		virtual void Present() = 0;
		virtual void EndFrame() = 0;
		virtual void FinishRendering() = 0;

		virtual BindlessSetManager& GetBindlessSetManager() = 0;

		virtual void FrameBufferReSized(int Width, int Height) = 0;
	private:
	};
}
