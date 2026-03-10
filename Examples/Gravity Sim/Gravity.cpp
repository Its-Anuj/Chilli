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

	Command.AddLoader<Chilli::ImageLoader>();
	Command.RegisterStore<Chilli::RawMeshData>();
	Command.RegisterStore<Chilli::MeshLoaderData>();
	Command.AddLoader<Chilli::CGLFTMeshLoader>();
	Command.AddLoader<Chilli::TinyObjMeshLoader>();

	auto MeshLoaderData = Command.LoadMesh("Assets/Models/Monkey.obj");
	auto& MeshData = MeshLoaderData.ValPtr->Meshes[0];

	Chilli::VertexInputShaderLayout OurDesiredLayout;
	OurDesiredLayout.BeginBinding(0);
	OurDesiredLayout.AddAttribute(Chilli::ShaderObjectTypes::FLOAT3, "InPosition", int(Chilli::MeshAttribute::POSITION));
	OurDesiredLayout.AddAttribute(Chilli::ShaderObjectTypes::FLOAT3, "InNormal", int(Chilli::MeshAttribute::NORMAL));
	OurDesiredLayout.AddAttribute(Chilli::ShaderObjectTypes::FLOAT2, "InTexCoords", int(Chilli::MeshAttribute::TEXCOORD));
	OurDesiredLayout.AddAttribute(Chilli::ShaderObjectTypes::FLOAT3, "InColor", int(Chilli::MeshAttribute::COLOR));

	auto TorusMesh = Command.CreateMesh(MeshData, OurDesiredLayout);

	uint32_t WhiteColor = 0xFFFFFFFF;
	Chilli::ImageSpec ImageSpec{};
	ImageSpec.Format = Chilli::ImageFormat::RGBA8;
	ImageSpec.ImageData = &WhiteColor;
	ImageSpec.MipLevel = 1;
	ImageSpec.Resolution.Width = 1;
	ImageSpec.Resolution.Height = 1;
	ImageSpec.Resolution.Depth = 1;
	ImageSpec.Sample = Chilli::IMAGE_SAMPLE_COUNT_1_BIT;
	ImageSpec.State = Chilli::ResourceState::ShaderRead;
	ImageSpec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
	ImageSpec.Usage = Chilli::IMAGE_USAGE_SAMPLED_IMAGE;

	Data->Image = Command.AllocateImage(ImageSpec);
	Command.MapImageData(Data->Image, &WhiteColor, 1, 1);

	auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

	Chilli::TextureSpec TextureSpec;
	TextureSpec.Format = Chilli::ImageFormat::RGBA8;
	Data->Texture = Command.CreateTexture(Data->Image, TextureSpec);

	Data->EarthMaterial = Command.CreateMaterial(GeometryShaderProgram);
	MaterialSystem->SetAlbedoColor(Data->EarthMaterial, { 1.0f, 0.0f, 1.0f, 1.0f });
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
		{0.0f, 10.0f, 0} });
	Command.AddComponent<Chilli::MeshComponent>(Data->Earth, Chilli::MeshComponent{
		.MeshHandle = Data->SphereMesh,
		.MaterialHandle = Data->EarthMaterial
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

#define GRAVITATIONAL_CONSTANT 1

Chilli::Vec3 Normalize(Chilli::Vec3 Vector)
{
	float DistanceSq = (Vector.x * Vector.x) + (Vector.y * Vector.y) + (Vector.z * Vector.z);
	float Distance = sqrt(DistanceSq);
	return { Vector.x / Distance, Vector.y / Distance, Vector.z / Distance };
}

Chilli::Vec3 CalculateGravitationalForce(double MassA, double MassB, const Chilli::Vec3& BodyA, const Chilli::Vec3& BodyB)
{
	// 1. Get the directional difference (Vector from A pointing to B)
	Chilli::Vec3 Offset = BodyB - BodyA;

	// 2. Calculate Distance SQUARED
	float DistanceSq = (Offset.x * Offset.x) + (Offset.y * Offset.y) + (Offset.z * Offset.z);

	// Failsafe: Prevent division by zero if bodies perfectly overlap
	if (DistanceSq < 0.0001f) {
		return Chilli::Vec3{ 0.0f, 0.0f, 0.0f };
	}

	// 3. We DO need the actual distance now to "Normalize" the direction vector
	float Distance = sqrt(DistanceSq);

	// 4. Normalize the direction (make its length exactly 1.0)
	Chilli::Vec3 Direction = Normalize(Offset);

	// 5. Calculate scalar force magnitude
	float ForceMagnitude = (float)(GRAVITATIONAL_CONSTANT * MassA * MassB) / DistanceSq;

	// 6. Return the final Force Vector (Direction scaled by Force)
	return Chilli::Vec3{
		Direction.x * ForceMagnitude,
		Direction.y * ForceMagnitude,
		Direction.z * ForceMagnitude
	};
}

void SimulationUpdate(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	auto Data = Command.GetResource<Simulation>();
	auto FrameData = Command.GetResource<Chilli::BackBone::GenericFrameData>();
	auto dt = FrameData->FixedPhysicsData.Ticks;

	for (auto [Entity, Transform, BodyData] : Chilli::BackBone::QueryWithEntities<Chilli::TransformComponent, CelestialBody>(*Ctxt.Registry))
	{
		Transform->SetScale({ (float)BodyData->Radius * 2 });

		for (auto [OtherEntity, OtherTransform, OtherBodyData] : Chilli::BackBone::QueryWithEntities <Chilli::TransformComponent, CelestialBody>(*Ctxt.Registry))
		{
			if (Entity == OtherEntity)
				continue;
			auto ForceOnThis = CalculateGravitationalForce(BodyData->Mass, OtherBodyData->Mass, Transform->GetPosition(), OtherTransform->GetPosition());

			auto Acceleration = ForceOnThis / BodyData->Mass;
			BodyData->Velocity += {dt* Acceleration.x, dt* Acceleration.y, dt* Acceleration.z};
		}
	}
}

void PhysicsUpdate(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	auto FrameData = Command.GetResource<Chilli::BackBone::GenericFrameData>();
	auto dt = FrameData->FixedPhysicsData.Ticks;

	for (auto [Entity, Transform, BodyData] : Chilli::BackBone::QueryWithEntities<Chilli::TransformComponent, CelestialBody>(*Ctxt.Registry))
	{
		auto CurrentPos = Transform->GetPosition();

		// Position = OldPosition + (Velocity * dt)
		Transform->SetPosition(CurrentPos + (BodyData->Velocity * dt));
	}
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
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::SIMULATION, SimulationUpdate);
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::SIMULATION, PhysicsUpdate);
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::SHUTDOWN, SimulationShutDown);


	App.Run();
}
