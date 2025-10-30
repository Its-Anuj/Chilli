#pragma once

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
		RGBA8, D32, D24_S8, D32_S8
	};

	struct ImageSpec
	{
		struct {
			int Width, Height;
		} Resolution;
		ImageFormat Format;
		ImageType Type;
		void* ImageData = nullptr;
		bool YFlip = false;
	};

	class Image
	{
	public:
		virtual void Init(const ImageSpec& Spec) = 0;
		virtual void Destroy () = 0;

		static std::shared_ptr<Image> Create(const ImageSpec& Spec);
	};
}
