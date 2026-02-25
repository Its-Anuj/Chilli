#include "Ch_PCH.h"
#include "Chilli/Chilli.h"
#include "Profiling\Timer.h"
#include "glm/ext/matrix_transform.hpp"

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

struct GameResource
{
	Chilli::BackBone::Entity Player;
	Chilli::BackBone::Entity Camera;
	Chilli::BackBone::Entity RenderStatsText;
	Chilli::BackBone::Entity RenderDataStats;
	Chilli::BackBone::Entity UIPanel, UIHeader;
	Chilli::BackBone::Entity UIPanel2, UIHeader2;
	Chilli::BackBone::AssetHandle<Chilli::ShaderProgram> Program;
	Chilli::BackBone::AssetHandle<Chilli::Material> Material;
	Chilli::BackBone::AssetHandle<Chilli::Image> GrayImage;
	Chilli::BackBone::AssetHandle<Chilli::ImageData> GrayImageData;
	Chilli::BackBone::AssetHandle<Chilli::Texture> GrayTexture;
	Chilli::BackBone::AssetHandle<Chilli::Sampler> Sampler;
	Chilli::BackBone::AssetHandle<Chilli::EmberFont> AlkiaFont;
	Chilli::Scene GameScene;

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

	auto SquareMesh = Command.CreateMesh(Chilli::BasicShapes::SPHERE);

	auto GameData = Ctxt.Registry->GetResource<GameResource>();

	GameData->Program = GeometryShaderProgram;

	Chilli::SamplerSpec SamplerSpec;
	SamplerSpec.Filter = Chilli::SamplerFilter::NEAREST;
	SamplerSpec.Mode = Chilli::SamplerMode::REPEAT;
	GameData->Sampler = Command.CreateSampler(SamplerSpec);

	Command.AddLoader<Chilli::ImageLoader>();

	auto [GrayImage, GrayImageData] = Command.AllocateImage("Assets/Textures/download.jpg",
		Chilli::ImageFormat::RGBA8, Chilli::IMAGE_USAGE_SAMPLED_IMAGE, Chilli::ImageType::IMAGE_TYPE_2D);

	GameData->GrayImage = GrayImage;
	GameData->GrayImageData = GrayImageData;

	auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

	Chilli::TextureSpec TextureSpec;
	TextureSpec.Format = Chilli::ImageFormat::RGBA8;
	GameData->GrayTexture = Command.CreateTexture(GameData->GrayImage, TextureSpec);

	GameData->Material = Command.CreateMaterial(GeometryShaderProgram);
	MaterialSystem->SetAlbedoColor(GameData->Material, { 1.0f, 1.0f, 1.0f, 1.0f });
	MaterialSystem->SetAlbedoTexture(GameData->Material, GameData->GrayTexture);
	MaterialSystem->SetAlbedoSampler(GameData->Material, GameData->Sampler);

	GameData->Player = Command.CreateEntity();
	Command.AddComponent<Chilli::TransformComponent>(GameData->Player, Chilli::TransformComponent(
	));

	Command.AddComponent<Chilli::MeshComponent>(GameData->Player, Chilli::MeshComponent{
		.MeshHandle = SquareMesh,
		.MaterialHandle = GameData->Material
		});

	auto Pepper = Chilli::Pepper(Ctxt);

	Chilli::PepperTransform panelTransform;

	GameData->AlkiaFont = Command.LoadFont_TTF("Assets/Alkia.ttf");
	
	// Size: 20% width, 20% height (example)
	panelTransform.PercentageDimensions = { 0.2f, 0.25f };

	// Position: Top-right corner
	panelTransform.PercentagePosition = { 0.75f, 0.3f }; // X=0.8 → right, Y=0 → top

	panelTransform.AnchorX = Chilli::AnchorX::RIGHT;  // Align right
	panelTransform.AnchorY = Chilli::AnchorY::TOP;    // Align top

