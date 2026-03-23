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

	const MaterialShaderData& MaterialSystem::GetMaterialShaderData(BackBone::AssetHandle<Material> Handle)
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

	void SceneManager::LoadScene(const std::string& path) // Clears and loads new
	{

	}
		
	void SceneManager::SaveScene(const std::string& path)
	{
	}

	void SceneManager::AppendScene(const std::string& path)
	{
	}

	void SceneManager::SetActiveScene(Scene* Sc)
	{
		_ActiveScene = Sc;

		auto Camera = _World->GetComponent<CameraComponent>(Sc->MainCamera);
		if (Camera == nullptr)
			CH_CORE_ERROR("Camera is Needed! Scene: {}", Sc->Name);
	}

	const Scene* SceneManager::GetActiveScene() const
	{
		if (_ActiveScene == nullptr)
			CH_CORE_ERROR("NO ACTIVE SCENEN");
		return _ActiveScene;
	}
}
