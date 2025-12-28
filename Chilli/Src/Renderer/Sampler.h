#pragma once

namespace Chilli
{
	enum class CompareOp {
		NEVER = 0,
		LESS = 1,
		EQUAL = 2,
		LESS_OR_EQUAL = 3,
		GREATER = 4,
		NOT_EQUAL = 5,
		GREATER_OR_EQUAL = 6,
		ALWAYS = 7,
	};

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

		// NEW FIELDS
		SamplerFilter MipmapMode; // Usually Linear for smooth transitions
		float MaxAnisotropy;      // 1.0f (off) to 16.0f
		float MinLod;             // Usually 0.0f
		float MaxLod;             // Usually VK_LOD_CLAMP_NONE or texture.MipCount

		// Logic/State
		bool bEnableCompare = false;      // Crucial if you ever want to do Shadow Mapping
		CompareOp bCompareOp = CompareOp::ALWAYS;

		bool Equals(const SamplerSpec& o) const
		{
			return Mode == o.Mode &&
				Filter == o.Filter &&
				MipmapMode == o.MipmapMode &&
				MaxAnisotropy == o.MaxAnisotropy &&
				MinLod == o.MinLod &&
				MaxLod == o.MaxLod &&
				bEnableCompare == o.bEnableCompare &&
				bCompareOp == o.bCompareOp;
		}

		bool operator==(const SamplerSpec& b)
		{
			const auto& a = *this;
			return a.Mode == b.Mode &&
				a.Filter == b.Filter &&
				a.MipmapMode == b.MipmapMode &&
				a.MaxAnisotropy == b.MaxAnisotropy &&
				a.MinLod == b.MinLod &&
				a.MaxLod == b.MaxLod &&
				a.bEnableCompare == b.bEnableCompare &&
				a.bCompareOp == b.bCompareOp;
		}

	};

	struct Sampler
	{
		SamplerSpec Spec;
		uint32_t SamplerHandle = UINT32_MAX;
	};
}
