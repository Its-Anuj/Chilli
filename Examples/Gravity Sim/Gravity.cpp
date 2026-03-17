#include "Ch_PCH.h"
#include "Chilli/Chilli.h"
#include "Profiling\Timer.h"
#include "glm/ext/matrix_transform.hpp"

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

struct Simulation
{
	Chilli::BackBone::Entity Window;
	Chilli::BackBone::Entity Earth;
	Chilli::BackBone::Entity Moon;
	Chilli::BackBone::Entity Saturn;
	Chilli::BackBone::Entity Camera;
	Chilli::BackBone::AssetHandle<Chilli::Material> EarthMaterial;
	Chilli::BackBone::AssetHandle<Chilli::Material> MoonMaterial;
	Chilli::BackBone::AssetHandle<Chilli::Material> SaturnMaterial;
	Chilli::BackBone::AssetHandle<Chilli::Mesh> SphereMesh;
	Chilli::BackBone::AssetHandle<Chilli::Sampler> Sampler;
	Chilli::BackBone::AssetHandle<Chilli::ShaderProgram> Program;
	Chilli::BackBone::AssetHandle<Chilli::Image> Image;
	Chilli::BackBone::AssetHandle<Chilli::Texture> Texture;
	Chilli::Scene Scene;
};

struct CelestialBody
{
	double Mass = 0.0f;
	double Radius = 0.0f;
	Chilli::Vec3 Velocity{ 0.0f };
};

void OnWindowCreate(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);

	Command.AddResource<Simulation>();
	Command.RegisterComponent<CelestialBody>();

	Command.GetResource<Simulation>()->Window = Command.SpwanWindow(Chilli::WindowSpec{ "Gravity Sim", {800, 600} });
}

