#include "Ch_PCH.h"
#include "DeafultExtensions.h"
#include "Profiling\Timer.h"

namespace Chilli
{
#pragma region Blaze 
	void OnBlazeSetup(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto BlazeResource = Command.GetResource<Chilli::BlazeResource>();

		BlazeResource->Shader = Command.CreateShaderProgram();
		auto BlazeVertShader = Command.CreateShaderModule("Assets/Shaders/blaze_vert.spv", ShaderStageType::SHADER_STAGE_VERTEX);
		auto BlazeFragShader = Command.CreateShaderModule("Assets/Shaders/blaze_frag.spv", ShaderStageType::SHADER_STAGE_FRAGMENT);

		Command.AttachShaderModule(BlazeResource->Shader, BlazeVertShader);
		Command.AttachShaderModule(BlazeResource->Shader, BlazeFragShader);
		Command.LinkShaderProgram(BlazeResource->Shader);
	}

	void OnBlazeUpdate(BackBone::SystemContext& Ctxt)
	{
	}

	void OnBlazeShutDown(BackBone::SystemContext& Ctxt)
	{
	}

	void OnBlazeRenderOverlay(BackBone::SystemContext& Ctxt, RenderPassDesc& Info)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
		auto BlazeResource = Command.GetResource<Chilli::BlazeResource>();

		for (auto [Mesh] : BackBone::Query<BlazeMeshComponent>(*Ctxt.Registry))
		{
			if (Mesh->Mode != BlazeMode::OVERLAY)
				continue;

			auto ActiveShader = BlazeResource->Shader;

			if (RenderService->GetActivePipelineStateInfo().LineWidth != Mesh->LineWidth)
				RenderService->SetLineWidth(Mesh->LineWidth);

			RenderService->BindShaderProgram(ActiveShader.ValPtr->RawProgramHandle);

			BlazeInlineUniformDataStruct Data{
				.ModeFlag = uint32_t(Mesh->Mode),
				.Color = Mesh->Color
			};

			RenderService->BindVertexBuffer({ Mesh->VertexBuffer.ValPtr->RawBufferHandle });
			RenderService->PushInlineUniformData(ActiveShader.ValPtr->RawProgramHandle,
				SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, &Data, sizeof(Data), 0);

			RenderService->DrawArray(Mesh->LineCount, 1, 0, 0, 0);
		}
	}

	void OnBlazeRenderWorldSpace(BackBone::SystemContext& Ctxt, RenderPassDesc& Info)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
		auto BlazeResource = Command.GetResource<Chilli::BlazeResource>();

		for (auto [Mesh] : BackBone::Query<BlazeMeshComponent>(*Ctxt.Registry))
		{
			if (Mesh->Mode != BlazeMode::WORLDSPACE)
				continue;

			auto ActiveShader = BlazeResource->Shader;

			if (RenderService->GetActivePipelineStateInfo().LineWidth != Mesh->LineWidth)
				RenderService->SetLineWidth(Mesh->LineWidth);

			RenderService->BindShaderProgram(ActiveShader.ValPtr->RawProgramHandle);

			BlazeInlineUniformDataStruct Data{
				.ModeFlag = uint32_t(Mesh->Mode),
				.Color = Mesh->Color
			};

			RenderService->BindVertexBuffer({ Mesh->VertexBuffer.ValPtr->RawBufferHandle });
			RenderService->PushInlineUniformData(ActiveShader.ValPtr->RawProgramHandle,
				SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, &Data, sizeof(Data), 0);

			RenderService->DrawArray(Mesh->LineCount, 1, 0, 0, 0);
		}
	}

	void OnBlazeRenderXRay(BackBone::SystemContext& Ctxt, RenderPassDesc& Info)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
		auto BlazeResource = Command.GetResource<Chilli::BlazeResource>();

		for (auto [Mesh] : BackBone::Query<BlazeMeshComponent>(*Ctxt.Registry))
		{
			if (Mesh->Mode != BlazeMode::XRAY)
				continue;

			if (RenderService->GetActivePipelineStateInfo().LineWidth != Mesh->LineWidth)
				RenderService->SetLineWidth(Mesh->LineWidth);

			auto ActiveShader = BlazeResource->Shader;

			RenderService->BindShaderProgram(ActiveShader.ValPtr->RawProgramHandle);

			BlazeInlineUniformDataStruct Data{
				.ModeFlag = uint32_t(Mesh->Mode),
				.Color = Mesh->Color
			};

			RenderService->BindVertexBuffer({ Mesh->VertexBuffer.ValPtr->RawBufferHandle });
			RenderService->PushInlineUniformData(ActiveShader.ValPtr->RawProgramHandle,
				SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, &Data, sizeof(Data), 0);

			RenderService->DrawArray(Mesh->LineCount, 1, 0, 0, 0);
		}
	}

	void OnBlazeRenderGizmo(BackBone::SystemContext& Ctxt, RenderPassDesc& Info)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
		auto BlazeResource = Command.GetResource<Chilli::BlazeResource>();

		for (auto [Mesh] : BackBone::Query<BlazeMeshComponent>(*Ctxt.Registry))
		{
			if (Mesh->Mode != BlazeMode::GiIZMO)
				continue;

			if (RenderService->GetActivePipelineStateInfo().LineWidth != Mesh->LineWidth)
				RenderService->SetLineWidth(Mesh->LineWidth);

			auto ActiveShader = BlazeResource->Shader;

			RenderService->BindShaderProgram(ActiveShader.ValPtr->RawProgramHandle);

			BlazeInlineUniformDataStruct Data{
				.ModeFlag = uint32_t(Mesh->Mode),
				.Color = Mesh->Color
			};

			RenderService->BindVertexBuffer({ Mesh->VertexBuffer.ValPtr->RawBufferHandle });
			RenderService->PushInlineUniformData(ActiveShader.ValPtr->RawProgramHandle,
				SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, &Data, sizeof(Data), 0);

			RenderService->DrawArray(Mesh->LineCount, 1, 0, 0, 0);
		}
	}

	void BlazeExtension::Build(BackBone::App& App)
	{
		App.Registry.AddResource<BlazeResource>();

		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::START_UP, OnBlazeSetup);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnBlazeUpdate);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::SHUTDOWN, OnBlazeShutDown);
	}
