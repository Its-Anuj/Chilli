#pragma once

#include "Maths.h"
#include "BackBone.h"
#include "Image.h"

namespace Chilli
{
	inline std::string GetFileNameWithExtension(const std::string& path)
	{
		size_t lastSlash = path.find_last_of("/\\"); // works for Windows & Linux paths
		if (lastSlash == std::string::npos)
			return path; // no directory part
		return path.substr(lastSlash + 1);
	}

	inline std::string GetFileExtension(const std::string& path)
	{
		// Find last dot after the last path separator
		const size_t slash = path.find_last_of("/\\");
		const size_t dot = path.find_last_of('.');

		if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
			return ""; // no extension

		return path.substr(dot + 1); // without dot: "png"
	}

	inline uint64_t HashString64(const std::string& str)
	{
		const uint64_t FNV_offset_basis = 14695981039346656037ull;
		const uint64_t FNV_prime = 1099511628211ull;

		uint64_t hash = FNV_offset_basis;

		for (char c : str) {
			hash ^= static_cast<uint64_t>(c);
			hash *= FNV_prime;
		}

		return hash;
	}

	struct IndexEntry {
		static constexpr uint32_t INVALID_INDEX = 0xFFFFFFFF;
		uint64_t Hash = 0;
		uint32_t DataIndex = INVALID_INDEX; // invalid by default

		bool IsValid() const { return DataIndex != INVALID_INDEX; }

		bool operator<(const IndexEntry& other) const { return Hash < other.Hash; }
		bool operator<(uint64_t otherHash) const { return Hash < otherHash; }

		// Insert sorted, but if hash exists and is invalid, reuse it
		static void AddOrUpdateSorted(std::vector<IndexEntry>& indexMap, const IndexEntry& val)
		{
			auto it = std::lower_bound(indexMap.begin(), indexMap.end(), val.Hash,
				[](const IndexEntry& a, uint64_t b) { return a.Hash < b; });

			if (it != indexMap.end() && it->Hash == val.Hash) {
				// Update existing (reuse invalid slot)
				it->DataIndex = val.DataIndex;
			}
			else {
				indexMap.insert(it, val);
			}
		}

		// Find only if valid
		static int FindIndex(const std::vector<IndexEntry>& indexMap, uint64_t hash)
		{
			auto it = std::lower_bound(indexMap.begin(), indexMap.end(), hash,
				[](const IndexEntry& a, uint64_t b) { return a.Hash < b; });

			if (it != indexMap.end() && it->Hash == hash && it->IsValid())
				return static_cast<int>(it->DataIndex);

			return -1;
		}

		// Mark entry invalid
		static bool Invalidate(std::vector<IndexEntry>& indexMap, uint64_t hash)
		{
			auto it = std::lower_bound(indexMap.begin(), indexMap.end(), hash,
				[](const IndexEntry& a, uint64_t b) { return a.Hash < b; });

			if (it != indexMap.end() && it->Hash == hash && it->IsValid()) {
				it->DataIndex = INVALID_INDEX;
				return true;
			}
			return false;
		}

	};

	struct ImageData
	{
		void* Pixels = nullptr;
		IVec2 Resolution = { 0,0 };
		uint32_t NumChannels = 0;
		ImageFormat Format = ImageFormat::NONE;
		std::string FileName;
		std::string FilePath;

		~ImageData() {

		}
	};

	// 1. The Interface (The Contract)
	class IAssetLoader {
	public:
		virtual ~IAssetLoader() = default;

		// Every loader must implement these
		virtual void Unload(BackBone::SystemContext& Ctxt, const std::string& Path) = 0;

		virtual std::vector<std::string> GetExtensions() = 0;
		virtual bool DoesExist(const std::string& Path) const = 0;
		virtual int __FindIndex(const std::string& Path) const = 0;
	};
	
	template<typename T>
	class BaseLoader : public IAssetLoader {
	public:
		// This is the specific Load for this type
		virtual BackBone::AssetHandle<T> LoadTyped(BackBone::SystemContext& Ctxt, const std::string& Path) = 0;

		// We can provide a common Unload for the handle
		virtual void Unload(BackBone::SystemContext& Ctxt, const BackBone::AssetHandle<T>& Handle) = 0;
	};

	template<typename T>
	concept DerivedFromIAssetLoader = std::is_base_of_v<IAssetLoader, T>;

	class ImageLoader : public BaseLoader<ImageData>
	{
	public:
		ImageLoader() {}
		~ImageLoader();
		