void SimulationSetup(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	auto Data = Command.GetResource<Simulation>();

	// Create Shaders
	auto GeometryPassVertexShader = Command.CreateShaderModule("Assets/Shaders/shader_vert.spv",
		Chilli::ShaderStageType::SHADER_STAGE_VERTEX);
	auto GeometryPassFragShader = Command.CreateShaderModule("Assets/Shaders/shader_frag.spv",
		Chilli::ShaderStageType::SHADER_STAGE_FRAGMENT);

	auto GeometryShaderProgram = Command.CreateShaderProgram();
	Command.AttachShaderModule(GeometryShaderProgram, GeometryPassVertexShader);
	Command.AttachShaderModule(GeometryShaderProgram, GeometryPassFragShader);
	Command.LinkShaderProgram(GeometryShaderProgram);

	Chilli::SamplerSpec SamplerSpec;
	SamplerSpec.Filter = Chilli::SamplerFilter::NEAREST;
	SamplerSpec.Mode = Chilli::SamplerMode::REPEAT;
	Data->Sampler = Command.CreateSampler(SamplerSpec);

	Chilli::VertexInputShaderLayout OurDesiredLayout;
	OurDesiredLayout.BeginBinding(0);
	OurDesiredLayout.AddAttribute(Chilli::ShaderObjectTypes::FLOAT3, "InPosition", int(Chilli::MeshAttribute::POSITION));
	OurDesiredLayout.AddAttribute(Chilli::ShaderObjectTypes::FLOAT3, "InNormal", int(Chilli::MeshAttribute::NORMAL));
	OurDesiredLayout.AddAttribute(Chilli::ShaderObjectTypes::FLOAT2, "InTexCoords", int(Chilli::MeshAttribute::TEXCOORD));
	OurDesiredLayout.AddAttribute(Chilli::ShaderObjectTypes::FLOAT3, "InColor", int(Chilli::MeshAttribute::COLOR));

	std::vector<Chilli::Vertex> Vertices;
	std::vector<uint32_t> Indices;
	Command.GenerateSphere(32, 32, Vertices, Indices);

	// 4. Clean up the generator

	Chilli::MeshCreateInfo Info{};
	Info.VertCount = Vertices.size();
	Info.Vertices = Vertices.data();
	Info.IndexCount = Indices.size();
	Info.Indicies = Indices.data();
	Info.IndexType = Chilli::IndexBufferType::UINT32_T;
	Info.MeshLayout = OurDesiredLayout;
	Info.IndexBufferState = Chilli::BufferState::STATIC_DRAW;

	auto TorusMesh = Command.CreateMesh(Info);

	uint32_t Width = 1;
	uint32_t Height = 1;

	uint32_t WhiteColor = 0xFFFFFFFF;
	Chilli::ImageSpec ImageSpec{};
	ImageSpec.Format = Chilli::ImageFormat::RGBA8;
	ImageSpec.ImageData = &WhiteColor;
	ImageSpec.MipLevel = 1;
	ImageSpec.Resolution.Width = Width;
	ImageSpec.Resolution.Height = Height;
	ImageSpec.Resolution.Depth = 1;
	ImageSpec.Sample = Chilli::IMAGE_SAMPLE_COUNT_1_BIT;
	ImageSpec.State = Chilli::ResourceState::ShaderRead;
	ImageSpec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
	ImageSpec.Usage = Chilli::IMAGE_USAGE_SAMPLED_IMAGE;

	Data->Image = Command.AllocateImage(ImageSpec);
	Command.MapImageData(Data->Image, &WhiteColor, Width, Height);

	auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

	Chilli::TextureSpec TextureSpec;
	TextureSpec.Format = Chilli::ImageFormat::RGBA8;
	Data->Texture = Command.CreateTexture(Data->Image, TextureSpec);

	Data->EarthMaterial = Command.CreateMaterial(GeometryShaderProgram);
	MaterialSystem->SetAlbedoColor(Data->EarthMaterial, { 1.0f, 1.0f, 1.0f, 1.0f });
	MaterialSystem->SetAlbedoTexture(Data->EarthMaterial, Data->Texture);
	MaterialSystem->SetAlbedoSampler(Data->EarthMaterial, Data->Sampler);

	Data->MoonMaterial = Command.CreateMaterial(GeometryShaderProgram);
	MaterialSystem->SetAlbedoColor(Data->MoonMaterial, { 0.0f, 0.0f, 1.0f, 1.0f });
	MaterialSystem->SetAlbedoTexture(Data->MoonMaterial, Data->Texture);
	MaterialSystem->SetAlbedoSampler(Data->MoonMaterial, Data->Sampler);

	Data->SaturnMaterial = Command.CreateMaterial(GeometryShaderProgram);
	MaterialSystem->SetAlbedoColor(Data->SaturnMaterial, { 0.0f, 1.0f, 1.0f, 1.0f });
	MaterialSystem->SetAlbedoTexture(Data->SaturnMaterial, Data->Texture);
	MaterialSystem->SetAlbedoSampler(Data->SaturnMaterial, Data->Sampler);

	Data->SphereMesh = Command.CreateMesh(Chilli::BasicShapes::SPHERE);

	Data->Earth = Command.CreateEntity();

	Command.AddComponent<CelestialBody>(Data->Earth, CelestialBody{
		.Mass = 10,
		.Radius = 1
		});
	Command.AddComponent<Chilli::TransformComponent>(Data->Earth, Chilli::TransformComponent{
		{0, 0, 0}, {10.0f, 10.0f, 10.0f}, {90.0f, 0.0f, 0.0f} });
	Command.AddComponent<Chilli::MeshComponent>(Data->Earth, Chilli::MeshComponent{
		.MeshHandle = TorusMesh,
		.MaterialHandle = Data->EarthMaterial
		});
	Command.AddComponent<Chilli::RigidBody>(Data->Earth, Chilli::RigidBody{
		.Mass = 1000.0f
		});
	Data->Moon = Command.CreateEntity();

	Command.AddComponent<CelestialBody>(Data->Moon, CelestialBody{
		.Mass = 2,
		.Radius = 0.2,
		.Velocity = {0, 0, 2}
		});
	Command.AddComponent<Chilli::TransformComponent>(Data->Moon, Chilli::TransformComponent{
		{10, 0.0f ,0.0f} });
	Command.AddComponent<Chilli::MeshComponent>(Data->Moon, Chilli::MeshComponent{
		.MeshHandle = Data->SphereMesh,
		.MaterialHandle = Data->MoonMaterial
		});

	Data->Saturn = Command.CreateEntity();

	Command.AddComponent<CelestialBody>(Data->Saturn, CelestialBody{
		.Mass = 1,
		.Radius = 0.1,
		.Velocity = {0, 0, 2}
		});
	Command.AddComponent<Chilli::TransformComponent>(Data->Saturn, Chilli::TransformComponent{
		{10.0f, 0.0f ,10.0f} });
	Command.AddComponent<Chilli::MeshComponent>(Data->Saturn, Chilli::MeshComponent{
		.MeshHandle = TorusMesh,
		.MaterialHandle = Data->SaturnMaterial
		});

	Data->Camera = Chilli::CameraBundle::Create3D(Ctxt, { 0.0f, 10.0f, 10 });
	Command.AddComponent<Chilli::Deafult3DCameraController>(Data->Camera, Chilli::Deafult3DCameraController());

	Data->Scene.MainCamera = Data->Camera;

	auto SceneManager = Command.GetService<Chilli::SceneManager>();
	SceneManager->SetActiveScene(&Data->Scene);
}

void SimulationShutDown(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	auto Data = Command.GetResource<Simulation>();
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
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::START_UP, SimulationSetup);
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::SHUTDOWN, SimulationShutDown);


	App.Run();
}
