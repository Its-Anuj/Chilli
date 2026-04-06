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
	Chilli::BackBone::Entity Square;
	Chilli::BackBone::Entity Plane;
	Chilli::BackBone::Entity Camera;
	Chilli::BackBone::AssetHandle<Chilli::Material> OurMaterial;
	Chilli::Scene Scene;
};

struct HealthComponent
{
	uint32_t MaxHealth = 100;
	uint32_t CurrentHealth = 0;
};

struct AttackComponent
{
	uint32_t Damage = 10;
};

void OnWindowCreate(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);

	Command.AddResource<Simulation>();

	Command.GetResource<Simulation>()->Window = Command.SpwanWindow(Chilli::WindowSpec{ "Gravity Sim", {800, 600} });
}

void OnStartUp(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	auto SimulationResource = Command.GetResource<Simulation>();

	auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
	auto RenderResource = Command.GetResource<Chilli::RenderResource>();

	auto	OurMaterial = MaterialSystem->CreateMaterial(RenderResource->DeafultShaderProgram);
	MaterialSystem->CopyMaterial(RenderResource->DeafultMaterial, OurMaterial);
	MaterialSystem->SetAlbedoColor(OurMaterial, { 1.0f, 		0.0f, 0.0f, 0.0f });

	auto SquareMesh = Command.CreateMesh(Chilli::BasicShapes::CUBE);

	Chilli::RigidBody PlaneRigidBody;
	PlaneRigidBody.Mass = 10.0f;
	PlaneRigidBody.Layer = Chilli::Layers::STATIC;
	PlaneRigidBody.MotionType = Chilli::MotionType::STATIC;
	PlaneRigidBody.GravityFactor = 0.0f;

	Chilli::Collider PlaneCollider;
	PlaneCollider.IsTrigger = false;
	PlaneCollider.Shape.AABB.HalfExtent = { 5, 0.5, 5 };
	PlaneCollider.Type = Chilli::ColliderType::BOX;

	auto Plane = Command.CreateEntity();
	Command.AddComponent<Chilli::TransformComponent>(Plane, {
		{0.0f, -5.0f, 0.0f}, {10.0f, 1.0f, 10.0f} });

	Command.AddComponent(Plane, PlaneRigidBody);
	Command.AddComponent(Plane, PlaneCollider);

	Command.AddComponent<Chilli::MeshComponent>(Plane, Chilli::MeshComponent{
		.MeshHandle = SquareMesh,
		.MaterialHandle = OurMaterial });

	auto Square = Command.CreateEntity();

	Chilli::RigidBody SquareRigidBody;
	SquareRigidBody.Mass = 10.0f;
	SquareRigidBody.Layer = Chilli::Layers::DYNAMIC;
	SquareRigidBody.MotionType = Chilli::MotionType::DYNAMIC;
	SquareRigidBody.GravityFactor = 1.0f;

	Chilli::Collider SquareCollider;
	SquareCollider.IsTrigger = false;
	SquareCollider.Shape.AABB.HalfExtent = { 0.5, 0.5, 0.5 };
	SquareCollider.Type = Chilli::ColliderType::BOX;

	Command.AddComponent(Square, SquareRigidBody);
	Command.AddComponent(Square, SquareCollider);

	Command.AddComponent<Chilli::TransformComponent>(Square, {});
	Command.AddComponent<Chilli::MeshComponent>(Square, Chilli::MeshComponent{
		.MeshHandle = SquareMesh,
		.MaterialHandle = OurMaterial });

	SimulationResource->Camera = Chilli::CameraBundle::Create3D(Ctxt, {0,0, 5});
	Command.AddComponent(SimulationResource->Camera, Chilli::Deafult3DCameraController());

	auto ActiveScene = Command.CreateScene();
	ActiveScene.ValPtr->Name = "ActiveScene";
	ActiveScene.ValPtr->MainCamera = SimulationResource->Camera;
	Command.SetActiveScene(ActiveScene);

	SimulationResource->Square = Square;
	SimulationResource->Plane = Plane;
	SimulationResource->OurMaterial = OurMaterial;
}

void InputTest(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	auto SimulationResource = Command.GetResource<Simulation>();
	auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
	Chilli::CameraBundle::Update3DCamera(SimulationResource->Camera, Ctxt);

}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Chilli::Log::Init();
	CH_CORE_INFO("LOG");

	Chilli::BackBone::App App;

	Chilli::RenderExtensionConfig RenderConfig{};
	RenderConfig.Spec.VSync = true;
	RenderConfig.Spec.EnableValidation = true;

	App.SystemScheduler.AddSystemOverLayBefore(Chilli::BackBone::ScheduleTimer::START_UP, OnWindowCreate);

	Chilli::DeafultExtensionConfig DeafultConfig;
	DeafultConfig.RenderConfig = RenderConfig;

	App.Extensions.AddExtension(std::make_unique<Chilli::DeafultExtension>(DeafultConfig), true, &App);
	App.SystemScheduler.AddSystemOverLayBefore(Chilli::BackBone::ScheduleTimer::UPDATE, InputTest);
	App.SystemScheduler.AddSystem(Chilli::BackBone::ScheduleTimer::START_UP, OnStartUp);
	CH_CORE_INFO("LOG");

	App.Run();

	std::cin.get();
	return true;
}
