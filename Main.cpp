#include "Ch_PCH.h"
#include "Chilli/Chilli.h"
#include "Profiling\Timer.h"
#include "glm/ext/matrix_transform.hpp"

struct GameResource
{
	Chilli::BackBone::Entity Player;
	Chilli::BackBone::Entity Camera;
	Chilli::BackBone::AssetHandle<Chilli::ShaderProgram> Program;
	Chilli::BackBone::AssetHandle<Chilli::Material> Material;
	Chilli::BackBone::AssetHandle<Chilli::Image> GrayImage;
	Chilli::BackBone::AssetHandle<Chilli::ImageData> GrayImageData;
	Chilli::BackBone::AssetHandle<Chilli::Texture> GrayTexture;
	Chilli::BackBone::AssetHandle<Chilli::Sampler> Sampler;

	uint32_t GameWindow;
};

void OnWindowCreate(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	Command.AddResource<GameResource>();
	auto GameData = Command.GetResource<GameResource>();
	GameData->GameWindow = Command.SpwanWindow(Chilli::WindowSpec{ "Chilli", {800, 600} });
}

void OnGameCreate(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);

	auto RenderCommandService = Command.GetService<Chilli::RenderCommand>();
	auto RenderService = Command.GetService<Chilli::Renderer>();

	// Create Shaders
	auto GeometryPassVertexShader = Command.CreateShaderModule("Assets/Shaders/shader_vert.spv",
		Chilli::ShaderStageType::SHADER_STAGE_VERTEX);
	auto GeometryPassFragShader = Command.CreateShaderModule("Assets/Shaders/shader_frag.spv",
		Chilli::ShaderStageType::SHADER_STAGE_FRAGMENT);

	auto GeometryShaderProgram = Command.CreateShaderProgram();
	Command.AttachShaderModule(GeometryShaderProgram, GeometryPassVertexShader);
	Command.AttachShaderModule(GeometryShaderProgram, GeometryPassFragShader);
	Command.LinkShaderProgram(GeometryShaderProgram);

	// Create Mesh

	Chilli::MeshCreateInfo SquareInfo{};

	std::vector<Chilli::Vertex> SquareVertices = {
		// Position              // UV
		{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f } }, // Bottom-left
		{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f } }, // Bottom-right
		{ {  0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f } }, // Top-right
		{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f } }  // Top-left
	};

	std::vector<uint32_t> SquareIndices = {
		0, 1, 2, // First triangle
		2, 3, 0  // Second triangle
	};

	SquareInfo.VertCount = SquareVertices.size();
	SquareInfo.Vertices = SquareVertices.data();
	SquareInfo.VerticesSize = sizeof(Chilli::Vertex) * SquareVertices.size();
	SquareInfo.IndexCount = SquareIndices.size();
	SquareInfo.Indicies = SquareIndices.data();
	SquareInfo.IndiciesSize = sizeof(uint32_t) * SquareIndices.size();
	SquareInfo.IndexType = Chilli::IndexBufferType::UINT32_T;
	SquareInfo.State = Chilli::BufferState::STATIC_DRAW;
	auto SquareMesh = Command.CreateMesh(SquareInfo);

	auto GameData = Ctxt.Registry->GetResource<GameResource>();

	GameData->Program = GeometryShaderProgram;

	Chilli::SamplerSpec SamplerSpec;
	SamplerSpec.Filter = Chilli::SamplerFilter::NEAREST;
	SamplerSpec.Mode = Chilli::SamplerMode::REPEAT;
	GameData->Sampler = Command.CreateSampler(SamplerSpec);

	Command.AddLoader<Chilli::ImageLoader>();

	auto [GrayImage, GrayImageData] = Command.AllocateImage("Assets/Textures/download.jpg",
		Chilli::ImageFormat::RGBA8, Chilli::IMAGE_USAGE_SAMPLED_IMAGE, Chilli::ImageType::IMAGE_TYPE_2D, 1);

	GameData->GrayImage = GrayImage;
	GameData->GrayImageData = GrayImageData;

	Chilli::TextureSpec TextureSpec;
	TextureSpec.Format = Chilli::ImageFormat::RGBA8;
	GameData->GrayTexture = Command.CreateTexture(GameData->GrayImage, TextureSpec);

	Chilli::Material Material;
	Material.ShaderProgramId = GeometryShaderProgram;
	Material.AlbedoTextureHandle = GameData->GrayTexture;
	Material.AlbedoSamplerHandle = GameData->Sampler;
	GameData->Material = Command.CreateMaterial(Material);

	GameData->Player = Command.CreateEntity();
	Command.AddComponent<Chilli::TransformComponent>(GameData->Player, Chilli::TransformComponent(
		Chilli::Vec3(),
		Chilli::Vec3(),
		Chilli::Vec3()
	));
	Command.AddComponent<Chilli::MeshComponent>(GameData->Player, Chilli::MeshComponent{
		.MeshHandle = SquareMesh,
		.MaterialHandle = GameData->Material
		});

}

void OnGamePlay(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	auto GameData = Command.GetResource<GameResource>();
	auto FrameData = Command.GetResource<Chilli::BackBone::GenericFrameData>();
	auto InputManager = Command.GetService<Chilli::Input>();

	auto Ts = FrameData->Ts;

	auto PlayerTranform = Ctxt.Registry->GetComponent<Chilli::TransformComponent>(GameData->Player);

	if (InputManager->IsKeyDown(Chilli::Input_key_A))
		PlayerTranform->Position.x += 0.1f * Ts;
	if (InputManager->IsKeyDown(Chilli::Input_key_D))
		PlayerTranform->Position.x -= 0.1f * Ts;

	if (InputManager->IsKeyDown(Chilli::Input_key_W))
		PlayerTranform->Position.y += 0.1f * Ts;
	if (InputManager->IsKeyDown(Chilli::Input_key_S))
		PlayerTranform->Position.y -= 0.1f * Ts;

	auto RenderService = Command.GetService<Chilli::Renderer>();
	RenderService->UpdateMaterialTextureData(GameData->Material.ValPtr->RawMaterialHandle,
		GameData->GrayTexture.ValPtr->RawTextureHandle,
		"Texture", Chilli::ResourceState::ShaderRead);
	RenderService->UpdateMaterialSamplerData(GameData->Material.ValPtr->RawMaterialHandle,
		GameData->Sampler.ValPtr->SamplerHandle,
		"Sampler");
}

void OnGameShutDown(Chilli::BackBone::SystemContext& Ctxt)
{
	auto GameData = Ctxt.Registry->GetResource<GameResource>();
	auto Command = Chilli::Command(Ctxt);
	auto RenderCommandService = Command.GetService<Chilli::RenderCommand>();

	Command.DestroyEntity(GameData->Player);
	Command.DestroyEntity(GameData->Camera);

	Command.UnloadAsset(GameData->GrayImageData.ValPtr->FilePath);
}

int main()
{
	Chilli::Log::Init();

	Chilli::BackBone::App App;

	Chilli::RenderExtensionConfig RenderConfig{};
	RenderConfig.Spec.VSync = true;
	RenderConfig.Spec.EnableValidation = true;

	App.SystemScheduler.AddSystemOverLayBefore(Chilli::BackBone::ScheduleTimer::START_UP, OnWindowCreate);

	App.Extensions.AddExtension(std::make_unique<Chilli::DeafultExtension>(
		Chilli::DeafultExtensionConfig{
			.RenderConfig = RenderConfig
		}), true, &App);

	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::START_UP, OnGameCreate);

	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::UPDATE, OnGamePlay);
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::SHUTDOWN, OnGameShutDown);

	App.Run();
}
