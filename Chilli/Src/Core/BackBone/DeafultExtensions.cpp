#include "Ch_PCH.h"
#include "DeafultExtensions.h"
#include "Window/Window.h"
#include "Profiling\Timer.h"

namespace Chilli
{
	// Submit all mesh to be rendererd
	void OnRender(BackBone::SystemContext& Ctxt)
	{

	}

	// Submits the Frame and stops recording
	void OnRenderEnd(BackBone::SystemContext& Ctxt)
	{
	}

	void RenderSystem::OnCreate(BackBone::SystemContext& Registry)
	{
		CHILLI_DEBUG_TIMER("RenderSystem::OnCreate");

		GraphcisBackendCreateSpec Spec{};
		Spec.Type = GraphicsBackendType::VULKAN_1_3;
		Spec.Name = "Chilli";
		Spec.MaxFrameInFlight = 2;
		Spec.EnableValidation = true;
		Spec.DeviceFeatures.EnableDescriptorIndexing = true;
		Spec.DeviceFeatures.EnableSwapChain = true;

		Spec.ViewPortResized = true;
		Spec.VSync = true;
		Spec.MaxFrameInFlight = 2;
		_MaxFrameInFlight = 2;

		auto WindowStore = Registry.AssetRegistry.GetStore<Window>();
		for (auto [WindowComp] : BackBone::Query<WindowComponent>(Registry.Registry))
		{
			auto Win = WindowStore->Get(WindowComp->WindowHandle);
			if (Win->IsActive())
			{
				Spec.WindowSurfaceHandle = Win->GetRawHandle();
				Spec.ViewPortSize = { Win->GetWidth(), Win->GetHeight() };
				Spec.ViewPortResized = true;
			}
		}
		Renderer* RenderService = nullptr;

		{
			if (Spec.Type == GraphicsBackendType::VULKAN_1_3)
				CHILLI_DEBUG_TIMER("Renderer Initializing: VULKAN_1_3");

			Registry.ServiceRegistry.RegisterService<Renderer>(std::make_shared<Renderer>());
			RenderService = Registry.ServiceRegistry.GetService<Renderer>();
			RenderService->Init(Spec);
		}

		Registry.ServiceRegistry.RegisterService<RenderCommand>(RenderService->CreateRenderCommand());
		CH_CORE_TRACE("Graphcis Backend Using: {}", RenderService->GetName());
		auto RenderCommandService = Registry.ServiceRegistry.GetService<RenderCommand>();

		Registry.AssetRegistry.RegisterStore<Mesh>();
		Registry.AssetRegistry.RegisterStore<GraphicsPipeline>();

		RenderCommandService->AllocateCommandBuffers(CommandBufferPurpose::GRAPHICS, _CommandBuffers, 2);
	}

	void RenderSystem::Run(BackBone::SystemContext& Registry)
	{
		auto& ActiveCommandBuffer = _CommandBuffers[_CurrentFrameCount];
		// Begin Recording
		auto RenderCommandService = Registry.ServiceRegistry.GetService<RenderCommand>();
		auto RenderService = Registry.ServiceRegistry.GetService<Renderer>();

		RenderService->RenderBegin(ActiveCommandBuffer, _CurrentFrameCount);

		ColorAttachment DeafultColorAttachment;
		DeafultColorAttachment.ClearColor = { 0.4f, 0.5f, 1.0f, 1.0f };
		DeafultColorAttachment.UseSwapChainImage = true;

		RenderPass DeafultPass;
		DeafultPass.ColorAttachmentCount = 1;
		DeafultPass.pColorAttachments = &DeafultColorAttachment;

		RenderService->BeginRenderPass(DeafultPass);

		for (auto [MeshComp] : BackBone::Query<MeshComponent>(Registry.Registry))
		{
			RenderService->BindGraphicsPipeline(MeshComp->ShaderHandle.Handle);
			size_t VBs[] = { MeshComp->MeshHandle.Handle };
			RenderService->BindVertexBuffers(VBs, 1);
			RenderService->SetScissorSize(800, 600);
			RenderService->SetViewPortSize(800, 600);

			RenderService->DrawArrays(3);
		}

		RenderService->EndRenderPass();
		RenderService->RenderEnd(ActiveCommandBuffer);

		_CurrentFrameCount = (_CurrentFrameCount + 1) % _MaxFrameInFlight;
	}

