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

		auto DefferedResource = Command.GetResource< DefferedRenderingResource>();

		auto ActiveMaterial = DefferedResource->ScreenMaterial.ValPtr;
		auto ActiveShader = DefferedResource->ScreenShaderProgram;

		RenderService->BindShaderProgram(DefferedResource->ScreenShaderProgram.ValPtr->RawProgramHandle);
		RenderService->BindMaterailData(DefferedResource->ScreenMaterial.ValPtr->RawMaterialHandle);

		DrawPushShaderInlineUniformData PushData;
		PushData.ObjectIndex = RenderService->GetTextureShaderIndex(ActiveMaterial->AlbedoTextureHandle.ValPtr->RawTextureHandle);
		PushData.MaterialIndex = RenderService->GetSamplerShaderIndex(ActiveMaterial->AlbedoSamplerHandle.ValPtr->SamplerHandle);

		RenderService->PushInlineUniformData(ActiveShader.ValPtr->RawProgramHandle,
			SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, &PushData, sizeof(PushData), 0);

		RenderService->BindVertexBuffer({ DefferedResource->ScreenRenderMesh.ValPtr->VBHandle.ValPtr->RawBufferHandle });
		RenderService->BindIndexBuffer(DefferedResource->ScreenRenderMesh.ValPtr->IBHandle.ValPtr->RawBufferHandle, IndexBufferType::UINT16_T);

		RenderService->DrawIndexed(DefferedResource->ScreenRenderMesh.ValPtr->IndexCount, 1, 0, 0, 0);
	}

	void OnRenderExtensionGeometryPassRender(BackBone::SystemContext& Ctxt, RenderPassInfo& Pass)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();

		for (auto [Transform, MeshComp] : BackBone::Query<TransformComponent, MeshComponent>(*Ctxt.Registry))
		{
			auto ActiveMaterial = MeshComp->MaterialHandle.ValPtr;
			auto ActiveShader = MeshComp->MaterialHandle.ValPtr->ShaderProgramId;
			auto ActiveMesh = MeshComp->MeshHandle.ValPtr;

			RenderService->BindShaderProgram(ActiveShader.ValPtr->RawProgramHandle);
			RenderService->BindMaterailData(MeshComp->MaterialHandle.ValPtr->RawMaterialHandle);

			RenderService->BindVertexBuffer({ ActiveMesh->VBHandle.ValPtr->RawBufferHandle });
			RenderService->BindIndexBuffer(ActiveMesh->IBHandle.ValPtr->RawBufferHandle, IndexBufferType::UINT16_T);

			DrawPushShaderInlineUniformData PushData;
			PushData.ObjectIndex = RenderService->GetTextureShaderIndex(ActiveMaterial->AlbedoTextureHandle.ValPtr->RawTextureHandle);
			PushData.MaterialIndex = RenderService->GetSamplerShaderIndex(ActiveMaterial->AlbedoSamplerHandle.ValPtr->SamplerHandle);

			RenderService->PushInlineUniformData(ActiveShader.ValPtr->RawProgramHandle,
				SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, &PushData, sizeof(PushData), 0);

			RenderService->DrawIndexed(ActiveMesh->IndexCount, 1, 0, 0, 0);
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

		RenderService->PushUpdateGlobalShaderData(GlobalData);
	}

	void OnRenderExtensionRender(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();

		auto RenderGraph = Command.GetResource<Chilli::RenderGraph>();

		for (auto& Pass : *RenderGraph)
		{
			for (auto& Barrier : Pass.Pass.PrePassBarriers)
				RenderService->PushPipelienBarrier(Barrier, RenderStreamTypes::GRAPHICS);
			RenderService->BeginRenderPass(Pass.Pass.Info);
			RenderService->SetFullPipelineState(Pass.Info);
			RenderService->SetVertexInputLayout(Pass.Layout);

			Pass.RenderFn(Ctxt, Pass.Pass.Info);

			RenderService->EndRenderPass();
			for (auto& Barrier : Pass.Pass.PostPassBarriers)
				RenderService->PushPipelienBarrier(Barrier, RenderStreamTypes::GRAPHICS);
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

		RenderPassCompiler Compiler;
		uint32_t GeometryPassIndex = 0;
		uint32_t ScreenPassIndex = 0;
		uint32_t PresentPassIndex = 0;

		{
			ImageSpec Spec;
			Spec.Format = ImageFormat::RGBA8;
			Spec.ImageData = nullptr;
			Spec.MipLevel = 1;
			Spec.Resolution.Width = 800;
			Spec.Resolution.Height = 600;
			Spec.Resolution.Depth = 1;
			Spec.Type = ImageType::IMAGE_TYPE_2D;
			Spec.Usage = IMAGE_USAGE_COLOR_ATTACHMENT | IMAGE_USAGE_SAMPLED_IMAGE;
			Spec.State = ResourceState::ShaderRead;
			DefferedResource->GeometryColorImage = Command.AllocateImage(Spec);

			TextureSpec TextureSpec;
			TextureSpec.Format = Spec.Format;
			DefferedResource->GeometryColorTexture = Command.CreateTexture(DefferedResource->GeometryColorImage, TextureSpec);

			ColorAttachment Color;
			Color.ColorTexture = DefferedResource->GeometryColorTexture.ValPtr->RawTextureHandle;
			Color.LoadOp = AttachmentLoadOp::CLEAR;
			Color.StoreOp = AttachmentStoreOp::STORE;
			Color.UseSwapChainImage = false;
			Color.ClearColor = { 0.2f, 0.8f, 0.2f, 1.0f };

			RenderPassBuilder GeometryPass("GeometryPass");
			GeometryPass.AddImageBarrier(true, DefferedResource->GeometryColorImage.ValPtr->RawImageHandle, 
				ResourceState::ShaderRead, ResourceState::RenderTarget, false);
			GeometryPass.AddImageBarrier(false, DefferedResource->GeometryColorImage.ValPtr->RawImageHandle,
				ResourceState::RenderTarget, ResourceState::ShaderRead, false);
			GeometryPass.AddColor(Color);
			GeometryPass.SetArea(800, 600);

			GeometryPassIndex = Compiler.PushPass(GeometryPass.Build());
		}

		{
			Chilli::MeshCreateInfo SquareInfo{};

			std::vector<Chilli::Vertex> SquareVertices = {
				// Position              // UV
				{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } }, // Bottom-left
				{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } }, // Bottom-right
				{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f } }, // Top-right
				{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f } }  // Top-left
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
			DefferedResource->ScreenRenderMesh = Command.CreateMesh(SquareInfo);

			// Create Shaders
			auto GeometryPassVertexShader = Command.CreateShaderModule("Assets/Shaders/screen_vert.spv",
				Chilli::ShaderStageType::SHADER_STAGE_VERTEX);
			auto GeometryPassFragShader = Command.CreateShaderModule("Assets/Shaders/screen_frag.spv",
				Chilli::ShaderStageType::SHADER_STAGE_FRAGMENT);

			auto GeometryShaderProgram = Command.CreateShaderProgram();
			Command.AttachShaderModule(GeometryShaderProgram, GeometryPassVertexShader);
			Command.AttachShaderModule(GeometryShaderProgram, GeometryPassFragShader);
			Command.LinkShaderProgram(GeometryShaderProgram);
			DefferedResource->ScreenShaderProgram = GeometryShaderProgram;

			Chilli::SamplerSpec SamplerSpec;
			SamplerSpec.Filter = Chilli::SamplerFilter::NEAREST;
			SamplerSpec.Mode = Chilli::SamplerMode::REPEAT;
			auto Sampler = Command.CreateSampler(SamplerSpec);

			Chilli::Material Material;
			Material.ShaderProgramId = GeometryShaderProgram;
			Material.AlbedoTextureHandle = DefferedResource->GeometryColorTexture;
			Material.AlbedoSamplerHandle = Sampler;
			DefferedResource->ScreenMaterial = Command.CreateMaterial(Material);

			RenderPassBuilder ScreenPass("ScreenPass");
			ScreenPass.AddSwapChain({ 0.2f, 0.2f, 0.2f, 1.0f });
			ScreenPass.AddImageBarrier(true, UINT32_MAX, ResourceState::Present, ResourceState::RenderTarget, true);
			ScreenPass.SetArea(800, 600);


			auto RenderService = Command.GetService<Chilli::Renderer>();
			RenderService->UpdateMaterialTextureData(DefferedResource->ScreenMaterial.ValPtr->RawMaterialHandle,
				DefferedResource->GeometryColorTexture.ValPtr->RawTextureHandle,
				"ScreenTexture", Chilli::ResourceState::ShaderRead);
			RenderService->UpdateMaterialSamplerData(DefferedResource->ScreenMaterial.ValPtr->RawMaterialHandle,
				Sampler.ValPtr->SamplerHandle,
				"ScreenSampler");

			ScreenPassIndex = Compiler.PushPass(ScreenPass.Build());
		}

		{
			ColorAttachment Color;
			Color.LoadOp = AttachmentLoadOp::LOAD;
			Color.StoreOp = AttachmentStoreOp::STORE;
			Color.UseSwapChainImage = true;

			RenderPassBuilder PresentPass("PresentPass");
			PresentPass.AddColor(Color);
			PresentPass.AddPostPipelineBarrier(UINT32_MAX, ResourceState::RenderTarget, ResourceState::Present,
				false, true);
			PresentPass.SetArea(800, 600);

			PresentPassIndex = Compiler.PushPass(PresentPass.Build());
		}

		auto CompiledPasses = Compiler.Compile();

		PipelineBuilder ScreenPipelineBuilder;
		PipelineBuilder PresentPipelineBuilder;

		VertexInputShaderLayout GeometryLayout;
		GeometryLayout.BeginBinding(0);
		GeometryLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
		GeometryLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 1);

		RenderGraphPass GeometryGraphPass;
		GeometryGraphPass.DebugName = "GeometryPass";
		GeometryGraphPass.Pass = CompiledPasses[GeometryPassIndex];
		GeometryGraphPass.RenderFn = OnRenderExtensionGeometryPassRender;
		GeometryGraphPass.Info = ScreenPipelineBuilder.Default()
			.AddColorBlend(ColorBlendAttachmentState::OpaquePass())
			.Build();
		GeometryGraphPass.Layout = GeometryLayout;
		GeometryGraphPass.SortOrder = 0;

		VertexInputShaderLayout ScreenLayout;
		ScreenLayout.BeginBinding(0);
		ScreenLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
		ScreenLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 1);

		RenderGraphPass ScreenGraphPass;
		ScreenGraphPass.DebugName = "ScreenPass";
		ScreenGraphPass.Pass = CompiledPasses[ScreenPassIndex];
		ScreenGraphPass.RenderFn = OnRenderExtensionScreenPassRender;
		ScreenGraphPass.Info = ScreenPipelineBuilder.Default()
			.AddColorBlend(ColorBlendAttachmentState::OpaquePass())
			.Build();
		ScreenGraphPass.Layout = ScreenLayout;
		ScreenGraphPass.SortOrder = 1;

		RenderGraphPass PresentGraphPass;
		PresentGraphPass.DebugName = "PresentPass";
		PresentGraphPass.Pass = CompiledPasses[PresentPassIndex];
		PresentGraphPass.RenderFn = [](BackBone::SystemContext& Ctxt, RenderPassInfo& Pass) {};
		PresentGraphPass.Info = PresentPipelineBuilder.Default()
			.AddColorBlend(ColorBlendAttachmentState::OpaquePass())
			.Build();
		PresentGraphPass.SortOrder = 2;

		RenderGraph->PushGraphPass(GeometryGraphPass);
		RenderGraph->PushGraphPass(ScreenGraphPass);
		RenderGraph->PushGraphPass(PresentGraphPass);
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
		ImageSpec.State = ResourceState::ShaderRead;
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

