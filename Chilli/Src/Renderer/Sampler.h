#pragma once

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

	struct Sampler
	{
		SamplerMode Mode;
		SamplerFilter Filter;
		uint32_t SamplreHandle;
	};
}