	void RenderSystem::OnTerminate(BackBone::SystemContext& Registry)
	{
		auto RenderCommandService = Registry.ServiceRegistry.GetService<RenderCommand>();
		auto MeshStore = Registry.AssetRegistry.GetStore<Mesh>();
		auto RenderService = Registry.ServiceRegistry.GetService<Renderer>();

		for (auto [MeshComp] : BackBone::Query<MeshComponent>(Registry.Registry))
		{
			auto Mesh = MeshStore->Get(MeshComp->MeshHandle);
			MeshStore->Remove(MeshComp->MeshHandle);
			RenderCommandService->FreeBuffer(Mesh->VBHandle);
		}

		auto ShaderStore = Registry.AssetRegistry.GetStore<GraphicsPipeline>();
		for (auto& Shader : ShaderStore->Store)
		{
			RenderCommandService->DestroyGraphicsPipeline(Shader.PipelineHandle);
		}

		RenderCommandService->Terminate();
		RenderService->Terminate();
	}

	void DeafultExtension::Build(BackBone::App& App)
	{
		App.ServiceRegsitry.RegisterService<Chilli::Input>(std::make_shared<Chilli::Input>());
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE_BEGIN, std::make_unique<WindowSystem>(), App);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::RENDER_BEGIN, std::make_unique<RenderSystem>(), App);
		App.SystemScheduler.AddSystemFunction(BackBone::ScheduleTimer::RENDER, OnRender);
		App.SystemScheduler.AddSystemFunction(BackBone::ScheduleTimer::RENDER_END, OnRenderEnd);
	}

	void WindowSystem::OnCreate(BackBone::SystemContext& Registry)
	{
		Registry.AssetRegistry.RegisterStore<Window>();
		auto WindowStore = Registry.AssetRegistry.GetStore<Window>();
		auto InputService = Registry.ServiceRegistry.GetService<Input>();

		for (auto [WindowComp] : BackBone::Query<WindowComponent>(Registry.Registry))
		{
			auto WinHandle = WindowStore->Add(Window());
			WindowComp->WindowHandle = WinHandle;

			auto Win = WindowStore->Get(WinHandle);

			Win->Init(WindowComp->Spec);
			Win->SetEventCallback(WindowComp->EventCallback);
		}
	}

	void WindowSystem::OnBeforeRun(BackBone::SystemContext& Registry)
	{
	}

	void WindowSystem::Run(BackBone::SystemContext& Registry)
	{
		auto InputService = Registry.ServiceRegistry.GetService<Input>();

		for (auto [WindowComp] : BackBone::Query<WindowComponent>(Registry.Registry))
		{
			auto Win = WindowComp->WindowHandle.ValPtr;
			if (Win->IsActive()) {
				Win->PollEvents();
				InputService->SetActiveWindow(Win->GetRawHandle());
				break;
			}
		}

		if (InputService->GetKeyState(Input_key_W) == InputResult::INPUT_PRESS)
			CH_CORE_TRACE("Pressed W Once");
	}

	void WindowSystem::OnTerminate(BackBone::SystemContext& Registry)
	{
		auto WindowStore = Registry.AssetRegistry.GetStore<Window>();

		for (auto [WindowComp] : BackBone::Query<WindowComponent>(Registry.Registry))
		{
			WindowComp->WindowHandle.ValPtr->Terminate();
			WindowStore->Remove(WindowComp->WindowHandle);
		}
	}
}

