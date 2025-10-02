#pragma once

namespace Chilli
{
	enum class ImageFormat
	{
		RGBA8
	};

	enum class ImageLayout
	{
		COLOR,
		DEPTH
	};

	enum class ImageType
	{
		IMAGE_TYPE_1D,
		IMAGE_TYPE_2D,
		IMAGE_TYPE_3D
	};

	enum class ImageTiling
	{
		IMAGE_TILING_OPTIONAL
	};

	struct ImageSpec
	{
		struct {
			int Width, Height;
		} Resolution;
		ImageFormat Format;
		ImageType Type;
		ImageTiling Tiling;
		void* ImageData = nullptr;
	};

	class Image
	{
	public:
		virtual const ImageSpec& GetSpec() const = 0;
		virtual void LoadImageData(void* ImageData) = 0;
	protected:
	};
}
