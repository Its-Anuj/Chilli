#pragma once

#include "RenderCore.h"

namespace Chilli
{
	enum class ImageType
	{
		IMAGE_TYPE_1D,
		IMAGE_TYPE_2D,
		IMAGE_TYPE_3D,
	};

	enum class ImageFormat
	{
		NONE,

		RGBA8,
		SRGBA8,
		R16F,
		RG16F,
		RGBA16F,
		RGBA32F,

		BGRA8,

		S8I,
		D32F,
		D32F_S8I
	};

	enum ImageUsage : uint32_t {
		IMAGE_USAGE_NONE = 0,
		IMAGE_USAGE_SAMPLED_IMAGE = 1 << 0,
		IMAGE_USAGE_COLOR_ATTACHMENT = 1 << 1,
		IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT = 1 << 2,
		IMAGE_USAGE_STORAGE_IMAGE = 1 << 3,
		IMAGE_USAGE_TRANSFER_SRC = 1 << 4,
		IMAGE_USAGE_TRANSFER_DST = 1 << 5,
		IMAGE_USAGE_INPUT_ATTACHMENT = 1 << 6,
		IMAGE_USAGE_PRESENT_IMAGE = 1 << 7
	};

	struct ImageSpec
	{
		struct {
			int Width, Height, Depth;
		} Resolution;
		ImageFormat Format = ImageFormat::RGBA8;
		ImageType Type = ImageType::IMAGE_TYPE_2D;
		void* ImageData = nullptr;
		bool YFlip = false;
		uint32_t Usage = ImageUsage::IMAGE_USAGE_SAMPLED_IMAGE;
		uint32_t MipLevel = 1;
		ResourceState State = ResourceState::Undefined;
	};

	struct Image
	{
		uint32_t RawImageHandle = UINT32_MAX;
		ImageSpec Spec;
	};

	inline bool ValidateImageState(
		uint32_t usage,
		ResourceState state
	)
	{
		switch (state)
		{
		case ResourceState::Undefined:
			return true;

		case ResourceState::ShaderRead:
			// Sampled in graphics / fragment
			return (usage & IMAGE_USAGE_SAMPLED_IMAGE) != 0;

		case ResourceState::RenderTarget:
			return (usage & IMAGE_USAGE_COLOR_ATTACHMENT) != 0;

		case ResourceState::DepthWrite:
			return (usage & IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT) != 0;

		case ResourceState::ComputeRead:
		case ResourceState::ComputeWrite:
			// Compute requires storage image
			return (usage & IMAGE_USAGE_STORAGE_IMAGE) != 0;

		case ResourceState::CopySrc:
			return (usage & IMAGE_USAGE_TRANSFER_SRC) != 0;

		case ResourceState::CopyDst:
			return (usage & IMAGE_USAGE_TRANSFER_DST) != 0;

		case ResourceState::Present:
			return (usage & IMAGE_USAGE_PRESENT_IMAGE) != 0;

		default:
			return false;
		}
	}

}
