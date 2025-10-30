#pragma once

#include "RenderAPI.h"

namespace Chilli
{
	struct RendererInitSpec
	{
		RenderAPIType Type;
		void* GlfwWindow;
		uint16_t InFrameFlightCount = 0;
		bool VSync = false;

		struct {
			int x, y;
		} InitialWindowSize;
		struct {
			int x, y;
		} InitialFrameBufferSize;
		const char* Name;
		bool EnableValidation = false;
	};

	struct RenderCommand
	{
		Object& RenderObject;
	};

	class Renderer
	{
	public:
		static Renderer& Get()
		{
			static Renderer Instance;
			return Instance;
		}

		static bool Init(const RendererInitSpec& Spec);
		static void ShutDown();

		static bool BeginFrame();
		static void BeginScene();
		static void BeginRenderPass();

		static void Submit(const std::shared_ptr<GraphicsPipeline>& Shader, const std::shared_ptr<VertexBuffer>& VB);
		static void Submit(const std::shared_ptr<GraphicsPipeline>& Shader, const std::shared_ptr<VertexBuffer>& VB
			, const std::shared_ptr<IndexBuffer>& IB);

		static void Submit(const std::shared_ptr<GraphicsPipeline>& Shader, const std::shared_ptr<VertexBuffer>& VB
			, const std::shared_ptr<IndexBuffer>& IB, const RenderCommandSpec& CommandSpec);

		static void EndRenderPass();
		static void EndScene();
		static void Render();
		static void Present();
		static void EndFrame();
		static void FinishRendering();

		static RenderAPIType GetType() { return Get()._Api->GetType(); }
		static const char* GetName(){ return Get()._Api->GetName(); };

		static void FrameBufferReSized(int Width, int Height);
		static BindlessSetManager& GetBindlessSetManager();

	private:
		Renderer() {}
		~Renderer() {}

	private:
		RenderAPI* _Api;
	};
}
