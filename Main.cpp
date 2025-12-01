#include "Ch_PCH.h"
#include "Chilli/Chilli.h"
#include "Profiling\Timer.h"

struct GameResource
{
	Chilli::BackBone::Entity Player;
	Chilli::BackBone::Entity Camera;

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

	auto MeshStore = Command.GetStore<Chilli::Mesh>();

	Chilli::BackBone::AssetHandle<Chilli::Mesh> SquareMeshID;
	{
		SquareMeshID = Command.CreateMesh(Chilli::BasicShapes::QUAD);
	}

	Chilli::GraphicsPipelineCreateInfo ShaderCreateInfo{};
	ShaderCreateInfo.VertPath = "Assets/Shaders/shader_vert.spv";
	ShaderCreateInfo.FragPath = "Assets/Shaders/shader_frag.spv";
	ShaderCreateInfo.UseSwapChainColorFormat = false;
	ShaderCreateInfo.ColorFormat = Chilli::ImageFormat::RGBA8;

	auto ShaderHandle = Command.CreateGraphicsPipeline(ShaderCreateInfo);

	auto DeafultSamplerHandle = Command.CreateSampler({
		.Mode = Chilli::SamplerMode::REPEAT,
		.Filter = Chilli::SamplerFilter::LINEAR,
		});

	Chilli::ImageSpec ImgSpec{};
	auto DeafultTextureHandle = Command.CreateTexture(ImgSpec, "Assets/Textures/Deafult.png");

	auto DeafultMaterial = Command.AddMaterial(Chilli::Material{
		.AlbedoTextureHandle = DeafultTextureHandle,
		.AlbedoSamplerHandle = DeafultSamplerHandle,
		.AlbedoColor = {1.0f, 0.0f, 1.0f, 1.0f},
		.GraphicsPipelineId = ShaderHandle,
		});

	auto BlueMaterial = Command.AddMaterial(Chilli::Material{
		.AlbedoTextureHandle = DeafultTextureHandle,
		.AlbedoSamplerHandle = DeafultSamplerHandle,
		.AlbedoColor = {1.0f, 1.0f, 1.0f, 1.0f},
		.GraphicsPipelineId = ShaderHandle,
		});

	Ctxt.Registry->AddResource<GameResource	>();
	auto GameData = Ctxt.Registry->GetResource<GameResource>();

	GameData->Player = Command.CreateEntity();
	Command.AddComponent<Chilli::MeshComponent>(GameData->Player, Chilli::MeshComponent{
		.MeshHandle = SquareMeshID,
		.MaterialHandle = DeafultMaterial
		});
	Command.AddComponent<Chilli::TransformComponent>(GameData->Player, Chilli::TransformComponent{
		{0.0f, 0.0f, 0.0f},
		{1.0f},
		{0.0f, 0.0f, 0.0f},
		});

	GameData->Camera = Chilli::CameraBundle::Create3D(Ctxt, { 0, 0, 3.0f });

	auto Panel  = Command.CreateEntity();
	Command.AddComponent<Chilli::PepperElement>(Panel, Chilli::PepperElement());
	Command.AddComponent<Chilli::PepperTransform>(Panel, Chilli::PepperTransform{
		.PercentageDimensions = {10, 10},
		.PercentagePosition = {20, 20},
		.AnchorX = Chilli::AnchorX::CENTER,
		.AnchorY = Chilli::AnchorY::CENTER,
		.Anchor = {0,0},
		});

	auto Panel2 = Command.CreateEntity();
	Command.AddComponent<Chilli::PepperElement>(Panel2, Chilli::PepperElement());
	Command.AddComponent<Chilli::PepperTransform>(Panel2, Chilli::PepperTransform{
		.PercentageDimensions = {10, 10},
		.PercentagePosition = {70, 70},
		.AnchorX = Chilli::AnchorX::CENTER,
		.AnchorY = Chilli::AnchorY::CENTER,
		.Anchor = {0,0},
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
}

void OnGameShutDown(Chilli::BackBone::SystemContext& Ctxt)
{
	auto GameData = Ctxt.Registry->GetResource<GameResource>();
	auto Command = Chilli::Command(Ctxt);

	Command.DestroyEntity(GameData->Player);
	Command.DestroyEntity(GameData->Camera);
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
