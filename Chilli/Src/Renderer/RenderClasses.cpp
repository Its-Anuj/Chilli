#include "Ch_PCH.h"
#include "RenderClasses.h"

#include "vulkan\vulkan.h"
#include "vk_mem_alloc.h"
#include "Vulkan/VulkanBackend.h"

namespace Chilli
{
	GraphicsBackendApi* GraphicsBackendApi::Create(const GraphcisBackendCreateSpec& Spec)
	{
		if (Spec.Type == GraphicsBackendType::VULKAN_1_3)
		{
			return new VulkanGraphicsBackendApi(Spec);
			//return nullptr;
		}
	}
	
	void GraphicsBackendApi::Terminate(GraphicsBackendApi* Api, bool Delete)
	{
		Api->Terminate();
		if (Delete)
			delete Api;
	}

	void Renderer::Init(const GraphcisBackendCreateSpec& Spec)
	{
		_Api = std::shared_ptr<GraphicsBackendApi>(GraphicsBackendApi::Create(Spec));
	}

	void Renderer::Terminate()
	{
		GraphicsBackendApi::Terminate(_Api.get(), false);
	}

	std::shared_ptr<RenderCommand> Renderer::CreateRenderCommand()
	{
		return std::make_shared<RenderCommand>(_Api);
	}
}
