#include "AssetLoader.h"
#include "Ch_PCH.h"-
#include "AssetLoader.h"
#include "Profiling\Timer.h"

#include "DeafultExtensions.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Chilli
{
	uint32_t GetNewAssetLoaderID() {
		static uint32_t NewID = 0;
		return NewID++;
	}

	AssetLoader::AssetLoader(BackBone::SystemContext& Ctxt)
	{
		Ctxt.AssetRegistry->RegisterStore<ImageData>();
		_Ctxt = Ctxt;
	}

	ImageLoader::~ImageLoader()
	{
		for (int i = 0; i < _IndexMaps.size(); i++)
		{
			if (_IndexMaps[i].IsValid())
				CH_CORE_ERROR("All Image Data Must be Unloaded!");
		}
	}

	BackBone::AssetHandle<ImageData> ImageLoader::Load(BackBone::SystemContext& Ctxt, const std::string& Path)
	{
		auto Command = Chilli::Command(Ctxt);
		auto ImageDataStore = Command.GetStore<ImageData>();

		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));

		int Index = IndexEntry::FindIndex(_IndexMaps, HashValue);
		if (Index != -1)
			return _ImageDataHandles[Index];

		ImageData NewData{};
		int Width, Height, NumChannels;

		NewData.Pixels = stbi_load(Path.c_str(), &Width, &Height, &NumChannels, STBI_rgb_alpha);
		if (NewData.Pixels == nullptr)
			CH_CORE_ERROR("Given Path {} doesnot result any pixel data", Path);
		NewData.Resolution = { Width, Height };
		NewData.NumChannels = NumChannels;
		NewData.FileName = GetFileNameWithExtension(Path);
		NewData.FilePath = Path;
		NewData.Format = ImageFormat::RGBA8;

		auto ImageDataHandle = ImageDataStore->Add(NewData);

		uint32_t NewIndex = static_cast<uint32_t>(_ImageDataHandles.size());
		_ImageDataHandles.push_back(ImageDataHandle);

		IndexEntry::AddOrUpdateSorted(_IndexMaps, { HashValue, NewIndex });

		return ImageDataHandle;
	}

	void ImageLoader::Unload(BackBone::SystemContext& Ctxt, const std::string& Path)
	{
		auto Command = Chilli::Command(Ctxt);
		auto ImageDataStore = Command.GetStore<ImageData>();

		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));

		int Index = IndexEntry::FindIndex(_IndexMaps, HashValue);
		if (Index == -1)
			return;

		auto Handle = ImageDataStore->Get(_ImageDataHandles[Index]);
		if (!Handle)
			return;

		stbi_image_free(Handle->Pixels);

		ImageDataStore->Remove(_ImageDataHandles[Index]);

		// Invalidate index entry
		IndexEntry::Invalidate(_IndexMaps, HashValue);
	}

	void ImageLoader::Unload(BackBone::SystemContext& Ctxt, const BackBone::AssetHandle<ImageData>& Data)
	{
		Unload(Ctxt, Data.ValPtr->FilePath);
	}
}
