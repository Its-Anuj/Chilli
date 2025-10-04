#pragma once

#include "Image.h"

namespace Chilli
{
	struct TextureSpec : public ImageSpec
	{
		const char* FilePath;
	};

	class Texture
	{
	public:
		virtual const ImageSpec& GetSpec() const = 0;
		virtual std::shared_ptr<Image>& GetImage() = 0;
	private:
	};
}
