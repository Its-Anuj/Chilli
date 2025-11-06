#pragma once

#include "Maths.h"

namespace Chilli
{
	enum class RenderPassAttachmentTypes { COLOR, DEPTH };

	struct ColorAttachment
	{
		TextureHandle ColorTexture;
		bool UseSwapChainImage = true;
		Vec4 ClearColor;
	};

	struct DepthAttachment
	{
		TextureHandle DepthTexture;
		Vec2 NearFar;
	};

	struct RenderPassInfo
	{
		ColorAttachment* ColorAttachments = nullptr;
		uint32_t ColorAttachmentCount = 0;
		DepthAttachment* DepthAttachment = nullptr;
	};
}
