#include "Renderer.h"
#include "Ch_PCH.h"
#include "Window/Window.h"
#include "Renderer.h"

#include "vulkan/vulkan.h"

#include "Vulkan/VulkanRenderer.h"

namespace Chilli
{
	void Renderer::Init(RenderAPITypes Type, const RendererSpec& RenderSpec)
	{
		if (Type == RenderAPITypes::VULKAN1_3)
		{
			Get()._Api = new VulkanRenderer();
			VulkanRendererSpec Spec{};
			Spec.Name = "Chilli";
			Spec.EnableValidationLayer = RenderSpec.EnableValidation;
			Spec.FrameBufferSize.x = RenderSpec.RenderWindow->GetFrameBufferSize().x;
			Spec.FrameBufferSize.y = RenderSpec.RenderWindow->GetFrameBufferSize().y;
			Spec.Win32Surface = RenderSpec.RenderWindow->GetWin32Surface();
			Spec.InFrameFlightCount = RenderSpec.InFrameFlightCount;
			Spec.VSync = RenderSpec.VSync;
			Spec.GLFWWINDOW = RenderSpec.RenderWindow->GetRawHandle();

			Get()._Api->Init(&Spec);
		}
	}

	void Renderer::ShutDown()
	{
		Get()._Api->ShutDown();
		delete Get()._Api;
	}

	std::shared_ptr<ResourceFactory> Renderer::GetResourceFactory()
	{
		return Get()._Api->GetResourceFactory();
	}
	
	bool Renderer::BeginFrame()
	{
		return Get()._Api->BeginFrame();
	}

	bool Renderer::BeginRenderPass(const BeginRenderPassInfo& Info)
	{
		return Get()._Api->BeginRenderPass(Info);
	}

	void Renderer::Submit(const std::shared_ptr<GraphicsPipeline>& Pipeline, const std::shared_ptr<VertexBuffer>& VB
		, const std::shared_ptr<IndexBuffer>& IB)
	{
		Get()._Api->Submit(Pipeline, VB, IB);
	}

	void Renderer::EndRenderPass()
	{
		Get()._Api->EndRenderPass();
	}

	void Renderer::RenderFrame()
	{
		Get()._Api->RenderFrame();
	}

	void Renderer::Present()
	{
		Get()._Api->Present();
	}

	void Renderer::EndFrame()
	{
		Get()._Api->EndFrame();
	}

	void Renderer::FinishRendering()
	{
		Get()._Api->FinishRendering();
	}

	void Renderer::FrameBufferReSized(int Width, int Height)
	{
		Get()._Api->FrameBufferReSized(Width, Height);
	}

	Vec2 Chilli::Renderer::GetFrameBufferSize()
	{
		return Vec2(Get()._Api->GetFrameBufferWidth(), Get()._Api->GetFrameBufferHeight());
	}
}
