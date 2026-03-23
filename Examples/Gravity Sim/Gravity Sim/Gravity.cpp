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
	Chilli::BackBone::Entity Cube;
	Chilli::BackBone::Entity Ground;
	Chilli::BackBone::Entity Camera;
	Chilli::BackBone::AssetHandle<Chilli::Mesh> SphereMesh;
	Chilli::Scene Scene;
};

void OnWindowCreate(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);

	Command.AddResource<Simulation>();

	Command.GetResource<Simulation>()->Window = Command.SpwanWindow(Chilli::WindowSpec{ "Gravity Sim", {800, 600} });
}

void SimulationSetup(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	auto Data = Command.GetResource<Simulation>();

	auto CubeMesh = Command.CreateMesh(Chilli::BasicShapes::CUBE);

	Data->Cube = Command.CreateEntity();
	Command.AddComponent<Chilli::TransformComponent>(Data->Cube, Chilli::TransformComponent{});
	Command.AddComponent<Chilli::RigidBody>(Data->Cube, Chilli::RigidBody{
		.Mass = 1,
		.IsStatic = false 
		});
	Command.AddComponent<Chilli::MeshComponent>(Data->Cube, Chilli::MeshComponent{
		.MeshHandle = CubeMesh
		});

	Data->Ground = Command.CreateEntity();
	Command.AddComponent<Chilli::TransformComponent>(Data->Ground, Chilli::TransformComponent{
		{0.0f, -5.0f, 0.0f}, {10.0f, 0.2f, 10.0f } });

	Command.AddComponent<Chilli::RigidBody>(Data->Ground, Chilli::RigidBody{
		.Mass = 10.0f,
		.IsStatic = true
		});
	Command.AddComponent<Chilli::MeshComponent>(Data->Ground, Chilli::MeshComponent{
		.MeshHandle = CubeMesh
		});

	Chilli::Collider CubeCollider;
	CubeCollider.Type = Chilli::ColliderType::BOX;
	CubeCollider.IsTrigger = false;
	CubeCollider.Box = Chilli::AABB::FromCenter(
		Chilli::Vec3{ 0.0f, 0.0f, 0.0f },   // center — matches transform position
		Chilli::Vec3{ 0.5f, 0.5f, 0.5f }    // half extents — 1x1x1 cube
	);

	// Platform collider — wide and flat, centered at origin
	Chilli::Collider PlatformCollider;
	PlatformCollider.Type = Chilli::ColliderType::BOX;
	PlatformCollider.IsTrigger = false;
	PlatformCollider.Box = Chilli::AABB::FromCenter(
		Chilli::Vec3{ 0.0f, 0.0f, 0.0f },   // center — matches transform position
		Chilli::Vec3{ 5.0f, 0.1f, 5.0f }    // half extents — 10 wide, 0.2 tall, 10 deep
	);
	Command.AddComponent<Chilli::Collider>(Data->Cube, CubeCollider);
	Command.AddComponent<Chilli::Collider>(Data->Ground, PlatformCollider);

	Data->Camera = Chilli::CameraBundle::Create3D(Ctxt, { 0.0f, 0.0f, 5 });
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
