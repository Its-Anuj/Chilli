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

		auto Command = Chilli::Command(Ctxt);

		auto RenderGraph = Command.GetResource<Chilli::RenderGraph>();
		auto RenderService = Ctxt.ServiceRegistry->GetService<Renderer>();

		for (auto& RenderPass : RenderGraph->Passes)
		{
			//CHILLI_DEBUG_TIMER("OnRenderPass:: {}", RenderPass.DebugName);
			RenderService->BeginRenderPass(RenderPass.Pass);
			RenderPass.RenderFn(Ctxt, RenderPass.Pass);
			RenderService->EndRenderPass();
		}
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

	void OnRenderGeometryPass(BackBone::SystemContext& Ctxt, RenderPassInfo& Pass)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();
		auto RenderInfo = Ctxt.Registry->GetResource< RenderResource>();

		uint32_t ActivePipelineHandle = BackBone::npos;

		struct
		{
			uint32_t ObjectIndex;
			uint32_t MaterialIndex;
		} PushConstantData;

		for (auto [EntityID, TransformComp, MeshComp] : BackBone::QueryWithEntities<TransformComponent, MeshComponent>(*Ctxt.Registry))
		{
			auto ActivePipeline = MeshComp->MaterialHandle.ValPtr->GraphicsPipelineId;
			if (ActivePipelineHandle != ActivePipeline.Handle)
			{
				RenderService->BindGraphicsPipeline(ActivePipeline.ValPtr->PipelineHandle);
				RenderService->SetScissorSize(Pass.RenderArea.x, Pass.RenderArea.y);
				RenderService->SetViewPortSize(Pass.RenderArea.x, Pass.RenderArea.y);
				RenderInfo->RenderDebugInfo.PipelineCount++;
				ActivePipelineHandle = ActivePipeline.Handle;
			}

			if (RenderInfo->OldTransformComponents[EntityID].Position != TransformComp->Position ||
				RenderInfo->OldTransformComponents[EntityID].Scale != TransformComp->Scale ||
				RenderInfo->OldTransformComponents[EntityID].Rotation != TransformComp->Rotation)
			{
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

				RenderInfo->OldTransformComponents[EntityID] = *TransformComp;
			}

			PushConstantData.MaterialIndex = *RenderService->GetMap(BindlessSetTypes::MATERIAl).Get(
				MeshComp->MaterialHandle.Handle
			);

			PushConstantData.ObjectIndex = *RenderService->GetMap(BindlessSetTypes::PER_OBJECT).Get(
				EntityID
			);

			RenderService->SendPushConstant(SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, (void*)&PushConstantData,
				sizeof(PushConstantData), 0);

			uint32_t VBs[] = { MeshComp->MeshHandle.ValPtr->VBHandle };
			RenderService->BindVertexBuffers(VBs, 1);

			auto MeshData = MeshComp->MeshHandle.ValPtr;
			if (MeshData->IBHandle != SparseSet<uint32_t>::npos)
			{
				RenderService->BindIndexBuffer(MeshData->IBHandle, MeshData->IBType);
				RenderService->DrawIndexed(MeshData->IndexCount);
			}
			else
				RenderService->DrawArrays(MeshComp->MeshHandle.ValPtr->VertexCount);

			RenderInfo->RenderDebugInfo.DrawCallsCount++;
		}
	}

	void OnRenderScenePass(BackBone::SystemContext& Ctxt, RenderPassInfo& Pass)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();
		auto RenderInfo = Ctxt.Registry->GetResource< RenderResource>();
		uint32_t ActivePipelineHandle = BackBone::npos;

		struct
		{
			uint32_t ObjectIndex;
			uint32_t MaterialIndex;
		} PushConstantData;

		for (auto [EntityID, ScreenComp] : BackBone::QueryWithEntities<SceneRenderSurfaceComponent>(*Ctxt.Registry))
		{
			auto ActivePipeline = ScreenComp->ScreenMaterialHandle.ValPtr->GraphicsPipelineId;
			if (ActivePipelineHandle != ActivePipeline.Handle)
			{
				RenderService->BindGraphicsPipeline(ActivePipeline.ValPtr->PipelineHandle);
				RenderService->SetScissorSize(true);
				RenderService->SetViewPortSize(true);
				RenderInfo->RenderDebugInfo.PipelineCount++;
				ActivePipelineHandle = ActivePipeline.Handle;
			}

			PushConstantData.MaterialIndex = *RenderService->GetMap(BindlessSetTypes::MATERIAl).Get(
				ScreenComp->ScreenMaterialHandle.Handle
			);

			PushConstantData.ObjectIndex = *RenderService->GetMap(BindlessSetTypes::PER_OBJECT).Get(
				EntityID
			);

			RenderService->SendPushConstant(SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, (void*)&PushConstantData,
				sizeof(PushConstantData), 0);

			auto MeshData = ScreenComp->ScreenMeshHandle.ValPtr;
			uint32_t VBs[] = { MeshData->VBHandle };
			RenderService->BindVertexBuffers(VBs, 1);

			if (MeshData->IBHandle != SparseSet<uint32_t>::npos)
			{
				RenderService->BindIndexBuffer(MeshData->IBHandle, MeshData->IBType);
				RenderService->DrawIndexed(MeshData->IndexCount);
			}
			else
				RenderService->DrawArrays(MeshData->VertexCount);

			RenderInfo->RenderDebugInfo.DrawCallsCount++;
		}
	}

	void OnDefferedRenderingSetup(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		Command.RegisterComponent<SceneRenderSurfaceComponent>();
		Command.AddResource< DefferedRenderingResource>();

		auto RenderGraph = Command.GetResource<Chilli::RenderGraph>();
		auto Config = Command.GetResource<RenderExtensionConfig>();
		auto DefferedPassResource = Command.GetResource< DefferedRenderingResource>();
		auto ActiveWindow = Command.GetActiveWindow();

		Chilli::ImageSpec PassImgSpec{};
		PassImgSpec.Resolution.Width = 800;
		PassImgSpec.Resolution.Height = 600;
		PassImgSpec.Usage = Chilli::ImageUsage::IMAGE_USAGE_COLOR_ATTACHMENT |
			ImageUsage::IMAGE_USAGE_SAMPLED_IMAGE;
		PassImgSpec.Format = Chilli::ImageFormat::RGBA8;
		DefferedPassResource->GeometryColorOutput = Command.CreateTexture(PassImgSpec, nullptr);

		Chilli::GraphicsPipelineCreateInfo ShaderCreateInfo{};
		ShaderCreateInfo.VertPath = "Assets/Shaders/screen_vert.spv";
		ShaderCreateInfo.FragPath = "Assets/Shaders/screen_frag.spv";
		ShaderCreateInfo.UseSwapChainColorFormat = true;
		ShaderCreateInfo.EnableDepthStencil = false;

		DefferedPassResource->ScenePipeline = Command.CreateGraphicsPipeline(ShaderCreateInfo);
		DefferedPassResource->SceneRenderMesh = Command.CreateMesh(Chilli::BasicShapes::QUAD, 1.0f);

		BackBone::AssetHandle<Sampler> ScreenSamplerHandle{};
		for (auto SamplerHandle : Command.GetStore<Sampler>()->GetHandles())
		{
			auto Sampler = SamplerHandle.ValPtr;
			if (Sampler->Filter == SamplerFilter::LINEAR && Sampler->Mode == SamplerMode::CLAMP_TO_EDGE)
				ScreenSamplerHandle = SamplerHandle;
		}

		if (ScreenSamplerHandle.ValPtr == nullptr)
			ScreenSamplerHandle = Command.CreateSampler({
				.Mode = Chilli::SamplerMode::CLAMP_TO_EDGE,
				.Filter = Chilli::SamplerFilter::LINEAR,
				});

		auto SceneSurfaceComponent = SceneRenderSurfaceComponent{
			.ScreenMeshHandle = DefferedPassResource->SceneRenderMesh,
			.ScreenMaterialHandle = Command.AddMaterial(Material{
				.AlbedoTextureHandle = DefferedPassResource->GeometryColorOutput,
				.AlbedoSamplerHandle = ScreenSamplerHandle,
				.AlbedoColor = {1.0f, 1.0f, 1.0f, 1.0f},
				.GraphicsPipelineId = DefferedPassResource->ScenePipeline,
			}) };

		DefferedPassResource->SceneEntity = Command.CreateEntity();
		Command.AddComponent<SceneRenderSurfaceComponent>(DefferedPassResource->SceneEntity, SceneSurfaceComponent);

		{
			ColorAttachment GeometryColorAttachment;
			GeometryColorAttachment.UseSwapChainImage = false;
			GeometryColorAttachment.ColorTexture = DefferedPassResource->GeometryColorOutput;
			GeometryColorAttachment.ClearColor = { 0.4f, 0.6f, 0.8f, 1.0f };
			GeometryColorAttachment.LoadOp = AttachmentLoadOp::CLEAR;
			GeometryColorAttachment.StoreOp = AttachmentStoreOp::STORE;

			RenderPassInfo GeometryPassInfo;
			GeometryPassInfo.DebugName = "GeometryPass";
			GeometryPassInfo.ColorAttachments.push_back(GeometryColorAttachment);
			GeometryPassInfo.RenderArea = { DefferedPassResource->GeometryColorOutput.ValPtr->ImgSpec.Resolution.Width,
				DefferedPassResource->GeometryColorOutput.ValPtr->ImgSpec.Resolution.Height };

			RenderGraphPass GeometryPass;
			GeometryPass.Pass = GeometryPassInfo;
			GeometryPass.RenderFn = OnRenderGeometryPass;
			GeometryPass.SortOrder = 0;
			GeometryPass.DebugName = "GeometryPass";
			RenderGraph->Passes.push_back(GeometryPass);
		}
		{
			ColorAttachment ScreenColorAttachment;
			ScreenColorAttachment.UseSwapChainImage = true;
			ScreenColorAttachment.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			ScreenColorAttachment.LoadOp = AttachmentLoadOp::CLEAR;
			ScreenColorAttachment.StoreOp = AttachmentStoreOp::STORE;

			RenderPassInfo ScenePassInfo;
			ScenePassInfo.DebugName = "ScenePass";
			ScenePassInfo.ColorAttachments.push_back(ScreenColorAttachment);
			ScenePassInfo.RenderArea = {
				ActiveWindow->GetWidth(),
				ActiveWindow->GetHeight()
			};

			RenderGraphPass ScenePass;
			ScenePass.Pass = ScenePassInfo;
			ScenePass.RenderFn = OnRenderScenePass;
			ScenePass.SortOrder = 1;
			ScenePass.DebugName = "ScenePass";
			RenderGraph->Passes.push_back(ScenePass);
		}
		{
			ColorAttachment UIColorAttachment;
			UIColorAttachment.UseSwapChainImage = true;
			UIColorAttachment.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			UIColorAttachment.LoadOp = AttachmentLoadOp::LOAD;
			UIColorAttachment.StoreOp = AttachmentStoreOp::STORE;

			RenderPassInfo UIPassInfo;
			UIPassInfo.DebugName = "UIPass";
			UIPassInfo.ColorAttachments.push_back(UIColorAttachment);
			UIPassInfo.RenderArea = {
				ActiveWindow->GetWidth(),
				ActiveWindow->GetHeight()
			};

			RenderGraphPass UIPass;
			UIPass.Pass = UIPassInfo;
			UIPass.RenderFn = OnPepperRender;
			UIPass.SortOrder = 2;
			UIPass.DebugName = "UIPass";
			RenderGraph->Passes.push_back(UIPass);
		}
	}

	void OnDefferedRenderingCleanUp(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderGraph = Command.GetResource<Chilli::RenderGraph>();
		std::vector<BackBone::Entity> SceneEntities;

		for (auto [EntityID, ScreenComp] : BackBone::QueryWithEntities<SceneRenderSurfaceComponent>(*Ctxt.Registry))
		{
			Command.DestroyMesh(ScreenComp->ScreenMeshHandle);
			Command.DestroyGraphicsPipeline(ScreenComp->ScreenMaterialHandle.ValPtr->GraphicsPipelineId);
			SceneEntities.push_back(EntityID);
		}

		for (auto& Entity : SceneEntities)
			Command.DestroyEntity(Entity);
	}

	void OnDefferedRenderingUpdate(BackBone::SystemContext& Ctxt)
	{
		auto RenderInfo = Ctxt.Registry->GetResource< RenderResource>();
		if (RenderInfo->ContinueRender == false) return;

		auto Command = Chilli::Command(Ctxt);
		auto EventService = Ctxt.ServiceRegistry->GetService<EventHandler>();
		auto Read = EventService->GetEventStorage< FrameBufferResizeEvent>();
		auto RenderGraph = Command.GetResource<Chilli::RenderGraph>();

		if (Read->ActiveSize > 0)
			for (auto Resize : *Read) {
				RenderGraph->Passes[1].Pass.RenderArea = { Resize.GetX(), Resize.GetY() };
				RenderGraph->Passes[2].Pass.RenderArea = { Resize.GetX(), Resize.GetY() };
			}
	}

	void OnRenderStartUp(BackBone::SystemContext& Ctxt)
	{
		CHILLI_DEBUG_TIMER("RenderSystem::OnCreate");
		auto Command = Chilli::Command(Ctxt);

		Ctxt.Registry->AddResource< RenderResource>();
		auto RenderInfo = Ctxt.Registry->GetResource< RenderResource>();

		auto Config = Command.GetResource<RenderExtensionConfig>();
		RenderInfo->MaxFrameInFlight = Config->Spec.MaxFrameInFlight;

		auto WindowStore = Ctxt.AssetRegistry->GetStore<Window>();

		auto WindowService = Ctxt.ServiceRegistry->GetService<WindowManager>();
		auto Win = WindowService->GetActiveWindow();

		Config->Spec.WindowSurfaceHandle = Win->GetRawHandle();
		Config->Spec.ViewPortSize = { Win->GetWidth(), Win->GetHeight() };
		Config->Spec.ViewPortResized = true;

		if (Config->Spec.Type == GraphicsBackendType::VULKAN_1_3)
			CHILLI_DEBUG_TIMER("Renderer Initializing: VULKAN_1_3");

		Ctxt.ServiceRegistry->RegisterService<Renderer>(std::make_shared<Renderer>());
		auto RenderService = Ctxt.ServiceRegistry->GetService<Renderer>();
		RenderService->Init(Config->Spec);

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
			for (auto& Resize : *Read)
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
		ActiveCommandBuffer.State = CommandBufferSubmitState::COMMAND_BUFFER_SUBMIT_STATE_SIMULTANEOUS_USE;

		RenderInfo->ContinueRender = RenderService->RenderBegin(ActiveCommandBuffer,
			RenderInfo->CurrentFrameCount);
		RenderInfo->ActiveCommandBuffer = ActiveCommandBuffer;

		GlobalShaderData GlobalData{};
		GlobalData.ResolutionTime = { 800, 600 ,10, 0 };
		RenderService->UpdateGlobalShaderData(GlobalData);

		RenderInfo->CurrentFrameCount = (RenderInfo->CurrentFrameCount + 1) % RenderInfo->MaxFrameInFlight;
	}

	void OnRenderFinishRendering(BackBone::SystemContext& Registry)
	{
		auto RenderCommandService = Registry.ServiceRegistry->GetService<RenderCommand>();
		RenderCommandService->PrepareForShutDown();
	}

	void OnRenderShutDown(BackBone::SystemContext& Registry)
	{
		CHILLI_DEBUG_TIMER("OnRenderShutDown::");
		auto RenderCommandService = Registry.ServiceRegistry->GetService<RenderCommand>();
		auto RenderService = Registry.ServiceRegistry->GetService<Renderer>();

		auto Command = Chilli::Command(Registry);
		{
			CHILLI_DEBUG_TIMER("OnAppShutDown::MeshStore::");
			for (auto Mesh : Command.GetStore<Chilli::Mesh>()->GetRefs()) {
				Command.FreeMesh(*Mesh);
			}
		} {
			CHILLI_DEBUG_TIMER("OnAppShutDown::TextureStore::");
			for (auto Texture : Command.GetStore<Chilli::Texture>()->GetRefs())
				Command.FreeTexture(*Texture);
		}
		{
			CHILLI_DEBUG_TIMER("OnAppShutDown::SamplerStore::");
			for (auto Sampler : Command.GetStore<Chilli::Sampler>()->GetRefs())
				Command.FreeSampler(*Sampler);
		}
		{
			CHILLI_DEBUG_TIMER("OnAppShutDown::GraphicsPipelineStore::");
			for (auto Shader : Command.GetStore<Chilli::GraphicsPipeline>()->GetRefs())
				Command.FreeGraphicsPipeline(*Shader);
		}
		RenderCommandService->Terminate();
		RenderService->Terminate();
	}

	void RenderExtension::Build(BackBone::App& App)
	{
		App.Registry.AddResource<RenderGraph>();
		App.Registry.AddResource<RenderExtensionConfig>();

		auto Config = App.Registry.GetResource<RenderExtensionConfig>();
		*Config = _Config;

		App.Registry.Register<Chilli::MeshComponent>();

		App.AssetRegistry.RegisterStore<Mesh>();
		App.AssetRegistry.RegisterStore<Texture>();
		App.AssetRegistry.RegisterStore<Sampler>();
		App.AssetRegistry.RegisterStore<GraphicsPipeline>();
		App.AssetRegistry.RegisterStore<Material>();

		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::START_UP, OnRenderStartUp);

		if (_Config.UsingType == RendererType::DEFFERED)
		{
			App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::START_UP, OnDefferedRenderingSetup);
			App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnDefferedRenderingUpdate);
			App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::SHUTDOWN, OnDefferedRenderingCleanUp);
		}

		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::RENDER, OnRenderBefore);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::RENDER, OnRender);
		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::RENDER, OnRenderEnd);

		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::SHUTDOWN, OnRenderFinishRendering);
		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::SHUTDOWN, OnRenderShutDown);
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

	void WindowExtension::Build(BackBone::App& App)
	{
		App.ServiceRegistry.RegisterService<Chilli::Input>(std::make_shared<Chilli::Input>());
		App.ServiceRegistry.RegisterService< EventHandler>(std::make_shared< EventHandler>());
		App.ServiceRegistry.RegisterService< WindowManager>(std::make_shared<WindowManager>());

		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::START_UP, OnWindowStartUp);
		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::UPDATE, OnWindowRun);
		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::SHUTDOWN, OnWindowShutDown);
	}
