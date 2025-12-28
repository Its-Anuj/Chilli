#pragma once

#include "Image.h"
#include "Sampler.h"

namespace Chilli
{
    enum class ComponentSwizzle {
        IDENTITY = 0,
        R, G, B, A,
        ZERO,
        ONE
    };

    struct ComponentMapping {
        ComponentSwizzle r = ComponentSwizzle::IDENTITY;
        ComponentSwizzle g = ComponentSwizzle::IDENTITY;
        ComponentSwizzle b = ComponentSwizzle::IDENTITY;
        ComponentSwizzle a = ComponentSwizzle::IDENTITY;
    };

    struct TextureSpec {
        std::string FilePath;

        ImageFormat Format = ImageFormat::NONE;

        // --- NEW VIEW CRITERIA ---
        ComponentMapping Swizzle;

        // Allows user to pick a subset of the image (e.g., just Mip 0)
        uint32_t BaseMipLevel = 0;
        uint32_t MipCount = UINT32_MAX; // UINT32_MAX means "all remaining mips"

        uint32_t BaseArrayLayer = 0;
        uint32_t LayerCount = 1;
    };

	struct Texture
	{
		BackBone::AssetHandle<Image>  ImageHandle;
        TextureSpec Spec;
		uint32_t RawTextureHandle = UINT32_MAX;
	};
}

