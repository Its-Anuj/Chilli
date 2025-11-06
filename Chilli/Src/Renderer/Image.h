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

	enum ImageUsage : uint32_t {
		NONE = 0,
		SAMPLED_IMAGE = 1 << 0,
		COLOR_ATTACHMENT= 1 << 1,
		DEPTH_STENCIL_ATTACHMENT = 1 << 2,
		STORAGE_IMAGE= 1 << 3,
		TRANSFER_SRC = 1 << 4,
		TRANSFER_DST = 1 << 5,
		INPUT_ATTACHMENT= 1 << 6,
		PRESENT_IMAGE = 1 << 7
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
		uint32_t Usage = ImageUsage::SAMPLED_IMAGE;
	};

	class Image
	{
	public:
		virtual void Init(const ImageSpec& Spec) = 0;
		virtual void Destroy () = 0;

		static std::shared_ptr<Image> Create(const ImageSpec& Spec);
	};
}