#pragma endregion

#pragma region Render Extension
	uint32_t GetNewEventID()
	{
		static uint32_t NewID = 0;
		return NewID++;
	}

	void OnRenderExtensionsSetup(BackBone::SystemContext& Ctxt)
	{
		std::cout << ("RenderSystem::OnCreate") << "\n";
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

		Command.AddLoader<Chilli::ImageLoader>();
		Command.RegisterStore<Chilli::RawMeshData>();
		Command.RegisterStore<Chilli::MeshLoaderData>();
		Command.AddLoader<Chilli::CGLFTMeshLoader>();
		Command.AddLoader<Chilli::TinyObjMeshLoader>();

		auto RenderService = Ctxt.ServiceRegistry->GetService<Renderer>();
		RenderService->Init(Config->Spec);

		Ctxt.ServiceRegistry->RegisterService<RenderCommand>(RenderService->CreateRenderCommand());
		Ctxt.ServiceRegistry->RegisterService<MaterialSystem>(std::make_shared<MaterialSystem>(Ctxt));

		CH_CORE_TRACE("Graphcis Backend Using: {}", RenderService->GetName());
		auto RenderCommandService = Ctxt.ServiceRegistry->GetService<RenderCommand>();

		auto Resource = Command.GetResource<RenderResource>();

		Command.GetResource<RenderResource>()->DeafultShaderProgram = Command.GetStore<ShaderProgram>()->Add(RenderService->GetDeafultShaderProgram());

		Chilli::SamplerSpec SamplerSpec;
		SamplerSpec.Filter = Chilli::SamplerFilter::NEAREST;
		SamplerSpec.Mode = Chilli::SamplerMode::REPEAT;
		Resource->DeafultSampler = Command.CreateSampler(SamplerSpec);

		uint32_t Width = 1;
		uint32_t Height = 1;

		uint32_t WhiteColor = 0xFFFFFFFF;
		Chilli::ImageSpec ImageSpec{};
		ImageSpec.Format = Chilli::ImageFormat::RGBA8;
		ImageSpec.ImageData = &WhiteColor;
		ImageSpec.MipLevel = 1;
		ImageSpec.Resolution.Width = Width;
		ImageSpec.Resolution.Height = Height;
		ImageSpec.Resolution.Depth = 1;
		ImageSpec.Sample = Chilli::IMAGE_SAMPLE_COUNT_1_BIT;
		ImageSpec.State = Chilli::ResourceState::ShaderRead;
		ImageSpec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
		ImageSpec.Usage = Chilli::IMAGE_USAGE_SAMPLED_IMAGE;

		Resource->DeafultImage = Command.AllocateImage(ImageSpec);
		Command.MapImageData(Resource->DeafultImage, &WhiteColor, Width, Height);

		Chilli::TextureSpec TextureSpec;
		TextureSpec.Format = Chilli::ImageFormat::RGBA8;
		Resource->DeafultTexture = Command.CreateTexture(Resource->DeafultImage, TextureSpec);

		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

		Resource->DeafultMaterial = Command.CreateMaterial(Command.GetDeafultShaderProgram());
		MaterialSystem->SetAlbedoColor(Resource->DeafultMaterial, { 1.0f, 1.0f, 1.0f, 1.0f });
		MaterialSystem->SetAlbedoTexture(Resource->DeafultMaterial, Resource->DeafultTexture);
		MaterialSystem->SetAlbedoSampler(Resource->DeafultMaterial, Resource->DeafultSampler);

		Resource->RenderGraph = std::make_shared<Chilli::RenderGraph>();

		CH_CORE_TRACE("Render Extension Init: Successful API: {}", RenderService->GetName());
	}

	void OnRenderExtensionsCleanUp(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		CHILLI_DEBUG_TIMER("OnRenderShutDown::");
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();
		auto RenderGraph = Command.GetResource<Chilli::RenderResource>()->RenderGraph;
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

		for (auto& Pass : *RenderGraph)
		{
			Pass->Teardown(Command);
		}
		// Clear Mesh
		auto MeshStore = Command.GetStore<Mesh>();
		for (auto& MeshInfo : *MeshStore)
		{
			Command.DestroyBuffer(MeshInfo->VertexBufferHandles[0]);
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
		auto RenderGraph = Command.GetResource<Chilli::RenderResource>()->RenderGraph;
		auto RenderService = Command.GetService<Renderer>();

		IVec2 ViewPort;
		bool ReSize = false;

		for (auto& Event : EventReader<FrameBufferResizeEvent>(EventService))
		{
			ReSize = true;
			ViewPort.x = Event.GetX();
			ViewPort.y = Event.GetY();
		}

		if (ReSize)
		{
			for (auto& Pass : *RenderGraph)
			{
				Pass->OnResize(Command, ViewPort.x, ViewPort.y);
				RenderService->PushFrameBufferResize(ViewPort);
			}
		}
	}

	void OnRenderExtensionScreenPassRender(BackBone::SystemContext& Ctxt, RenderPassDesc& Pass)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

		auto DefferedResource = Command.GetResource< DefferedRenderingResource>();
		auto RenderResource = Command.GetResource< Chilli::RenderResource>();

		auto ActiveMaterial = DefferedResource->ScreenMaterial.ValPtr;
		auto ActiveShader = DefferedResource->ScreenShaderProgram;

		if (MaterialSystem->ShouldMaterialShaderDataUpdate(DefferedResource->ScreenMaterial))
		{
			MaterialShaderData MaterialData = MaterialSystem->GetMaterialShaderData(DefferedResource->ScreenMaterial);
			RenderService->UpdateMaterialShaderData(MaterialSystem->GetRawMaterialHandle(DefferedResource->ScreenMaterial), MaterialData);
		}

		if (MaterialSystem->ShouldMaterialShaderDataUpdate(RenderResource->DeafultMaterial))
		{
			MaterialShaderData MaterialData = MaterialSystem->GetMaterialShaderData(RenderResource->DeafultMaterial);
			RenderService->UpdateMaterialShaderData(MaterialSystem->GetRawMaterialHandle(RenderResource->DeafultMaterial), MaterialData);
		}

		RenderService->BindShaderProgram(DefferedResource->ScreenShaderProgram.ValPtr->RawProgramHandle);
		RenderService->BindMaterailData(MaterialSystem->GetRawMaterialHandle(DefferedResource->ScreenMaterial));

		auto MatIndex = RenderService->GetMaterialShaderIndex(MaterialSystem->GetRawMaterialHandle(DefferedResource->ScreenMaterial));

		DrawPushShaderInlineUniformData PushData;
		PushData.MaterialIndex = MatIndex;
		PushData.ObjectIndex = 0;

		RenderService->PushInlineUniformData(ActiveShader.ValPtr->RawProgramHandle,
			SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, &PushData, sizeof(PushData), 0);

		uint32_t Buffers[16] = { 0 };
		uint32_t BindingCount = 0;

		for (int i = 0; i < DefferedResource->ScreenRenderMesh.ValPtr->ActiveVBHandlesCount; i++)
		{
			Buffers[i] = DefferedResource->ScreenRenderMesh.ValPtr->VertexBufferHandles[i].ValPtr->RawBufferHandle;
			BindingCount++;
		}

		RenderService->BindVertexBuffer(Buffers, BindingCount);

		RenderService->BindIndexBuffer(DefferedResource->ScreenRenderMesh.ValPtr->IBHandle.ValPtr->RawBufferHandle,
			DefferedResource->ScreenRenderMesh.ValPtr->IBType);

		RenderService->DrawIndexed(DefferedResource->ScreenRenderMesh.ValPtr->IndexCount, 1, 0, 0, 0);
	}

	void OnRenderExtensionGeometryPassRender(BackBone::SystemContext& Ctxt, RenderPassDesc& Pass)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
		auto RenderResource = Command.GetResource<Chilli::RenderResource>();

		for (auto [Entity, Transform, MeshComp] : BackBone::QueryWithEntities<TransformComponent, MeshComponent>(*Ctxt.Registry))
		{
			uint32_t RawMaterialHandle = 1;
			BackBone::AssetHandle<Material> ActiveMaterial = MeshComp->MaterialHandle;
			BackBone::AssetHandle<ShaderProgram> ActiveShader;

			if (MeshComp->MaterialHandle.IsValid() == false)
			{
				RawMaterialHandle = MaterialSystem->GetRawMaterialHandle(RenderResource->DeafultMaterial);
				ActiveShader = RenderResource->DeafultShaderProgram;
				ActiveMaterial = RenderResource->DeafultMaterial;
			}
			else
			{
				RawMaterialHandle = MaterialSystem->GetRawMaterialHandle(MeshComp->MaterialHandle);
				ActiveShader = MaterialSystem->GetShaderProgramID(MeshComp->MaterialHandle);
			}

			if (MaterialSystem->ShouldMaterialShaderDataUpdate(ActiveMaterial))
			{
				MaterialShaderData MaterialData = MaterialSystem->GetMaterialShaderData(ActiveMaterial);
				RenderService->UpdateMaterialShaderData(MaterialSystem->GetRawMaterialHandle(ActiveMaterial), MaterialData);
			}

			auto ActiveMesh = MeshComp->MeshHandle.ValPtr;

			RenderService->BindShaderProgram(ActiveShader.ValPtr->RawProgramHandle);
			RenderService->BindMaterailData(RawMaterialHandle);

			uint32_t Buffers[16] = { 0 };
			uint32_t BindingCount = 0;

			for (int i = 0; i < ActiveMesh->ActiveVBHandlesCount; i++)
			{
				Buffers[i] = ActiveMesh->VertexBufferHandles[i].ValPtr->RawBufferHandle;
				BindingCount++;
			}

			RenderService->BindVertexBuffer(Buffers, BindingCount);
			RenderService->BindIndexBuffer(ActiveMesh->IBHandle.ValPtr->RawBufferHandle, ActiveMesh->IBType);

			auto OldTransform = RenderResource->LastTransformVersion.Get(Entity);

			if (OldTransform == nullptr)
			{
				RenderResource::LastTransformStruct LastTransform;
				LastTransform.GenerationVal = Command.GetEntityGeneration(Entity);
				RenderResource->LastTransformVersion.Insert(Entity, LastTransform);
				OldTransform = RenderResource->LastTransformVersion.Get(Entity);
			}

			if (OldTransform->GenerationVal != Command.GetEntityGeneration(Entity))
			{
				OldTransform->Version = UINT32_MAX;
				OldTransform->GenerationVal = Command.GetEntityGeneration(Entity);
			}

			if (OldTransform->Version != Transform->GetVersion())
			{
				ObjectShaderData Data;
				Data.TransformationMat = Transform->GetWorldMatrix();

				OldTransform->GenerationVal = Command.GetEntityGeneration(Entity);
				OldTransform->Version = Transform->GetVersion();

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
		auto RenderResource = Command.GetResource<Chilli::RenderResource>();
		auto RenderGraph = Command.GetResource<Chilli::RenderResource>()->RenderGraph;
		RenderResource->SwapChainState = ResourceState::Present;

		for (int PassIdx = 0; PassIdx < RenderGraph->GetPasses().size(); PassIdx++)
		{
			auto& Pass = RenderGraph->GetPasses()[PassIdx];

			RenderService->PushPipelineBarriers(Pass->GetDesc().PrePassBarriers.data(),
				Pass->GetDesc().PrePassBarrierCount, RenderStreamTypes::GRAPHICS);
			{
				auto& PrePassChanges = RenderGraph->GetPrePassChanges()[PassIdx];

				for (size_t i = 0; i < PrePassChanges.ResourceChangeBufferHandles.size(); i++)
				{
					PrePassChanges.ResourceChangeBufferHandles[i].ValPtr->ResourceState = PrePassChanges.ResourceChangeBufferStates[i];
				}

				for (size_t i = 0; i < PrePassChanges.ResourceChangeImageHandles.size(); i++)
				{
					if (PrePassChanges.ResourceChangeImageHandles[i].IsSwapChain)
						RenderResource->SwapChainState = PrePassChanges.ResourceChangeImageStates[i];
					else
						PrePassChanges.ResourceChangeImageHandles[i].Texture.ValPtr->ImageHandle.ValPtr->Spec.State = PrePassChanges.ResourceChangeImageStates[i];
				}
			}

			RenderService->BeginRenderPass(Pass->GetDesc());
			Pass->Execute(Command, Pass->GetDesc());

			RenderService->EndRenderPass();

			RenderService->PushPipelineBarriers(Pass->GetDesc().PostPassBarriers.data(),
				Pass->GetDesc().PostPassBarrierCount, RenderStreamTypes::GRAPHICS);

			{
				auto& PostPassChanges = RenderGraph->GetPostPassChanges()[PassIdx];

				for (size_t i = 0; i < PostPassChanges.ResourceChangeBufferHandles.size(); i++)
				{
					PostPassChanges.ResourceChangeBufferHandles[i].ValPtr->ResourceState = PostPassChanges.ResourceChangeBufferStates[i];
				}

				for (size_t i = 0; i < PostPassChanges.ResourceChangeImageHandles.size(); i++)
				{
					if (PostPassChanges.ResourceChangeImageHandles[i].IsSwapChain)
						RenderResource->SwapChainState = PostPassChanges.ResourceChangeImageStates[i];
					else
						PostPassChanges.ResourceChangeImageHandles[i].Texture.ValPtr->ImageHandle.ValPtr->Spec.State = PostPassChanges.ResourceChangeImageStates[i];
				}
			}
		}
	}

	void OnRenderExtensionRenderEnd(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();

		RenderService->EndFrame();
	}

	class GeometryPass : public Chilli::RenderGraphPass
	{
	public:
		RenderPassDesc Setup(Chilli::Command& Command, RenderGraphRegistry& Registry) override
		{
			auto ActiveWindow = Command.GetActiveWindow();
			auto ActiveWindowSize = IVec2{ ActiveWindow->GetWidth(), ActiveWindow->GetHeight() };

			ImageSpec ColorTargetImageSpec;
			ColorTargetImageSpec.Format = ImageFormat::RGBA8;
			ColorTargetImageSpec.ImageData = nullptr;
			ColorTargetImageSpec.MipLevel = 1;
			ColorTargetImageSpec.Resolution = { ActiveWindowSize.x, ActiveWindowSize.y, 1 };
			ColorTargetImageSpec.Sample = IMAGE_SAMPLE_COUNT_1_BIT;
			ColorTargetImageSpec.State = ResourceState::ShaderRead;
			ColorTargetImageSpec.Type = ImageType::IMAGE_TYPE_2D;
			ColorTargetImageSpec.Usage = IMAGE_USAGE_COLOR_ATTACHMENT | IMAGE_USAGE_SAMPLED_IMAGE;
			_ColorTargetImage = Command.AllocateImage(ColorTargetImageSpec);

			TextureSpec ColorTargetTextureSpec;
			ColorTargetTextureSpec.Format = ImageFormat::RGBA8;

			_ColorTargetTexture = Command.CreateTexture(_ColorTargetImage, ColorTargetTextureSpec);

			RenderPassDesc Desc;
			Desc.Name = "GeometryPass";
			Desc.Stage = RenderPassStage::AfterOpaque;

			ColorAttachment ColorTargetAttachment;
			ColorTargetAttachment.LoadOp = AttachmentLoadOp::CLEAR;
			ColorTargetAttachment.StoreOp = AttachmentStoreOp::STORE;
			ColorTargetAttachment.InitialState = ResourceState::RenderTarget;
			ColorTargetAttachment.FinalState = ResourceState::ShaderRead;
			ColorTargetAttachment.UseSwapChainImage = false;
			ColorTargetAttachment.ClearColor = { 0, 1, 0, 1 };
			ColorTargetAttachment.ColorTexture = _ColorTargetTexture.ValPtr->RawTextureHandle;

			_Resources.ColorAttachments[Desc.ColorAttachmentCount].IsSwapChain = false;
			_Resources.ColorAttachments[Desc.ColorAttachmentCount].Color = _ColorTargetTexture;
			Desc.ColorAttachments[Desc.ColorAttachmentCount++] = ColorTargetAttachment;

			Registry.AddImageResource("GeometryColor", _ColorTargetTexture);

			Desc.RenderArea = { ActiveWindowSize.x, ActiveWindowSize.y };
			_Desc = Desc;

			_MeshLayout.BeginBinding(0);
			_MeshLayout.AddAttribute(ShaderObjectTypes::FLOAT3,
				"InPosition", 0);
			_MeshLayout.AddAttribute(ShaderObjectTypes::FLOAT2,
				"InTexCoords", 1);

			PipelineBuilder Builder;
			_PipelineState = Builder.Default().
				AddColorBlend(ColorBlendAttachmentState::OpaquePass())
				.Build();
			return Desc;
		}

		void OnResize(Chilli::Command& Command, uint32_t Width, uint32_t Height)
		{
			_Desc.RenderArea = { (int)Width, (int)Height };
		}

		void Execute(Chilli::Command& Command, const RenderPassDesc& Desc) override
		{
			auto RenderService = Command.GetService<Renderer>();
			auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
		}

		RGKey GetColorTargetTextureKey() {
			return "GeometryColor";
		}

	private:
		BackBone::AssetHandle<Image> _ColorTargetImage;
		BackBone::AssetHandle<Texture> _ColorTargetTexture;
		PipelineStateInfo _PipelineState;
		VertexInputShaderLayout _MeshLayout;
	};

	class ScreenPass : public Chilli::RenderGraphPass
	{
	public:
		ScreenPass(RGKey ColorDisplayKey)
			:_ScreenColorDisplayTextureKey(ColorDisplayKey)
		{

		}

		RenderPassDesc Setup(Chilli::Command& Command, RenderGraphRegistry& Registry) override
		{
			RenderPassDesc Desc;
			Desc.Name = "ScreenPass";
			Desc.Stage = RenderPassStage::AfterOpaque;

			ColorAttachment Color;
			Color.LoadOp = AttachmentLoadOp::CLEAR;
			Color.StoreOp = AttachmentStoreOp::STORE;
			Color.InitialState = ResourceState::RenderTarget;
			Color.FinalState = ResourceState::RenderTarget;
			Color.UseSwapChainImage = true;
			Color.ClearColor = { 0, 0, 0, 1 };
			_Resources.ColorAttachments[Desc.ColorAttachmentCount].IsSwapChain = true;
			Desc.ColorAttachments[Desc.ColorAttachmentCount++] = Color;

			Desc.RenderArea = { 800, 600 };
			_Desc = Desc;

			auto ScreenVertexShader = Command.CreateShaderModule("Assets/Shaders/screen_vert.spv",
				ShaderStageType::SHADER_STAGE_VERTEX);
			auto ScreenFragShader = Command.CreateShaderModule("Assets/Shaders/screen_frag.spv",
				ShaderStageType::SHADER_STAGE_FRAGMENT);

			_ScreenShader = Command.CreateShaderProgram();
			Command.AttachShaderModule(_ScreenShader, ScreenVertexShader);
			Command.AttachShaderModule(_ScreenShader, ScreenFragShader);
			Command.LinkShaderProgram(_ScreenShader);

			struct ScreenVertex
			{
				Vec3 Position;
				Vec2 TexCoords;
			};

			ScreenVertex vertices[4] =
			{
				// Position              // TexCoords
				{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f } }, // Bottom-left
				{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f } }, // Bottom-right
				{ {  0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f } }, // Top-right
				{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f } }  // Top-left
			};

			uint32_t indices[6] =
			{
				0, 1, 2, // First triangle
				2, 3, 0  // Second triangle
			};

			MeshCreateInfo SquareCreateInfo{};
			SquareCreateInfo.Vertices = &vertices[0];
			SquareCreateInfo.VertCount = 4;
			SquareCreateInfo.IndexBufferState = BufferState::STATIC_DRAW;
			SquareCreateInfo.IndexType = IndexBufferType::UINT32_T;
			SquareCreateInfo.Indicies = &indices[0];
			SquareCreateInfo.IndexCount = 6;
			SquareCreateInfo.InstanceCount = 1;

			SquareCreateInfo.MeshLayout.BeginBinding(0);
			SquareCreateInfo.MeshLayout.AddAttribute(ShaderObjectTypes::FLOAT3,
				"InPosition", 0);
			SquareCreateInfo.MeshLayout.AddAttribute(ShaderObjectTypes::FLOAT2,
				"InTexCoords", 1);

			_ScreenMeshLayout = SquareCreateInfo.MeshLayout;

			_ScreenMesh = Command.CreateMesh(SquareCreateInfo);

			PipelineBuilder Builder;
			_ScreenPipelineState = Builder.Default().
				AddColorBlend(ColorBlendAttachmentState::OpaquePass())
				.Build();

			SamplerSpec SamplerSpec;
			SamplerSpec.Mode = SamplerMode::REPEAT;
			SamplerSpec.Filter = SamplerFilter::LINEAR;
			_ScreenSampler = Command.CreateSampler(SamplerSpec);

			auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
			_ScreenMaterial = MaterialSystem->CreateMaterial(_ScreenShader);
			MaterialSystem->SetAlbedoColor(_ScreenMaterial, { 1.0f, 0, 0, 1.0f });
			MaterialSystem->SetAlbedoTexture(_ScreenMaterial, _ScreenColorDisplayTexture);
			MaterialSystem->SetAlbedoSampler(_ScreenMaterial, _ScreenSampler);

			auto RenderService = Command.GetService<Renderer>();

			_ScreenColorDisplayTexture = Registry.GetImageResource(_ScreenColorDisplayTextureKey);

			RenderService->UpdateMaterialSamplerData(MaterialSystem->GetRawMaterialHandle(_ScreenMaterial),
				_ScreenSampler.ValPtr->SamplerHandle, "InSampler", 0);
			RenderService->UpdateMaterialTextureData(MaterialSystem->GetRawMaterialHandle(_ScreenMaterial),
				_ScreenColorDisplayTexture.ValPtr->RawTextureHandle, "InColorTarget", ResourceState::ShaderRead, 0);

			return Desc;
		}

		void OnResize(Chilli::Command& Command, uint32_t Width, uint32_t Height)
		{
			_Desc.RenderArea = { (int)Width, (int)Height };
		}

		void Execute(Chilli::Command& Command, const RenderPassDesc& Desc) override
		{
			auto RenderService = Command.GetService<Renderer>();
			auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

			// Issue draw calls here — Command wraps your existing API
			RenderService->BindShaderProgram(_ScreenShader.ValPtr->RawProgramHandle);

			RenderService->SetFullPipelineState(_ScreenPipelineState);
			RenderService->SetVertexInputLayout(_ScreenMeshLayout);

			RenderService->BindMaterailData(MaterialSystem->GetRawMaterialHandle(_ScreenMaterial));

			RenderService->BindVertexBuffer({ _ScreenMesh.ValPtr->VertexBufferHandles[0].ValPtr->RawBufferHandle });
			RenderService->BindIndexBuffer(_ScreenMesh.ValPtr->IBHandle.ValPtr->RawBufferHandle,
				_ScreenMesh.ValPtr->IBType);
			RenderService->DrawIndexed(_ScreenMesh.ValPtr->IndexCount, 1, 0, 0, 0);
		}

	private:
		BackBone::AssetHandle<ShaderProgram> _ScreenShader;
		BackBone::AssetHandle<Mesh> _ScreenMesh;
		BackBone::AssetHandle<Material> _ScreenMaterial;
		BackBone::AssetHandle<Sampler> _ScreenSampler;
		BackBone::AssetHandle<Texture> _ScreenColorDisplayTexture;
		PipelineStateInfo _ScreenPipelineState;
		VertexInputShaderLayout _ScreenMeshLayout;
		RGKey _ScreenColorDisplayTextureKey;
	};

	class PresentPass : public Chilli::RenderGraphPass
	{
	public:
		RenderPassDesc Setup(Chilli::Command& Command, RenderGraphRegistry& Registry) override
		{
			RenderPassDesc Desc;
			Desc.Name = "PresentPass";
			Desc.Stage = RenderPassStage::BeforePresent;

			ColorAttachment Color;
			Color.LoadOp = AttachmentLoadOp::LOAD;
			Color.StoreOp = AttachmentStoreOp::STORE;
			Color.InitialState = ResourceState::Present;
			Color.FinalState = ResourceState::Present;
			Color.UseSwapChainImage = true;

			_Resources.ColorAttachments[Desc.ColorAttachmentCount].IsSwapChain = true;
			Desc.ColorAttachments[Desc.ColorAttachmentCount++] = Color;

			Desc.RenderArea = { 800, 600 };

			_Desc = Desc;

			return Desc;
		}

		void OnResize(Chilli::Command& Command, uint32_t Width, uint32_t Height)
		{
			_Desc.RenderArea = { (int)Width, (int)Height };
		}

		void Execute(Chilli::Command& Command, const RenderPassDesc& Desc) override
		{
			// Issue draw calls here — Command wraps your existing API
		}

	private:
	};

