#pragma once

#include "Image.h"
#include "Samplers.h"
#include "UUID/UUID.h"

namespace Chilli
{
	struct TextureSpec : public ImageSpec
	{
		std::string FilePath;
		bool UseFilePath = true;
	};

	class Texture
	{
	public:
		virtual void Init(const TextureSpec& Spec) = 0;
		virtual void Destroy() = 0;

		virtual const TextureSpec& GetSpec() const = 0;
		virtual void* GetNativeHandle() const = 0;

		static std::shared_ptr< Texture> Create(const TextureSpec& Spec);
		
		const UUID& ID() const { return _ID; }
	private:
		UUID _ID;
	};
	using TextureHandle = std::shared_ptr<Texture>;

	class TextureManager
	{
	public:
		TextureManager() {}
		~TextureManager() {}

		const UUID& Add(const TextureSpec& Spec);
		const TextureHandle& Get(UUID ID);
		bool Exist(UUID ID);

		void Flush();
		void Destroy(const UUID& ID);

		size_t Count() const { return _Textures.size(); }

	private:
		std::unordered_map<UUID, TextureHandle> _Textures;
	};
}
