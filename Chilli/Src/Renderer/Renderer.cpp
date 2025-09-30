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
	
	void Renderer::BeginFrame()
	{
		Get()._Api->BeginFrame();
	}

	void Renderer::BeginRenderPass()
	{
		Get()._Api->BeginRenderPass();
	}

	void Renderer::Submit(const std::shared_ptr<GraphicsPipeline>& Pipeline)
	{
		Get()._Api->Submit(Pipeline);
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

	void Chilli::Renderer::FinishRendering()
	{
		Get()._Api->FinishRendering();
	}
}
