#include "Ch_PCH.h"
#include "Renderer.h"

#include "Vulkan/VulkanRenderer.h"

namespace Chilli
{
	void Renderer::Init(RenderAPITypes Type)
	{
		if (Type == RenderAPITypes::VULKAN1_3)
		{
			Get()._Api = new VulkanRenderer();
		}
	}

	void Renderer::ShutDown()
	{
		delete Get()._Api;
	}
}
