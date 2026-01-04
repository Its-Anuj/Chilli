#include "RenderClasses.h"
#include "RenderClasses.h"
#include "RenderClasses.h"
#include "RenderClasses.h"
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
			return new VulkanGraphicsBackend(Spec);
			//return nullptr;
		}
	}

	void GraphicsBackendApi::Terminate(GraphicsBackendApi* Api, bool Delete)
	{
		Api->Terminate();
		if (Delete)
			delete Api;
	}

}
