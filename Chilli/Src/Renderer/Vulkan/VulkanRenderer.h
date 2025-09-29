#pragma once

#include "RenderAPI.h"

namespace Chilli
{
	class VulkanRenderer : public RenderAPI
	{
	public:
		VulkanRenderer() {}
		~VulkanRenderer() {}

		inline RenderAPITypes GetType() override {
			return  RenderAPITypes::VULKAN1_3;
		}

		inline const char* GetName() override {
			return "VULKAN_1_3";
		}
	private:
	};
}
