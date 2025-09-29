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
}
