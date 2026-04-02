#pragma once

#include "BackBone\BackBone.h"
#include "Texture.h"
#include "Pipeline.h"
#include "Maths.h"

namespace Chilli
{
	class MaterialSystem;
	struct MaterialShaderData;

	struct MaterialView
	{
		BackBone::AssetHandle <Texture> AlbedoTextureHandle;
		BackBone::AssetHandle <Sampler> AlbedoSamplerHandle;
		Vec4 AlbedoColor;

		BackBone::AssetHandle<ShaderProgram> ShaderProgramId;
	};

		struct Material
		{
		private:
			BackBone::AssetHandle <Texture> AlbedoTextureHandle;
			BackBone::AssetHandle <Sampler> AlbedoSamplerHandle;
			Vec4 AlbedoColor;

			BackBone::AssetHandle<ShaderProgram> ShaderProgramId;

			uint32_t RawMaterialHandle = UINT32_MAX;

			uint32_t Version = 0;
			uint32_t LastUploadedVersion = 0;

			friend class MaterialSystem;
		};

	class MaterialSystem
	{
	public:
		MaterialSystem(BackBone::SystemContext& Ctxt) { _Ctxt = Ctxt; }

		// Material is based on program it is needed to even initialize
		BackBone::AssetHandle<Material> CreateMaterial(BackBone::AssetHandle<ShaderProgram> Program);
		BackBone::AssetHandle<Material> CloneMaterial(BackBone::AssetHandle<ShaderProgram> Program) {}
		void CopyMaterial(BackBone::AssetHandle<Material> Src, BackBone::AssetHandle<Material> Dst);
		// Destroy fully from the store
		void DestroyMaterial(BackBone::AssetHandle<Material> Mat);
		// Doesnot Destroy from the store
		void ClearMaterialData(BackBone::AssetHandle<Material> Mat);

		// For Rendering and Backend immutable
		uint32_t GetRawMaterialHandle(BackBone::AssetHandle<Material> Handle) {
			auto Mat = GetMaterial(Handle);
			return Mat->RawMaterialHandle;
		}

		// For Rendering and Backend immutable
		BackBone::AssetHandle<ShaderProgram> GetShaderProgramID(BackBone::AssetHandle<Material> Handle) {
			auto Mat = GetMaterial(Handle);
			return Mat->ShaderProgramId;
		}

		void SetAlbedoColor(BackBone::AssetHandle<Material> Handle, const Vec4& Color)
		{
			auto Mat = GetMaterial(Handle);
			Mat->AlbedoColor = Color;
			Mat->Version++;
		}

		Vec4 GetAlbedoColor(BackBone::AssetHandle<Material> Handle)
		{
			auto Mat = GetMaterial(Handle);
			return Mat->AlbedoColor;
		}

		void SetAlbedoTexture(BackBone::AssetHandle<Material> Handle, BackBone::AssetHandle <Texture> AlbedoTextureHandle)
		{
			auto Mat = GetMaterial(Handle);
			Mat->AlbedoTextureHandle = AlbedoTextureHandle;
			Mat->Version++;
		}

		void SetAlbedoSampler(BackBone::AssetHandle<Material> Handle, BackBone::AssetHandle <Sampler> SamplerHandle)
		{
			auto Mat = GetMaterial(Handle);
			Mat->AlbedoSamplerHandle = SamplerHandle;
			Mat->Version++;
		}

		bool ShouldMaterialShaderDataUpdate(BackBone::AssetHandle<Material> Mat);
		MaterialShaderData GetMaterialShaderData(BackBone::AssetHandle<Material> Mat);

		// Just a view into changeable components
		MaterialView GetView(BackBone::AssetHandle<Material> Handle)
		{
			auto Mat = GetMaterial(Handle);

			MaterialView View;
			View.AlbedoColor = Mat->AlbedoColor;
			View.ShaderProgramId = Mat->ShaderProgramId;
			View.AlbedoSamplerHandle = Mat->AlbedoSamplerHandle;
			View.AlbedoTextureHandle = Mat->AlbedoTextureHandle;
			return View;
		}

	private:
		Material* GetMaterial(BackBone::AssetHandle<Material> Handle);

		BackBone::SystemContext _Ctxt;
	};

}
