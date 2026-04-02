#include "Ch_PCH.h"
#include "RenderClasses.h"

namespace Chilli
{
	void Renderer::Init(const GraphcisBackendCreateSpec& Spec)
	{
		_Api = std::shared_ptr<GraphicsBackendApi>(GraphicsBackendApi::Create(Spec));
		_RenderPerFrameArena.Prepare(5 * 1024 * 1024);
		_InlineUniformDataAllocator.Ref(_RenderPerFrameArena, 1 * 1024 * 128);

		_MaxFramesInFlight = Spec.MaxFrameInFlight;
		_FrameIndex = 0;
		_FramePackets.resize(_MaxFramesInFlight);

		auto VertexShader = _Api->CreateShaderModule("Assets/Shaders/shader_vert.spv",
			Chilli::ShaderStageType::SHADER_STAGE_VERTEX);
		auto FragShader = _Api->CreateShaderModule("Assets/Shaders/shader_frag.spv",
			Chilli::ShaderStageType::SHADER_STAGE_FRAGMENT);

		auto Program = _Api->MakeShaderProgram();
		_Api->AttachShader(Program, VertexShader);
		_Api->AttachShader(Program, FragShader);
		_Api->LinkShaderProgram(Program);

		_DeafultShaderProgram.RawProgramHandle = Program;
	}

	void Renderer::Terminate()
	{
		GraphicsBackendApi::Terminate(_Api.get(), false);
	}

	void Renderer::BeginFrame()
	{
		_FramePackets[_FrameIndex].Graphics_Stream.BeginFrame(_FrameIndex);
	}

	void Renderer::Clear()
	{
		_FramePackets[_FrameIndex].Graphics_Stream.Clear();
		_FramePackets[_FrameIndex].Compute_Stream.Clear();
		_FramePackets[_FrameIndex].Transfer_Stream.Clear();
	}

	void Renderer::EndFrame()
	{
		_FramePackets[_FrameIndex].Graphics_Stream.EndFrame();
		_Api->EndFrame(_FramePackets[_FrameIndex]);
		this->Clear();
		_FrameIndex = (_FrameIndex + 1) % _MaxFramesInFlight;
	}

	std::shared_ptr<RenderCommand> Renderer::CreateRenderCommand()
	{
		return std::make_shared<RenderCommand>(_Api);
	}

	static inline void HashCombine(uint64_t& seed, uint64_t value)
	{
		const uint64_t fnv_offset = 1469598103934665603ull;
		const uint64_t fnv_prime = 1099511628211ull;

		seed ^= value;
		seed *= fnv_prime;
	}

	uint64_t ShaderDescriptorBindingFlags::HashFlag(const ShaderDescriptorBindingFlags& Flag)
	{
		uint64_t hash = 1469598103934665603ull; // FNV64 Offset Basis

		// ---- Simple booleans ----
		HashCombine(hash, Flag.Set);
		HashCombine(hash, Flag.Binding);

		HashCombine(hash, Flag.EnableBindless);
		HashCombine(hash, Flag.EnableStreaming);
		HashCombine(hash, Flag.EnablePartiallyBound);
		HashCombine(hash, Flag.EnableVariableSize);
		HashCombine(hash, Flag.VariableSize);

		HashCombine(hash, Flag.StageFlags);
		return hash;
	}