#pragma region RenderGraph
	void RenderGraph::Build(Chilli::Command& Command)
	{
		// 1. Initial Setup: Call Setup() on all passes to gather their Descriptors
		for (auto& pass : _Passes) {
			pass->Setup(Command, _Registry);
			// IsValid Pass
			auto IsValid = this->_IsPassValid(pass.get());
			if (IsValid == RenderGraphErrorCodes::NONE)
				CH_CORE_INFO("Valid Pass: {}", pass->GetDesc().Name);
			else {
				CH_CORE_INFO("Invalid Valid Pass: {} Error: {}", pass->GetDesc().Name, RenderGraphErrorCodesToString(IsValid));
			}
		}

		// 2. Sorting by Stage (Primary Sort)
		std::sort(_Passes.begin(), _Passes.end(), [](const auto& a, const auto& b) {
			return (uint32_t)a->GetDesc().Stage < (uint32_t)b->GetDesc().Stage;
			});

		// 3. Apply Order Overrides (Secondary Sort)
		for (const auto& override : _OrderOverrides) {
			auto it = std::find_if(_Passes.begin(), _Passes.end(), [&](const auto& p) {
				return p->GetDesc().Name == override.RelativeTo;
				});

			if (it != _Passes.end()) {
				// Remove the pass from its current position if it exists in _Passes
				auto passIt = std::find(_Passes.begin(), _Passes.end(), override.Pass);
				if (passIt != _Passes.end()) _Passes.erase(passIt);

				// Re-insert Relative to the target
				if (override.Before) _Passes.insert(it, override.Pass);
				else _Passes.insert(std::next(it), override.Pass);
			}
		}

		uint32_t SwapChainHandle = UINT32_MAX;

		std::unordered_map<uint32_t, ResourceState> CurrentImageResourceStates;
		std::unordered_map<uint32_t, ResourceState> CurrentBufferResourceStates;
		CurrentImageResourceStates[SwapChainHandle] = ResourceState::Present;

		_PrePassChanges.resize(_Passes.size());
		_PostPassChanges.resize(_Passes.size());

		for (int i = 0; i < _Passes.size(); i++)
		{
			auto& Pass = _Passes[i];
			auto& PrePasChange = _PrePassChanges[i];
			auto& PostPasChange = _PostPassChanges[i];
			auto& Desc = Pass->GetDesc();
			auto& DescResources = Pass->GetResources();

			for (int x = 0; x < Desc.ColorAttachmentCount; x++)
			{
				auto& ColorAttachment = Desc.ColorAttachments[x];

				auto Texture = DescResources.ColorAttachments[x].Color;

				PipelineBarrier PrePass, PostPass;

				if (ColorAttachment.UseSwapChainImage)
				{
					if (CurrentImageResourceStates[SwapChainHandle] != ColorAttachment.InitialState)
					{
						// Add a Pre Pass
						PrePass = FillImagePipelineBarrier(UINT32_MAX, true, CurrentImageResourceStates[SwapChainHandle],
							ColorAttachment.InitialState, 0, 1, 0, 1);
						CurrentImageResourceStates[SwapChainHandle] = ColorAttachment.InitialState;
						Desc.PrePassBarriers[Desc.PrePassBarrierCount++] = PrePass;

						ResourceStateChange::ImageResource ImageResource;
						ImageResource.IsSwapChain = true;
						PrePasChange.ResourceChangeImageHandles.push_back(ImageResource);
						PrePasChange.ResourceChangeImageStates.push_back(ColorAttachment.InitialState);
					}
					if (ColorAttachment.FinalState != ColorAttachment.InitialState)
					{
						// Add a Pre Pass
						PostPass = FillImagePipelineBarrier(UINT32_MAX, true, ColorAttachment.InitialState, ColorAttachment.FinalState, 0, 1, 0, 1);
						CurrentImageResourceStates[SwapChainHandle] = ColorAttachment.FinalState;
						Desc.PostPassBarriers[Desc.PostPassBarrierCount++] = PostPass;

						ResourceStateChange::ImageResource ImageResource;
						ImageResource.IsSwapChain = true;
						PostPasChange.ResourceChangeImageHandles.push_back(ImageResource);
						PostPasChange.ResourceChangeImageStates.push_back(ColorAttachment.FinalState);
					}
				}
				else
				{
					// check if key exists
					if (CurrentImageResourceStates.find(ColorAttachment.ColorTexture) != CurrentImageResourceStates.end()) {
						// key exists
					}
					else {
						CurrentImageResourceStates[ColorAttachment.ColorTexture] = DescResources.ColorAttachments[x].Color.ValPtr->ImageHandle.ValPtr->Spec.State;
					}

					if (CurrentImageResourceStates[ColorAttachment.ColorTexture] != ColorAttachment.InitialState)
					{
						// Add a Pre Pass
						PrePass = FillImagePipelineBarrier(ColorAttachment.ColorTexture, false, CurrentImageResourceStates[ColorAttachment.ColorTexture],
							ColorAttachment.InitialState, 0, 1, 0, 1);
						CurrentImageResourceStates[ColorAttachment.ColorTexture] = ColorAttachment.InitialState;
						Desc.PrePassBarriers[Desc.PrePassBarrierCount++] = PrePass;

						ResourceStateChange::ImageResource ImageResource;
						ImageResource.IsSwapChain = false;
						ImageResource.Texture = DescResources.ColorAttachments[x].Color;
						PrePasChange.ResourceChangeImageHandles.push_back(ImageResource);
						PrePasChange.ResourceChangeImageStates.push_back(ColorAttachment.InitialState);

					}
					if (ColorAttachment.FinalState != ColorAttachment.InitialState)
					{
						// Add a Pre Pass
						PostPass = FillImagePipelineBarrier(ColorAttachment.ColorTexture, false, ColorAttachment.InitialState, ColorAttachment.FinalState, 0, 1, 0, 1);
						CurrentImageResourceStates[ColorAttachment.ColorTexture] = ColorAttachment.FinalState;
						Desc.PostPassBarriers[Desc.PostPassBarrierCount++] = PostPass;

						ResourceStateChange::ImageResource ImageResource;
						ImageResource.IsSwapChain = false;
						ImageResource.Texture = DescResources.ColorAttachments[x].Color;
						PostPasChange.ResourceChangeImageHandles.push_back(ImageResource);
						PostPasChange.ResourceChangeImageStates.push_back(ColorAttachment.FinalState);
					}
				}

			}

			for (int x = 0; x < Desc.InputAttachmentCount; x++)
			{
				auto& InputAttachment = Desc.InputAttachments[x];

				auto Texture = DescResources.InputAttachments[x];

				PipelineBarrier Barrier;
				// check if key exists
				if (CurrentImageResourceStates.find(InputAttachment) != CurrentImageResourceStates.end()) {
					// key exists
				}
				else {
					CurrentImageResourceStates[InputAttachment] = DescResources.InputAttachments[x].ValPtr->ImageHandle.ValPtr->Spec.State;
				}

				if (CurrentImageResourceStates[InputAttachment] != ResourceState::ShaderRead)
				{
					// Add a Pre Pass
					Barrier = FillImagePipelineBarrier(InputAttachment, false, CurrentImageResourceStates[InputAttachment],
						ResourceState::ShaderRead, 0, 1, 0, 1);
					CurrentImageResourceStates[InputAttachment] = ResourceState::ShaderRead;
					Desc.PrePassBarriers[Desc.PrePassBarrierCount++] = Barrier;

					ResourceStateChange::ImageResource ImageResource;
					ImageResource.IsSwapChain = false;
					ImageResource.Texture = DescResources.InputAttachments[x];
					PrePasChange.ResourceChangeImageHandles.push_back(ImageResource);
					PrePasChange.ResourceChangeImageStates.push_back(ResourceState::ShaderRead);
				}
			}

			for (int x = 0; x < Desc.BufferDependencyCount; x++)
			{
				auto& Dependency = Desc.BufferDependencies[x];

				auto Buffer = DescResources.BufferDependencies[x];

				PipelineBarrier Barrier;
				// check if key exists
				if (CurrentBufferResourceStates.find(Dependency.Handle) != CurrentBufferResourceStates.end()) {
					// key exists
				}
				else {
					CurrentBufferResourceStates[Dependency.Handle] = DescResources.BufferDependencies[x].ValPtr->ResourceState;
				}

				if (CurrentBufferResourceStates[Dependency.Handle] != Dependency.RequiredState)
				{
					// Add a Pre Pass
					Barrier = FillBufferPipelineBarrier(Dependency.Handle, CurrentBufferResourceStates[Dependency.Handle],
						Dependency.RequiredState, 0);;
					CurrentBufferResourceStates[Dependency.Handle] = Dependency.RequiredState;
					Desc.PrePassBarriers[Desc.PrePassBarrierCount++] = Barrier;

					PrePasChange.ResourceChangeBufferHandles.push_back(DescResources.BufferDependencies[x]);
					PrePasChange.ResourceChangeBufferStates.push_back(Dependency.RequiredState);
				}
			}
			// Depth Pass
			if (Desc.HasDepthStencil)
			{
				auto& DepthAttachment = Desc.DepthStencil;

				auto Texture = DescResources.DepthTexture;

				// check if key exists
				if (CurrentImageResourceStates.find(DepthAttachment.DepthTexture) != CurrentImageResourceStates.end()) {
					// key exists
				}
				else {
					CurrentImageResourceStates[DepthAttachment.DepthTexture] = DescResources.DepthTexture.ValPtr->ImageHandle.ValPtr->Spec.State;
				}
				if (CurrentImageResourceStates[DepthAttachment.DepthTexture] != ResourceState::DepthWrite)
				{
					auto Bar = FillImagePipelineBarrier(DepthAttachment.DepthTexture, false, CurrentImageResourceStates[DepthAttachment.DepthTexture],
						ResourceState::DepthWrite, 0, 1, 0, 1);

					ResourceStateChange::ImageResource ImageResource;
					ImageResource.IsSwapChain = false;
					ImageResource.Texture = Texture;
					PrePasChange.ResourceChangeImageHandles.push_back(ImageResource);
					PrePasChange.ResourceChangeImageStates.push_back(ResourceState::DepthWrite);

					Desc.PrePassBarriers[Desc.PrePassBarrierCount++] = Bar;
				}

			}
		}

		for (auto& Pass : _Passes)
		{
			CH_CORE_INFO("Name: {}", Pass->GetDesc().Name);
		}
	}

	RenderGraphErrorCodes RenderGraph::_IsPassValid(const RenderGraphPass* Pass)
	{
		auto Desc = Pass->GetDesc();
		auto Resources = Pass->GetResources();

		// Depth check
		if (Desc.HasDepthStencil != Resources.HasDepthStencil)
			return RenderGraphErrorCodes::DESC_RESOURCES_DEPTH_MISMATCH;
		else
		{
			if (Desc.HasDepthStencil)
			{
				if (Desc.DepthStencil.DepthTexture == UINT32_MAX)
					return RenderGraphErrorCodes::DESC_DEPTH_TEXTURE_NULL;

				if (!Resources.DepthTexture.IsValid())
					return RenderGraphErrorCodes::RESOURCES_DEPTH_TEXTURE_NULL;

				if (Desc.DepthStencil.DepthTexture != Resources.DepthTexture.ValPtr->RawTextureHandle)
					return RenderGraphErrorCodes::DESC_RESOURCES_DEPTH_MISMATCH;

				auto Resolution = Resources.DepthTexture.ValPtr->ImageHandle.ValPtr->Spec.Resolution;

				if (Resolution.Width != Desc.RenderArea.x ||
					Resolution.Height != Desc.RenderArea.y)
				{
					return RenderGraphErrorCodes::DESC_RENDERAREA_RESOURCES_DEPTH_TEXTURE_MISMATCH;
				}
			}
		}

		// Color attachments
		for (int i = 0; i < Desc.ColorAttachmentCount; i++)
		{
			if (Resources.ColorAttachments[i].IsSwapChain != Desc.ColorAttachments[i].UseSwapChainImage)
				return RenderGraphErrorCodes::DESC_RESOURCES_COLOR_SWAPCHAIN_MISMATCH;

			if (!Desc.ColorAttachments[i].UseSwapChainImage)
			{
				if (Desc.ColorAttachments[i].ColorTexture == UINT32_MAX)
					return RenderGraphErrorCodes::DESC_COLOR_TEXTURE_NULL;

				if (!Resources.ColorAttachments[i].Color.IsValid())
					return RenderGraphErrorCodes::RESOURCES_COLOR_TEXTURE_NULL;

				if (Desc.ColorAttachments[i].ColorTexture != Resources.ColorAttachments[i].Color.ValPtr->RawTextureHandle)
					return RenderGraphErrorCodes::DESC_RESOURCES_COLOR_TEXTURE_MISMATCH;
			}
		}

		for (int i = 0; i < Resources.ColorAttachments.size(); i++)
		{
			if (Resources.ColorAttachments[i].Color.IsValid() &&
				Desc.ColorAttachments[i].ColorTexture == UINT32_MAX)
			{
				return RenderGraphErrorCodes::DESC_RESOURCES_COLOR_TEXTURE_NOT_ONE_TO_ONE;
			}
		}

		// Input attachments
		for (int i = 0; i < Desc.InputAttachmentCount; i++)
		{
			if (Desc.InputAttachments[i] == UINT32_MAX)
				return RenderGraphErrorCodes::DESC_INPUT_TEXTURE_NULL;

			if (!Resources.InputAttachments[i].IsValid())
				return RenderGraphErrorCodes::RESOURCES_INPUT_TEXTURE_NULL;

			if (Desc.InputAttachments[i] != Resources.InputAttachments[i].ValPtr->RawTextureHandle)
				return RenderGraphErrorCodes::DESC_RESOURCES_INPUT_TEXTURE_MISMATCH;
		}

		for (int i = 0; i < Resources.InputAttachments.size(); i++)
		{
			if (Resources.InputAttachments[i].IsValid() &&
				Desc.InputAttachments[i] == UINT32_MAX)
			{
				return RenderGraphErrorCodes::DESC_RESOURCES_INPUT_TEXTURE_NOT_ONE_TO_ONE;
			}
		}

		// Render area checks (color)
		for (int i = 0; i < Desc.ColorAttachmentCount; i++)
		{
			if (!Resources.ColorAttachments[i].IsSwapChain)
			{
				auto Resolution = Resources.ColorAttachments[i].Color.ValPtr->ImageHandle.ValPtr->Spec.Resolution;

				if (Resolution.Width != Desc.RenderArea.x ||
					Resolution.Height != Desc.RenderArea.y)
				{
					return RenderGraphErrorCodes::DESC_RENDERAREA_RESOURCES_COLOR_TEXTURE_MISMATCH;
				}
			}
		}

		return RenderGraphErrorCodes::NONE;
	}

	void RenderGraph::AddBarrier(std::array<PipelineBarrier, CHILLI_MAX_PASS_BARRIERS>& container,
		uint32_t& counter, uint32_t handle,
		ResourceState oldState, ResourceState newState, bool IsSwapChain)
	{
		if (counter >= CHILLI_MAX_PASS_BARRIERS) return;

		PipelineBarrier barrier{};
		barrier.Image.Handle = handle;
		barrier.OldState = oldState;
		barrier.NewState = newState;
		barrier.Image.IsSwapChain = IsSwapChain;
		// Use your existing Lookup Table logic
		SyncMapping src = GetSyncInfo(oldState);
		SyncMapping dst = GetSyncInfo(newState);

		barrier.SrcStage = src.Stage;
		barrier.DstStage = dst.Stage;
		barrier.SrcAccess = src.Access;
		barrier.DstAccess = dst.Access;

		container[counter++] = barrier;
	}
