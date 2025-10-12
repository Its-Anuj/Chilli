#pragma once

#include "Image.h"

namespace Chilli
{
	struct TextureSpec 
	{
		const char* FilePath;
		ImageTiling	 Tiling;
		ImageType Type;
	};

	class Texture
	{
	public:
		virtual const TextureSpec& GetSpec() const = 0;

	private:
	};
}
