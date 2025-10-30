#pragma once

#include "UUID/UUID.h"

namespace Chilli
{
	enum class SamplerMode
	{
		REPEAT,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER
	};

	enum class SamplerFilter
	{
		NEAREST,
		LINEAR
	};

	struct SamplerSpec
	{
		SamplerMode Mode;
		SamplerFilter Filter;
	};

	class Sampler
	{
	public:
		virtual void Init(const SamplerSpec& Spec) = 0;
		virtual void Destroy() = 0;

		virtual const SamplerSpec& GetSpec() const = 0;
		virtual void* GetNativeHandle() const = 0;

		static std::shared_ptr< Sampler> Create(const SamplerSpec& Spec);

		const UUID& ID() const { return _ID; }
	private:
		UUID _ID;
	};

	using SamplerHandle = std::shared_ptr< Sampler>;

	class SamplerManager
	{
	public:
		SamplerManager() {}
		~SamplerManager() {}

		const UUID& Add(const SamplerSpec& Spec);
		const SamplerHandle& Get(UUID ID);
		bool Exist(UUID ID);

		void Flush();
		void Destroy(const UUID& ID);

		size_t Count() const { return _Samplers.size(); }

	private:
		std::unordered_map<UUID, SamplerHandle> _Samplers;
	};
}
