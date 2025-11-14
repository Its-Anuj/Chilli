#include "Ch_PCH.h"
#include "Chilli/Chilli.h"
#include "Profiling\Timer.h"

void EventCallBack(Chilli::Event& e)
{
}

void RenderPrintFunction(Chilli::BackBone::SystemContext& Ctxt)
{
}
int main()
{
	Chilli::Log::Init();

	Chilli::BackBone::App App;
	App.Registry.Register<Chilli::WindowComponent>();
	App.Registry.Register<Chilli::MeshComponent>();

	auto WindowEntity = App.Registry.Create();
	App.Registry.AddComponent<Chilli::WindowComponent>(WindowEntity, Chilli::WindowComponent{
		.Spec = Chilli::WindowSpec{"Chilli", {800, 600}},
		.EventCallback = EventCallBack
		});

	App.Extensions.AddExtension(std::make_unique<Chilli::DeafultExtension>(), true, &App);

	auto RenderCommandService = App.ServiceRegsitry.GetService<Chilli::RenderCommand>();
	auto MeshStore = App.AssetRegistry.GetStore<Chilli::Mesh>();

	Chilli::BufferCreateInfo VBInfo{};

	const float Vertices[9] = {
		0.0, -0.5, 0.0,
	0.5, 0.5,0.0,
	-0.5, 0.5,0.0
	};

	VBInfo.SizeInBytes = 9 * 4;
	VBInfo.Type = Chilli::BUFFER_TYPE_VERTEX;
	VBInfo.State = Chilli::BufferState::DYNAMIC_DRAW;
	VBInfo.Data = (void*)Vertices;

	auto VBHandle = RenderCommandService->AllocateBuffer(VBInfo);
	RenderCommandService->MapBufferData(VBHandle, (void*)Vertices, VBInfo.SizeInBytes, 0);

	Chilli::Mesh SquareMesh;
	SquareMesh.VBHandle = VBHandle;
	SquareMesh.Layout = { {Chilli::ShaderObjectTypes::FLOAT3, "InPosition"} };
	auto MeshID = MeshStore->Add(SquareMesh);

	Chilli::GraphicsPipelineCreateInfo ShaderCreateInfo{};
	ShaderCreateInfo.VertPath = "Assets/Shaders/shader_vert.spv";
	ShaderCreateInfo.FragPath = "Assets/Shaders/shader_frag.spv";
	ShaderCreateInfo.UseSwapChainColorFormat = true;

	auto ShaderStore = App.AssetRegistry.GetStore<Chilli::GraphicsPipeline>();
	auto ShaderHandle = ShaderStore->Add({ RenderCommandService->CreateGraphicsPipeline(ShaderCreateInfo) });

	auto SquareEntity = App.Registry.Create();
	App.Registry.AddComponent<Chilli::MeshComponent>(SquareEntity, Chilli::MeshComponent{
		.MeshHandle = MeshID,
		.ShaderHandle = ShaderHandle
		});

	App.SystemScheduler.AddSystemFunction(Chilli::BackBone::ScheduleTimer::RENDER, RenderPrintFunction);

	App.Run();

	App.Registry.Destroy(WindowEntity);
	App.Registry.Destroy(SquareEntity);
}
