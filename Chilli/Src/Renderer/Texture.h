#pragma once

#include "Image.h"

namespace Chilli
{
	enum class ImageAspect
	{
		DEPTH,COLOR
	};

	struct TextureSpec : public ImageSpec
	{
		const char* FilePath;
		ImageAspect Aspect;
	};

	class Texture
	{
	public:
		virtual const ImageSpec& GetSpec() const = 0;
		virtual std::shared_ptr<Image>& GetImage() = 0;
	private:
	};
}
