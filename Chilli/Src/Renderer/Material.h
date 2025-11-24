#pragma once

#include "BackBone\BackBone.h"
#include "Texture.h"
#include "Pipeline.h"

namespace Chilli
{
	struct Material
	{
		BackBone::AssetHandle <Texture> AlbedoTextureHandle;
		BackBone::AssetHandle <Sampler> AlbedoSamplerHandle;
		Vec4 AlbedoColor;

		BackBone::AssetHandle<GraphicsPipeline> GraphicsPipelineId;
	};
}
