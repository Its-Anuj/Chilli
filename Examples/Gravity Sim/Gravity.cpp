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
	Chilli::BackBone::Entity Slider;
	Chilli::BackBone::Entity Platform;
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

void OnButtonClick(Chilli::BackBone::SystemContext& Ctxt, Chilli::Event& e, Chilli::PepperEventTypes Type,
	Chilli::BackBone::Entity Entity)
{
	CH_CORE_INFO("Button Click");
}

void OnTextBoxEnter(Chilli::BackBone::SystemContext& Ctxt, Chilli::Event& e, Chilli::PepperEventTypes Type,
	Chilli::BackBone::Entity Entity)
{
	CH_CORE_INFO("TextBox Enter");
}

void OnCheckBox(Chilli::BackBone::SystemContext& Ctxt, Chilli::Event& e, Chilli::PepperEventTypes Type,
	Chilli::BackBone::Entity Entity)
{
	auto Command = Chilli::Command(Ctxt);
	auto Pepper = Chilli::Pepper(Ctxt);
	auto PepperElement = Command.GetComponent<Chilli::PepperElement>(Entity);
	auto CheckBox = Command.GetComponent<Chilli::CheckBoxComponent>(Entity);

	if (CheckBox->State == true)
	{
		Pepper.SetMaterialColor(Entity, { 0.0f, 0.0f, 0.0f, 1.0f });
	}
	else
	{
		Pepper.SetMaterialColor(Entity, { 1.0f, 0.0f, 0.0f, 1.0f });
	}
}

void CreateCheckBox(Chilli::BackBone::SystemContext& Ctxt, Chilli::PepperActionKey Key,
	const Chilli::PepperActionCallBackFn& CallBack)
{
	auto Command = Chilli::Command(Ctxt);
	auto PepperResource = Command.GetResource<Chilli::PepperResource>();

	auto Pepper = Chilli::Pepper(Ctxt);
	auto CheckBox = Command.CreateEntity();

	Chilli::PepperTransform panelTransform;

	panelTransform.PercentageDimensions = { 0.05f, 0.08f };

	// Position: Top-right corner
	panelTransform.PercentagePosition = { 0.55f, 0.5f };

	panelTransform.AnchorX = Chilli::AnchorX::CENTER;  // Align right
	panelTransform.AnchorY = Chilli::AnchorY::TOP;    // Align top

	Chilli::PepperElement PepperElement;
	PepperElement.Flags = Chilli::PEPPER_ELEMENT_VISIBLE;
	strncpy(PepperElement.Name, "CheckBox", 32);
	PepperElement.Type = Chilli::PepperElementTypes::BUTTON;

	Command.AddComponent(CheckBox, panelTransform);
	Command.AddComponent(CheckBox, PepperElement);
	Command.AddComponent(CheckBox, Chilli::InteractionState());
	Command.AddComponent(CheckBox, Chilli::CheckBoxComponent{
		.OnStateChange = Pepper.RegisterAction(Key, CallBack),
		});
	Pepper.SetMaterialTexture(CheckBox, PepperResource->SliderCircleTexture);
}

void CreateTextBox(Chilli::BackBone::SystemContext& Ctxt, Chilli::PepperActionKey Key,
	const Chilli::PepperActionCallBackFn& CallBack,
	Chilli::BackBone::AssetHandle<Chilli::FlameFont> Font)
{
	auto Command = Chilli::Command(Ctxt);

	auto Pepper = Chilli::Pepper(Ctxt);
	auto TextBox = Command.CreateEntity();
	auto TextRenderContext = Command.CreateEntity();

	Chilli::PepperTransform panelTransform;

	panelTransform.PercentageDimensions = { 0.2f, 0.04f };

	// Position: Top-right corner
	panelTransform.PercentagePosition = { 0.55f, 0.5f };

	panelTransform.AnchorX = Chilli::AnchorX::CENTER;  // Align right
	panelTransform.AnchorY = Chilli::AnchorY::TOP;    // Align top

	Chilli::PepperElement PepperElement;
	PepperElement.Flags = Chilli::PEPPER_ELEMENT_VISIBLE;
	strncpy(PepperElement.Name, "CheckBox", 32);
	PepperElement.Type = Chilli::PepperElementTypes::PANEL;
	panelTransform.ZOrder = 0;

	Chilli::FlameTextComponent TextComp;
	TextComp.Font = Font;

	Command.AddComponent(TextRenderContext, panelTransform);
	Command.AddComponent(TextRenderContext, TextComp);

	Command.AddComponent(TextBox, panelTransform);
	Command.AddComponent(TextBox, PepperElement);
	Command.AddComponent(TextBox, Chilli::InteractionState());
	Command.AddComponent(TextBox, Chilli::TextBoxComponent{
		.OnSubmit = Pepper.RegisterAction("OnTextBoxEnter",OnTextBoxEnter),
		.TextRenderContext = TextRenderContext
		});
	Pepper.SetMaterialColor(TextBox, { 0.0f,0.0f, 1.0f, 1.0f });
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

	SimulationResource->Camera = Chilli::CameraBundle::Create3D(Ctxt, { 0,0, 5 });
	Command.AddComponent(SimulationResource->Camera, Chilli::Deafult3DCameraController());
	Command.SetParentEntity(SimulationResource->Camera, Square);

	auto Pepper = Chilli::Pepper(Ctxt);
	auto Button = Command.CreateEntity();

	Chilli::PepperTransform panelTransform;

	// Size: 20% width, 20% height (example)
	panelTransform.PercentageDimensions = { 0.1f, 0.05f };

	// Position: Top-right corner
	panelTransform.PercentagePosition = { 0.75f, 0.3f };

	panelTransform.AnchorX = Chilli::AnchorX::RIGHT;  // Align right
	panelTransform.AnchorY = Chilli::AnchorY::TOP;    // Align top

	Chilli::PepperElement PepperElement;
	PepperElement.Flags = Chilli::PEPPER_ELEMENT_VISIBLE;
	strncpy(PepperElement.Name, "Button", 32);
	PepperElement.Type = Chilli::PepperElementTypes::BUTTON;

	Command.AddComponent(Button, panelTransform);
	Command.AddComponent(Button, PepperElement);
	Command.AddComponent(Button, Chilli::InteractionState());
	Command.AddComponent(Button, Chilli::ButtonComponent{
		.OnClick = Pepper.RegisterAction("OnButtonClick", OnButtonClick)
		});

	auto ActiveScene = Command.CreateScene();
	ActiveScene.ValPtr->Name = "ActiveScene";
	ActiveScene.ValPtr->MainCamera = SimulationResource->Camera;
	Command.SetActiveScene(ActiveScene);

	//SimulationResource->Slider = Pepper.CreateSlider(Ctxt, "OnSliderChange", OnSliderChanged,
	// Chilli::BackBone::npos);

	CreateCheckBox(Ctxt, "OnCheckBox", OnCheckBox);
	auto Font = Command.LoadFont_Msdf("Assets/Fonts/Alkia.msdf", "Assets/Fonts/Alkia.png");
	//CreateTextBox(Ctxt, "OnCheckBox", OnCheckBox, Font);

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
