#pragma once

namespace Chilli
{
	enum class ImageFormat
	{
		RGBA8,
		D32,
		D24_S8,
		D32_S8
	};

	enum class ImageUsage
	{
		SAMPLED,
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
		IMAGE_TILING_OPTIOMAL
	};

	struct ImageSpec
	{
		struct {
			int Width, Height;
		} Resolution;
		void* ImageData = nullptr;
		ImageFormat Format = ImageFormat::RGBA8;
		ImageType Type = ImageType::IMAGE_TYPE_2D;
		ImageTiling Tiling = ImageTiling::IMAGE_TILING_OPTIOMAL;
		ImageUsage Usage = ImageUsage::COLOR;
	};

	class Image
	{
	public:
		virtual const ImageSpec& GetSpec() const = 0;
		virtual void LoadImageData(void* ImageData) = 0;
		virtual void LoadImageData(void* ImageData, int Width, int Height) = 0;
	};
}
