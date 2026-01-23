#include "DeafultExtensions.h"
#include "Ch_PCH.h"
#include "DeafultExtensions.h"
#include "Window/Window.h"
#include "Profiling\Timer.h"

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

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
		Ctxt.ServiceRegistry->RegisterService<MaterialSystem>(std::make_shared<MaterialSystem>(Ctxt));

		CH_CORE_TRACE("Graphcis Backend Using: {}", RenderService->GetName());
		auto RenderCommandService = Ctxt.ServiceRegistry->GetService<RenderCommand>();
	}

	void OnRenderExtensionsCleanUp(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		CHILLI_DEBUG_TIMER("OnRenderShutDown::");
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

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
		for (auto Mat : MaterialStore->GetHandles())
		{
			MaterialSystem->ClearMaterialData(Mat);
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
			RenderGraph->Passes[1].Pass.Info.RenderArea = { Event.GetX(), Event.GetY() };
			RenderGraph->Passes[2].Pass.Info.RenderArea = { Event.GetX(), Event.GetY() };
			RenderGraph->Passes[3].Pass.Info.RenderArea = { Event.GetX(), Event.GetY() };
		}
	}

	void OnRenderExtensionScreenPassRender(BackBone::SystemContext& Ctxt, RenderPassInfo& Pass)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

		auto DefferedResource = Command.GetResource< DefferedRenderingResource>();

		auto ActiveMaterial = DefferedResource->ScreenMaterial.ValPtr;
		auto ActiveShader = DefferedResource->ScreenShaderProgram;

		RenderService->BindShaderProgram(DefferedResource->ScreenShaderProgram.ValPtr->RawProgramHandle);
		RenderService->BindMaterailData(MaterialSystem->GetRawMaterialHandle(DefferedResource->ScreenMaterial));

		if (MaterialSystem->ShouldMaterialShaderDataUpdate(DefferedResource->ScreenMaterial))
		{
			MaterialShaderData MaterialData = MaterialSystem->GetMaterialShaderData(DefferedResource->ScreenMaterial);
			RenderService->UpdateMaterialShaderData(MaterialSystem->GetRawMaterialHandle(DefferedResource->ScreenMaterial), MaterialData);
		}

		auto MatIndex = RenderService->GetMaterialShaderIndex(MaterialSystem->GetRawMaterialHandle(DefferedResource->ScreenMaterial));

		DrawPushShaderInlineUniformData PushData;
		PushData.MaterialIndex = MatIndex;

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
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

		for (auto [Entity, Transform, MeshComp] : BackBone::QueryWithEntities<TransformComponent, MeshComponent>(*Ctxt.Registry))
		{
			auto ActiveShader = MaterialSystem->GetShaderProgramID(MeshComp->MaterialHandle);
			auto ActiveMesh = MeshComp->MeshHandle.ValPtr;
			auto RawMaterialHandle = MaterialSystem->GetRawMaterialHandle(MeshComp->MaterialHandle);

			RenderService->BindShaderProgram(ActiveShader.ValPtr->RawProgramHandle);
			RenderService->BindMaterailData(RawMaterialHandle);

			RenderService->BindVertexBuffer({ ActiveMesh->VBHandle.ValPtr->RawBufferHandle });
			RenderService->BindIndexBuffer(ActiveMesh->IBHandle.ValPtr->RawBufferHandle, IndexBufferType::UINT16_T);

			if (MaterialSystem->ShouldMaterialShaderDataUpdate(MeshComp->MaterialHandle))
			{
				MaterialShaderData MaterialData = MaterialSystem->GetMaterialShaderData(MeshComp->MaterialHandle);
				RenderService->UpdateMaterialShaderData(MaterialSystem->GetRawMaterialHandle(MeshComp->MaterialHandle), MaterialData);
			}

			if (Transform->IsDirty())
			{
				ObjectShaderData Data;
				Data.TransformationMat = Transform->GetWorldMatrix();
				RenderService->UpdateObjectShaderData(Entity, Data);
			}

			auto MatIndex = RenderService->GetMaterialShaderIndex(RawMaterialHandle);

			DrawPushShaderInlineUniformData PushData;
			PushData.MaterialIndex = MatIndex;
			PushData.ObjectIndex = RenderService->GetObjectShaderIndex(Entity);

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
		IVec2 WindowSize = { ActiveWindow->GetWidth(), ActiveWindow->GetHeight() };

		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

		RenderPassCompiler Compiler;
		uint32_t GeometryPassIndex = 0;
		uint32_t ScreenPassIndex = 0;
		uint32_t UIPepperPassIndex = 0;
		uint32_t PresentPassIndex = 0;
		auto GeometryPassMSAA = IMAGE_SAMPLE_COUNT_8_BIT;

		{
			ImageSpec ColorMSAAImageSpec;
			ColorMSAAImageSpec.Format = ImageFormat::RGBA8;
			ColorMSAAImageSpec.ImageData = nullptr;
			ColorMSAAImageSpec.MipLevel = 1;
			ColorMSAAImageSpec.Resolution.Width = WindowSize.x;
			ColorMSAAImageSpec.Resolution.Height = WindowSize.y;
			ColorMSAAImageSpec.Resolution.Depth = 1;
			ColorMSAAImageSpec.Type = ImageType::IMAGE_TYPE_2D;
			ColorMSAAImageSpec.Usage = IMAGE_USAGE_COLOR_ATTACHMENT | IMAGE_USAGE_TRANSIENT_ATTACHMENT;
			ColorMSAAImageSpec.State = ResourceState::RenderTarget;
			ColorMSAAImageSpec.Sample = GeometryPassMSAA;
			DefferedResource->GeometryColorMSAAImage = Command.AllocateImage(ColorMSAAImageSpec);

			TextureSpec ColorMSAATextureSpec;
			ColorMSAATextureSpec.Format = ColorMSAAImageSpec.Format;
			DefferedResource->GeometryColorMSAATexture = Command.CreateTexture(DefferedResource->GeometryColorMSAAImage, ColorMSAATextureSpec);

			ImageSpec ColorImageSpec;
			ColorImageSpec.Format = ImageFormat::RGBA8;
			ColorImageSpec.ImageData = nullptr;
			ColorImageSpec.MipLevel = 1;
			ColorImageSpec.Resolution.Width = WindowSize.x;
			ColorImageSpec.Resolution.Height = WindowSize.y;
			ColorImageSpec.Resolution.Depth = 1;
			ColorImageSpec.Type = ImageType::IMAGE_TYPE_2D;
			ColorImageSpec.Usage = IMAGE_USAGE_COLOR_ATTACHMENT | IMAGE_USAGE_SAMPLED_IMAGE;
			ColorImageSpec.State = ResourceState::ShaderRead;
			DefferedResource->GeometryColorImage = Command.AllocateImage(ColorImageSpec);
			
			TextureSpec ColorTextureSpec;
			ColorTextureSpec.Format = ColorImageSpec.Format;
			DefferedResource->GeometryColorTexture = Command.CreateTexture(DefferedResource->GeometryColorImage, ColorTextureSpec);

			ImageSpec DepthImageSpec;
			DepthImageSpec.Format = ImageFormat::D32F_S8I;
			DepthImageSpec.ImageData = nullptr;
			DepthImageSpec.MipLevel = 1;
			DepthImageSpec.Resolution.Width = WindowSize.x;
			DepthImageSpec.Resolution.Height = WindowSize.y;
			DepthImageSpec.Resolution.Depth = 1;
			DepthImageSpec.Type = ImageType::IMAGE_TYPE_2D;
			DepthImageSpec.Usage = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT | IMAGE_USAGE_SAMPLED_IMAGE;
			DepthImageSpec.State = ResourceState::ShaderRead;
			DepthImageSpec.Sample = GeometryPassMSAA;
			DefferedResource->GeometryDepthImage = Command.AllocateImage(DepthImageSpec);

			TextureSpec NewTextureSpec;
			NewTextureSpec.Format = DepthImageSpec.Format;
			DefferedResource->GeometryDepthTexture = Command.CreateTexture(DefferedResource->GeometryDepthImage, NewTextureSpec);

			TextureSpec DepthViewTextureSpec;
			DepthViewTextureSpec.Format = DepthImageSpec.Format;
			DepthViewTextureSpec.Aspect = IMAGE_ASPECT_DEPTH;
			DefferedResource->GeometryDepthViewTexture = Command.CreateTexture(DefferedResource->GeometryDepthImage, DepthViewTextureSpec);

			// 1. The Color Attachment (The Multisampled Buffer)
			ColorAttachment Color;
			// This MUST be the 4-sample MSAA image
			Color.ColorTexture = DefferedResource->GeometryColorMSAAImage.ValPtr->RawImageHandle;
			// This MUST be the 1-sample texture you want to use later
			Color.ResolveTexture = DefferedResource->GeometryColorImage.ValPtr->RawImageHandle;
			Color.LoadOp = AttachmentLoadOp::CLEAR;
			// Optimization: We don't need the 4x data after the pass, just the 1x resolve
			Color.StoreOp = AttachmentStoreOp::DONT_CARE;
			Color.ResolveOp = ResolveMode::AVERAGE;
			Color.ClearColor = { 0.2f, 0.8f, 0.2f, 1.0f };
			Color.UseSwapChainImage = false;

			DepthAttachment Depth;
			Depth.DepthTexture = DefferedResource->GeometryDepthTexture.ValPtr->RawTextureHandle;
			Depth.LoadOp = AttachmentLoadOp::CLEAR;
			Depth.StoreOp = AttachmentStoreOp::STORE;

			RenderPassBuilder GeometryPass("GeometryPass");

			GeometryPass.AddImageBarrier(true, DefferedResource->GeometryColorImage.ValPtr->RawImageHandle,
				ResourceState::ShaderRead, ResourceState::RenderTarget, false);
			GeometryPass.AddImageBarrier(false, DefferedResource->GeometryColorImage.ValPtr->RawImageHandle,
				ResourceState::RenderTarget, ResourceState::ShaderRead, false);

			GeometryPass.AddImageBarrier(true, DefferedResource->GeometryDepthImage.ValPtr->RawImageHandle,
				ResourceState::ShaderRead, ResourceState::DepthWrite, false);
			GeometryPass.AddImageBarrier(false, DefferedResource->GeometryDepthImage.ValPtr->RawImageHandle,
				ResourceState::DepthWrite, ResourceState::ShaderRead, false);
			GeometryPass.AddColor(Color);
			GeometryPass.SetDepth(Depth);

			auto PrePass = GetEmberPrePassPipelineBarrier(Ctxt);
			for (auto& Barrier : PrePass)
				GeometryPass.AddPrePipelineBarrier(Barrier);

			PrePass = GetEmberPostPassPipelineBarrier(Ctxt);
			for (auto& Barrier : PrePass)
				GeometryPass.AddPostPipelineBarrier(Barrier);

			GeometryPass.SetArea(WindowSize.x, WindowSize.y);

			GeometryPassIndex = Compiler.PushPass(GeometryPass.Build());
		}

		{
			Chilli::MeshCreateInfo SquareInfo{};

			std::vector<Chilli::Vertex2D> SquareVertices = {
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
			SquareInfo.VerticesSize = sizeof(Chilli::Vertex2D) * SquareVertices.size();
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

			DefferedResource->ScreenMaterial = Command.CreateMaterial(GeometryShaderProgram);
			MaterialSystem->SetAlbedoColor(DefferedResource->ScreenMaterial, { 1.0f, 1.0f, 1.0f, 1.0f });
			MaterialSystem->SetAlbedoTexture(DefferedResource->ScreenMaterial, DefferedResource->GeometryColorTexture);
			MaterialSystem->SetAlbedoSampler(DefferedResource->ScreenMaterial, Sampler);

			ColorAttachment Color;
			Color.LoadOp = AttachmentLoadOp::CLEAR;
			Color.StoreOp = AttachmentStoreOp::STORE;
			Color.UseSwapChainImage = true;
			Color.ClearColor = { 0.2f, 0.2f, 0.2f, 1.0f };

			RenderPassBuilder ScreenPass("ScreenPass");
			ScreenPass.AddColor(Color);
			ScreenPass.AddImageBarrier(true, UINT32_MAX, ResourceState::Present, ResourceState::RenderTarget, true);
			ScreenPass.SetArea(WindowSize.x, WindowSize.y);

			ScreenPassIndex = Compiler.PushPass(ScreenPass.Build());
		}

		{
			ColorAttachment Color;
			Color.LoadOp = AttachmentLoadOp::LOAD;
			Color.StoreOp = AttachmentStoreOp::STORE;
			Color.UseSwapChainImage = true;

			RenderPassBuilder UIPepperPass("UIPepperPass");
			UIPepperPass.AddColor(Color);
			UIPepperPass.SetArea(WindowSize.x, WindowSize.y);

			auto UIPepperPrePassBarriers = GetPepperPrePassPipelineBarrier(Ctxt);
			for (auto& Barrier : UIPepperPrePassBarriers)
				UIPepperPass.AddPrePipelineBarrier(Barrier);

			auto UIPepperPostPassBarriers = GetPepperPostPassPipelineBarrier(Ctxt);
			for (auto& Barrier : UIPepperPostPassBarriers)
				UIPepperPass.AddPostPipelineBarrier(Barrier);

			UIPepperPassIndex = Compiler.PushPass(UIPepperPass.Build());
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
			PresentPass.SetArea(WindowSize.x, WindowSize.y);

			PresentPassIndex = Compiler.PushPass(PresentPass.Build());
		}

		auto CompiledPasses = Compiler.Compile();

		PipelineBuilder PipelineBuilder;

		VertexInputShaderLayout GeometryLayout;
		GeometryLayout.BeginBinding(0);
		GeometryLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
		GeometryLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InNormal", 1);
		GeometryLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 2);

		RenderGraphPass GeometryGraphPass;
		GeometryGraphPass.DebugName = "GeometryPass";
		GeometryGraphPass.Pass = CompiledPasses[GeometryPassIndex];
		GeometryGraphPass.RenderFn = OnRenderExtensionGeometryPassRender;
		GeometryGraphPass.Info = PipelineBuilder.Default()
			.AddColorBlend(ColorBlendAttachmentState::OpaquePass())
			.SetDepth(true, true, CompareOp::LESS)
			.SetMSAA(
				GeometryPassMSAA, // samples: Must match your MSAA image (4)
				0xFFFFFFFF,               // mask: Enable all 32 possible samples (standard)
				false                     // alphaToCoverage: Keep false unless doing foliage/fences
			)
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
		ScreenGraphPass.Info = PipelineBuilder.Default()
			.AddColorBlend(ColorBlendAttachmentState::OpaquePass())
			.Build();
		ScreenGraphPass.Layout = ScreenLayout;
		ScreenGraphPass.SortOrder = 1;

		VertexInputShaderLayout UIPepperLayout;
		UIPepperLayout.BeginBinding(0);
		UIPepperLayout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
		UIPepperLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 1);
		UIPepperLayout.AddAttribute(ShaderObjectTypes::UINT1, "InPepperMaterialIndex", 2);

		RenderGraphPass UIPepperGraphPass;
		UIPepperGraphPass.DebugName = "UIPepperGraphPass";
		UIPepperGraphPass.Pass = CompiledPasses[UIPepperPassIndex];
		UIPepperGraphPass.RenderFn = OnPepperRender;
		UIPepperGraphPass.Info = PipelineBuilder.Default()
			.AddColorBlend(ColorBlendAttachmentState::OpaquePass())
			.Build();
		UIPepperGraphPass.Layout = UIPepperLayout;
		UIPepperGraphPass.SortOrder = 2;

		RenderGraphPass PresentGraphPass;
		PresentGraphPass.DebugName = "PresentPass";
		PresentGraphPass.Pass = CompiledPasses[PresentPassIndex];
		PresentGraphPass.RenderFn = [](BackBone::SystemContext& Ctxt, RenderPassInfo& Pass) {};
		PresentGraphPass.Info = PipelineBuilder.Default()
			.AddColorBlend(ColorBlendAttachmentState::OpaquePass())
			.Build();
		PresentGraphPass.SortOrder = 3;

		RenderGraph->PushGraphPass(GeometryGraphPass);
		RenderGraph->PushGraphPass(ScreenGraphPass);
		RenderGraph->PushGraphPass(UIPepperGraphPass);
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
		App.ServiceRegistry.RegisterService<SceneManager>(std::make_shared<SceneManager>(App.Ctxt.Registry));

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

		_Config.PepperConfig.MaxFramesInFlight = _Config.RenderConfig.Spec.MaxFrameInFlight;

		App.Extensions.AddExtension(std::make_unique<WindowExtension>(_Config.WindowConfig), true, &App);
		App.Extensions.AddExtension(std::make_unique<CameraExtension>(), true, &App);
		App.Extensions.AddExtension(std::make_unique<PepperExtension>(_Config.PepperConfig), true, &App);
		App.Extensions.AddExtension(std::make_unique<RenderExtension>(_Config.RenderConfig), true, &App);
		// Services etc...
	}

