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

		uint32_t RawMaterialHandle = UINT32_MAX;
		BackBone::AssetHandle<ShaderProgram> ShaderProgramId;
	};
}
