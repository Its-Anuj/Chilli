#pragma once

namespace Chilli
{
	struct DepthAttachment
	{
		std::shared_ptr<class Texture> TextureAttachment;
		bool UseSwapChainImage = false;
	};

	struct ColorAttachment
	{
		std::shared_ptr<class Texture> TextureAttachment;
		bool UseSwapChainImage = true;

		struct {
			float r, g, b, w;
		} ClearColor;
	};

	struct RenderPass
	{
		ColorAttachment* ColorAttachments = nullptr;
		uint32_t ColorAttachmentCount = 0;
		DepthAttachment* DepthAttachment = nullptr;
	};
}