#pragma endregion

#pragma region Window System
	void WindowManager::InitDefaultCursors(BackBone::AssetStore<Cursor>* CursorStore)
	{
		Window::Setup();
		for (int i = 0; i < int(DeafultCursorTypes::Count); i++)
		{
			Cursor DeafultCursor;
			DeafultCursor.RawCursorHandle = Window::CreateCursor(DeafultCursorTypes(i));
			_DeafultCursors[i] = CursorStore->Add(DeafultCursor);
		}
	}

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
			EventService->Register< SetCursorEvent>();
		}
	}

	void OnWindowRun(BackBone::SystemContext& Registry)
	{
		auto InputService = Registry.ServiceRegistry->GetService<Input>();
		auto EventService = Registry.ServiceRegistry->GetService<EventHandler>();
		auto WindowService = Registry.ServiceRegistry->GetService<WindowManager>();

		EventService->Clear<WindowResizeEvent>();
		EventService->Clear<WindowMinimizedEvent>();
		EventService->Clear<WindowCloseEvent>();
		EventService->Clear<FrameBufferResizeEvent>();
		EventService->Clear<WindowCloseEvent>();
		EventService->Clear<KeyPressedEvent>();
		EventService->Clear<KeyRepeatEvent>();
		EventService->Clear<KeyReleasedEvent>();
		EventService->Clear<MouseButtonPressedEvent>();
		EventService->Clear<MouseButtonRepeatEvent>();
		EventService->Clear<MouseButtonReleasedEvent>();
		EventService->Clear<CursorPosEvent>();
		EventService->Clear<MouseScrollEvent>();

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
		App.AssetRegistry.RegisterStore<Cursor>();

		App.ServiceRegistry.RegisterService<Chilli::Input>(std::make_shared<Chilli::Input>());
		App.ServiceRegistry.RegisterService< EventHandler>(std::make_shared< EventHandler>());
		App.ServiceRegistry.RegisterService< WindowManager>(std::make_shared<WindowManager>(App.AssetRegistry.GetStore<Cursor>()));

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

	void Command::MapMeshVertexBufferData(BackBone::AssetHandle<Mesh> Handle, void* Data, uint32_t Count,
		size_t Size, uint32_t Offset)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<Chilli::RenderCommand>();
		auto MeshStore = _Ctxt.AssetRegistry->GetStore<Chilli::Mesh>();
		CH_CORE_ASSERT(MeshStore != nullptr, "NO MESH STORE: NOPE");
		CH_CORE_ASSERT(RenderCommandService != nullptr, "NO RENDER COMMAAND SERVICE: NOPE");

		auto Mesh = MeshStore->Get(Handle);

		CH_CORE_ASSERT(Mesh != nullptr, "Mesh Not Found");
		CH_CORE_ASSERT(Mesh->VBHandle.ValPtr->CreateInfo.SizeInBytes >= Size, "Given Size Exceeds Mesh Initialized Size");

		Mesh->VertexCount = Count;
		this->MapBufferData(Mesh->VBHandle, Data, Size, Offset);
	}

	void Command::MapMeshIndexBufferData(BackBone::AssetHandle<Mesh> Handle, void* Data, uint32_t Count,
		size_t Size, uint32_t Offset)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<Chilli::RenderCommand>();
		auto MeshStore = _Ctxt.AssetRegistry->GetStore<Chilli::Mesh>();
		CH_CORE_ASSERT(MeshStore != nullptr, "NO MESH STORE: NOPE");
		CH_CORE_ASSERT(RenderCommandService != nullptr, "NO RENDER COMMAAND SERVICE: NOPE");

		auto Mesh = MeshStore->Get(Handle);

		CH_CORE_ASSERT(Mesh != nullptr, "Mesh Not Found");
		CH_CORE_ASSERT(Mesh->IBHandle.ValPtr->CreateInfo.SizeInBytes >= Size, "Given Size Exceeds Mesh Initialized Size");

		Mesh->IndexCount = Count;
		this->MapBufferData(Mesh->IBHandle, Data, Size, Offset);
	}

	BackBone::AssetHandle<Mesh> Command::CreateMesh(BasicShapes Shape)
	{
		std::vector<Chilli::Vertex> Vertices;
		std::vector<uint32_t> Indices;

		switch (Shape)
		{
		case BasicShapes::TRIANGLE:
		{
			Vertices = {
				{ {  0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.5f, 0.0f } },
				{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
				{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } }
			};
			Indices = { 0, 1, 2 };
			break;
		}
		case BasicShapes::QUAD:
		{
			Vertices = {
				{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
				{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
				{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }
			};
			Indices = { 0, 1, 2, 2, 3, 0 };
			break;
		}
		case BasicShapes::CUBE:
		{
			// Using your provided cube data
			Vertices = {
				// Front, Back, Left, Right, Top, Bottom (24 vertices)
				{{-0.5f,-0.5f, 0.5f},{0,0, 1},{0,1}}, {{ 0.5f,-0.5f, 0.5f},{0,0, 1},{1,1}}, {{ 0.5f, 0.5f, 0.5f},{0,0, 1},{1,0}}, {{-0.5f, 0.5f, 0.5f},{0,0, 1},{0,0}},
				{{ 0.5f,-0.5f,-0.5f},{0,0,-1},{0,1}}, {{-0.5f,-0.5f,-0.5f},{0,0,-1},{1,1}}, {{-0.5f, 0.5f,-0.5f},{0,0,-1},{1,0}}, {{ 0.5f, 0.5f,-0.5f},{0,0,-1},{0,0}},
				{{-0.5f,-0.5f,-0.5f},{-1,0,0},{0,1}}, {{-0.5f,-0.5f, 0.5f},{-1,0,0},{1,1}}, {{-0.5f, 0.5f, 0.5f},{-1,0,0},{1,0}}, {{-0.5f, 0.5f,-0.5f},{-1,0,0},{0,0}},
				{{ 0.5f,-0.5f, 0.5f},{ 1,0,0},{0,1}}, {{ 0.5f,-0.5f,-0.5f},{ 1,0,0},{1,1}}, {{ 0.5f, 0.5f,-0.5f},{ 1,0,0},{1,0}}, {{ 0.5f, 0.5f, 0.5f},{ 1,0,0},{0,0}},
				{{-0.5f, 0.5f, 0.5f},{ 0,1,0},{0,1}}, {{ 0.5f, 0.5f, 0.5f},{ 0,1,0},{1,1}}, {{ 0.5f, 0.5f,-0.5f},{ 0,1,0},{1,0}}, {{-0.5f, 0.5f,-0.5f},{ 0,1,0},{0,0}},
				{{-0.5f,-0.5f,-0.5f},{ 0,-1,0},{0,1}}, {{ 0.5f,-0.5f,-0.5f},{ 0,-1,0},{1,1}}, {{ 0.5f,-0.5f, 0.5f},{ 0,-1,0},{1,0}}, {{-0.5f,-0.5f, 0.5f},{ 0,-1,0},{0,0}}
			};
			Indices = {
				0,1,2, 2,3,0, 4,5,6, 6,7,4, 8,9,10, 10,11,8,
				12,13,14, 14,15,12, 16,17,18, 18,19,16, 20,21,22, 22,23,20
			};
			break;
		}
		case BasicShapes::SPHERE:
		{
			const unsigned int X_SEGMENTS = 32;
			const unsigned int Y_SEGMENTS = 32;
			const float PI = 3.14159265359f;

			for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
			{
				for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
				{
					float xSegment = (float)x / (float)X_SEGMENTS;
					float ySegment = (float)y / (float)Y_SEGMENTS;

					// Calculate spherical coordinates
					float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
					float yPos = std::cos(ySegment * PI); // Top is 1, bottom is -1
					float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

					Chilli::Vertex v;
					v.Position = { xPos * 0.5f, yPos * 0.5f, zPos * 0.5f }; // 0.5 radius
					v.Normal = { xPos, yPos, zPos }; // On a unit sphere, position = normal
					v.UV = { xSegment, ySegment };

					Vertices.push_back(v);
				}
			}

			// Generate Indices
			for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
			{
				for (unsigned int x = 0; x < X_SEGMENTS; ++x)
				{
					Indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					Indices.push_back(y * (X_SEGMENTS + 1) + x);
					Indices.push_back(y * (X_SEGMENTS + 1) + (x + 1));

					Indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					Indices.push_back(y * (X_SEGMENTS + 1) + (x + 1));
					Indices.push_back((y + 1) * (X_SEGMENTS + 1) + (x + 1));
				}
			}
			break;
		}
		}

		Chilli::MeshCreateInfo Info{};
		Info.VertCount = Vertices.size();
		Info.Vertices = Vertices.data();
		Info.VerticesSize = sizeof(Chilli::Vertex) * Vertices.size();
		Info.IndexCount = Indices.size();
		Info.Indicies = Indices.data();
		Info.IndiciesSize = sizeof(uint32_t) * Indices.size();
		Info.IndexType = Chilli::IndexBufferType::UINT32_T;
		Info.State = Chilli::BufferState::STATIC_DRAW;

		// Use your existing CreateMesh(MeshCreateInfo) to talk to the GPU
		return CreateMesh(Info);
	}

	void Command::DestroyMesh(BackBone::AssetHandle<Mesh> mesh, bool Free)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<Chilli::RenderCommand>();
		auto MeshStore = _Ctxt.AssetRegistry->GetStore<Chilli::Mesh>();
		CH_CORE_ASSERT(MeshStore != nullptr, "NO MESH STORE: NOPE");
		CH_CORE_ASSERT(RenderCommandService != nullptr, "NO RENDER COMMAAND SERVICE: NOPE");

		auto Mesh = MeshStore->Get(mesh);

		CH_CORE_ASSERT(Mesh != nullptr, "Mesh Not Found");

		DestroyBuffer(Mesh->VBHandle);
		if (Mesh->IndexCount != 0)
			DestroyBuffer(Mesh->IBHandle);

		if (Free)
			MeshStore->Remove(mesh);
	}

	void Command::FreeMesh(BackBone::AssetHandle<Mesh> mesh)
	{
		DestroyMesh(mesh, true);
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

	BackBone::AssetHandle<EmberFont> Command::LoadFont_TTF(const std::string& Path)
	{
		auto FontStore = GetStore<EmberFont>();

		EmberFont Font;
		Font.FontSource = this->LoadAsset<STBITTF_Data>(Path);

		auto Spec = Font.FontSource.ValPtr->GetImageCreateInfo();
		Font.FontImage = this->AllocateImage(Spec);
		MapImageData(Font.FontImage, Font.FontSource.ValPtr->Pixels, Font.FontSource.ValPtr->Width,
			Font.FontSource.ValPtr->Height);
		TextureSpec TexSpec;
		TexSpec.Format = Font.FontImage.ValPtr->Spec.Format;

		Font.FontTexture = this->CreateTexture(Font.FontImage, TexSpec);

		auto EmberResource = GetResource<Chilli::EmberResource>();
		EmberResource->UpdateFontAtlasTextureMap(GetService<Renderer>(), GetService<MaterialSystem>(), Font.FontTexture);

		return FontStore->Add(Font);
	}

	void Command::UnLoadFont_TTF(BackBone::AssetHandle<EmberFont> FontHandle)
	{
		auto FontStore = GetStore<EmberFont>();

		UnloadAsset(FontHandle.ValPtr->FontSource.ValPtr->FilePath);
		DestroyTexture(FontHandle.ValPtr->FontTexture);
		DestroyImage(FontHandle.ValPtr->FontImage);

		FontStore->Remove(FontHandle);
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

	BackBone::AssetHandle<Material> Command::CreateMaterial(BackBone::AssetHandle<ShaderProgram> Program)
	{
		auto MaterialSystem = this->GetService<Chilli::MaterialSystem>();

		return MaterialSystem->CreateMaterial(Program);
	}

	void Command::DestroyMaterial(const BackBone::AssetHandle<Material>& Mat)
	{
		auto MaterialSystem = this->GetService<Chilli::MaterialSystem>();

		MaterialSystem->DestroyMaterial(Mat);
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

	BackBone::AssetHandle<Image> Command::AllocateImage(ImageSpec& Spec)
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
		if (MipLevel == -1)
			ImageSpec.MipLevel = static_cast<uint32_t>(std::floor(std::log2(std::max(ImageData.ValPtr->Resolution.x, ImageData.ValPtr->Resolution.y)))) + 1;
		else
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

	BackBone::AssetHandle<Texture> Command::CreateTexture(const BackBone::AssetHandle<Image>& ImageHandle,  TextureSpec& Spec)
	{
		auto TextureStore = GetStore<Texture>();
		auto RenderCommandService = GetService<RenderCommand>();

		Texture NewTexture;
		NewTexture.ImageHandle = ImageHandle;
		NewTexture.RawTextureHandle = RenderCommandService->CreateTexture(
			ImageHandle.ValPtr->RawImageHandle,
			Spec
		);
		NewTexture.Spec = Spec;

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

#pragma region Camera Extension
	void OnCameraSystem(Chilli::BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Ctxt.ServiceRegistry->GetService<Renderer>();
		auto Window = Command.GetService<WindowManager>()->GetActiveWindow();
		auto ActiveScene = Command.GetService<SceneManager>()->GetActiveScene();

		for (auto [Entity, Camera, Transform] :
			BackBone::QueryWithEntities<CameraComponent, TransformComponent>(*Ctxt.Registry))
		{
			if (ActiveScene->MainCamera != Entity)
				continue;
			// 1. Get the pre-calculated World Matrix (which includes your Pitch/Yaw)
			glm::mat4 Model = Transform->GetWorldMatrix();

			// 2. Extract Position and Directions from the Matrix
			// Column 3 is Position, Column 2 is Z-Axis (Forward), Column 1 is Y-Axis (Up)
			glm::vec3 CameraPos = glm::vec3(Model[3]);

			// In Vulkan/OpenGL, Forward is usually -Z
			glm::vec3 Forward = -glm::vec3(Model[2]);
			glm::vec3 Up = glm::vec3(Model[1]);

			// 3. Create the View Matrix looking at (Pos + Forward)
			glm::mat4 view = glm::lookAt(CameraPos, CameraPos + Forward, Up);

			// 4. Calculate Projection
			glm::mat4 projection;
			if (Camera->Is_Orthro)
			{
				projection = glm::ortho(-Camera->Orthro_Size.x, Camera->Orthro_Size.x,
					-Camera->Orthro_Size.y, Camera->Orthro_Size.y,
					Camera->Near_Clip, Camera->Far_Clip);
			}
			else {
				projection = glm::perspective(glm::radians(Camera->Fov),
					Window->GetAspectRatio(),
					Camera->Near_Clip, Camera->Far_Clip);
			}

			// 5. Build and Push Scene Data
			SceneData SceneData{};
			SceneData.CameraPos = { CameraPos.x, CameraPos.y, CameraPos.z, 1.0f };

			// Final Matrix: Projection * View
			SceneData.ViewProjMatrix = projection * view;

			RenderService->PushUpdateSceneShaderData(0, SceneData);

			break;
		}
	}
	void Update3DCamera(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Input = Ctxt.ServiceRegistry->GetService<Chilli::Input>();
		auto FrameData = Command.GetResource<Chilli::BackBone::GenericFrameData>();
		auto Scene = Command.GetService<SceneManager>();
		auto ActiveScene = Scene->GetActiveScene();
		float DT = FrameData->Ts.GetSecond();

		for (auto [Entity, Camera, Control, Transform] :
			BackBone::QueryWithEntities<CameraComponent, Deafult3DCameraController, TransformComponent>(*Ctxt.Registry))
		{
			if (ActiveScene->MainCamera != Entity)
				continue;

			// 1. Handle Rotation ONLY on Left Mouse Button
			if (Input->IsMouseButtonDown(Input_mouse_Left))
			{
				// Get movement since last frame
				IVec2 Mouse_Delta = Input->GetCursorDelta();

				Control->Yaw += Mouse_Delta.x * Control->Look_Sensitivity;

				float Y_Mult = Control->Invert_Y ? 1.0f : -1.0f;
				Control->Pitch += Mouse_Delta.y * Control->Look_Sensitivity * Y_Mult;

				// Constrain Pitch to avoid flipping the camera over the poles
				Control->Pitch = glm::clamp(Control->Pitch, -89.0f, 89.0f);

				Transform->SetRotation({ Control->Pitch, Control->Yaw, 0.0f });
			}

			// 2. Handle Movement (WASD + Space/Ctrl)
			// We get the current matrix to extract the 'Look' vectors
			glm::mat4 Mat = Transform->GetWorldMatrix();

			// Extract Forward (-Z) and Right (+X) from matrix columns
			Vec3 Forward = -Vec3(Mat[2].x, Mat[2].y, Mat[2].z);
			Vec3 Right = Vec3(Mat[0].x, Mat[0].y, Mat[0].z);

			float S = Control->Move_Speed * DT;

			// Horizontal Movement
			if (Input->IsKeyDown(Input_key_W)) Transform->Move(Forward * S);
			if (Input->IsKeyDown(Input_key_S)) Transform->Move(-Forward * S);
			if (Input->IsKeyDown(Input_key_A)) Transform->Move(-Right * S);
			if (Input->IsKeyDown(Input_key_D)) Transform->Move(Right * S);

			// Vertical Movement
			if (Input->IsKeyDown(Input_key_Space))       Transform->MoveY(S);
			if (Input->IsKeyDown(Input_key_LeftControl)) Transform->MoveY(-S);

			break;
		}
	}

	void CameraExtension::Build(BackBone::App& App)
	{
		auto Command = Chilli::Command(App.Ctxt);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, Update3DCamera);
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

			return Chilli::BackBone::Entity();
		}
	}
#pragma endregion
#pragma region Ember
	BackBone::AssetHandle<STBITTF_Data> STBITTF_Loader::LoadTyped(BackBone::SystemContext& Ctxt, const std::string& Path)
	{
		auto Command = Chilli::Command(Ctxt);
		auto StbiStore = Command.GetStore<STBITTF_Data>();

		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));

		int Index = IndexEntry::FindIndex(_IndexMaps, HashValue);
		if (Index != -1)
			return _STBITTF_Handles[Index];

		// 1. Read TTF file
		std::ifstream file(Path, std::ios::binary | std::ios::ate);
		if (!file.is_open()) return {};

		size_t fileSize = file.tellg();
		std::vector<unsigned char> ttfBuffer(fileSize);
		file.seekg(0);
		file.read((char*)ttfBuffer.data(), fileSize);

		// 2. Init Font Info
		stbtt_fontinfo fontInfo;
		if (!stbtt_InitFont(&fontInfo, ttfBuffer.data(), 0)) return {};

		STBITTF_Data fontData;
		fontData.FilePath = Path;
		fontData.FontSize = 32.0f;

		// Get Vertical Metrics
		int ascent, descent, lineGap;
		stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
		float scale = stbtt_ScaleForPixelHeight(&fontInfo, fontData.FontSize);

		fontData.Ascent = (float)ascent * scale;
		fontData.Descent = (float)descent * scale;
		fontData.LineGap = (float)lineGap * scale;

		// 3. Determine Atlas Size (1024 is required for LoveDays + Oversampling)
		fontData.Width = 1024;
		fontData.Height = 1024;
		fontData.Pixels = malloc(fontData.Width * fontData.Height);
		memset(fontData.Pixels, 0, fontData.Width * fontData.Height);
		fontData.FileName = GetFileNameWithExtension(Path);

		// 4. Pack Font into Atlas
		stbtt_pack_context packCtxt;
		if (!stbtt_PackBegin(&packCtxt, (unsigned char*)fontData.Pixels,
			fontData.Width, fontData.Height, 0, 1, nullptr))
		{
			free(fontData.Pixels);
			return {};
		}

		stbtt_PackSetOversampling(&packCtxt, 2, 2);

		stbtt_packedchar tempPacked[96];

		// --- FIX STARTS HERE ---
		// Initialize the struct to zero so that pointers like array_of_unicode_codepoints are NULL
		stbtt_pack_range range{};
		// Alternatively: memset(&range, 0, sizeof(range));

		range.font_size = fontData.FontSize;
		range.first_unicode_codepoint_in_range = 32;
		range.num_chars = 96;
		range.chardata_for_range = tempPacked;
		// range.array_of_unicode_codepoints is now NULL because of the {} initialization
		// --- FIX ENDS HERE ---

		if (!stbtt_PackFontRanges(&packCtxt, ttfBuffer.data(), 0, &range, 1))
		{
			stbtt_PackEnd(&packCtxt);
			free(fontData.Pixels);
			return {};
		}
		stbtt_PackEnd(&packCtxt);

		// 5. Convert to your Engine's BakedChar format
		for (int i = 0; i < 96; ++i) {
			fontData.BakedChars[i].x0 = tempPacked[i].x0;
			fontData.BakedChars[i].y0 = tempPacked[i].y0;
			fontData.BakedChars[i].x1 = tempPacked[i].x1;
			fontData.BakedChars[i].y1 = tempPacked[i].y1;
			fontData.BakedChars[i].xoff = tempPacked[i].xoff;
			fontData.BakedChars[i].yoff = tempPacked[i].yoff;
			fontData.BakedChars[i].xadvance = tempPacked[i].xadvance;
			fontData.BakedChars[i].xoff2 = tempPacked[i].xoff2;
			fontData.BakedChars[i].yoff2 = tempPacked[i].yoff2;
		}

		auto StbiDataHandle = StbiStore->Add(fontData);

		// Prevent destructor from freeing the memory we just stored
		fontData.Pixels = nullptr;

		uint32_t NewIndex = static_cast<uint32_t>(_STBITTF_Handles.size());
		_STBITTF_Handles.push_back(StbiDataHandle);
		IndexEntry::AddOrUpdateSorted(_IndexMaps, { HashValue, NewIndex });

		return StbiDataHandle;
	}
	void STBITTF_Loader::Unload(BackBone::SystemContext& Ctxt, const std::string& Path)
	{
		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));

		// 1. Find the local index in our tracking vector
		int LocalIndex = IndexEntry::FindIndex(_IndexMaps, HashValue);

		if (LocalIndex != -1 && LocalIndex < _STBITTF_Handles.size()) {
			auto Handle = _STBITTF_Handles[LocalIndex];

			if (Handle.IsValid()) {
				// 2. Remove from the global store (StbiStore)
				// This is crucial for freeing the malloc'd pixels via the STBITTF_Data destructor
				auto Command = Chilli::Command(Ctxt);
				auto StbiStore = Command.GetStore<STBITTF_Data>();
				StbiStore->Remove(Handle);

				// 3. Clear our local handle to drop the reference
				_STBITTF_Handles[LocalIndex] = BackBone::AssetHandle<STBITTF_Data>();
			}

			// 4. Mark the index entry as invalid so it's not found again
			IndexEntry::Invalidate(_IndexMaps, HashValue);
		}
	}

	void STBITTF_Loader::Unload(BackBone::SystemContext& Ctxt, const BackBone::AssetHandle<STBITTF_Data>& Data)
	{
		if (Data.IsValid()) {
			// Redirect to the path-based unload to keep logic centralized
			Unload(Ctxt, Data.ValPtr->FilePath);
		}
	}

	bool STBITTF_Loader::DoesExist(const std::string& Path) const
	{
		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));
		int Index = IndexEntry::FindIndex(_IndexMaps, HashValue);
		if (Index != -1)
			return true;
		return false;
	}

	void OnEmberStartUp(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto EmberResource = Command.GetResource<Chilli::EmberResource>();
		auto RenderService = Command.GetService<Renderer>();
		auto Config = Command.GetResource<Chilli::EmberExtensionConfig>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
		Ctxt.AssetRegistry->RegisterStore<STBITTF_Data>();
		Ctxt.AssetRegistry->RegisterStore<EmberFont>();

		Command.AddLoader<STBITTF_Loader>();

		// Prepare Shader
		auto PepperVertexShader = Command.CreateShaderModule("Assets/Shaders/ember_vert.spv",
			Chilli::ShaderStageType::SHADER_STAGE_VERTEX);
		auto PepperFragShader = Command.CreateShaderModule("Assets/Shaders/ember_frag.spv",
			Chilli::ShaderStageType::SHADER_STAGE_FRAGMENT);

		EmberResource->EmberShaderProgram = Command.CreateShaderProgram();
		Command.AttachShaderModule(EmberResource->EmberShaderProgram, PepperVertexShader);
		Command.AttachShaderModule(EmberResource->EmberShaderProgram, PepperFragShader);
		Command.LinkShaderProgram(EmberResource->EmberShaderProgram);

		EmberResource->ContextMaterial = Command.CreateMaterial(EmberResource->EmberShaderProgram);
		{
			// 1. Define the 4 corners of the quad (Pos and UVs are handled by instance math)
			EmberFontVertex quadVertices[] = {
				{ {0.0f, 0.0f} }, { {1.0f, 0.0f} },
				{ {0.0f, 1.0f} }, { {1.0f, 1.0f} }
			};

			BufferCreateInfo EmberVertexVBInfo{};
			EmberVertexVBInfo.Data = &quadVertices;
			EmberVertexVBInfo.SizeInBytes = sizeof(EmberFontVertex) * 4;
			EmberVertexVBInfo.State = BufferState::STATIC_DRAW;
			EmberVertexVBInfo.Type = BUFFER_TYPE_VERTEX;

			EmberResource->EmberVertexVB = Command.CreateBuffer(EmberVertexVBInfo, "EmberVertexBuffer");
		}
		{
			uint32_t quadIndices[] = { 0, 1, 2, 1, 3, 2 };

			BufferCreateInfo Infp{};
			Infp.Data = quadIndices;
			Infp.SizeInBytes = sizeof(uint32_t) * 6;
			Infp.State = BufferState::STATIC_DRAW;
			Infp.Type = BUFFER_TYPE_INDEX;

			EmberResource->EmberVertexIB = Command.CreateBuffer(Infp, "EmberIndexBuffer");
		}
		{
			BufferCreateInfo Infp{};
			Infp.Data = nullptr;
			Infp.SizeInBytes = sizeof(EmberFontInstance) * EmberResource->MaxCharacterCount;
			Infp.State = BufferState::DYNAMIC_DRAW;
			Infp.Type = BUFFER_TYPE_VERTEX;

			EmberResource->EmberFontVBs.resize(Config->MaxFrameInFlight);

			for (auto& Buffer : EmberResource->EmberFontVBs)
				Buffer = Command.CreateBuffer(Infp, "FontVertexBuffer");
		}


		Chilli::SamplerSpec SamplerSpec;
		SamplerSpec.Filter = Chilli::SamplerFilter::LINEAR;
		SamplerSpec.Mode = Chilli::SamplerMode::CLAMP_TO_BORDER;
		auto Sampler = Command.CreateSampler(SamplerSpec);

		RenderService->UpdateMaterialSamplerData(MaterialSystem->GetRawMaterialHandle(EmberResource->ContextMaterial),
			Sampler.ValPtr->SamplerHandle, "fontAtlasSampler", 0);
	}

	void OnEmberUpdate(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();
		auto EmberResource = Command.GetResource<Chilli::EmberResource>();

		EmberResource->EmberInstanceData.clear();
		EmberResource->GeoCharacterCount = 0;
		EmberResource->UICharacterCount = 0;
		// --- PASS 1: GEOMETRY (World Space) ---
		for (auto [Transform, Text] : BackBone::Query<TransformComponent, EmberTextComponent>(*Ctxt.Registry))
		{
			if (!Text->Font.IsValid())
				continue;

			auto fontData = Text->Font.ValPtr->FontSource.ValPtr;

			const Vec3& pos = Transform->GetPosition();
			const Vec3& scale = Transform->GetScale();

			float curX = pos.x;
			float curY = pos.y;
			float s = scale.x;

			for (char c : Text->Content)
			{
				if (c == '\n')
				{
					curX = pos.x;
					curY += fontData->GetLineHeight() * s;
					continue;
				}

				if (c < 32 || c > 126)
					continue;

				const auto& bc = fontData->GetChar(c);

				EmberFontInstance instance;
				instance.instPos = {
					curX + (bc.xoff * s),
					curY + (bc.yoff * s)
				};

				instance.instSize = {
					(bc.xoff2 - bc.xoff) * s,
					(bc.yoff2 - bc.yoff) * s
				};

				fontData->GetUVs(c, instance.instUVOffset, instance.instUVRange);
				instance.FontIndex =
					EmberResource->GetFontAtlasTextureShaderIndex(
						Text->Font.ValPtr->FontTexture
					);

				EmberResource->EmberInstanceData.push_back(instance);
				EmberResource->GeoCharacterCount++;

				curX += bc.xadvance * s;
			}
		}

		// --- PASS 2: UI (Pepper Space) ---
		for (auto [Transform, Text] : BackBone::Query<PepperTransform, EmberTextComponent>(*Ctxt.Registry))
		{
			if (!Text->Font.IsValid()) continue;
			auto fontData = Text->Font.ValPtr->FontSource.ValPtr;

			// 1. Apply the Pivot offset to find the top-left start of the text box
			// This matches the logic: pixelX0 = transform.ActualPosition.x - pivot.x
			float startX = Transform->ActualPosition.x - static_cast<float>(Transform->Pivot.x);
			float startY = Transform->ActualPosition.y - static_cast<float>(Transform->Pivot.y);

			float curX = startX;
			float curY = startY;

			float s = Text->FontSize / fontData->FontSize;

			for (char c : Text->Content)
			{
				if (c == '\n') {
					curX = startX; // Reset to the pivoted start X
					curY += fontData->GetLineHeight() * s;
					continue;
				}
				if (c < 32 || c > 126) continue;

				const auto& bc = fontData->GetChar(c);
				EmberFontInstance instance;

				// 2. Position the character
				// We add the character offset to our pivoted start coordinates
				instance.instPos.x = curX + (bc.xoff * s);

				// Note: Check your Y direction. 
				// If your text is drawing upside down or offset, change '-' to '+' 
				// depending on if your font metrics are Y-up or Y-down.
				instance.instPos.y = curY - (bc.yoff * s);

				instance.instSize = { (bc.xoff2 - bc.xoff) * s, (bc.yoff2 - bc.yoff) * s };

				fontData->GetUVs(c, instance.instUVOffset, instance.instUVRange);
				instance.FontIndex = EmberResource->GetFontAtlasTextureShaderIndex(Text->Font.ValPtr->FontTexture);

				EmberResource->EmberInstanceData.push_back(instance);
				EmberResource->UICharacterCount++;

				curX += bc.xadvance * s;
			}
		}

		// --- UPLOAD ALL DATA ---
		if (!EmberResource->EmberInstanceData.empty()) {
			Command.MapBufferData(EmberResource->EmberFontVBs[RenderService->GetCurrentFrameIndex()],
				EmberResource->EmberInstanceData.data(),
				sizeof(EmberFontInstance) * EmberResource->EmberInstanceData.size(), 0);
		}
	}

	void OnEmberRenderUIPass(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();

		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

		auto EmberResource = Command.GetResource<Chilli::EmberResource>();

		auto ActiveMaterial = EmberResource->ContextMaterial.ValPtr;
		auto ActiveShader = EmberResource->EmberShaderProgram;
		auto ActiveWindow = Command.GetActiveWindow();

		VertexInputShaderLayout EmberLayout;

		// Binding 0: Static Unit Quad (Standard Vertex Rate)
		// This buffer typically contains 4-6 vertices forming a (0,0) to (1,1) square
		EmberLayout.BeginBinding(0, false); // isInstanced = false
		EmberLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "inVertexPos", 0);

		// Binding 1: Character Instance Data (Instance Rate)
		// The GPU will step through this buffer once per character drawn
		EmberLayout.BeginBinding(1, true); // isInstanced = true
		EmberLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "instPos", 1);
		EmberLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "instSize", 2);
		EmberLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "instUVOffset", 3);
		EmberLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "instUVRange", 4);
		EmberLayout.AddAttribute(ShaderObjectTypes::INT1, "instFontIndex", 5);
		RenderService->SetVertexInputLayout(EmberLayout);

		PipelineBuilder UITextPipelineInfoBuilder;
		RenderService->SetFullPipelineState(UITextPipelineInfoBuilder.
			Default()
			.AddColorBlend(ColorBlendAttachmentState::AlphaBlend())
			.SetRasterizer(CullMode::None, FrontFaceMode::Counter_Clock_Wise, PolygonMode::Fill)
			.Build());

		RenderService->BindShaderProgram(EmberResource->EmberShaderProgram.ValPtr->RawProgramHandle);
		RenderService->BindMaterailData(MaterialSystem->GetRawMaterialHandle(EmberResource->ContextMaterial));

		struct Push {
			alignas(16) glm::mat4 projection;
			alignas(16) Vec4 textColor;
		}pc;

		// Use ortho projection
		float width = (float)ActiveWindow->GetWidth();
		float height = (float)ActiveWindow->GetHeight();

		glm::mat4 projection = glm::mat4(1.0f);
		projection[0][0] = 2.0f / width;
		projection[1][1] = -2.0f / height;  // Note the negative for Vulkan Y-down
		projection[2][2] = 1.0f;
		projection[3][0] = -1.0f;
		projection[3][1] = 1.0f;
		projection[3][3] = 1.0f;

		pc.projection = projection;
		//pc.projection[1][1] *= -1; // Flip Y for Vulkan coordinate system
		//pc.projection = glm::mat4(1.0f);
		pc.textColor = { 1.0f, 1.0f, 1.0f, 1.0f };

		RenderService->PushInlineUniformData(ActiveShader.ValPtr->RawProgramHandle,
			SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, &pc, sizeof(pc), 0);

		RenderService->BindVertexBuffer({
			EmberResource->EmberVertexVB.ValPtr->RawBufferHandle,
			EmberResource->EmberFontVBs[RenderService->GetCurrentFrameIndex()].ValPtr->RawBufferHandle
			});
		RenderService->BindIndexBuffer(EmberResource->EmberVertexIB.ValPtr->RawBufferHandle, IndexBufferType::UINT32_T);

		RenderService->DrawIndexed(6, EmberResource->UICharacterCount, 0, 0, EmberResource->GeoCharacterCount);
	}

	std::vector<PipelineBarrier> GetEmberPrePassPipelineBarrier(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();
		auto EmberResource = Command.GetResource<Chilli::EmberResource>();

		PipelineBarrier InstanceVertexBarrier{};
		InstanceVertexBarrier.BufferBarrier.Handle = EmberResource->EmberFontVBs[RenderService->GetCurrentFrameIndex()].ValPtr->RawBufferHandle;
		InstanceVertexBarrier.BufferBarrier.Offset = 0;
		InstanceVertexBarrier.BufferBarrier.Size = CH_BUFFER_WHOLE_SIZE;
		InstanceVertexBarrier.OldState = ResourceState::HostWrite;
		InstanceVertexBarrier.NewState = ResourceState::VertexRead;

		return { InstanceVertexBarrier };
	}

	std::vector<PipelineBarrier> GetEmberPostPassPipelineBarrier(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();
		auto EmberResource = Command.GetResource<Chilli::EmberResource>();

		PipelineBarrier InstanceVertexBarrier{};
		InstanceVertexBarrier.BufferBarrier.Handle = EmberResource->EmberFontVBs[RenderService->GetCurrentFrameIndex()].ValPtr->RawBufferHandle;
		InstanceVertexBarrier.BufferBarrier.Offset = 0;
		InstanceVertexBarrier.BufferBarrier.Size = CH_BUFFER_WHOLE_SIZE;
		InstanceVertexBarrier.OldState = ResourceState::VertexRead;
		InstanceVertexBarrier.NewState = ResourceState::HostWrite;

		return { InstanceVertexBarrier };
	}

	void EmberResource::UpdateFontAtlasTextureMap(Renderer* RenderService, MaterialSystem* MaterialSystem,
		BackBone::AssetHandle<Texture> Tex)
	{
		ShaderArrayFontAtlasMetaData Data;
		Data.Tex = Tex;
		Data.ShaderIndex = _FontAtlasTextureMapCount;
		FontAtlasTextureMap.Insert(Tex.Handle, Data);

		RenderService->UpdateMaterialTextureData(MaterialSystem->GetRawMaterialHandle(this->ContextMaterial),
			Tex.ValPtr->RawTextureHandle, "fontAtlas", ResourceState::ShaderRead, _FontAtlasTextureMapCount);
	}

	void EmberExtension::Build(BackBone::App& App)
	{
		App.Registry.AddResource<EmberResource>();
		App.Registry.AddResource<EmberExtensionConfig>();

		auto Config = App.Registry.GetResource<EmberExtensionConfig>();
		*Config = _Config;

		App.Registry.Register<Chilli::EmberTextComponent>();

		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::START_UP, OnEmberStartUp);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnEmberUpdate);
	}