		virtual BackBone::AssetHandle<ImageData> LoadTyped(BackBone::SystemContext& Ctxt, const std::string& Path) override;
		virtual void Unload(BackBone::SystemContext& Ctxt, const std::string& Path) override;
		virtual void Unload(BackBone::SystemContext& Ctxt, const BackBone::AssetHandle<ImageData>& Data) override;

		virtual bool DoesExist(const std::string& Path) const override { return true; }
		virtual int __FindIndex(const std::string& Path) const override { return 0; }

		virtual std::vector<std::string> GetExtensions() override { return { ".png", ".jpg" }; }
	private:
		std::vector<IndexEntry> _IndexMaps;
		std::vector<BackBone::AssetHandle<ImageData>> _ImageDataHandles;   // Maps Hash index to _ImageDatas index
	};

	uint32_t GetNewAssetLoaderID();

	template<typename _AssetType>
	uint32_t GetAssetLoaderID()
	{
		static uint32_t ID = GetNewAssetLoaderID();
		return ID;
	}

	class AssetLoader
	{
	public:
		AssetLoader(BackBone::SystemContext& Ctxt);
		~AssetLoader(){
			for (auto Loader : _AssetLoaders)
				delete Loader;
		}

		template<DerivedFromIAssetLoader _T>
		void AddLoader()
		{
			auto ID = GetAssetLoaderID<_T>();
			if (ID < _AssetLoaders.size())
				return;

			auto* NewLoader = new _T();

			// Helper to normalize extension to lowercase
			auto Normalize = [](std::string ext)
				{
					std::transform(ext.begin(), ext.end(), ext.begin(),
						[](unsigned char c) { return std::tolower(c); });
					return ext;
				};

			// First pass: check for duplicates
			for (const auto& Extension : NewLoader->GetExtensions())
			{
				std::string NormExt = Normalize(Extension);
				if (_ExtensionMap.find(NormExt) != _ExtensionMap.end())
				{
					// Loader for this extension already exists
					delete NewLoader;
					return;
				}
			}

			// Register this loader
			const size_t LoaderIndex = _AssetLoaders.size();

			for (const auto& Extension : NewLoader->GetExtensions())
			{
				std::string NormExt = Normalize(Extension);
				_ExtensionMap[NormExt] = LoaderIndex;
			}

			_AssetLoaders.push_back(NewLoader);
		}

		template<DerivedFromIAssetLoader _T>
		_T* GetLoader()
		{
			auto ID = GetAssetLoaderID<_T>();
			if (ID <= _AssetLoaders.size())
				return (_T*)_AssetLoaders[ID];
			return nullptr;
		}

		// Generic Load by Path
		template<typename _AssetType>
		BackBone::AssetHandle<_AssetType> Load(const std::string& Path)
		{
			std::string Extension = GetFileExtension(Path);
			// Add leading dot if your GetFileExtension doesn't provide it
			if (!Extension.empty() && Extension[0] != '.') Extension = "." + Extension;

			auto it = _ExtensionMap.find(Extension);
			if (it != _ExtensionMap.end())
			{
				// 1. Get the generic loader
				IAssetLoader* loader = _AssetLoaders[it->second];

				// 2. Cast it to the typed version of itself
				// We assume the user called Load<ImageData> for a .png
				auto typedLoader = static_cast<BaseLoader<_AssetType>*>(loader);

				return typedLoader->LoadTyped(_Ctxt, Path);
			}

			// Return an empty/null handle if no loader found
			return BackBone::AssetHandle<_AssetType>();
		}

		// Generic Unload by Path
		void Unload(const std::string& Path)
		{
			std::string Extension = GetFileExtension(Path);
			if (!Extension.empty() && Extension[0] != '.') Extension = "." + Extension;

			auto it = _ExtensionMap.find(Extension);
			if (it != _ExtensionMap.end())
			{
				_AssetLoaders[it->second]->Unload(_Ctxt, Path);
			}
		}
		
		// Getters for Inspection
		const std::unordered_map<std::string, uint32_t>& GetExtensionMap() const { return _ExtensionMap; }

		bool IsExtensionSupported(const std::string& ext) const {
			return _ExtensionMap.find(ext) != _ExtensionMap.end();
		}

	private:
		BackBone::SystemContext _Ctxt;
		std::unordered_map<std::string, uint32_t> _ExtensionMap;
		std::vector< IAssetLoader*> _AssetLoaders;
	};
}
