#include "DeafultExtensions.h"
#include "DeafultExtensions.h"
#include "DeafultExtensions.h"
#include "DeafultExtensions.h"
#include "DeafultExtensions.h"
#include "Ch_PCH.h"
#include "DeafultExtensions.h"
#include "Window/Window.h"
#include "Profiling\Timer.h"

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Chilli
{
#pragma region Render System
	// Submit all mesh to be rendererd
	void OnRender(BackBone::SystemContext& Ctxt)
	{
		auto RenderInfo = Ctxt.Registry->GetResource< RenderResource>();
		if (RenderInfo->ContinueRender == false) return;
		//CHILLI_DEBUG_TIMER("OnRender::");

		ColorAttachment DeafultColorAttachment;
		DeafultColorAttachment.ClearColor = { 0.4f, 0.5f, 1.0f, 1.0f };
		DeafultColorAttachment.UseSwapChainImage = true;

		RenderPass DeafultPass;
		DeafultPass.ColorAttachmentCount = 1;
		DeafultPass.pColorAttachments = &DeafultColorAttachment;
		auto RenderService = Ctxt.ServiceRegistry->GetService<Renderer>();

		SceneShaderData SceneData{};
		SceneData.CameraPos = { 1.0f, 1.0f, 0.8f, 1.0f };
		RenderService->UpdateSceneShaderData(SceneData);

		RenderService->BeginRenderPass(DeafultPass);
		uint32_t ActivePipelineHandle = BackBone::npos;
		auto MeshStore = Ctxt.AssetRegistry->GetStore<Mesh>();

		for (auto [EntityID, TransformComp, MeshComp] : BackBone::QueryWithEntities<TransformComponent, MeshComponent>(*Ctxt.Registry))
		{
			auto ActivePipeline = MeshComp->MaterialHandle.ValPtr->GraphicsPipelineId;
			if (ActivePipelineHandle != ActivePipeline.Handle)
			{
				RenderService->BindGraphicsPipeline(ActivePipeline.ValPtr->PipelineHandle);
				RenderService->SetScissorSize(true);
				RenderService->SetViewPortSize(true);
				RenderInfo->RenderDebugInfo.PipelineCount++;
				ActivePipelineHandle = ActivePipeline.Handle;
			}

			struct
			{
				uint32_t ObjectIndex;
				uint32_t MaterialIndex;
			} PushConstantData;

			// Create the model matrix
			glm::mat4 modelMatrix = glm::mat4(1.0f); // Start with identity matrix
			modelMatrix = glm::translate(modelMatrix, ToGlmVec3(TransformComp->Position)); // Apply translation
			modelMatrix = glm::rotate(modelMatrix, glm::radians(TransformComp->Rotation.x),
				glm::vec3(1.0f, 0.0f, 0.0f)); // Apply rotation (convert degrees to radians)
			modelMatrix = glm::rotate(modelMatrix, glm::radians(TransformComp->Rotation.y),
				glm::vec3(0.0f, 1.0f, 0.0f)); // Apply rotation (convert degrees to radians)
			modelMatrix = glm::rotate(modelMatrix, glm::radians(TransformComp->Rotation.z),
				glm::vec3(0.0f, 0.0f, 1.0f)); // Apply rotation (convert degrees to radians)
			modelMatrix = glm::scale(modelMatrix, ToGlmVec3(TransformComp->Scale)); // Apply scaling
			RenderService->UpdateObjectSSBO(modelMatrix, EntityID);

			PushConstantData.MaterialIndex = *RenderService->GetMap(BindlessSetTypes::MATERIAl).Get(
				MeshComp->MaterialHandle.Handle
			);

			PushConstantData.ObjectIndex = *RenderService->GetMap(BindlessSetTypes::PER_OBJECT).Get(
				EntityID
			);

			RenderService->SendPushConstant(ShaderStageType::VERTEX, (void*)&PushConstantData,
				sizeof(PushConstantData), 0);

			uint32_t VBs[] = { MeshComp->MeshHandle.ValPtr->VBHandle };
			RenderService->BindVertexBuffers(VBs, 1);
			RenderService->DrawArrays(MeshComp->MeshHandle.ValPtr->VertexCount);

			RenderInfo->RenderDebugInfo.DrawCallsCount++;
		}

		RenderService->EndRenderPass();
	}

	// Submits the Frame and stops recording
	void OnRenderEnd(BackBone::SystemContext& Ctxt)
	{
		auto RenderInfo = Ctxt.Registry->GetResource< RenderResource>();
		if (RenderInfo->ContinueRender == false) return;
		//CHILLI_DEBUG_TIMER("OnRenderEnd::");

		auto RenderService = Ctxt.ServiceRegistry->GetService<Renderer>();
		RenderService->RenderEnd();
	}

	void OnRenderStartUp(BackBone::SystemContext& Ctxt)
	{
		CHILLI_DEBUG_TIMER("RenderSystem::OnCreate");

		Ctxt.Registry->AddResource< RenderResource>();
		auto RenderInfo = Ctxt.Registry->GetResource< RenderResource>();

		GraphcisBackendCreateSpec Spec{};
		Spec.Type = GraphicsBackendType::VULKAN_1_3;
		Spec.Name = "Chilli";
		Spec.EnableValidation = true;
		Spec.DeviceFeatures.EnableDescriptorIndexing = true;
		Spec.DeviceFeatures.EnableSwapChain = true;

		Spec.VSync = true;
		RenderInfo->MaxFrameInFlight = 3;
		Spec.MaxFrameInFlight = RenderInfo->MaxFrameInFlight;

		auto WindowStore = Ctxt.AssetRegistry->GetStore<Window>();

		auto WindowService = Ctxt.ServiceRegistry->GetService<WindowManager>();
		auto Win = WindowService->GetActiveWindow();

		Spec.WindowSurfaceHandle = Win->GetRawHandle();
		Spec.ViewPortSize = { Win->GetWidth(), Win->GetHeight() };
		Spec.ViewPortResized = true;

		Renderer* RenderService = nullptr;

		{
			if (Spec.Type == GraphicsBackendType::VULKAN_1_3)
				CHILLI_DEBUG_TIMER("Renderer Initializing: VULKAN_1_3");

			Ctxt.ServiceRegistry->RegisterService<Renderer>(std::make_shared<Renderer>());
			RenderService = Ctxt.ServiceRegistry->GetService<Renderer>();
			RenderService->Init(Spec);
		}

		Ctxt.ServiceRegistry->RegisterService<RenderCommand>(RenderService->CreateRenderCommand());
		CH_CORE_TRACE("Graphcis Backend Using: {}", RenderService->GetName());
		auto RenderCommandService = Ctxt.ServiceRegistry->GetService<RenderCommand>();

		RenderCommandService->AllocateCommandBuffers(CommandBufferPurpose::GRAPHICS,
			RenderInfo->CommandBuffers, RenderInfo->MaxFrameInFlight);
	}

	void OnRenderBefore(BackBone::SystemContext& Ctxt)
	{
		// Begin Recording
		auto RenderCommandService = Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto RenderService = Ctxt.ServiceRegistry->GetService<Renderer>();
		auto EventService = Ctxt.ServiceRegistry->GetService<EventHandler>();

		auto Read = EventService->GetEventStorage< FrameBufferResizeEvent>();
		auto RenderInfo = Ctxt.Registry->GetResource< RenderResource>();

		// On Frame Buffer ReSize Our Event system writes FrameBufferResizeEvent every frame and this tores it
		if (Read->ActiveSize > 0)
		{
			for (auto Resize : *Read)
			{
				RenderInfo->FrameBufferSize = Resize;
				RenderInfo->FrameBufferReSized = true;
			}
		}
		// This makes sure we only use the last FrameBufferResizeEvent to Resize the SwapChain
		else if (Read->ActiveSize == 0 && RenderInfo->FrameBufferReSized)
		{
			RenderService->FrameBufferResize(RenderInfo->FrameBufferSize.GetX(),
				RenderInfo->FrameBufferSize.GetY());
			RenderInfo->FrameBufferReSized = false;
		}

		auto& ActiveCommandBuffer = RenderInfo->CommandBuffers[RenderInfo->CurrentFrameCount];

		RenderInfo->ContinueRender = RenderService->RenderBegin(ActiveCommandBuffer,
			RenderInfo->CurrentFrameCount);
		RenderInfo->ActiveCommandBuffer = ActiveCommandBuffer;

		GlobalShaderData GlobalData{};
		GlobalData.ResolutionTime = { 800, 600 ,10, 0 };
		RenderService->UpdateGlobalShaderData(GlobalData);

		RenderInfo->CurrentFrameCount = (RenderInfo->CurrentFrameCount + 1) % RenderInfo->MaxFrameInFlight;
	}

	void OnRenderShutDown(BackBone::SystemContext& Registry)
	{
		auto RenderCommandService = Registry.ServiceRegistry->GetService<RenderCommand>();
		auto MeshStore = Registry.AssetRegistry->GetStore<Mesh>();
		auto RenderService = Registry.ServiceRegistry->GetService<Renderer>();
		auto TextureStore = Registry.AssetRegistry->GetStore<Texture>();
		auto SamplerStore = Registry.AssetRegistry->GetStore<Sampler>();

		for (auto& Mesh : *MeshStore)
			RenderCommandService->FreeBuffer(Mesh->VBHandle);

		for (auto& Texture : *TextureStore)
			RenderCommandService->FreeTexture(Texture->RawTextureHandle);

		for (auto& Sampler : *SamplerStore)
			RenderCommandService->DestroySampler(Sampler->SamplreHandle);

		auto ShaderStore = Registry.AssetRegistry->GetStore<GraphicsPipeline>();
		for (auto& Shader : *ShaderStore)
		{
			RenderCommandService->DestroyGraphicsPipeline(Shader->PipelineHandle);
		}

		RenderCommandService->Terminate();
		RenderService->Terminate();
	}
#pragma endregion 

#pragma region Window System
	void OnWindowStartUp(BackBone::SystemContext& Registry)
	{
		auto InputService = Registry.ServiceRegistry->GetService<Input>();
		auto EventService = Registry.ServiceRegistry->GetService<EventHandler>();

		{
			EventService->Register<WindowResizeEvent>();
			EventService->Register<WindowMinimizedEvent>();
			EventService->Register<WindowCloseEvent>();
			EventService->Register<FrameBufferResizeEvent>();
			EventService->Register<WindowCloseEvent>();
			EventService->Register<KeyPressedEvent>();
			EventService->Register<KeyRepeatEvent>();
			EventService->Register<KeyReleasedEvent>();
			EventService->Register<MouseButtonPressedEvent>();
			EventService->Register<MouseButtonRepeatEvent>();
			EventService->Register<MouseButtonReleasedEvent>();
			EventService->Register<CursorPosEvent>();
			EventService->Register<MouseScrollEvent>();
		}
	}

	void OnWindowRun(BackBone::SystemContext& Registry)
	{
		auto InputService = Registry.ServiceRegistry->GetService<Input>();
		auto EventService = Registry.ServiceRegistry->GetService<EventHandler>();
		auto WindowService = Registry.ServiceRegistry->GetService<WindowManager>();
		EventService->ClearAll();

		WindowService->Update();
		InputService->UpdateEvents(EventService);

		auto Read = EventService->GetEventStorage<WindowCloseEvent>();
		auto FrameData = Registry.Registry->GetResource<BackBone::GenericFrameData>();
		if (Read->ActiveSize > 0)
		{
			FrameData->IsRunning = false;
		}

		if (InputService->IsKeyDown(Input_key_W) && InputService->IsModActive(Input_mod_Shift))
			CH_CORE_TRACE("Pressed W");
		if (InputService->IsMouseButtonDown(Input_mouse_Left) && InputService->IsModActive(Input_mod_Shift))
			CH_CORE_TRACE("Mouse");
	}

	void OnWindowShutDown(BackBone::SystemContext& Registry)
	{
		auto WindowService = Registry.ServiceRegistry->GetService<WindowManager>();
		WindowService->Clear();

		auto InputService = Registry.ServiceRegistry->GetService<Input>();
		InputService->Terminate();
	}
#pragma endregion 

	void DeafultExtension::Build(BackBone::App& App)
	{
		App.ServiceRegistry.RegisterService<Chilli::Input>(std::make_shared<Chilli::Input>());
		App.ServiceRegistry.RegisterService< EventHandler>(std::make_shared< EventHandler>());
		App.ServiceRegistry.RegisterService< WindowManager>(std::make_shared<WindowManager>());
		App.Registry.Register<Chilli::MeshComponent>();
		App.Registry.Register<Chilli::TransformComponent>();

		App.AssetRegistry.RegisterStore<Mesh>();
		App.AssetRegistry.RegisterStore<Texture>();
		App.AssetRegistry.RegisterStore<Sampler>();
		App.AssetRegistry.RegisterStore<GraphicsPipeline>();
		App.AssetRegistry.RegisterStore<Material>();

		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::START_UP, OnWindowStartUp);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE_BEGIN, OnWindowRun);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::SHUTDOWN, OnWindowShutDown);

		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::START_UP, OnRenderStartUp);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::RENDER_BEGIN, OnRenderBefore);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::RENDER, OnRender);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::RENDER_END, OnRenderEnd);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::SHUTDOWN, OnRenderShutDown);

		// Services etc...
	}