#pragma endregion

#pragma region Pepper

#pragma region Pepper Material System

	BackBone::AssetHandle<Texture> Pepper::GetDeafultPepperTexture()
	{
		return GetPepperResource()->PepperDeafultTexture;
	}

	bool Pepper::ShouldMaterialShaderDataUpdate(BackBone::Entity Entity)
	{
		auto PepperMat = GetMaterial(Entity);
		return PepperMat->Version != PepperMat->LastUploadedVersion;
	}

	PepperShaderMaterialData Pepper::GetMaterialShaderData(BackBone::Entity Entity)
	{
		auto Command = Chilli::Command(_Ctxt);
		auto RenderService = Command.GetService<Renderer>();

		auto Mat = GetMaterial(Entity);
		Mat->LastUploadedVersion = Mat->Version;

		auto PepperResource = GetPepperResource();

		PepperShaderMaterialData Data;
		Data.Color = Mat->Color;

		if (Mat->TextureHandle.Handle == BackBone::npos)
			Data.TextureIndex = RenderService->GetTextureShaderIndex(PepperResource->PepperDeafultTexture.ValPtr->RawTextureHandle);
		else
			Data.TextureIndex = RenderService->GetTextureShaderIndex(Mat->TextureHandle.ValPtr->RawTextureHandle);

		if (Mat->SamplerHandle.Handle == BackBone::npos)
			Data.SamplerIndex = RenderService->GetSamplerShaderIndex(PepperResource->PepperDeafultSampler.ValPtr->SamplerHandle);
		else
			Data.SamplerIndex = RenderService->GetSamplerShaderIndex(Mat->SamplerHandle.ValPtr->SamplerHandle);

		return Data;
	}

	PepperMaterial* Pepper::GetMaterial(BackBone::Entity Entity)
	{
		auto Command = Chilli::Command(_Ctxt);
		return &Command.GetComponent<PepperElement>(Entity)->Material;
	}

	PepperElement* Pepper::GetElement(BackBone::Entity Entity)
	{
		auto Command = Chilli::Command(_Ctxt);
		return Command.GetComponent<PepperElement>(Entity);
	}

	PepperTransform* Pepper::GetTransform(BackBone::Entity Entity)
	{
		auto Command = Chilli::Command(_Ctxt);
		return Command.GetComponent<PepperTransform>(Entity);
	}

	PepperResource* Pepper::GetPepperResource()
	{
		auto Command = Chilli::Command(_Ctxt);
		return Command.GetResource<PepperResource>();
	}

