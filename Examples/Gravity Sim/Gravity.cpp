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

void SimulationCameraUpdate(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	auto Data = Command.GetResource<Simulation>();

	Chilli::CameraBundle::Update3DCamera(Data->Camera, Ctxt);
}

void SimulationSetup(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	auto Data = Command.GetResource<Simulation>();

	auto CubeMesh = Command.CreateMesh(Chilli::BasicShapes::CUBE);
	auto SphereMesh = Command.CreateMesh(Chilli::BasicShapes::SPHERE);
	auto ConeMesh = Command.CreateMesh(Chilli::BasicShapes::CONE);

	auto Cone = Command.CreateEntity();
	Command.AddComponent<Chilli::TransformComponent>(Cone, Chilli::TransformComponent{});

	Command.AddComponent<Chilli::MeshComponent>(Cone, Chilli::MeshComponent{
		.MeshHandle = ConeMesh
		});

	Data->Cube = Command.CreateEntity();
	Command.AddComponent<Chilli::TransformComponent>(Data->Cube, Chilli::TransformComponent{});

	Chilli::RigidBody CubeRigidBody;
	CubeRigidBody.GravityFactor = 1.0f;
	CubeRigidBody.Mass = 10.0f;
	CubeRigidBody.Layer = Chilli::Layers::DYNAMIC;
	CubeRigidBody.MotionType = Chilli::MotionType::DYNAMIC;
	CubeRigidBody.Velocity = { 0, -0, 0 };
	CubeRigidBody.Restitution = 0.8f;

	Command.AddComponent<Chilli::RigidBody>(Data->Cube, CubeRigidBody);

	Chilli::Collider CubeCollider;
	CubeCollider.Type = Chilli::ColliderType::SPHERE;
	CubeCollider.IsTrigger = false;
	CubeCollider.Shape.Sphere.Radius = 0.5f;

	Command.AddComponent<Chilli::Collider>(Data->Cube, CubeCollider);

	Command.AddComponent<Chilli::MeshComponent>(Data->Cube, Chilli::MeshComponent{
		.MeshHandle = SphereMesh
		});

	Data->Ground = Command.CreateEntity();
	Command.AddComponent<Chilli::TransformComponent>(Data->Ground, Chilli::TransformComponent{
		{0.0f, -5.0f, 0.0f}, {10.0f, 1.0f, 10.0f } });

	Command.AddComponent<Chilli::MeshComponent>(Data->Ground, Chilli::MeshComponent{
		.MeshHandle = CubeMesh
		});

	Chilli::RigidBody PlatformRigidBody;
	PlatformRigidBody.GravityFactor = 1.0f;
	PlatformRigidBody.Mass = 10.0f;
	PlatformRigidBody.Layer = Chilli::Layers::STATIC;
	PlatformRigidBody.MotionType = Chilli::MotionType::STATIC;

	Command.AddComponent<Chilli::RigidBody>(Data->Ground, PlatformRigidBody);

	Chilli::Collider PlatformCollider;
	PlatformCollider.Type = Chilli::ColliderType::BOX;
	PlatformCollider.IsTrigger = false;
	PlatformCollider.Shape.AABB.HalfExtent = { 5, 0.5f, 5 };

	Command.AddComponent<Chilli::Collider>(Data->Ground, PlatformCollider);

	Data->Camera = Chilli::CameraBundle::Create3D(Ctxt, { 0.0f, 0.0f, 5 });
	Command.AddComponent<Chilli::Deafult3DCameraController>(Data->Camera, Chilli::Deafult3DCameraController());

	Data->Scene.MainCamera = Data->Camera;

	auto SceneManager = Command.GetService<Chilli::SceneManager>();
	SceneManager->SetActiveScene(&Data->Scene);
}

void OnCubeMovement(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	auto Data = Command.GetResource<Simulation>();

	auto PlayerRigidBody = Command.GetComponent<Chilli::RigidBody>(Data->Cube);
	if (Command.GetService<Chilli::Input>()->IsKeyDown(Chilli::Input_key_P))
		PlayerRigidBody->AddImpulse({ 0.0f, 0.0f, 20.0f });
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

	Chilli::DeafultExtensionConfig DeafultConfig;
	DeafultConfig.RenderConfig = RenderConfig;

	App.Extensions.AddExtension(std::make_unique<Chilli::DeafultExtension>(DeafultConfig), true, &App);
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::START_UP, SimulationSetup);
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::FIXED_PHYSICS, OnCubeMovement);
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::UPDATE, SimulationCameraUpdate);
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::SHUTDOWN, SimulationShutDown);


	App.Run();
}
