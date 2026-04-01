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
	auto[Image, ImageData] = Command.AllocateImage("Assets/Textures/Deafult.png", Chilli::ImageFormat::RGBA8, Chilli::IMAGE_USAGE_SAMPLED_IMAGE, Chilli::ImageType::IMAGE_TYPE_2D, 1, true);

	Chilli::TextureSpec TexSpec{};
	TexSpec.Format = Chilli::ImageFormat::RGBA8;
	auto Texturre = Command.CreateTexture(Image, TexSpec);

	auto Index = Command.GetService<Chilli::Renderer>()->GetTextureShaderIndex(Texturre.ValPtr->RawTextureHandle);

	auto SquareMesh = Command.CreateMesh(Chilli::BasicShapes::TRIANGLE);
	auto Square = Command.CreateEntity();
	Command.AddComponent<Chilli::TransformComponent>(Square, {});
	Command.AddComponent<Chilli::MeshComponent>(Square, Chilli::MeshComponent{
		.MeshHandle = SquareMesh });
}

void InputTest(Chilli::BackBone::SystemContext& Ctxt)
{
	auto Command = Chilli::Command(Ctxt);
	if (Command.IsKeyPressed(Chilli::Input_key_T))
		CH_CORE_INFO("T");
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
