#pragma once

#include "Image.h"

namespace Chilli
{
	enum class SamplerMode
	{
		REPEAT,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER
	};

	enum class SamplerFilter
	{
		NEAREST,
		LINEAR
	};

	struct TextureSpec
	{
		const char* FilePath;
		ImageTiling	 Tiling;
		ImageType Type;
		SamplerMode Mode = SamplerMode::REPEAT;
		SamplerFilter Filter = SamplerFilter::LINEAR;
		ImageUsage Usage = ImageUsage::COLOR;
		ImageFormat Format = ImageFormat::RGBA8;
		bool YFlip = true;
		struct {
			int Width, Height;
		}Resolution;
	};

	class Texture
	{
	public:
		virtual const TextureSpec& GetSpec() const = 0;

	private:
	};
}
