#include "Ch_PCH.h"
#include "DeafultExtensions.h"
#include "Window/Window.h"
#include "Profiling\Timer.h"

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Chilli
{
#pragma region Render Extension
	uint32_t GetNewEventID()
	{
		static uint32_t NewID = 0;
		return NewID++;
	}

	void OnRenderExtensionsSetup(BackBone::SystemContext& Ctxt)
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
		Ctxt.ServiceRegistry->RegisterService<AssetLoader>(std::make_shared<AssetLoader>(Ctxt));
		auto RenderService = Ctxt.ServiceRegistry->GetService<Renderer>();
		RenderService->Init(Config->Spec);

		Ctxt.ServiceRegistry->RegisterService<RenderCommand>(RenderService->CreateRenderCommand());
		CH_CORE_TRACE("Graphcis Backend Using: {}", RenderService->GetName());
		auto RenderCommandService = Ctxt.ServiceRegistry->GetService<RenderCommand>();
	}

	void OnRenderExtensionsCleanUp(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		CHILLI_DEBUG_TIMER("OnRenderShutDown::");
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();

		// Clear Mesh
		auto MeshStore = Command.GetStore<Mesh>();
		for (auto& MeshInfo : *MeshStore)
		{
			Command.DestroyBuffer(MeshInfo->VBHandle);
			if (MeshInfo->IBHandle.ValPtr != nullptr)
				Command.DestroyBuffer(MeshInfo->IBHandle);
		}

		auto BufferStore = Command.GetStore<Buffer>();
		for (auto& Buffer : *BufferStore)
			RenderCommandService->FreeBuffer(Buffer->RawBufferHandle);

		auto ShaderProgramStore = Command.GetStore<ShaderProgram>();
		for (auto& Program : *ShaderProgramStore)
			RenderCommandService->ClearShaderProgram(Program->RawProgramHandle);

		auto MaterialStore = Command.GetStore<Material>();
		for (auto& Mat : *MaterialStore)
		{
			RenderCommandService->ClearMaterialData(Mat->RawMaterialHandle);
			Mat->RawMaterialHandle = UINT32_MAX;
		}

		auto ImageStore = Command.GetStore<Image>();
		for (auto& Image : *ImageStore)
			RenderCommandService->DestroyImage(Image->RawImageHandle);

		auto TextureStore = Command.GetStore<Texture>();
		for (auto& Tex : *TextureStore)
			RenderCommandService->DestroyTexture(Tex->RawTextureHandle);

		auto SamplerStore = Command.GetStore<Sampler>();
		for (auto& Sampler : *SamplerStore)
			RenderCommandService->DestroySampler(Sampler->SamplerHandle);

		RenderCommandService->Terminate();
		RenderService->Terminate();
	}

	void OnRenderExtensionFinishRendering(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		RenderCommandService->PrepareForShutDown();
	}

	void OnRenderExtensionDefferedRenderingUpdate(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto EventService = Command.GetService<EventHandler>();
		auto RenderGraph = Command.GetResource<Chilli::RenderGraph>();

		for (auto& Event : EventReader<FrameBufferResizeEvent>(EventService))
		{
			RenderGraph->Passes[0].Pass.Info.RenderArea = { Event.GetX(), Event.GetY() };
		}
	}

	void OnRenderExtensionScreenPassRender(BackBone::SystemContext& Ctxt, RenderPassInfo& Pass)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();

	}

	void OnRenderExtensionGeometryPassRender(BackBone::SystemContext& Ctxt, RenderPassInfo& Pass)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();

		for (auto [Transform, MeshComp] : BackBone::Query<TransformComponent, MeshComponent>(*Ctxt.Registry))
		{
			auto Mesh = MeshComp->MeshHandle.ValPtr;
			RenderService->UseShaderProgram(MeshComp->MaterialHandle.ValPtr->ShaderProgramId.ValPtr->RawProgramHandle);

			RenderService->BindVertexBuffer(Mesh->VBHandle.ValPtr->RawBufferHandle);
			if (Mesh->IBHandle.ValPtr == nullptr)
				RenderService->UnBindIndexBuffer();
			else
				RenderService->BindIndexBuffer(Mesh->IBHandle.ValPtr->RawBufferHandle);

			RenderService->UseMaterial(MeshComp->MaterialHandle.ValPtr->RawMaterialHandle);

			uint32_t ElementCount = 0;
			ElementCount = Mesh->VertexCount;
			if (Mesh->IBHandle.ValPtr != nullptr)
				ElementCount = Mesh->IndexCount;

			auto ActiveMaterial = MeshComp->MaterialHandle.ValPtr;

			struct PushConstant {
				int ObjectIndex;
				int MaterialIndex;
			} DrawPushData;

			DrawPushData.MaterialIndex = RenderService->GetMaterialShaderIndex(ActiveMaterial->RawMaterialHandle);

			MaterialShaderData MaterialData;
			MaterialData.AlbedoTextureIndex = RenderService->GetTextureShaderIndex(ActiveMaterial->AlbedoTextureHandle.ValPtr->RawTextureHandle);
			MaterialData.AlbedoSamplerIndex = RenderService->GetSamplerShaderIndex(ActiveMaterial->AlbedoSamplerHandle.ValPtr->SamplerHandle);

			RenderService->UpdateMaterialShaderData(MeshComp->MaterialHandle, MaterialData);

			RenderService->PushInlineUniformData(SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, &DrawPushData,
				sizeof(DrawPushData), 0);

			RenderService->Draw(ElementCount, 1, 0, 0);
		}
	}

	void OnRenderExtensionRenderBegin(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();

		RenderService->BeginFrame();
		GlobalShaderData GlobalData;
		GlobalData.ResolutionTime = { float(Command.GetActiveWindow()->GetWidth()), float(Command.GetActiveWindow()->GetHeight()),
		GetWindowTime(), 0.0f };
		RenderService->UpdateGlobalShaderData(GlobalData);
	}

	void OnRenderExtensionRender(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();

		auto RenderGraph = Command.GetResource<Chilli::RenderGraph>();

		for (auto& Pass : *RenderGraph)
		{
			RenderService->SetActiveCompiledRenderPass(Pass.Pass);
			RenderService->SetActivePipelineState(Pass.Pass.Info.InitialPipelineState);
			Pass.RenderFn(Ctxt, Pass.Pass.Info);
		}
	}

	void OnRenderExtensionRenderEnd(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();

		RenderService->EndFrame();
	}

	void OnRenderExtensionDefferedRenderingSetup(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();

		auto RenderGraph = Command.GetResource<Chilli::RenderGraph>();
		Command.AddResource< DefferedRenderingResource>();
		auto DefferedResource = Command.GetResource< DefferedRenderingResource>();

		auto ActiveWindow = Command.GetActiveWindow();

		// === Color Blend State (Single Opaque Attachment) ===
		RenderPassCompiler RenderPasses;

		uint32_t GeometryPassIndex;
		uint32_t ScreenPassIndex;

		// Gemoetry Pass
		{
			ImageSpec GeometryColorImageSpec;
			GeometryColorImageSpec.Format = ImageFormat::RGBA8;
			GeometryColorImageSpec.MipLevel = 1;
			GeometryColorImageSpec.Resolution = { ActiveWindow->GetWidth(), ActiveWindow->GetHeight() , 1 };
			GeometryColorImageSpec.Type = ImageType::IMAGE_TYPE_2D;
			GeometryColorImageSpec.Usage = IMAGE_USAGE_SAMPLED_IMAGE | IMAGE_USAGE_COLOR_ATTACHMENT;
			DefferedResource->GeometryColorImage = Command.AllocateImage(GeometryColorImageSpec);

			TextureSpec GeometryColorTextureSpec;
			GeometryColorTextureSpec.Format = GeometryColorImageSpec.Format;
			DefferedResource->GeometryColorTexture = Command.CreateTexture(DefferedResource->GeometryColorImage, GeometryColorTextureSpec);

			RenderPassBuilder GeometryPassBuilder("GeometryPass");

			ColorAttachment GeometryColorAttachment;
			GeometryColorAttachment.ClearColor = { 0.3f, 0.4f, 0.6f, 1.0f };
			GeometryColorAttachment.LoadOp = AttachmentLoadOp::CLEAR;
			GeometryColorAttachment.StoreOp = AttachmentStoreOp::STORE;
			GeometryColorAttachment.UseSwapChainImage = false;
			GeometryColorAttachment.ColorTexture = DefferedResource->GeometryColorTexture.ValPtr->RawTextureHandle;

			// The ColorBlendAttachmentState for opaque rendering (no blending)
			ColorBlendAttachmentState opaqueBlend = ColorBlendAttachmentState::OpaquePass();

			VertexInputShaderLayout SceneVertexLayout;
			SceneVertexLayout.BeginBinding(0);
			SceneVertexLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
			SceneVertexLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 1);

			GeometryPassIndex = RenderPasses.PushPass(
				GeometryPassBuilder
				.AddColor(GeometryColorAttachment)
				.SetArea(ActiveWindow->GetWidth(), ActiveWindow->GetHeight())
				.SetPipeline(PipelineBuilder::Default()
					.SetVertexLayout(SceneVertexLayout)
					.AddColorBlend(opaqueBlend)
					.Build())
				.Build()
			);
		}

		{
			// Create Shaders
			auto ScreenVertexShader = Command.CreateShaderModule("Assets/Shaders/screen_vert.spv",
				Chilli::ShaderStageType::SHADER_STAGE_VERTEX);
			auto ScreenFragShader = Command.CreateShaderModule("Assets/Shaders/screen_frag.spv",
				Chilli::ShaderStageType::SHADER_STAGE_FRAGMENT);

			DefferedResource->ScreenShaderProgram = Command.CreateShaderProgram();
			Command.AttachShaderModule(DefferedResource->ScreenShaderProgram, ScreenVertexShader);
			Command.AttachShaderModule(DefferedResource->ScreenShaderProgram, ScreenFragShader);
			Command.LinkShaderProgram(DefferedResource->ScreenShaderProgram);

			Chilli::SamplerSpec SamplerSpec;
			SamplerSpec.Filter = Chilli::SamplerFilter::NEAREST;
			SamplerSpec.Mode = Chilli::SamplerMode::REPEAT;
			auto MaterialSampler = Command.CreateSampler(SamplerSpec);

			Chilli::Material Material;
			Material.ShaderProgramId = DefferedResource->ScreenShaderProgram;
			Material.AlbedoTextureHandle = DefferedResource->GeometryColorTexture;
			Material.AlbedoSamplerHandle = MaterialSampler;
			DefferedResource->ScreenMaterial = Command.CreateMaterial(Material);

			Chilli::MeshCreateInfo SquareInfo{};

			std::vector<Chilli::Vertex> SquareVertices = {
				// Position              // UV
				{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f } }, // Bottom-left
				{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f } }, // Bottom-right
				{ {  0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f } }, // Top-right
				{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f } }  // Top-left
			};

			std::vector<uint32_t> SquareIndices = {
				0, 1, 2, // First triangle
				2, 3, 0  // Second triangle
			};

			SquareInfo.VertCount = SquareVertices.size();
			SquareInfo.Vertices = SquareVertices.data();
			SquareInfo.VerticesSize = sizeof(Chilli::Vertex) * SquareVertices.size();
			SquareInfo.IndexCount = SquareIndices.size();
			SquareInfo.Indicies = SquareIndices.data();
			SquareInfo.IndiciesSize = sizeof(uint32_t) * SquareIndices.size();
			SquareInfo.IndexType = Chilli::IndexBufferType::UINT32_T;
			SquareInfo.State = Chilli::BufferState::STATIC_DRAW;
			auto SquareMesh = Command.CreateMesh(SquareInfo);

			DefferedResource->ScreenRenderMesh = SquareMesh;

			DefferedResource->SceneEntity = Command.CreateEntity();
			Command.AddComponent<Chilli::SceneRenderSurfaceComponent>(DefferedResource->SceneEntity,
				Chilli::SceneRenderSurfaceComponent{
					.ScreenMeshHandle = DefferedResource->ScreenRenderMesh,
					.ScreenMaterialHandle = DefferedResource->ScreenMaterial
				});

			// The ColorBlendAttachmentState for opaque rendering (no blending)
			ColorBlendAttachmentState opaqueBlend = ColorBlendAttachmentState::OpaquePass();

			VertexInputShaderLayout SceneVertexLayout;
			SceneVertexLayout.BeginBinding(0);
			SceneVertexLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
			SceneVertexLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 1);

			RenderPassBuilder ScreenPassBuilder("ScreenPass");

			ScreenPassIndex = RenderPasses.PushPass(
				ScreenPassBuilder
				.AddSwapChain({ 0.3f, 0.4f, 0.6f, 1.0f })
				.SetArea(ActiveWindow->GetWidth(), ActiveWindow->GetHeight())
				.SetPipeline(PipelineBuilder::Default()
					.SetVertexLayout(SceneVertexLayout)
					.AddColorBlend(opaqueBlend)
					.Build())
				.AddPipelineBarrier(DefferedResource->GeometryColorTexture.ValPtr->RawTextureHandle, ResourceState::RenderTarget,
					ResourceState::ShaderRead)
				.SetSwapChainPipelineBarrier(ResourceState::RenderTarget)
				.Build()
			);
		}
		RenderPassBuilder PresentPassBuilder("PresentPass");

		ColorAttachment PresentColorAttachment;
		PresentColorAttachment.ClearColor = { 0.3f, 0.4f, 0.6f, 1.0f };
		PresentColorAttachment.LoadOp = AttachmentLoadOp::LOAD;
		PresentColorAttachment.StoreOp = AttachmentStoreOp::STORE;
		PresentColorAttachment.UseSwapChainImage = true;

		auto PresentPassIndex = RenderPasses.PushPass(
			PresentPassBuilder
			.AddColor(PresentColorAttachment)
			.SetArea(ActiveWindow->GetWidth(), ActiveWindow->GetHeight())
			.SetSwapChainPipelineBarrier(ResourceState::Present)
			.Build()
		);

		auto RenderFramePasses = RenderPasses.Compile();

		RenderGraphPass GeometryGraphPass;
		GeometryGraphPass.DebugName = "GeometryPass";
		GeometryGraphPass.SortOrder = 0;
		//GeometryGraphPass.Pass = RenderFramePasses[GeometryPassIndex];
		//GeometryGraphPass.RenderFn = OnRenderExtensionGeometryPassRender;

		RenderGraphPass ScreenGraphPass;
		ScreenGraphPass.DebugName = "ScreenPass";
		ScreenGraphPass.SortOrder = 1;
		ScreenGraphPass.Pass = RenderFramePasses[ScreenPassIndex];
		ScreenGraphPass.RenderFn = OnRenderExtensionScreenPassRender;

		RenderGraphPass PresentGraphPass;
		PresentGraphPass.DebugName = "PresentPass";
		PresentGraphPass.SortOrder = 2;
		PresentGraphPass.Pass = RenderFramePasses[PresentPassIndex];
		PresentGraphPass.RenderFn = [](BackBone::SystemContext& Ctxt, RenderPassInfo& Pass) {};

		//RenderGraph->Passes.push_back(GeometryGraphPass);
		RenderGraph->Passes.push_back(ScreenGraphPass);
		RenderGraph->Passes.push_back(PresentGraphPass);
	}

	void RenderExtension::Build(BackBone::App& App)
	{
		App.Registry.AddResource<RenderGraph>();
		App.Registry.AddResource<RenderExtensionConfig>();

		auto Config = App.Registry.GetResource<RenderExtensionConfig>();
		*Config = _Config;

		App.Registry.Register<Chilli::MeshComponent>();

		App.AssetRegistry.RegisterStore<Buffer>();
		App.AssetRegistry.RegisterStore<Mesh>();
		App.AssetRegistry.RegisterStore<Image>();
		App.AssetRegistry.RegisterStore<Texture>();
		App.AssetRegistry.RegisterStore<Sampler>();
		App.AssetRegistry.RegisterStore<ShaderModule>();
		App.AssetRegistry.RegisterStore<ShaderProgram>();
		App.AssetRegistry.RegisterStore<Material>();

		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::START_UP, OnRenderExtensionsSetup);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::START_UP, OnRenderExtensionDefferedRenderingSetup);

		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnRenderExtensionDefferedRenderingUpdate);

		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::RENDER, OnRenderExtensionRenderBegin);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::RENDER,
			OnRenderExtensionRender);
		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::RENDER, OnRenderExtensionRenderEnd);

		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::SHUTDOWN, OnRenderExtensionFinishRendering);
		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::SHUTDOWN, OnRenderExtensionsCleanUp);
	}
