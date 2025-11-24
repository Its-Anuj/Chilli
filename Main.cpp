#include "Ch_PCH.h"
#include "Chilli/Chilli.h"
#include "Profiling\Timer.h"

struct GameResource
{
	Chilli::BackBone::Entity Player;
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
		const std::vector<float> Vertices = {
			// x      y      z      u    v

			// Triangle 1
			-0.5f, -0.5f, 0.0f,    0.0f, 0.0f,   // bottom-left
			 0.5f, -0.5f, 0.0f,    1.0f, 0.0f,   // bottom-right
			 0.5f,  0.5f, 0.0f,    1.0f, 1.0f,   // top-right

			 // Triangle 2
			  0.5f,  0.5f, 0.0f,    1.0f, 1.0f,   // top-right
			 -0.5f,  0.5f, 0.0f,    0.0f, 1.0f,   // top-left
			 -0.5f, -0.5f, 0.0f,    0.0f, 0.0f    // bottom-left
		};

		Chilli::BufferCreateInfo VBInfo{};
		VBInfo.SizeInBytes = Vertices.size() * sizeof(float);
		VBInfo.Type = Chilli::BUFFER_TYPE_VERTEX;
		VBInfo.State = Chilli::BufferState::STATIC_DRAW;
		VBInfo.Data = (void*)Vertices.data();
		VBInfo.Count = Vertices.size();

		Chilli::MeshCreateInfo SquareInfo{};
		SquareInfo.VBs.push_back(VBInfo);
		SquareInfo.Layout.push_back({ Chilli::ShaderObjectTypes::FLOAT3, "InPosition" });
		SquareInfo.Layout.push_back({ Chilli::ShaderObjectTypes::FLOAT2, "InTexCoord" });
		auto SquareMesh = Command.CreateMesh(SquareInfo);
		SquareMeshID = MeshStore->Add(SquareMesh);
	}

	Chilli::GraphicsPipelineCreateInfo ShaderCreateInfo{};
	ShaderCreateInfo.VertPath = "Assets/Shaders/shader_vert.spv";
	ShaderCreateInfo.FragPath = "Assets/Shaders/shader_frag.spv";
	ShaderCreateInfo.UseSwapChainColorFormat = true;

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
		.Position = {0.0f, 0.0f, 0.0f},
		.Scale = {1.0f},
		.Rotation = {0.0f, 0.0f, 0.0f},
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
}

int main()
{
	Chilli::Log::Init();

	Chilli::BackBone::App App;

	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::START_UP, OnWindowCreate);
	App.Extensions.AddExtension(std::make_unique<Chilli::DeafultExtension>(), true, &App);
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::START_UP, OnGameCreate);

	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::UPDATE, OnGamePlay);
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::SHUTDOWN, OnGameShutDown);

	App.Run();
}
