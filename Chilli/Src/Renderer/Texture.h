#pragma once

#include "Image.h"
#include "Sampler.h"

namespace Chilli
{
	struct Texture
	{
		ImageSpec ImgSpec;
		std::string FilePath;
		uint32_t RawTextureHandle;
	};
}