	Chilli::EmberTextComponent RenderStatsTextData;
	RenderStatsTextData.Font = GameData->AlkiaFont;
	RenderStatsTextData.FontSize = 20.0f; // Adjust as needed
	RenderStatsTextData.IsDirty = true;

	// Create entity
	GameData->RenderStatsText = Command.CreateEntity();
	Command.AddComponent<Chilli::PepperTransform>(GameData->RenderStatsText, panelTransform);
	Command.AddComponent<Chilli::EmberTextComponent>(GameData->RenderStatsText, RenderStatsTextData);


	GameData->Camera = Chilli::CameraBundle::Create3D(Ctxt);
	Command.AddComponent<Chilli::Deafult3DCameraController>(GameData->Camera, Chilli::Deafult3DCameraController());

	GameData->GameScene.MainCamera = GameData->Camera;

	auto SceneManager = Command.GetService<Chilli::SceneManager>();
	SceneManager->SetActiveScene(&GameData->GameScene);
}

void UpdateRenderStatsText(const Chilli::RenderDeviceStats& stats, Chilli::EmberTextComponent& textComp)
{
	// Build multi-line string
	std::ostringstream ss;

	ss << "Render Stats:\n";
	ss << "Images: " << stats.TotalImagesAllocated << "\n";
	ss << "Textures: " << stats.TotalTexturesCreated << "\n";
	ss << "Buffers: " << stats.TotalBuffersCreated << "\n";
	ss << "Pipelines: " << stats.ActivePipelines << "\n\n";

	ss << "Memory (VRAM):\n";
	ss << "Allocated: " << stats.AllocatedVRAM / 1024 << " KB\n";
	ss << "Used: " << stats.UsedVRAM / 1024 << " KB\n";
	ss << "Staging: " << stats.StagingBufferMemory / 1024 << " KB\n\n";

	ss << "Frame Stats:\n";
	ss << "Indices: " << stats.IndiciesRendered << "\n";
	ss << "Vertices: " << stats.VerticesRendered << "\n";
	ss << "Triangles: " << stats.TrianglesPerFrame << "\n";
	ss << "Draw Calls: " << stats.DrawCallsPerFrame << "\n";
	ss << "Descriptor Binds: " << stats.DescriptorSetBinds << "\n";

	// Update the component
	textComp.Content = ss.str();
	textComp.IsDirty = true; // Tell Ember to rebuild glyphs
}

void OnGamePlay(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	auto GameData = Command.GetResource<GameResource>();
	auto FrameData = Command.GetResource<Chilli::BackBone::GenericFrameData>();
	auto InputManager = Command.GetService<Chilli::Input>();
	auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
	auto RenderService = Command.GetService<Chilli::Renderer>();

	auto Ts = FrameData->Ts;

	auto PlayerTranform = Ctxt.Registry->GetComponent<Chilli::TransformComponent>(GameData->Player);

	if (InputManager->IsKeyPressed(Chilli::Input_key_KpAdd))
	{
		MaterialSystem->SetAlbedoColor(GameData->Material,
			MaterialSystem->GetView(GameData->Material).AlbedoColor + Chilli::Vec4(0.01f, 0.0f, 0.0f, 0.0f));
	}
	PlayerTranform->RotateX(2 * Ts);
	// Inside your main update loop
	UpdateRenderStatsText(RenderService->GetRenderDeviceStats(),
		*Command.GetComponent<Chilli::EmberTextComponent>(GameData->RenderStatsText));
}

void OnGameShutDown(Chilli::BackBone::SystemContext& Ctxt)
{
	auto GameData = Ctxt.Registry->GetResource<GameResource>();
	auto Command = Chilli::Command(Ctxt);
	auto RenderCommandService = Command.GetService<Chilli::RenderCommand>();

	Command.DestroyEntity(GameData->Player);
	Command.DestroyEntity(GameData->Camera);

	Command.UnloadAsset(GameData->GrayImageData.ValPtr->FilePath);
	Command.UnloadAsset(GameData->AlkiaFont.ValPtr->FontSource.ValPtr->FilePath);
}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

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