	BackBone::AssetHandle<Material> MaterialSystem::CreateMaterial(BackBone::AssetHandle<ShaderProgram> Program)
	{
		auto Command = Chilli::Command(_Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();

		auto MaterialStore = Command.GetStore<Chilli::Material>();

		Material Mat;
		auto ReturnHandle = MaterialStore->Add(Mat);
		ReturnHandle.ValPtr->ShaderProgramId = Program;
		ReturnHandle.ValPtr->RawMaterialHandle = RenderCommandService->PrepareMaterialData(Program.ValPtr->RawProgramHandle);

		return ReturnHandle;
	}

	Material* MaterialSystem::GetMaterial(BackBone::AssetHandle<Material> Handle)
	{
		auto Command = Chilli::Command(_Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();

		auto MaterialStore = Command.GetStore<Chilli::Material>();

		auto Mat = MaterialStore->Get(Handle);
		return Mat;
	}

	void MaterialSystem::CopyMaterial(BackBone::AssetHandle<Material> Src, BackBone::AssetHandle<Material> Dst)
	{
		auto Command = Chilli::Command(_Ctxt);
		auto MaterialStore = Command.GetStore<Chilli::Material>();

		CH_CORE_ASSERT(MaterialStore->Get(Src) != nullptr, "Src Material Handle Not Found In Store!");
		CH_CORE_ASSERT(MaterialStore->Get(Dst) != nullptr, "Dst Material Handle Not Found In Store!");

		Dst.ValPtr->AlbedoColor = Src.ValPtr->AlbedoColor;
		Dst.ValPtr->AlbedoSamplerHandle = Src.ValPtr->AlbedoSamplerHandle;
		Dst.ValPtr->AlbedoTextureHandle = Src.ValPtr->AlbedoTextureHandle;
		Dst.ValPtr->Version = Src.ValPtr->Version;
		Dst.ValPtr->LastUploadedVersion = Src.ValPtr->Version;
	}

	MaterialShaderData MaterialSystem::GetMaterialShaderData(BackBone::AssetHandle<Material> Handle)
	{
		auto Command = Chilli::Command(_Ctxt);
		auto RenderService = Command.GetService<Renderer>();

		auto Mat = GetMaterial(Handle);
		Mat->LastUploadedVersion = Mat->Version;

		MaterialShaderData Data;
		Data.AlbedoColor = Mat->AlbedoColor;
		Data.AlbedoTextureIndex = RenderService->GetTextureShaderIndex(Mat->AlbedoTextureHandle.ValPtr->RawTextureHandle);
		Data.AlbedoSamplerIndex = RenderService->GetSamplerShaderIndex(Mat->AlbedoSamplerHandle.ValPtr->SamplerHandle);

		return Data;
	}

	void MaterialSystem::ClearMaterialData(BackBone::AssetHandle<Material> Mat)
	{
		auto Command = Chilli::Command(_Ctxt);

		auto MaterialStore = Command.GetStore<Chilli::Material>();
		auto RenderCommandService = Command.GetService<RenderCommand>();

		RenderCommandService->ClearMaterialData(Mat.ValPtr->RawMaterialHandle);
		Mat.ValPtr->RawMaterialHandle = UINT32_MAX;
	}

	bool MaterialSystem::ShouldMaterialShaderDataUpdate(BackBone::AssetHandle<Material> Handle)
	{
		auto Mat = GetMaterial(Handle);
		return Mat->Version != Mat->LastUploadedVersion;
	}

	void MaterialSystem::DestroyMaterial(BackBone::AssetHandle<Material> Mat)
	{
		auto Command = Chilli::Command(_Ctxt);

		auto MaterialStore = Command.GetStore<Chilli::Material>();
		auto RenderCommandService = Command.GetService<RenderCommand>();

		RenderCommandService->ClearMaterialData(Mat.ValPtr->RawMaterialHandle);
		Mat.ValPtr->RawMaterialHandle = UINT32_MAX;
		MaterialStore->Remove(Mat);
	}

	BackBone::AssetHandle<Scene> SceneManager::LoadScene(const std::string& path)
	{
		return BackBone::AssetHandle<Scene>();
	}

	void SceneManager::SaveScene(BackBone::AssetHandle<Scene>, const std::string& path)
	{

	}

	void SceneManager::AppendScene(BackBone::AssetHandle<Scene>, const std::string& path)
	{

	}
	
	BackBone::AssetHandle<Scene> SceneManager::CreateScene()
	{
		auto Command = Chilli::Command(_Ctxt);
		auto RenderService = Command.GetService<Renderer>();
		auto ScreenStore = Command.GetStore<Scene>();

		Scene NewScene{};
		return ScreenStore->Add(NewScene);
	}

	void SceneManager::PushUpdateShaderData(BackBone::AssetHandle<Scene> UpdateScene)
	{
		auto Command = Chilli::Command(_Ctxt);
		auto RenderService = Command.GetService<Renderer>();
		auto ScreenStore = Command.GetStore<Scene>();

		CH_CORE_ASSERT(ScreenStore->Get(UpdateScene) != nullptr, "UpdateScene not valid!");

		if (GetSceneShaderIndex(UpdateScene) == UINT32_MAX)
		{
			_SceneShaderIndexMap.Insert(UpdateScene.Handle, _SceneIdx);
			_SceneIdx++;
		}

		SceneData ShaderSceneData;
		ShaderSceneData.AmbientColor = UpdateScene.ValPtr->Settings.AmbientColor;
		ShaderSceneData.FogProperties = UpdateScene.ValPtr->Settings.FogProperties;
		ShaderSceneData.ScreenData = UpdateScene.ValPtr->Settings.ScreenData;
		ShaderSceneData.GlobalLightDir = UpdateScene.ValPtr->Settings.GlobalLightDir;

		auto CameraComp = Command.GetComponent<CameraComponent>(UpdateScene.ValPtr->MainCamera);
		ShaderSceneData.ViewProjMatrix = CameraComp->ViewProjMat;

		RenderService->PushUpdateSceneShaderData(*_SceneShaderIndexMap.Get(UpdateScene.Handle), ShaderSceneData);
	}
}