#pragma endregion

#pragma region Deafult Extension
	void DeafultExtension::Build(BackBone::App& App)
	{
		App.Registry.Register<Chilli::TransformComponent>();

		App.Extensions.AddExtension(std::make_unique<WindowExtension>(_Config.WindowConfig), true, &App);
		//App.Extensions.AddExtension(std::make_unique<CameraExtension>(), true, &App);
		//App.Extensions.AddExtension(std::make_unique<PepperExtension>(_Config.PepperConfig), true, &App);
		App.Extensions.AddExtension(std::make_unique<RenderExtension>(_Config.RenderConfig), true, &App);
		// Services etc...
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
		if (Read->GetActiveSize() > 0)
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

	BackBone::AssetHandle<Mesh> Command::CreateMesh(const MeshCreateInfo& Info)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<Chilli::RenderCommand>();
		auto MeshStore = _Ctxt.AssetRegistry->GetStore<Chilli::Mesh>();
		CH_CORE_ASSERT(MeshStore != nullptr, "NO MESH STORE: NOPE");
		CH_CORE_ASSERT(RenderCommandService != nullptr, "NO RENDER COMMAAND SERVICE: NOPE");
		CH_CORE_ASSERT(Info.VertCount != 0, "A Vertex Count is Needed: NOPE");

		Chilli::Mesh NewMesh;
		{
			BufferCreateInfo VertexBufferInfo;
			VertexBufferInfo.State = Info.State;
			VertexBufferInfo.Data = Info.Vertices;
			VertexBufferInfo.Type = BufferType::BUFFER_TYPE_VERTEX;
			VertexBufferInfo.SizeInBytes = Info.VerticesSize;

			NewMesh.VBHandle = this->CreateBuffer(VertexBufferInfo, "VertexBuffer");
			NewMesh.VertexCount = Info.VertCount;
		}
		if (Info.IndexCount > 0)
		{
			BufferCreateInfo IndexBufferInfo;
			IndexBufferInfo.State = Info.State;
			IndexBufferInfo.Data = Info.Indicies;
			IndexBufferInfo.Type = BufferType::BUFFER_TYPE_INDEX;
			IndexBufferInfo.SizeInBytes = Info.IndiciesSize;

			NewMesh.IBHandle = this->CreateBuffer(IndexBufferInfo, "IndexBuffer");
			NewMesh.IBType = Info.IndexType;
			NewMesh.IndexCount = Info.IndexCount;
		}

		return MeshStore->Add(NewMesh);
	}

	uint32_t Command::CreateEntity()
	{
		auto RenderService = _Ctxt.ServiceRegistry->GetService<Renderer>();
		auto ReturnIndex = _Ctxt.Registry->Create();

		//if (RenderService != nullptr)
		//	RenderService->UpdateObjectSSBO(glm::mat4(1.0f), ReturnIndex);

		//auto RenderGlobalResource = GetResource<RenderResource>();
		//if (RenderGlobalResource)
		//	RenderGlobalResource->OldTransformComponents.push_back(TransformComponent(
		//		{ (float)UINT32_MAX,(float)UINT32_MAX,(float)UINT32_MAX },
		//		{ (float)UINT32_MAX,(float)UINT32_MAX,(float)UINT32_MAX },
		//		{ (float)UINT32_MAX,(float)UINT32_MAX,(float)UINT32_MAX }
		//	));

		return ReturnIndex;
	}

	void Command::DestroyEntity(uint32_t EntityID)
	{
		_Ctxt.Registry->Destroy(EntityID);
	}

	BackBone::AssetHandle<ShaderModule> Command::CreateShaderModule(const char* FilePath, ShaderStageType Type)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto ShaderModuleStore = _Ctxt.AssetRegistry->GetStore<ShaderModule>();

		auto Module = RenderCommandService->CreateShaderModule(FilePath, Type);
		return ShaderModuleStore->Add(Module);
	}

	void Command::DestroyShaderModule(const BackBone::AssetHandle<ShaderModule>& Handle)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto ShaderModuleStore = _Ctxt.AssetRegistry->GetStore<ShaderModule>();

		auto Module = ShaderModuleStore->Get(Handle);
		CH_CORE_ASSERT(Module != nullptr, "Shader Module not Found is null");

		RenderCommandService->DestroyShaderModule(*Module);
		ShaderModuleStore->Remove(Handle);
	}

	BackBone::AssetHandle<ShaderProgram> Command::CreateShaderProgram()
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto ShaderProgramStore = _Ctxt.AssetRegistry->GetStore<ShaderProgram>();

		auto RawProgramID = RenderCommandService->MakeShaderProgram();
		ShaderProgram Program;
		Program.RawProgramHandle = RawProgramID;

		return ShaderProgramStore->Add(Program);
	}

	void Command::AttachShaderModule(const BackBone::AssetHandle<ShaderProgram>& Program, const BackBone::AssetHandle<ShaderModule>& Module)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto ShaderProgramStore = _Ctxt.AssetRegistry->GetStore<ShaderProgram>();
		auto ShaderModuleStore = _Ctxt.AssetRegistry->GetStore<ShaderModule>();

		auto RawProgram = ShaderProgramStore->Get(Program);
		CH_CORE_ASSERT(RawProgram != nullptr, "Shader Module not Found is null");

		auto RawModule = ShaderModuleStore->Get(Module);
		CH_CORE_ASSERT(RawModule != nullptr, "Shader Module not Found is null");

		RenderCommandService->AttachShader(RawProgram->RawProgramHandle, *RawModule);
	}

	void Command::LinkShaderProgram(const BackBone::AssetHandle<ShaderProgram>& Handle)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto ShaderProgramStore = _Ctxt.AssetRegistry->GetStore<ShaderProgram>();

		auto Program = ShaderProgramStore->Get(Handle);
		CH_CORE_ASSERT(Program != nullptr, "Shader Module not Found is null");

		RenderCommandService->LinkShaderProgram(Program->RawProgramHandle);
	}

	void Command::DestroyShaderProgram(const BackBone::AssetHandle<ShaderProgram>& Handle)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto ShaderProgramStore = _Ctxt.AssetRegistry->GetStore<ShaderProgram>();

		auto Program = ShaderProgramStore->Get(Handle);
		CH_CORE_ASSERT(Program != nullptr, "Shader Module not Found is null");

		RenderCommandService->ClearShaderProgram(Program->RawProgramHandle);
		ShaderProgramStore->Remove(Handle);
	}

	BackBone::AssetHandle<Buffer> Command::CreateBuffer(const BufferCreateInfo& Info, const char* DebugName)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto BufferStore = _Ctxt.AssetRegistry->GetStore<Buffer>();

		Buffer NewBuffer;
		NewBuffer.RawBufferHandle = RenderCommandService->AllocateBuffer(Info);
		NewBuffer.CreateInfo = Info;
		strcpy(NewBuffer.DebugName, DebugName);

		return BufferStore->Add(NewBuffer);
	}

	void Command::MapBufferData(const BackBone::AssetHandle<Buffer>& Handle, void* Data, size_t Size
		, size_t Offset)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto BufferStore = _Ctxt.AssetRegistry->GetStore<Buffer>();

		auto Buffer = BufferStore->Get(Handle);
		CH_CORE_ASSERT(Buffer != nullptr, "Buffer not Found is null");

		RenderCommandService->MapBufferData(Buffer->RawBufferHandle, Data, Size, Offset);
	}

	void Command::DestroyBuffer(const BackBone::AssetHandle<Buffer>& Handle)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto BufferStore = _Ctxt.AssetRegistry->GetStore<Buffer>();

		auto Buffer = BufferStore->Get(Handle);
		CH_CORE_ASSERT(Buffer != nullptr, "Buffer not Found is null");

		RenderCommandService->FreeBuffer(Buffer->RawBufferHandle);
		BufferStore->Remove(Handle);
	}

	uint32_t Command::SpwanWindow(const WindowSpec& Spec)
	{
		auto WindowService = _Ctxt.ServiceRegistry->GetService<WindowManager>();
		auto EventService = _Ctxt.ServiceRegistry->GetService<EventHandler>();

		CH_CORE_ASSERT(WindowService != nullptr, "Window Service is null");
		CH_CORE_ASSERT(EventService != nullptr, "Event Handle Service is null");

		auto Idx = WindowService->Create(Spec);
		WindowService->GetWindow(Idx)->SetEventCallback(EventService);

		return Idx;
	}

	void Command::DestroyWindow(uint32_t Idx)
	{
		auto WindowService = _Ctxt.ServiceRegistry->GetService<WindowManager>();
		CH_CORE_ASSERT(WindowService != nullptr, "Window Service is null");
		WindowService->DestroyWindow(Idx);
	}

	Window* Command::GetActiveWindow()
	{
		auto WindowService = _Ctxt.ServiceRegistry->GetService<WindowManager>();
		CH_CORE_ASSERT(WindowService != nullptr, "Window Service is null");
		return WindowService->GetActiveWindow();
	}

	BackBone::AssetHandle<Material> Command::CreateMaterial(const Material& Mat)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();

		auto MaterialStore = GetStore<Chilli::Material>();
		auto ReturnHandle = MaterialStore->Add(Mat);

		ReturnHandle.ValPtr->RawMaterialHandle = RenderCommandService->PrepareMaterialData(ReturnHandle);

		return ReturnHandle;
	}

	void Command::DestroyMaterial(const BackBone::AssetHandle<Material>& Mat)
	{
		auto MaterialStore = GetStore<Chilli::Material>();
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();

		RenderCommandService->ClearMaterialData(Mat);
		Mat.ValPtr->RawMaterialHandle = UINT32_MAX;
		MaterialStore->Remove(Mat);
	}

	BackBone::AssetHandle<Sampler> Command::CreateSampler(const SamplerSpec& Spec)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();

		auto SamplerStore = GetStore<Chilli::Sampler>();

		// Check if sampler with same spec already exists
		for (auto SamplerHandle : SamplerStore->GetHandles())
		{
			auto Sampler = SamplerHandle.ValPtr;
			if (Sampler->Spec == Spec)
				return SamplerHandle;
		}

		Sampler NewSampler;
		NewSampler.SamplerHandle = RenderCommandService->CreateSampler(Spec);
		NewSampler.Spec = Spec;

		return SamplerStore->Add(NewSampler);

	}

	void Command::DestroySampler(const BackBone::AssetHandle<Sampler>& sampler)
	{
		auto SamplerStore = GetStore<Chilli::Sampler>();
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();

		RenderCommandService->DestroySampler(SamplerStore->Get(sampler)->SamplerHandle);
		SamplerStore->Remove(sampler);
	}

	BackBone::AssetHandle<Image> Command::AllocateImage(const ImageSpec& Spec)
	{
		auto ImageStore = GetStore<Image>();
		auto RenderCommandService = GetService<RenderCommand>();

		Image NewImage;
		NewImage.RawImageHandle = RenderCommandService->AllocateImage(Spec);
		NewImage.Spec = Spec;

		return ImageStore->Add(NewImage);
	}

	std::pair<BackBone::AssetHandle<Image>, BackBone::AssetHandle<ImageData>> Command::AllocateImage(const char* FilePath,
		ImageFormat Format, uint32_t Usage, ImageType Type, uint32_t MipLevel, bool YFlip)
	{
		auto ImageData = this->LoadAsset<Chilli::ImageData>(FilePath);

		Chilli::ImageSpec ImageSpec;
		ImageSpec.Resolution = { ImageData.ValPtr->Resolution.x,
			ImageData.ValPtr->Resolution.y,
			1 };
		ImageSpec.Format = Format;
		ImageSpec.Usage = Usage;
		ImageSpec.Type = Type;
		ImageSpec.MipLevel = MipLevel;
		ImageSpec.YFlip = YFlip;
		auto Image = this->AllocateImage(ImageSpec);

		this->MapImageData(Image, (void*)ImageData.ValPtr->Pixels, ImageData.ValPtr->Resolution.x,
			ImageData.ValPtr->Resolution.y);

		return { Image, ImageData };
	}

	void Chilli::Command::DestroyImage(const BackBone::AssetHandle<Image>& ImageHandle)
	{
		auto ImageStore = GetStore<Image>();
		auto RenderCommandService = GetService<RenderCommand>();

		RenderCommandService->DestroyImage(ImageStore->Get(ImageHandle)->RawImageHandle);
		ImageStore->Remove(ImageHandle);
	}

	void Command::MapImageData(const BackBone::AssetHandle<Image>& ImageHandle, void* Data, int Width, int Height)
	{
		auto ImageStore = GetStore<Image>();
		auto RenderCommandService = GetService<RenderCommand>();

		RenderCommandService->MapImageData(ImageStore->Get(ImageHandle)->RawImageHandle,
			Data, Width, Height);
	}

	BackBone::AssetHandle<Texture> Command::CreateTexture(const BackBone::AssetHandle<Image>& ImageHandle, const TextureSpec& Spec)
	{
		auto TextureStore = GetStore<Texture>();
		auto RenderCommandService = GetService<RenderCommand>();

		Texture NewTexture;
		NewTexture.ImageHandle = ImageHandle;
		NewTexture.Spec = Spec;
		NewTexture.RawTextureHandle = RenderCommandService->CreateTexture(
			ImageHandle.ValPtr->RawImageHandle,
			Spec
		);

		return TextureStore->Add(NewTexture);
	}

	void Command::DestroyTexture(const BackBone::AssetHandle<Texture>& TextureHandle)
	{
		auto TextureStore = GetStore<Texture>();
		auto RenderCommandService = GetService<RenderCommand>();

		RenderCommandService->DestroyTexture(TextureStore->Get(TextureHandle)->RawTextureHandle);
		TextureStore->Remove(TextureHandle);
	}
#pragma endregion

}

