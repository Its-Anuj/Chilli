#pragma once

#include "Maths.h"
#include "RenderAPI.h"

namespace Chilli
{
	struct Window;
	struct RendererSpec
	{
		Window* RenderWindow;
		bool EnableValidation = true;
		uint16_t InFrameFlightCount = 0;
		bool VSync;
	};

	class Renderer
	{
	public:
		static Renderer& Get()
		{
			static Renderer Instance;
			return Instance;
		}

		static void Init(RenderAPITypes Type, const RendererSpec& RenderSpec);
		static void ShutDown();

		static const char* GetName() { return Get()._Api->GetName(); }
		static RenderAPITypes GetType() { return Get()._Api->GetType(); }

		static std::shared_ptr<ResourceFactory> GetResourceFactory();

		// Rendering related
		static bool BeginFrame();
		static bool BeginRenderPass(const BeginRenderPassInfo& Info);
		static void Submit(const std::shared_ptr<GraphicsPipeline>& Pipeline
			, const std::shared_ptr<VertexBuffer>& VB, const std::shared_ptr<IndexBuffer>& IB);
		static void EndRenderPass();
		static void RenderFrame();
		static void Present();
		static void EndFrame();
		static void FinishRendering();

		static void FrameBufferReSized(int Width, int Height);
		static Vec2 GetFrameBufferSize();
	private:
		RenderAPI* _Api;
	};
}