#pragma endregion

	void OnPepperStartUp(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto InputManager = Command.GetService<Chilli::Input>();

		auto WindowService = Command.GetService<Chilli::WindowManager>();
		auto PepperResource = Command.GetResource<Chilli::PepperResource>();
		auto Config = Command.GetResource<PepperExtensionConfig>();

		// Prepare Shader
		auto PepperVertexShader = Command.CreateShaderModule("Assets/Shaders/pepper_vert.spv",
			Chilli::ShaderStageType::SHADER_STAGE_VERTEX);
		auto PepperFragShader = Command.CreateShaderModule("Assets/Shaders/pepper_frag.spv",
			Chilli::ShaderStageType::SHADER_STAGE_FRAGMENT);

		PepperResource->PepperShaderProgram = Command.CreateShaderProgram();
		Command.AttachShaderModule(PepperResource->PepperShaderProgram, PepperVertexShader);
		Command.AttachShaderModule(PepperResource->PepperShaderProgram, PepperFragShader);
		Command.LinkShaderProgram(PepperResource->PepperShaderProgram);

		PepperResource->ContextMaterial = Command.CreateMaterial(PepperResource->PepperShaderProgram);

		// Create a Basic Image
		ImageSpec BasicImageSpec;
		BasicImageSpec.Resolution.Width = 1;
		BasicImageSpec.Resolution.Height = 1;
		BasicImageSpec.Resolution.Depth = 1;
		BasicImageSpec.Format = ImageFormat::RGBA8;
		BasicImageSpec.State = ResourceState::ShaderRead;
		BasicImageSpec.Type = ImageType::IMAGE_TYPE_2D;
		BasicImageSpec.Usage = IMAGE_USAGE_SAMPLED_IMAGE | IMAGE_USAGE_TRANSFER_DST;
		BasicImageSpec.ImageData = nullptr;
		PepperResource->PepperDeafultImage = Command.AllocateImage(BasicImageSpec);

		TextureSpec BasicTextureSpec;
		BasicTextureSpec.Format = BasicImageSpec.Format;
		PepperResource->PepperDeafultTexture = Command.CreateTexture(PepperResource->PepperDeafultImage, BasicTextureSpec);

		uint32_t BasicPepperColor = 0xFFFFFFFF;
		Command.MapImageData(PepperResource->PepperDeafultImage, &BasicPepperColor, 1, 1);

		Chilli::SamplerSpec SamplerSpec;
		SamplerSpec.Filter = Chilli::SamplerFilter::NEAREST;
		SamplerSpec.Mode = Chilli::SamplerMode::REPEAT;
		PepperResource->PepperDeafultSampler = Command.CreateSampler(SamplerSpec);

		MeshCreateInfo RenderMeshInfo;
		RenderMeshInfo.IndexCount = PepperResource->MeshQuadCount * 6;
		RenderMeshInfo.VertCount = PepperResource->MeshQuadCount * 4;
		RenderMeshInfo.IndiciesSize = sizeof(uint32_t) * RenderMeshInfo.IndexCount;
		RenderMeshInfo.VerticesSize = sizeof(PepperVertex) * RenderMeshInfo.VertCount;
		RenderMeshInfo.State = BufferState::DYNAMIC_DRAW;
		RenderMeshInfo.IndexType = IndexBufferType::UINT32_T;
		PepperResource->RenderMesh = Command.CreateMesh(RenderMeshInfo);

		PepperResource->QuadVertices.reserve(RenderMeshInfo.VertCount);
		PepperResource->QuadIndicies.reserve(RenderMeshInfo.IndexCount);
		PepperResource->PepperMaterialData.reserve(PepperResource->MeshQuadCount);

		PepperResource->PepperMaterialSSBO.resize(Config->MaxFramesInFlight);

		for (auto& SSBO : PepperResource->PepperMaterialSSBO)
		{
			BufferCreateInfo CreateInfo;
			CreateInfo.SizeInBytes = sizeof(PepperShaderMaterialData) * PepperResource->MeshQuadCount;
			CreateInfo.Type = BUFFER_TYPE_STORAGE;
			CreateInfo.State = BufferState::STREAM_DRAW;
			CreateInfo.Data = nullptr;

			SSBO = Command.CreateBuffer(CreateInfo, "PepperMaterialSSBO");
		}

		PepperResource->CursorPos = InputManager->GetCursorPos();
		PepperResource->WindowSize.x = Command.GetActiveWindow()->GetWidth();
		PepperResource->WindowSize.y = Command.GetActiveWindow()->GetHeight();

		auto CursorStore = Command.GetStore<Cursor>();
		auto ImageDataStore = Command.GetStore<ImageData>();

		uint32_t* Pixels = new uint32_t[20 * 20];
		for (int x = 0; x < 20; x++)
		{
			for (int y = 0; y < 20; y++)
			{
				Pixels[x * 20 + y] = 0xFF00FFFF;
			}

		}

		ImageData CursorData;
		CursorData.Pixels = Pixels;
		CursorData.Resolution.x = 20;
		CursorData.Resolution.y = 20;

		PepperResource->ResizeCursorImageData = ImageDataStore->Add(CursorData);

		Cursor ResizeCursor;
		ResizeCursor.Data = PepperResource->ResizeCursorImageData.ValPtr;
		ResizeCursor.HotX = 0;
		ResizeCursor.HotY = 0;
		ResizeCursor.RawCursorHandle = Chilli::Window::CreateCursor(&ResizeCursor);

		PepperResource->ResizeCursor = CursorStore->Add(ResizeCursor);

		for (int i = 0; i < int(DeafultCursorTypes::Count); i++)
		{
			PepperResource->DeafultCursors[int(i)] = WindowService->GetCursor(DeafultCursorTypes(i));
		}
	}

	void UpdatePepperMaterialData()
	{

	}

	bool IsCursorInside(const PepperTransform& transform, const IVec2& cursorPos, const IVec2& screenSize)
	{
		// 1. FLIP THE MOUSE Y to match your "Y-Up" coordinate system
		float invertedMouseY = static_cast<float>(screenSize.y) - static_cast<float>(cursorPos.y);

		// 2. Calculate the boundaries (Same as before)
		float minX = transform.ActualPosition.x - static_cast<float>(transform.Pivot.x);
		float minY = transform.ActualPosition.y - static_cast<float>(transform.Pivot.y);

		float maxX = minX + transform.ActualDimensions.x;
		float maxY = minY + transform.ActualDimensions.y;

		// 3. Perform the check using the inverted Y
		return (cursorPos.x >= minX && cursorPos.x <= maxX &&
			invertedMouseY >= minY && invertedMouseY <= maxY);
	}

	void GeneratePepperQuad(
		const PepperTransform& transform,
		const IVec2& screenSize, // Pass current swapchain/window size
		std::vector<PepperVertex>& outVertices,
		std::vector<uint32_t>& outIndices, uint32_t MaterialIndex)
	{
		// 1. Calculate pixel boundaries (as before)
		float pixelX0 = transform.ActualPosition.x - static_cast<float>(transform.Pivot.x);
		float pixelY0 = transform.ActualPosition.y - static_cast<float>(transform.Pivot.y);
		float pixelX1 = pixelX0 + transform.ActualDimensions.x;
		float pixelY1 = pixelY0 + transform.ActualDimensions.y;

		auto ToNDC = [&](float x, float y) -> Vec3 {
			return {
				(x / screenSize.x) * 2.0f - 1.0f,
				(y / screenSize.y) * 2.0f - 1.0f,
				1.0f - (static_cast<float>(transform.ZOrder) / 1000.0f) // Invert so higher is closer
			};
			};

		uint32_t vertexOffset = static_cast<uint32_t>(outVertices.size());

		// 3. Define Vertices in NDC
		outVertices.push_back({ ToNDC(pixelX0, pixelY0), {0.0f, 0.0f}, MaterialIndex }); // TL
		outVertices.push_back({ ToNDC(pixelX1, pixelY0), {1.0f, 0.0f}, MaterialIndex }); // TR
		outVertices.push_back({ ToNDC(pixelX0, pixelY1), {0.0f, 1.0f}, MaterialIndex }); // BL
		outVertices.push_back({ ToNDC(pixelX1, pixelY1), {1.0f, 1.0f}, MaterialIndex }); // BR

		// 4. Indices (Same as before)
		outIndices.push_back(vertexOffset + 0);
		outIndices.push_back(vertexOffset + 1);
		outIndices.push_back(vertexOffset + 2);
		outIndices.push_back(vertexOffset + 2);
		outIndices.push_back(vertexOffset + 1);
		outIndices.push_back(vertexOffset + 3);
	}

	void UpdateTransform(PepperTransform& transform, IVec2 screenSize)
	{
		transform.ActualDimensions.x = screenSize.x * transform.PercentageDimensions.x;
		transform.ActualDimensions.y = screenSize.y * transform.PercentageDimensions.y;

		transform.ActualPosition.x = screenSize.x * transform.PercentagePosition.x;
		transform.ActualPosition.y = screenSize.y * transform.PercentagePosition.y;

		// Handle X Pivot based on Anchor
		switch (transform.AnchorX) {
		case Chilli::AnchorX::LEFT:   transform.Pivot.x = 0; break;
		case Chilli::AnchorX::CENTER: transform.Pivot.x = static_cast<int>(transform.ActualDimensions.x * 0.5f); break;
		case Chilli::AnchorX::RIGHT:  transform.Pivot.x = static_cast<int>(transform.ActualDimensions.x); break;
		}

		// Handle Y Pivot based on Anchor
		switch (transform.AnchorY) {
		case Chilli::AnchorY::TOP:    transform.Pivot.y = 0; break;
		case Chilli::AnchorY::CENTER: transform.Pivot.y = static_cast<int>(transform.ActualDimensions.y * 0.5f); break;
		case Chilli::AnchorY::BOTTOM: transform.Pivot.y = static_cast<int>(transform.ActualDimensions.y); break;
		}
	}

	void ApplyMovementRecursive(BackBone::Entity entity, PepperTransform* ChangeTransform, IVec2 delta, PepperResource* res, BackBone::World& registry)
	{
		auto* transform = registry.GetComponent<PepperTransform>(entity);
		if (ChangeTransform != nullptr)
			transform = ChangeTransform;
		if (!transform) return;

		// 2. Apply the same delta to this entity
		transform->ActualPosition.x += static_cast<float>(delta.x);
		transform->ActualPosition.y -= static_cast<float>(delta.y); // Use the fixed Y logic

		// 3. Update percentages so it stays in sync
		transform->PercentagePosition.x = transform->ActualPosition.x / res->WindowSize.x;
		transform->PercentagePosition.y = transform->ActualPosition.y / res->WindowSize.y;

		// 4. Look up its children and move them too
		if (entity < res->EntityToPCMap.size()) {
			uint32_t mapIndex = res->EntityToPCMap[entity];
			auto* children = res->ParentChildMap.Get(mapIndex);
			if (children) {
				for (auto childEntity : *children) {
					ApplyMovementRecursive(childEntity, nullptr, delta, res, registry);
				}
			}
		}
	}

	ResizeEdge GetResizeEdge(const PepperTransform& transform, const IVec2& cursorPos, const IVec2& screenSize) {
		const float margin = 5.0f; // The "Magic Number"

		float invMouseY = static_cast<float>(screenSize.y) - static_cast<float>(cursorPos.y);

		float minX = transform.ActualPosition.x - transform.Pivot.x;
		float minY = transform.ActualPosition.y - transform.Pivot.y;
		float maxX = minX + transform.ActualDimensions.x;
		float maxY = minY + transform.ActualDimensions.y;

		// 1. Check if the mouse is even near the quad (with margin)
		if (cursorPos.x < minX - margin || cursorPos.x > maxX + margin ||
			invMouseY < minY - margin || invMouseY > maxY + margin) {
			return ResizeEdge::None;
		}

		bool left = std::abs(cursorPos.x - minX) <= margin;
		bool right = std::abs(cursorPos.x - maxX) <= margin;
		bool bottom = std::abs(invMouseY - minY) <= margin; // Using your Y-up logic
		bool top = std::abs(invMouseY - maxY) <= margin;

		// 2. Corner cases (Check these first!)
		if (left && top)     return ResizeEdge::TopLeft;
		if (left && bottom)  return ResizeEdge::BottomLeft;
		if (right && top)    return ResizeEdge::TopRight;
		if (right && bottom) return ResizeEdge::BottomRight;

		// 3. Side cases
		if (left)   return ResizeEdge::Left;
		if (right)  return ResizeEdge::Right;
		if (top)    return ResizeEdge::Top;
		if (bottom) return ResizeEdge::Bottom;

		return ResizeEdge::None;
	}

	void ApplyResize(PepperTransform& transform, ResizeEdge edge, IVec2 delta) {
		float dx = static_cast<float>(delta.x);
		float dy = static_cast<float>(delta.y); // Note: check if your Y is inverted

		const float MinSize = 20.0f; // Prevent window from disappearing

		// Horizontal Resize
		if (edge == ResizeEdge::Left || edge == ResizeEdge::TopLeft || edge == ResizeEdge::BottomLeft) {
			float oldWidth = transform.ActualDimensions.x;
			transform.ActualDimensions.x = std::max(MinSize, transform.ActualDimensions.x - dx);
			// Only move position if size didn't hit the minimum clamp
			if (transform.ActualDimensions.x > MinSize) transform.ActualPosition.x += dx;
		}
		else if (edge == ResizeEdge::Right || edge == ResizeEdge::TopRight || edge == ResizeEdge::BottomRight) {
			transform.ActualDimensions.x = std::max(MinSize, transform.ActualDimensions.x + dx);
		}

		// Vertical Resize (Assuming Y-Up logic based on your IsCursorInside)
		if (edge == ResizeEdge::Bottom || edge == ResizeEdge::BottomLeft || edge == ResizeEdge::BottomRight) {
			float oldHeight = transform.ActualDimensions.y;
			transform.ActualDimensions.y = std::max(MinSize, transform.ActualDimensions.y - dy);
			if (transform.ActualDimensions.y > MinSize) transform.ActualPosition.y += dy;
		}
		else if (edge == ResizeEdge::Top || edge == ResizeEdge::TopLeft || edge == ResizeEdge::TopRight) {
			transform.ActualDimensions.y = std::max(MinSize, transform.ActualDimensions.y + dy);
		}
	}

	void SetCursorForEdge(ResizeEdge edge, Chilli::EventHandler* EventManager,
		Window* UsingWindow, Chilli::PepperResource* res)
	{
		void* cursorPtr = nullptr;
		if (edge == ResizeEdge::Left || edge == ResizeEdge::Right)
			cursorPtr = res->DeafultCursors[int(DeafultCursorTypes::ResizeEW)].ValPtr;
		else if (edge == ResizeEdge::Top || edge == ResizeEdge::Bottom)
			cursorPtr = res->DeafultCursors[int(DeafultCursorTypes::ResizeNS)].ValPtr;
		else if (edge == ResizeEdge::TopLeft || edge == ResizeEdge::BottomRight)
			cursorPtr = res->DeafultCursors[int(DeafultCursorTypes::ResizeNWSE)].ValPtr;
		else if (edge == ResizeEdge::TopRight || edge == ResizeEdge::BottomLeft)
			cursorPtr = res->DeafultCursors[int(DeafultCursorTypes::ResizeNESW)].ValPtr;

		SetCursorEvent e((Cursor*)cursorPtr, UsingWindow, DeafultCursorTypes::Deafult);
		EventManager->Add<SetCursorEvent>(e);
	}

	void OnPepperUpdate(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
		auto EventManager = Command.GetService<Chilli::EventHandler>();

		auto PepperResource = Command.GetResource<Chilli::PepperResource>();
		auto InputManager = Command.GetService<Chilli::Input>();
		auto Pepper = Chilli::Pepper(Ctxt);
		auto WindowService = Command.GetResource<Chilli::WindowManager>();
		auto ActiveWindow = Command.GetActiveWindow();

		PepperResource->QuadVertices.clear();
		PepperResource->QuadIndicies.clear();
		PepperResource->PepperMaterialData.clear();
		PepperResource->QuadCount = 0;

		PepperResource->OldCursorPos = PepperResource->CursorPos;
		PepperResource->CursorPos = InputManager->GetCursorPos();

		PepperResource->OldWindowSize = PepperResource->WindowSize;

		PepperResource->WindowSize.x = ActiveWindow->GetWidth();
		PepperResource->WindowSize.y = ActiveWindow->GetHeight();

		// 1. Calculate Deltas
		IVec2 CursorDelta;
		CursorDelta.x = PepperResource->CursorPos.x - PepperResource->OldCursorPos.x;
		CursorDelta.y = PepperResource->CursorPos.y - PepperResource->OldCursorPos.y;
		Vec2 WindowSize = { (float)PepperResource->WindowSize.x, (float)PepperResource->WindowSize.y };
		bool LMouseButtonDown = InputManager->IsMouseButtonDown(Input_mouse_Left);

		static int InitialWindowResize = 0;
		bool WindowSizeResized = (PepperResource->OldWindowSize != PepperResource->WindowSize);
		if (InitialWindowResize == 0)
			WindowSizeResized = true;
		InitialWindowResize++;

		// --- PASS 1: Build the Hierarchy Map ---
		for (auto& vec : PepperResource->ParentChildMap) vec.clear();
		for (auto [Entity, ElementComp] : BackBone::QueryWithEntities<PepperElement>(*Ctxt.Registry)) {
			if (ElementComp->Parent != BackBone::npos) {
				auto pID = ElementComp->Parent;
				if (pID >= PepperResource->EntityToPCMap.size()) PepperResource->EntityToPCMap.resize(pID + 1);
				uint32_t& mIdx = PepperResource->EntityToPCMap[pID];
				if (mIdx == 0) mIdx = PepperResource->ParentChildMap.Create({});
				PepperResource->ParentChildMap.Get(mIdx)->push_back(Entity);
			}
		}

		// Run this first for EVERYONE
		for (auto [TransformComp] : BackBone::Query<PepperTransform>(*Ctxt.Registry))
		{
			if (WindowSizeResized) {
				UpdateTransform(*TransformComp, PepperResource->WindowSize);
			}
		}

		// --- PASS 2: Main Update Loop ---
		bool resizing = false;
		BackBone::Entity resizingEntity = BackBone::npos;
		ResizeEdge resizingEdge = ResizeEdge::None;
		static BackBone::Entity activeResizeEntity = BackBone::npos;
		static ResizeEdge activeResizeEdge = ResizeEdge::None;
		static bool wasLMouseDown = false;

		for (auto [Entity, TransformComp, ElementComp] : BackBone::QueryWithEntities<PepperTransform, PepperElement>(*Ctxt.Registry))
		{
			if (ElementComp->Parent != BackBone::npos) {
				auto parentID = ElementComp->Parent;
				if (parentID >= PepperResource->EntityToPCMap.size()) PepperResource->EntityToPCMap.resize(parentID + 1);
				uint32_t& mapIdx = PepperResource->EntityToPCMap[parentID];
				if (mapIdx == 0) mapIdx = PepperResource->ParentChildMap.Create({});
				auto* list = PepperResource->ParentChildMap.Get(mapIdx);
				list->push_back(Entity);
			}

			PepperTransform changePT = *TransformComp;
			bool IsMouseOver = IsCursorInside(changePT, PepperResource->CursorPos, PepperResource->WindowSize);

			auto Edge = GetResizeEdge(changePT, PepperResource->CursorPos, PepperResource->WindowSize);

			// --- RESIZE LOGIC ---
			if (activeResizeEntity == BackBone::npos && Edge != ResizeEdge::None && LMouseButtonDown && (ElementComp->Flags & PEPPER_ELEMENT_RESIZEABLE))
			{
				activeResizeEntity = Entity;
				activeResizeEdge = Edge;
				PepperResource->ActiveEdge = Edge;
			}

			if (activeResizeEntity == Entity && activeResizeEdge != ResizeEdge::None)
			{
				if (LMouseButtonDown)
				{
					ApplyResize(changePT, activeResizeEdge, CursorDelta);
					// Update percentages so resizing is persistent
					changePT.PercentageDimensions.x = changePT.ActualDimensions.x / PepperResource->WindowSize.x;
					changePT.PercentageDimensions.y = changePT.ActualDimensions.y / PepperResource->WindowSize.y;
					changePT.PercentagePosition.x = changePT.ActualPosition.x / PepperResource->WindowSize.x;
					changePT.PercentagePosition.y = changePT.ActualPosition.y / PepperResource->WindowSize.y;
					resizing = true;
					resizingEntity = Entity;
					SetCursorForEdge(activeResizeEdge, EventManager, ActiveWindow, PepperResource);
				}
				else
				{
					activeResizeEntity = BackBone::npos;
					activeResizeEdge = ResizeEdge::None;
					PepperResource->ActiveEdge = ResizeEdge::None;
				}
			}

			if (activeResizeEdge == ResizeEdge::None)
			{
				SetCursorForEdge(ResizeEdge::None, EventManager, ActiveWindow, PepperResource);
			}

			// --- MOVE LOGIC ---
			if (IsMouseOver && (ElementComp->Flags & PEPPER_ELEMENT_MOVEABLE) && LMouseButtonDown && activeResizeEntity == BackBone::npos)
			{
				BackBone::Entity rootToMove = (ElementComp->Parent != BackBone::npos) ? ElementComp->Parent : Entity;
				ApplyMovementRecursive(rootToMove, &changePT, CursorDelta, PepperResource, *Ctxt.Registry);
			}

			*TransformComp = changePT;

			if (ElementComp->Flags & PEPPER_ELEMENT_VISIBLE)
			{
				PepperResource->PepperMaterialData.push_back(Pepper.GetMaterialShaderData(Entity));
				GeneratePepperQuad(*TransformComp, PepperResource->WindowSize, PepperResource->QuadVertices,
					PepperResource->QuadIndicies, PepperResource->PepperMaterialData.size() - 1);
				PepperResource->QuadCount++;
			}
		}

		wasLMouseDown = LMouseButtonDown;

		if (PepperResource->QuadCount == 0)
		{
			return;
		}

		RenderService->UpdateMaterialBufferData(MaterialSystem->GetRawMaterialHandle(PepperResource->ContextMaterial),
			PepperResource->PepperMaterialSSBO[RenderService->GetCurrentFrameIndex()].ValPtr->RawBufferHandle,
			"PepperMaterialSSBO",
			PepperResource->PepperMaterialSSBO[RenderService->GetCurrentFrameIndex()].ValPtr->CreateInfo.SizeInBytes,
			0);

		Command.MapMeshVertexBufferData(PepperResource->RenderMesh, PepperResource->QuadVertices.data(),
			PepperResource->QuadVertices.size(), sizeof(PepperVertex) * PepperResource->QuadVertices.size(),
			0);
		Command.MapMeshIndexBufferData(PepperResource->RenderMesh, PepperResource->QuadIndicies.data(),
			PepperResource->QuadIndicies.size(), sizeof(uint32_t) * PepperResource->QuadIndicies.size(),
			0);
		Command.MapBufferData(PepperResource->PepperMaterialSSBO[RenderService->GetCurrentFrameIndex()],
			PepperResource->PepperMaterialData.data(),
			sizeof(PepperShaderMaterialData) * PepperResource->PepperMaterialData.size(), 0);
	}

	void OnPepperShutDown(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);

		auto PepperResource = Command.GetResource<Chilli::PepperResource>();
		auto Config = Command.GetResource<PepperExtensionConfig>();

		Command.DestroyMaterial(PepperResource->ContextMaterial);
		Command.DestroyShaderProgram(PepperResource->PepperShaderProgram);
		Command.DestroyTexture(PepperResource->PepperDeafultTexture);
		Command.DestroyImage(PepperResource->PepperDeafultImage);

		for (auto& SSBO : PepperResource->PepperMaterialSSBO)
			Command.DestroyBuffer(SSBO);
		Command.FreeMesh(PepperResource->RenderMesh);
	}

	void OnPepperRender(BackBone::SystemContext& Ctxt, RenderPassInfo& Pass)
	{
		auto Command = Chilli::Command(Ctxt);

		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
		auto PepperResource = Command.GetResource<Chilli::PepperResource>();
		auto InputManager = Command.GetService<Chilli::Input>();

		if (PepperResource->QuadCount == 0)
		{
			return;
		}

		if (InputManager->IsKeyDown(Input_key_0))
		{
			RenderService->SetFillMode(PolygonMode::Wireframe);
		}

		RenderService->BindShaderProgram(PepperResource->PepperShaderProgram.ValPtr->RawProgramHandle);

		RenderService->BindMaterailData(MaterialSystem->GetRawMaterialHandle(PepperResource->ContextMaterial));

		RenderService->BindVertexBuffer({ PepperResource->RenderMesh.ValPtr->VBHandle.ValPtr->RawBufferHandle });
		RenderService->BindIndexBuffer(PepperResource->RenderMesh.ValPtr->IBHandle.ValPtr->RawBufferHandle,
			IndexBufferType::UINT32_T);

		RenderService->DrawIndexed(PepperResource->QuadCount * 6, 1, 0, 0, 0);

		OnEmberRenderUIPass(Ctxt);
	}

	std::vector<PipelineBarrier> GetPepperPrePassPipelineBarrier(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto PepperResource = Command.GetResource<Chilli::PepperResource>();
		auto RenderService = Command.GetService<Renderer>();

		PipelineBarrier PrepareVertexBarrier{};
		PrepareVertexBarrier.BufferBarrier.Handle = PepperResource->RenderMesh.ValPtr->VBHandle.ValPtr->RawBufferHandle;
		PrepareVertexBarrier.BufferBarrier.Offset = 0;
		PrepareVertexBarrier.BufferBarrier.Size = CH_BUFFER_WHOLE_SIZE;
		PrepareVertexBarrier.OldState = ResourceState::HostWrite;
		PrepareVertexBarrier.NewState = ResourceState::VertexRead;

		PipelineBarrier PrepareIndexBarrier{};
		PrepareIndexBarrier.BufferBarrier.Handle = PepperResource->RenderMesh.ValPtr->IBHandle.ValPtr->RawBufferHandle;
		PrepareIndexBarrier.BufferBarrier.Offset = 0;
		PrepareIndexBarrier.BufferBarrier.Size = CH_BUFFER_WHOLE_SIZE;
		PrepareIndexBarrier.OldState = ResourceState::HostWrite;
		PrepareIndexBarrier.NewState = ResourceState::IndexRead;

		PipelineBarrier PrepareMaterialBarrier{};
		PrepareMaterialBarrier.BufferBarrier.Handle = PepperResource->PepperMaterialSSBO[RenderService->GetCurrentFrameIndex()].ValPtr->RawBufferHandle;
		PrepareMaterialBarrier.BufferBarrier.Offset = 0;
		PrepareMaterialBarrier.BufferBarrier.Size = CH_BUFFER_WHOLE_SIZE;
		PrepareMaterialBarrier.OldState = ResourceState::HostWrite;
		PrepareMaterialBarrier.NewState = ResourceState::ShaderRead;

		return { PrepareVertexBarrier, PrepareIndexBarrier, PrepareMaterialBarrier };
	}

	std::vector<PipelineBarrier> GetPepperPostPassPipelineBarrier(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto PepperResource = Command.GetResource<Chilli::PepperResource>();
		auto RenderService = Command.GetService<Renderer>();

		PipelineBarrier VertexBarrier{};
		VertexBarrier.BufferBarrier.Handle = PepperResource->RenderMesh.ValPtr->VBHandle.ValPtr->RawBufferHandle;
		VertexBarrier.BufferBarrier.Offset = 0;
		VertexBarrier.BufferBarrier.Size = CH_BUFFER_WHOLE_SIZE;
		VertexBarrier.OldState = ResourceState::VertexRead;
		VertexBarrier.NewState = ResourceState::HostWrite;

		PipelineBarrier IndexBarrier{};
		IndexBarrier.BufferBarrier.Handle = PepperResource->RenderMesh.ValPtr->IBHandle.ValPtr->RawBufferHandle;
		IndexBarrier.BufferBarrier.Offset = 0;
		IndexBarrier.BufferBarrier.Size = CH_BUFFER_WHOLE_SIZE;
		IndexBarrier.OldState = ResourceState::IndexRead;
		IndexBarrier.NewState = ResourceState::HostWrite;

		PipelineBarrier PrepareMaterialBarrier{};
		PrepareMaterialBarrier.BufferBarrier.Handle = PepperResource->PepperMaterialSSBO[RenderService->GetCurrentFrameIndex()].ValPtr->RawBufferHandle;
		PrepareMaterialBarrier.BufferBarrier.Offset = 0;
		PrepareMaterialBarrier.BufferBarrier.Size = CH_BUFFER_WHOLE_SIZE;
		PrepareMaterialBarrier.OldState = ResourceState::ShaderRead;
		PrepareMaterialBarrier.NewState = ResourceState::HostWrite;

		return { VertexBarrier, IndexBarrier, PrepareMaterialBarrier };
	}

	void PepperExtension::Build(BackBone::App& App)
	{
		App.Registry.AddResource<PepperResource>();
		App.Registry.AddResource<PepperExtensionConfig>();

		auto Config = App.Registry.GetResource<PepperExtensionConfig>();
		*Config = _Config;

		App.Registry.Register<Chilli::PepperTransform>();
		App.Registry.Register<Chilli::PepperElement>();

		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::START_UP, OnPepperStartUp);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnPepperUpdate);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::SHUTDOWN, OnPepperShutDown);

		EmberExtensionConfig EmberConfig;
		EmberConfig.MaxFrameInFlight = _Config.MaxFramesInFlight;

		App.Extensions.AddExtension(std::make_unique<EmberExtension>(EmberConfig), true, &App);
	}

#pragma endregion

}