#pragma endregion 

#pragma region Command
	BackBone::AssetHandle<Mesh> Command::CreateMesh(const BetterMeshCreateInfo& Info)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<Chilli::RenderCommand>();
		auto MeshStore = _Ctxt.AssetRegistry->GetStore<Chilli::Mesh>();
		CH_CORE_ASSERT(MeshStore != nullptr, "NO MESH STORE: NOPE");
		CH_CORE_ASSERT(RenderCommandService != nullptr, "NO RENDER COMMAAND SERVICE: NOPE");

		Chilli::Mesh NewMesh;

		{
			BufferCreateInfo VertexBufferInfo;
			VertexBufferInfo.State = Info.State;
			VertexBufferInfo.Data = Info.Vertices;
			VertexBufferInfo.Type = BufferType::BUFFER_TYPE_VERTEX;

			int SizePerCount = 0;
			int ElemetCount = 0;
			for (auto& Layout : Info.Layouts) {
				SizePerCount += ShaderTypeToSize(Layout.Type);
				ElemetCount += ShaderTypeToElementCount(Layout.Type);
			};
			VertexBufferInfo.SizeInBytes = SizePerCount * Info.VertCount;
			VertexBufferInfo.Count = Info.VertCount / ElemetCount;

			auto VertexBuffer = RenderCommandService->AllocateBuffer(VertexBufferInfo);

			NewMesh.VBHandle = VertexBuffer;
			NewMesh.VertexCount = VertexBufferInfo.Count;
		}
		{
			BufferCreateInfo IndexBufferInfo;
			IndexBufferInfo.State = Info.State;
			IndexBufferInfo.Count = Info.IndexCount;;
			IndexBufferInfo.Data = Info.Indicies;
			IndexBufferInfo.Type = BufferType::BUFFER_TYPE_INDEX;

			if (Info.IndexType == IndexBufferType::UINT16_T)
				IndexBufferInfo.SizeInBytes = sizeof(uint16_t) * Info.IndexCount;
			else if (Info.IndexType == IndexBufferType::UINT32_T)
				IndexBufferInfo.SizeInBytes = sizeof(uint32_t) * Info.IndexCount;

			auto IndexBuffer = RenderCommandService->AllocateBuffer(IndexBufferInfo);

			NewMesh.IBHandle = IndexBuffer;
			NewMesh.IBType = Info.IndexType;
			NewMesh.IndexCount = IndexBufferInfo.Count;
		}
		NewMesh.Layout = Info.Layouts;

		return MeshStore->Add(NewMesh);
	}

	BackBone::AssetHandle<Mesh> Command::CreateMesh(BasicShapes Shape, float Scale)
	{
		std::vector<uint32_t> Indicies;
		std::vector<float> Vertices;

		switch (Shape)
		{
		case BasicShapes::QUAD: {

			Vertices = {
				// x      y      z      u    v

				// Triangle 1
				Scale * -1.0f, Scale * -1.0f, Scale * 0.0f,    0.0f, 0.0f,   // bottom-left
				Scale * 1.0f, Scale * -1.0f, Scale * 0.0f,    1.0f, 0.0f,   // bottom-right
				Scale * 1.0f, Scale * 1.0f, Scale * 0.0f,    1.0f, 1.0f,   // top-right

				// Triangle 2
				Scale * 1.0f,  Scale * 1.0f,	Scale * 0.0f,    1.0f, 1.0f,   // top-right
				Scale * -1.0f,  Scale * 1.0f,	Scale * 0.0f,    0.0f, 1.0f,   // top-left
				Scale * -1.0f, Scale * -1.0f,	Scale * 0.0f,    0.0f, 0.0f    // bottom-left
			};

			Indicies = {
				0, 1, 2, 3, 4, 5
			};
		};
		};

		Chilli::BetterMeshCreateInfo MeshInfo{};
		MeshInfo.VertCount = Vertices.size();
		MeshInfo.Vertices = (void*)Vertices.data();
		MeshInfo.IndexCount = Indicies.size();
		MeshInfo.Indicies = (void*)Indicies.data();
		MeshInfo.IndexType = Chilli::IndexBufferType::UINT32_T;
		MeshInfo.Layouts = {
			{ Chilli::ShaderObjectTypes::FLOAT3, "InPosition" },
			{ Chilli::ShaderObjectTypes::FLOAT2, "InTexCoord" }
		};
		MeshInfo.State = Chilli::BufferState::STATIC_DRAW;

		return this->CreateMesh(MeshInfo);
	}

	void Command::DestroyMesh(BackBone::AssetHandle<Mesh> mesh, bool Free)
	{
		if (_MeshStore == nullptr)
			_MeshStore = GetStore<Chilli::Mesh>();

		CH_CORE_ASSERT(_MeshStore != nullptr, "NO MESH STORE: NOPE");
		CH_CORE_ASSERT(_MeshStore->Contains(mesh), "MEST NOT FOUND!");

		if (Free)
			FreeMesh(*mesh.ValPtr);

		_MeshStore->Remove(mesh);
	}

	void Command::FreeMesh(Mesh& mesh)
	{
		if (_RenderCommandService == nullptr)
			_RenderCommandService = GetService<Chilli::RenderCommand>();

		CH_CORE_ASSERT(_RenderCommandService != nullptr, "NO RENDER COMMAND SERVICE: NOPE");

		_RenderCommandService->FreeBuffer(mesh.VBHandle);
		if (mesh.IBHandle != BackBone::npos)
			_RenderCommandService->FreeBuffer(mesh.IBHandle);
		mesh.Layout.clear();
	}

	uint32_t Command::CreateEntity()
	{
		auto RenderService = _Ctxt.ServiceRegistry->GetService<Renderer>();
		auto ReturnIndex = _Ctxt.Registry->Create();

		if (RenderService != nullptr)
			RenderService->UpdateObjectSSBO(glm::mat4(1.0f), ReturnIndex);

		auto RenderGlobalResource = GetResource<RenderResource>();
		if (RenderGlobalResource)
			RenderGlobalResource->OldTransformComponents.push_back(TransformComponent(
				{ (float)UINT32_MAX,(float)UINT32_MAX,(float)UINT32_MAX },
				{ (float)UINT32_MAX,(float)UINT32_MAX,(float)UINT32_MAX },
				{ (float)UINT32_MAX,(float)UINT32_MAX,(float)UINT32_MAX }
			));

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

	void Command::FreeSampler(Sampler& sampler)
	{
		if (_RenderCommandService == nullptr)
			_RenderCommandService = GetService<Chilli::RenderCommand>();
		CH_CORE_ASSERT(_RenderCommandService != nullptr, "NO RENDER COMMAND SERVICE: NOPE");
		_RenderCommandService->DestroySampler(sampler.SamplreHandle);
	}

	void Command::DestroySampler(const BackBone::AssetHandle<Sampler>& sampler, bool Free)
	{
		if (_SamplerStore == nullptr)
			_SamplerStore = GetStore<Chilli::Sampler>();
		CH_CORE_ASSERT(_SamplerStore != nullptr, "NO SAMPLER STORE: NOPE");
		CH_CORE_ASSERT(_SamplerStore->Contains(sampler), "SAMPLER NOT FOUND!");

		if (Free)
			FreeSampler(*sampler.ValPtr);

		_SamplerStore->Remove(sampler);
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

	void Command::FreeGraphicsPipeline(GraphicsPipeline& Pipeline)
	{
		if (_RenderCommandService == nullptr)
			_RenderCommandService = GetService<Chilli::RenderCommand>();
		CH_CORE_ASSERT(_RenderCommandService != nullptr, "NO RENDER COMMAND SERVICE: NOPE");
		_RenderCommandService->DestroyGraphicsPipeline(Pipeline.PipelineHandle);
	}

	void Command::DestroyGraphicsPipeline(const BackBone::AssetHandle<GraphicsPipeline>& Pipeline, bool Free)
	{
		if (_GraphicsPipelineStore == nullptr)
			_GraphicsPipelineStore = GetStore<Chilli::GraphicsPipeline>();
		CH_CORE_ASSERT(_GraphicsPipelineStore != nullptr, "NO GRAPHICS PIPELINE STORE: NOPE");
		CH_CORE_ASSERT(_GraphicsPipelineStore->Contains(Pipeline), "PIPELINE NOT FOUND!");

		if (Free)
			FreeGraphicsPipeline(*Pipeline.ValPtr);

		_GraphicsPipelineStore->Remove(Pipeline);
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
			.FilePath = (FilePath == nullptr) ? " " : FilePath,
			.RawTextureHandle = _RenderCommandService->AllocateImage(ImgSpec,
									FilePath)
			});
	}

	void Command::LoadImageData(const BackBone::AssetHandle<Texture>& Tex, void* Data, IVec2 Resolution)
	{
		CH_CORE_ASSERT(_TextureStore != nullptr, "NO TEXTYRE STORE: NOPE");
		CH_CORE_ASSERT(_RenderCommandService != nullptr, "NO RENDER COMMAND SERVICE: NOPE");
		CH_CORE_ASSERT(_TextureStore->Contains(Tex), "TEXTURE NOT FOUND!");

		_RenderCommandService->LoadImageData(Tex.ValPtr->RawTextureHandle, Data, Resolution);
	}

	void Command::DestroyTexture(const BackBone::AssetHandle<Texture>& Tex, bool Free)
	{
		if (_TextureStore == nullptr)
			_TextureStore = GetStore<Chilli::Texture>();
		CH_CORE_ASSERT(_TextureStore != nullptr, "NO TEXTURESTORE: NOPE");
		CH_CORE_ASSERT(_TextureStore->Contains(Tex), "TEXTURE NOT FOUND!");

		if (Free)
			FreeTexture(*Tex.ValPtr);

		_TextureStore->Remove(Tex);
	}

	void Command::FreeTexture(const Texture& Tex)
	{
		if (_RenderCommandService == nullptr)
			_RenderCommandService = GetService<Chilli::RenderCommand>();
		CH_CORE_ASSERT(_RenderCommandService != nullptr, "NO RENDER COMMAND SERVICE: NOPE");

		_RenderCommandService->FreeTexture(Tex.RawTextureHandle);
	}

	BackBone::AssetHandle<Material> Command::AddMaterial(const Material& Mat)
	{
		if (_MaterialStore == nullptr)
			_MaterialStore = GetStore<Chilli::Material>();
		if (_RenderService == nullptr)
			_RenderService = GetService<Chilli::Renderer>();
		CH_CORE_ASSERT(_MaterialStore != nullptr, "NO MATERIAL STORE: NOPE");
		auto ReturnHandle = _MaterialStore->Add(Mat);

		if (_RenderService != nullptr)
			_RenderService->UpdateMaterialSSBO(ReturnHandle);
		return ReturnHandle;
	}

	Window* Command::GetActiveWindow()
	{
		if (_WindowService == nullptr)
			_WindowService = GetService<WindowManager>();
		CH_CORE_ASSERT(_WindowService != nullptr, "WINDOW SERVICE NOT AVALIABLE");
		return _WindowService->GetActiveWindow();
	}

