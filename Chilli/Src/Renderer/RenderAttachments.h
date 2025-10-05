#pragma once

#include "Texture.h"

namespace Chilli
{
	enum class DepthAttachmentFormat
	{
		D32_FlOAT
	};

	enum class AttachmentTypes
	{
		COLOR,
		DEPTH,
		DEPTH_STENCIL
	};

	struct DepthAttachment
	{
		std::shared_ptr<Texture> DepthTexture = nullptr;
		struct {
			uint32_t Near;
			float Far;
		} Planes;
		bool UseSwapChainTexture = false;
	};

	struct ColorAttachment
	{
		struct {
			float Near, Far;
		} Planes;
		// On UseSwapChainTexture is true you dont need to provide a image
		bool UseSwapChainTexture = false;
		// This is the texture where it will be written
		std::shared_ptr<Texture> ColorTexture;

		struct {
			float r, g, b, w;
		} ClearColor;
	};

	struct BeginRenderPassInfo
	{
		ColorAttachment* ColorAttachments;
		uint32_t ColorAttachmentCount;

		DepthAttachment DepthAttachment;
	};
}