#pragma endregion

		void OnRenderExtensionSetup(BackBone::SystemContext& Ctxt)
		{
			auto Command = Chilli::Command(Ctxt);
			auto RenderGraph = Command.GetResource<Chilli::RenderResource>()->RenderGraph;

			auto GeometryPass = std::make_shared<Chilli::GeometryPass>();

			RenderGraph->AddPass(GeometryPass);
			RenderGraph->AddPass<ScreenPass>(GeometryPass->GetColorTargetTextureKey());
			RenderGraph->AddPass<PresentPass>();

			RenderGraph->Build(Command);
		}

	void RenderExtension::Build(BackBone::App& App)
	{
		App.Registry.AddResource<RenderExtensionConfig>();
		auto Command = Chilli::Command(App.Ctxt);

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
		App.AssetRegistry.RegisterStore<ImageData>();
		App.ServiceRegistry.RegisterService<SceneManager>(std::make_shared<SceneManager>(App.Ctxt.Registry));
		App.ServiceRegistry.RegisterService<ParentChildMapTable>(std::make_shared<ParentChildMapTable>());

		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::START_UP, OnRenderExtensionsSetup);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::START_UP, OnRenderExtensionSetup);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::INPUT, OnRenderExtensionDefferedRenderingUpdate);

		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::RENDER, OnRenderExtensionRenderBegin);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::RENDER,
			OnRenderExtensionRender);
		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::RENDER, OnRenderExtensionRenderEnd);

		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::SHUTDOWN, OnRenderExtensionFinishRendering);
		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::SHUTDOWN, OnRenderExtensionsCleanUp);
	}
#pragma endregion

}