#pragma endregion
#pragma region Camera Extension

	void OnCameraSystem(Chilli::BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Ctxt.ServiceRegistry->GetService<Renderer>();

		for (auto [Entity, Tag, Camera, Transform] : BackBone::QueryWithEntities<MainCameraTag, CameraComponent, TransformComponent>(*Ctxt.Registry))
		{
			// Calculate matrices
			glm::vec3 CameraPos = ToGlmVec3(Transform->Position);

			// Simple forward direction (could enhance with proper rotation later)
			glm::vec3 Forward = glm::vec3(0, 0, -1);
			glm::vec3 Up = glm::vec3(0, 1, 0);

			glm::mat4 view = glm::lookAt(CameraPos, CameraPos + Forward, Up);

			glm::mat4 projection;
			if (Camera->Is_Orthro)
			{
				projection = glm::ortho(-Camera->Orthro_Size.x, Camera->Orthro_Size.x,
					-Camera->Orthro_Size.y, Camera->Orthro_Size.y,
					Camera->Near_Clip, Camera->Far_Clip);
			}
			else {
				projection = glm::perspective(glm::radians(Camera->Fov),
					Command.GetService<WindowManager>()->GetActiveWindow()->GetAspectRatio(),
					Camera->Near_Clip, Camera->Far_Clip);
			}

			SceneShaderData SceneData{};
			SceneData.CameraPos = { Transform->Position.x, Transform->Position.y, Transform->Position.z, 1.0f };
			SceneData.ViewProjMatrix = projection * view;
			RenderService->UpdateSceneShaderData(SceneData);

			break; // Only use first main camera found
		}
	}

	void CameraExtension::Build(BackBone::App& App)
	{
		auto Command = Chilli::Command(App.Ctxt);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnCameraSystem);
	}

	namespace CameraBundle
	{
		Chilli::BackBone::Entity Create3D(Chilli::BackBone::SystemContext& Ctxt, glm::vec3 Pos, bool Main_Cam)
		{
			auto Command = Chilli::Command(Ctxt);
			auto camera = Command.CreateEntity();

			Command.AddComponent<CameraComponent>(camera, CameraComponent{
				.Fov = 60.0f,
				.Near_Clip = 0.1f,
				.Far_Clip = 1000.0f
				});

			Command.AddComponent<TransformComponent>(camera, TransformComponent(
				FromGlmVec3(Pos),
				{ 1.0f, 1.0f, 1.0f },
				{ 0.0f, 0.0f, 0.0f }
			));

			if (Main_Cam) {
				Command.AddComponent<MainCameraTag>(camera, MainCameraTag());
			}

			return camera;
		}

		Chilli::BackBone::Entity CameraBundle::Create2D(Chilli::BackBone::SystemContext& Ctxt, bool Main_Cam, const IVec2& Resolution)
		{
			auto Command = Chilli::Command(Ctxt);
			auto camera = Command.CreateEntity();
			Command.AddComponent<CameraComponent>(camera, CameraComponent{
				.Near_Clip = 0.1f,
				.Far_Clip = 1000.0f,
				.Is_Orthro = true,
				.Orthro_Size = {(float)Resolution.x, (float)Resolution.y},
				});

			Command.AddComponent<TransformComponent>(camera, TransformComponent(
				{ 0.0f, 0.0f, 0.0f },
				{ 1.0f, 1.0f, 1.0f },
				{ 0.0f, 0.0f, 0.0f }
			));

			if (Main_Cam) {
				Command.AddComponent<MainCameraTag>(camera, MainCameraTag());
			}

			return Chilli::BackBone::Entity();
		}
	}