#pragma region Command
	Mesh Command::CreateMesh(const MeshCreateInfo& Info)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<Chilli::RenderCommand>();
		auto MeshStore = _Ctxt.AssetRegistry->GetStore<Chilli::Mesh>();

		auto VBHandle = RenderCommandService->AllocateBuffer(Info.VBs[0]);
		if (Info.VBs[0].State == BufferState::DYNAMIC_DRAW)
			RenderCommandService->MapBufferData(VBHandle, (void*)Info.VBs[0].Data, Info.VBs[0].SizeInBytes, 0);

		Chilli::Mesh NewMesh;
		NewMesh.VBHandle = VBHandle;
		NewMesh.Layout = Info.Layout;
		NewMesh.VertexCount = 0;

		for (auto& VBInfo : Info.VBs)
			NewMesh.VertexCount += VBInfo.Count;

		return NewMesh;
	}

	void Command::DestroyMesh(Mesh& mesh)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<Chilli::RenderCommand>();
		RenderCommandService->FreeBuffer(mesh.VBHandle);
		mesh.Layout.clear();
	}

	uint32_t Command::CreateEntity()
	{
		auto RenderService = _Ctxt.ServiceRegistry->GetService<Renderer>();
		auto ReturnIndex = _Ctxt.Registry->Create();

		if (RenderService != nullptr)
			RenderService->UpdateObjectSSBO(glm::mat4(1.0f), ReturnIndex);

		return ReturnIndex;
	}

	void Command::DestroyEntity(uint32_t EntityID)
	{
		_Ctxt.Registry->Destroy(EntityID);
	}

	uint32_t Command::SpwanWindow(const WindowSpec& Spec)
	{
		_WindowService = _Ctxt.ServiceRegistry->GetService<WindowManager>();
		_EventService = _Ctxt.ServiceRegistry->GetService<EventHandler>();
		auto Idx = _WindowService->Create(Spec);
		_WindowService->GetWindow(Idx)->SetEventCallback(_EventService);

		return Idx;
	}

	void Command::DestroyWindow(uint32_t Idx)
	{
		if (_WindowService == nullptr)
			return;
		_WindowService->DestroyWindow(Idx);
	}

	BackBone::AssetHandle<Sampler> Command::CreateSampler(const Sampler& Spec)
	{
		if (_SamplerStore == nullptr)
			_SamplerStore = GetStore<Chilli::Sampler>();
		if (_RenderCommandService == nullptr)
			_RenderCommandService = GetService<Chilli::RenderCommand>();

		CH_CORE_ASSERT(_SamplerStore != nullptr, "NO SAMPLER STORE: NOPE");
		CH_CORE_ASSERT(_RenderCommandService != nullptr, "NO RENDER COMMAND SERVICE: NOPE");

		return _SamplerStore->Add(Chilli::Sampler{
			.Mode = Spec.Mode,
			.Filter = Spec.Filter,
			.SamplreHandle = _RenderCommandService->CreateSampler(Chilli::Sampler{
				.Mode = Spec.Mode,
				.Filter = Spec.Filter,
			})
			});
	}

	void Command::DestroySampler(const BackBone::AssetHandle<Sampler>& sampler)
	{
		CH_CORE_ASSERT(_SamplerStore != nullptr, "NO SAMPLER STORE: NOPE");
		CH_CORE_ASSERT(_RenderCommandService != nullptr, "NO RENDER COMMAND SERVICE: NOPE");
		CH_CORE_ASSERT(_SamplerStore->Contains(sampler), "SAMPLER NOT FOUND!");

		_RenderCommandService->DestroySampler(sampler.ValPtr->SamplreHandle);
	}

	BackBone::AssetHandle<GraphicsPipeline> Command::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& Spec)
	{
		if (_GraphicsPipelineStore == nullptr)
			_GraphicsPipelineStore = GetStore<Chilli::GraphicsPipeline>();
		if (_RenderCommandService == nullptr)
			_RenderCommandService = GetService<Chilli::RenderCommand>();

		CH_CORE_ASSERT(_GraphicsPipelineStore != nullptr, "NO GRAPHICS PIPELINE STORE: NOPE");
		CH_CORE_ASSERT(_RenderCommandService != nullptr, "NO RENDER COMMAND SERVICE: NOPE");

		return _GraphicsPipelineStore->Add({ _RenderCommandService->CreateGraphicsPipeline(Spec) });
	}

	void Command::DestroyGraphicsPipeline(const BackBone::AssetHandle<GraphicsPipeline>& Pipeline)
	{
		CH_CORE_ASSERT(_GraphicsPipelineStore != nullptr, "NO GRAPHICS PIPELINE STORE: NOPE");
		CH_CORE_ASSERT(_RenderCommandService != nullptr, "NO RENDER COMMAND SERVICE: NOPE");
		CH_CORE_ASSERT(_GraphicsPipelineStore->Contains(Pipeline), "PIPELINE NOT FOUND!");

		_RenderCommandService->DestroyGraphicsPipeline(Pipeline.ValPtr->PipelineHandle);
	}

	BackBone::AssetHandle<Texture> Command::CreateTexture(const ImageSpec& ImgSpec, const char* FilePath)
	{
		if (_TextureStore == nullptr)
			_TextureStore = GetStore<Chilli::Texture>();
		if (_RenderCommandService == nullptr)
			_RenderCommandService = GetService<Chilli::RenderCommand>();

		CH_CORE_ASSERT(_TextureStore != nullptr, "NO TEXTYRE STORE: NOPE");
		CH_CORE_ASSERT(_RenderCommandService != nullptr, "NO RENDER COMMAND SERVICE: NOPE");

		return _TextureStore->Add(Chilli::Texture{
			.ImgSpec = ImgSpec,
			.FilePath = FilePath,
			.RawTextureHandle = _RenderCommandService->AllocateImage(ImgSpec,
									FilePath)
			});
	}

	void Command::DestroyTexture(const BackBone::AssetHandle<Texture>& Tex)
	{
		CH_CORE_ASSERT(_TextureStore != nullptr, "NO GRAPHICS PIPELINE STORE: NOPE");
		CH_CORE_ASSERT(_RenderCommandService != nullptr, "NO RENDER COMMAND SERVICE: NOPE");
		CH_CORE_ASSERT(_TextureStore->Contains(Tex), "PIPELINE NOT FOUND!");

		_RenderCommandService->FreeTexture(Tex.ValPtr->RawTextureHandle);
	}


	BackBone::AssetHandle<Material> Command::AddMaterial(const Material& Mat)
	{
		if (_MaterialStore == nullptr)
			_MaterialStore = GetStore<Chilli::Material>();
		if (_RenderService == nullptr)
			_RenderService = GetService<Chilli::Renderer>();
		CH_CORE_ASSERT(_MaterialStore != nullptr, "NO MATERIAL STORE: NOPE");
		auto ReturnHandle = _MaterialStore->Add(Mat);
	
		if(_RenderService != nullptr)
			_RenderService->UpdateMaterialSSBO(ReturnHandle);
		return ReturnHandle;
	}

#pragma endregion
}