#pragma endregion

#pragma region Deafult Extension
	void DeafultExtension::Build(BackBone::App& App)
	{
		App.Registry.Register<Chilli::TransformComponent>();

		App.Extensions.AddExtension(std::make_unique<WindowExtension>(_Config.WindowConfig), true, &App);
		App.Extensions.AddExtension(std::make_unique<CameraExtension>(), true, &App);
		App.Extensions.AddExtension(std::make_unique<PepperExtension>(_Config.PepperConfig), true, &App);
		App.Extensions.AddExtension(std::make_unique<RenderExtension>(_Config.RenderConfig), true, &App);
		// Services etc...
	}

#pragma endregion

#pragma region Pepper Extension

	//	Systems
	void OnPepperStartUp(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto PepperData = Command.GetResource<PepperResource>();
		auto RenderInfo = Command.GetResource<RenderResource>();

		// Create a Basic Square Mesh
		const std::vector<float> Vertices = {
			0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
			1.0f, 1.0f, 0.0f, 1.0f, 1.0f
		};
		std::vector<uint32_t> QuadIndices = {
				0, 1, 2,   // First triangle
				2, 1, 3    // Second triangle
		};

		Chilli::BetterMeshCreateInfo MeshInfo{};
		MeshInfo.VertCount = Vertices.size();
		MeshInfo.Vertices = (void*)Vertices.data();
		MeshInfo.IndexCount = QuadIndices.size();
		MeshInfo.Indicies = (void*)QuadIndices.data();
		MeshInfo.IndexType = Chilli::IndexBufferType::UINT32_T;
		MeshInfo.Layouts = {
			{ Chilli::ShaderObjectTypes::FLOAT3, "InPosition" },
			{ Chilli::ShaderObjectTypes::FLOAT2, "InTexCoord" }
		};
		MeshInfo.State = Chilli::BufferState::STATIC_DRAW;

		PepperData->SquareMesh = Command.CreateMesh(MeshInfo);

		// Shader
		Chilli::GraphicsPipelineCreateInfo ShaderCreateInfo{};
		ShaderCreateInfo.VertPath = "Assets/Shaders/pepper_vert.spv";
		ShaderCreateInfo.FragPath = "Assets/Shaders/pepper_frag.spv";
		ShaderCreateInfo.UseSwapChainColorFormat = true;
		ShaderCreateInfo.EnableDepthStencil = false;
		PepperData->PepperPipeline = Command.CreateGraphicsPipeline(ShaderCreateInfo);

		bool Found = false;
		BackBone::AssetHandle<Sampler> PepperSampler;
		for (auto SamplerHandle : Command.GetStore<Sampler>()->GetHandles())
		{
			if (SamplerHandle.ValPtr->Filter == SamplerFilter::LINEAR && SamplerHandle.ValPtr->Mode == SamplerMode::CLAMP_TO_EDGE)
			{
				Found = true;
				PepperSampler = SamplerHandle;
			}
		}

		if (Found == false)
		{
			// Create our Sampler
			PepperSampler = Command.CreateSampler(Sampler{
				.Mode = SamplerMode::CLAMP_TO_EDGE,
				.Filter = SamplerFilter::LINEAR,
				});
		}

		Chilli::ImageSpec ImgSpec{};
		PepperData->PepperDeafultTexture = Command.CreateTexture(ImgSpec, "Assets/Textures/Deafult.png");

		PepperData->BasicMaterial = Command.AddMaterial(Chilli::Material{
		.AlbedoTextureHandle = PepperData->PepperDeafultTexture,
		.AlbedoSamplerHandle = PepperSampler,
		.AlbedoColor = {1.0f, 0.0f, 1.0f, 1.0f},
		.GraphicsPipelineId = PepperData->PepperPipeline,
			});

		PepperData->CameraManager.Camera = Command.CreateEntity();

		{
			auto ActiveWindow = Command.GetService<WindowManager>()->GetActiveWindow();
			PepperData->InitialWindowSize.x = ActiveWindow->GetWidth();
			PepperData->InitialWindowSize.y = ActiveWindow->GetHeight();
			PepperData->CurrentWindowSize = {
				ActiveWindow->GetWidth(),
				ActiveWindow->GetHeight(),
			};
		}
		PepperData->BatchMesh.reserve(RenderInfo->MaxFrameInFlight);
		PepperData->QuadVertices.resize(RenderInfo->MaxFrameInFlight);
		PepperData->QuadIndicies.resize(RenderInfo->MaxFrameInFlight);

		for (int i = 0; i < RenderInfo->MaxFrameInFlight; i++)
		{
			Chilli::BetterMeshCreateInfo QuadBatchMeshInfo{};
			QuadBatchMeshInfo.IndexType = Chilli::IndexBufferType::UINT32_T;
			QuadBatchMeshInfo.Layouts = {
				{ Chilli::ShaderObjectTypes::FLOAT3, "InPosition" },
				{ Chilli::ShaderObjectTypes::FLOAT2, "InTexCoord" }
			};
			QuadBatchMeshInfo.VertCount = 5 * 4 * PepperResource::MeshQuadCount;
			QuadBatchMeshInfo.IndexCount = 6 * PepperResource::MeshQuadCount;
			QuadBatchMeshInfo.State = Chilli::BufferState::DYNAMIC_STREAM;
			PepperData->BatchMesh.push_back(Command.CreateMesh(QuadBatchMeshInfo));
			PepperData->QuadVertices[i].resize(5 * 4 * PepperResource::MeshQuadCount);
			PepperData->QuadIndicies[i].resize(6 * PepperResource::MeshQuadCount);
		}
	}

	//	Systems
	void OnPepperCleanUp(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto PepperData = Command.GetResource<PepperResource>();

		Command.DestroyMesh(PepperData->SquareMesh);
		Command.DestroyGraphicsPipeline(PepperData->PepperPipeline);
		Command.DestroyTexture(PepperData->PepperDeafultTexture);
		CH_CORE_TRACE("Pepper CleanUp");
	}
	void BuildQuadVertices(float* Out, const Vec3& Position, const Vec3& HalfScale)
	{
		const float x = Position.x;
		const float y = Position.y;
		const float z = Position.z;

		const float w = HalfScale.x;
		const float h = HalfScale.y;

		// Bottom-left
		Out[0] = x - w;
		Out[1] = y - h;
		Out[2] = z;
		Out[3] = 0.0f;
		Out[4] = 0.0f;

		// Bottom-right
		Out[5] = x + w;
		Out[6] = y - h;
		Out[7] = z;
		Out[8] = 1.0f;
		Out[9] = 0.0f;

		// Top-right
		Out[10] = x + w;
		Out[11] = y + h;
		Out[12] = z;
		Out[13] = 1.0f;
		Out[14] = 1.0f;

		// Top-left
		Out[15] = x - w;
		Out[16] = y + h;
		Out[17] = z;
		Out[18] = 0.0f;
		Out[19] = 1.0f;
	}

	void BuildQuadIndices(uint32_t* out, uint32_t baseVertex) {
		out[0] = baseVertex + 0;
		out[1] = baseVertex + 1;
		out[2] = baseVertex + 2;
		out[3] = baseVertex + 2;
		out[4] = baseVertex + 3;
		out[5] = baseVertex + 0;
	}

	void OnPepperUpdate(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto PepperData = Command.GetResource<PepperResource>();
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();
		auto RenderInfo = Ctxt.Registry->GetResource< RenderResource>();

		auto InputManager = Command.GetService<Input>();
		PepperData->CursorPos = InputManager->GetCursorPos();

		auto ActiveWindow = Command.GetActiveWindow();

		PepperData->CameraManager.OthroMat = glm::ortho(0.0f,
			(float)ActiveWindow->GetWidth(),
			0.0f,
			(float)ActiveWindow->GetHeight(),
			-1.0f,
			1.0f);
		PepperData->CurrentWindowSize.x = ActiveWindow->GetWidth();
		PepperData->CurrentWindowSize.y = ActiveWindow->GetHeight();

		PepperData->QuadCount = 0;
		static float x = 0;

		for (auto [EntityID, Element, Transform] : BackBone::QueryWithEntities<PepperElement, PepperTransform>(*Ctxt.Registry))
		{
			Vec3 Position = { (Transform->PercentagePosition.x / 100.0f) * PepperData->CurrentWindowSize.x,
				(Transform->PercentagePosition.y / 100.0f) * PepperData->CurrentWindowSize.y , 0.0f };
			Vec3 Scale = { (Transform->PercentageDimensions.x / 100.0f) * PepperData->CurrentWindowSize.x,
				(Transform->PercentageDimensions.y / 100.0f)* PepperData->CurrentWindowSize.y , 1.0f };
			x += 0.01f;;

			BuildQuadVertices(PepperData->QuadVertices[RenderInfo->CurrentFrameCount].data()
				+ PepperData->QuadCount * 4 * 5,
				Position, Scale * 0.5f);
			BuildQuadIndices(PepperData->QuadIndicies[RenderInfo->CurrentFrameCount].data()
				+ PepperData->QuadCount * 6
				, PepperData->QuadCount * 4);

			PepperData->QuadCount++;
		}
		auto CurrentMesh = PepperData->BatchMesh[RenderInfo->CurrentFrameCount].ValPtr;
		RenderCommandService->MapBufferData(CurrentMesh->VBHandle, PepperData->QuadVertices[RenderInfo->CurrentFrameCount].data(), PepperData->QuadCount * 5 * 4 * sizeof(float), 0);
		RenderCommandService->MapBufferData(CurrentMesh->IBHandle, PepperData->QuadIndicies[RenderInfo->CurrentFrameCount].data(), PepperData->QuadCount * 6 * sizeof(uint32_t), 0);

		RenderService->UpdateObjectSSBO(PepperData->CameraManager.OthroMat, PepperData->CameraManager.Camera);
	}

	void OnPepperRender(BackBone::SystemContext& Ctxt, RenderPassInfo& Pass)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();
		auto PepperData = Command.GetResource<PepperResource>();
		auto RenderInfo = Ctxt.Registry->GetResource< RenderResource>();

		RenderService->BindGraphicsPipeline(PepperData->PepperPipeline.ValPtr->PipelineHandle);
		RenderService->SetScissorSize(Pass.RenderArea.x, Pass.RenderArea.y);
		RenderService->SetViewPortSize(Pass.RenderArea.x, Pass.RenderArea.y);
		RenderInfo->RenderDebugInfo.PipelineCount++;

		struct
		{
			uint32_t ObjectIndex;
			uint32_t MaterialIndex;
		} PushConstantData;

		PushConstantData.MaterialIndex = *RenderService->GetMap(BindlessSetTypes::MATERIAl).Get(
			PepperData->BasicMaterial.Handle
		);

		PushConstantData.ObjectIndex = *RenderService->GetMap(BindlessSetTypes::PER_OBJECT).Get(
			PepperData->CameraManager.Camera
		);
		RenderService->SendPushConstant(SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, (void*)&PushConstantData,
			sizeof(PushConstantData), 0);

		auto CurrentMesh = PepperData->BatchMesh[RenderInfo->CurrentFrameCount];
		auto MeshData = CurrentMesh.ValPtr;
		uint32_t VBs[] = { MeshData->VBHandle };
		RenderService->BindVertexBuffers(VBs, 1);

		RenderService->BindIndexBuffer(MeshData->IBHandle, MeshData->IBType);
		RenderService->DrawIndexed(PepperData->QuadCount * 6);

		RenderInfo->RenderDebugInfo.DrawCallsCount++;
	}

	void PepperExtension::Build(BackBone::App& App)
	{
		auto Command = Chilli::Command(App.Ctxt);
		Command.AddResource<PepperResource>();
		App.Registry.Register<PepperElement>();
		App.Registry.Register<PepperTransform>();

		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::START_UP, OnPepperStartUp);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnPepperUpdate);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::SHUTDOWN, OnPepperCleanUp);
	}

#pragma endregion
}

