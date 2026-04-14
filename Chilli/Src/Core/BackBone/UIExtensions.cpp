#include "Ch_PCH.h"
#include "DeafultExtensions.h"
#include "Profiling\Timer.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "nlohmann\json.hpp"

namespace Chilli
{
#pragma region Flame

	MSDFData loadMSDFJson(const std::string& path) {
		std::ifstream f(path);
		nlohmann::json data = nlohmann::json::parse(f);

		MSDFData font;
		font.PixelRange = data["atlas"]["distanceRange"];
		font.AtlasWidth = data["atlas"]["width"];
		font.AtlasHeight = data["atlas"]["height"];

		for (auto& item : data["glyphs"]) {
			MSDFGlyphInfo g;
			g.unicode = item["unicode"];
			g.advance = item["advance"];

			// Not all glyphs (like spaces) have bounds
			if (item.contains("planeBounds")) {
				g.pl = item["planeBounds"]["left"];
				g.pb = item["planeBounds"]["bottom"];
				g.pr = item["planeBounds"]["right"];
				g.pt = item["planeBounds"]["top"];

				g.al = item["atlasBounds"]["left"];
				g.ab = item["atlasBounds"]["bottom"];
				g.ar = item["atlasBounds"]["right"];
				g.at = item["atlasBounds"]["top"];
			}

			font.GlyphMap.Insert(g.unicode, g);
		}

		return font;
	}

	Msdfgen_Loader::~Msdfgen_Loader()
	{
		for (int i = 0; i < _IndexMaps.size(); i++)
		{
			if (_IndexMaps[i].IsValid())
				CH_CORE_ERROR("All Msdf Data Must be Unloaded!");
		}
	}

	BackBone::AssetHandle<MSDFData> Msdfgen_Loader::LoadTyped(BackBone::SystemContext& Ctxt, const std::string& Path)
	{
		auto Command = Chilli::Command(Ctxt);
		auto MsdfStore = Command.GetStore<MSDFData>();

		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));

		int Index = IndexEntry::FindIndex(_IndexMaps, HashValue);
		if (Index != -1)
			return _Msdf_Handles[Index];

		auto Data = loadMSDFJson(Path);
		Data.FilePath = Path;

		auto Handle = MsdfStore->Add(Data);

		uint32_t NewIndex = static_cast<uint32_t>(_Msdf_Handles.size());
		_Msdf_Handles.push_back(Handle);
		IndexEntry::AddOrUpdateSorted(_IndexMaps, { HashValue, NewIndex });

		return Handle;
	}

	void Msdfgen_Loader::Unload(BackBone::SystemContext& Ctxt, const std::string& Path)
	{
		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));

		// 1. Find the local index in our tracking vector
		int LocalIndex = IndexEntry::FindIndex(_IndexMaps, HashValue);

		if (LocalIndex != -1 && LocalIndex < _Msdf_Handles.size()) {
			auto Handle = _Msdf_Handles[LocalIndex];

			if (Handle.IsValid()) {
				// 2. Remove from the global store (StbiStore)
				// This is crucial for freeing the malloc'd pixels via the STBITTF_Data destructor
				auto Command = Chilli::Command(Ctxt);
				auto MsdfStore = Command.GetStore<MSDFData>();

				MsdfStore->Remove(Handle);

				// 3. Clear our local handle to drop the reference
				_Msdf_Handles[LocalIndex] = BackBone::AssetHandle<MSDFData>();
			}

			// 4. Mark the index entry as invalid so it's not found again
			IndexEntry::Invalidate(_IndexMaps, HashValue);
		}
	}

	void Msdfgen_Loader::Unload(BackBone::SystemContext& Ctxt, const BackBone::AssetHandle<MSDFData>& Data)
	{
		if (Data.IsValid()) {
			// Redirect to the path-based unload to keep logic centralized
			Unload(Ctxt, Data.ValPtr->FilePath);
		}
	}

	bool Msdfgen_Loader::DoesExist(const std::string& Path) const
	{
		uint64_t HashValue = HashString64(GetFileNameWithExtension(Path));
		int Index = IndexEntry::FindIndex(_IndexMaps, HashValue);
		if (Index != -1)
			return true;
		return false;
	}

	void FlameResource::UpdateFontAtlasTextureMap(Renderer* RenderService, MaterialSystem* MaterialSystem,
		BackBone::AssetHandle<Texture> Tex)
	{
		ShaderArrayFontAtlasMetaData Data;
		Data.Tex = Tex;
		Data.ShaderIndex = _FontAtlasTextureMapCount;
		FontAtlasTextureMap.Insert(Tex.Handle, Data);

		RenderService->UpdateMaterialTextureData(MaterialSystem->GetRawMaterialHandle(this->Material),
			Tex.ValPtr->RawTextureHandle, "fontAtlas", ResourceState::ShaderRead, _FontAtlasTextureMapCount);
		_FontAtlasTextureMapCount++;
	}

	void OnFlameStartUp(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		Command.AddLoader<Msdfgen_Loader>();
		auto FlameResource = Command.GetResource<Chilli::FlameResource>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();
		auto RenderService = Command.GetService<Chilli::Renderer>();
		auto Config = Command.GetResource<FlameExtensionConfig>();

		// Prepare Shader
		auto VertexShader = Command.CreateShaderModule("Assets/Shaders/ember_vert.spv",
			Chilli::ShaderStageType::SHADER_STAGE_VERTEX);
		auto FragShader = Command.CreateShaderModule("Assets/Shaders/ember_frag.spv",
			Chilli::ShaderStageType::SHADER_STAGE_FRAGMENT);

		FlameResource->ShaderProgram = Command.CreateShaderProgram();
		Command.AttachShaderModule(FlameResource->ShaderProgram, VertexShader);
		Command.AttachShaderModule(FlameResource->ShaderProgram, FragShader);
		Command.LinkShaderProgram(FlameResource->ShaderProgram);

		FlameResource->Material = MaterialSystem->CreateMaterial(FlameResource->ShaderProgram);

		const uint32_t IntiailCharacterCount = 10000;

		Chilli::BufferCreateInfo VertexBufferInfo{};
		VertexBufferInfo.Data = nullptr;
		VertexBufferInfo.SizeInBytes = sizeof(FlameVertex) * IntiailCharacterCount * 6;
		VertexBufferInfo.State = BufferState::DYNAMIC_DRAW;
		VertexBufferInfo.Type = BUFFER_TYPE_VERTEX;
		FlameResource->VertexBuffer = Command.CreateBuffer(VertexBufferInfo);

		FlameResource->Vertices.reserve(IntiailCharacterCount);

		Chilli::SamplerSpec SamplerSpec;
		SamplerSpec.Filter = Chilli::SamplerFilter::LINEAR;
		SamplerSpec.Mode = Chilli::SamplerMode::CLAMP_TO_BORDER;
		auto Sampler = Command.CreateSampler(SamplerSpec);

		RenderService->UpdateMaterialSamplerData(MaterialSystem->GetRawMaterialHandle(FlameResource->Material),
			Sampler.ValPtr->SamplerHandle, "fontAtlasSampler", 0);

		FlameResource->MaterialSSBOs.resize(Config->MaxFrameInFlight);

		for (int i = 0; i < Config->MaxFrameInFlight; i++)
		{
			BufferCreateInfo SSBOInfo;
			SSBOInfo.Data = nullptr;
			SSBOInfo.SizeInBytes = sizeof(FlameMaterial) * 1000;
			SSBOInfo.State = BufferState::DYNAMIC_DRAW;
			SSBOInfo.Type = BUFFER_TYPE_STORAGE;
			FlameResource->MaterialSSBOs[i] = Command.CreateBuffer(SSBOInfo);
		}
	}

	void BuildVertexData(FlameTextComponent& text, const PepperTransform& transform,
		std::vector<FlameVertex>& OutVertices, FlameResource* Resource)
	{
		auto& fontAsset = text.Font.ValPtr;
		auto& msdf = fontAsset->RawFontData.ValPtr;

		// Calculate scaling: how many pixels is 1 'font unit'
		// Usually: DesiredHeight / LineHeight
		float fontScale = transform.ActualDimensions.y;

		float cursorX = transform.ActualPosition.x - transform.Pivot.x;
		float cursorY = transform.ActualPosition.y - transform.Pivot.y;

		for (char c : text.Content) {
			// Find glyph (Optimization: use a std::unordered_map<int, MSDFGlyphInfo> for O(1) lookup)
			const auto g = msdf->GlyphMap.Get((uint32_t)c);

			if (g == nullptr)
				continue;

			// Skip space-like characters but still advance the cursor
			if (g->al == 0 && g->ar == 0) {
				cursorX += g->advance * fontScale;
				continue;
			}// Direct mapping (Check if this fixes the missing letters)
			float u0 = g->al / msdf->AtlasWidth;
			float v0 = g->at / msdf->AtlasHeight;
			float u1 = g->ar / msdf->AtlasWidth;
			float v1 = g->ab / msdf->AtlasHeight;

			// 4. Positions: Baseline offsets (planeBounds) * scale
			float x0 = cursorX + (g->pl * fontScale);
			float y0 = cursorY - (g->pt * fontScale);
			float x1 = cursorX + (g->pr * fontScale);
			float y1 = cursorY - (g->pb * fontScale);

			// 5. Build Quad (Pos, UV)
			// Triangle A
			OutVertices.push_back(FlameVertex{ {x0, y0}, {u0, v0},
				Resource->GetFontAtlasTextureShaderIndex(text.Font.ValPtr->AtlasTexture), 0 });
			OutVertices.push_back(FlameVertex{ {x1, y0}, {u1, v0} ,
				Resource->GetFontAtlasTextureShaderIndex(text.Font.ValPtr->AtlasTexture), 0 });
			OutVertices.push_back(FlameVertex{ {x0, y1}, {u0, v1} ,
				Resource->GetFontAtlasTextureShaderIndex(text.Font.ValPtr->AtlasTexture), 0 });
			// Triangle B
			OutVertices.push_back(FlameVertex{ {x1, y0}, {u1, v0} ,
				Resource->GetFontAtlasTextureShaderIndex(text.Font.ValPtr->AtlasTexture), 0 });
			OutVertices.push_back(FlameVertex{ {x1, y1}, {u1, v1} ,
				Resource->GetFontAtlasTextureShaderIndex(text.Font.ValPtr->AtlasTexture), 0 });
			OutVertices.push_back(FlameVertex{ {x0, y1}, {u0, v1} ,
				Resource->GetFontAtlasTextureShaderIndex(text.Font.ValPtr->AtlasTexture), 0 });

			// 6. Advance for next character
			cursorX += g->advance * fontScale;
		}

		// After building, flag this for GPU upload
		text.IsDirty = true;
	}

	void OnFlameUpdate(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto FlameResource = Command.GetResource<Chilli::FlameResource>();
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

		FlameResource->Vertices.clear();
		FlameResource->UICharacterCount = 0;
		FlameResource->GeoCharacterCount = 0;

		for (auto [Entity, Text, Transform] : BackBone::QueryWithEntities<FlameTextComponent,
			PepperTransform>(*Ctxt.Registry))
		{
			if (Text->IsRender == false)
				continue;

			BuildVertexData(*Text, *Transform, FlameResource->Vertices, FlameResource);
			FlameResource->UICharacterCount += Text->Content.size();
		}

		if (FlameResource->UICharacterCount != 0 || FlameResource->GeoCharacterCount != 0)
			Command.MapBufferData(FlameResource->VertexBuffer, FlameResource->Vertices.data(),
				FlameResource->Vertices.size() * sizeof(FlameVertex), 0);
	}

	void OnFlameRenderUI(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderCommandService = Command.GetService<RenderCommand>();
		auto RenderService = Command.GetService<Renderer>();

		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

		auto FlameResource = Command.GetResource<Chilli::FlameResource>();

		auto ActiveMaterial = FlameResource->Material.ValPtr;
		auto ActiveShader = FlameResource->ShaderProgram;
		auto ActiveWindow = Command.GetActiveWindow();

		if (FlameResource->UICharacterCount == 0 && FlameResource->GeoCharacterCount == 0)
			return;

		VertexInputShaderLayout MeshLayout;
		MeshLayout.BeginBinding(0, false, BufferState::DYNAMIC_DRAW); // isd = false
		MeshLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "inPos", 0);
		MeshLayout.AddAttribute(ShaderObjectTypes::FLOAT2, "inTexCoords", 1);
		MeshLayout.AddAttribute(ShaderObjectTypes::UINT1, "inFontIndex", 2);
		MeshLayout.AddAttribute(ShaderObjectTypes::UINT1, "inMaterailIndex", 3);

		RenderService->SetVertexInputLayout(MeshLayout);

		PipelineBuilder UITextPipelineInfoBuilder;
		RenderService->SetFullPipelineState(UITextPipelineInfoBuilder.
			Default()
			.AddColorBlend(ColorBlendAttachmentState::AlphaBlend())
			.SetRasterizer(CullMode::None, FrontFaceMode::Clock_Wise, PolygonMode::Fill)
			.SetDepth(CH_TRUE, CH_FALSE, CompareOp::LESS_OR_EQUAL)
			.Build());

		RenderService->BindShaderProgram(FlameResource->ShaderProgram.ValPtr->RawProgramHandle);
		RenderService->BindMaterailData(MaterialSystem->GetRawMaterialHandle(FlameResource->Material));

		struct Push {
			alignas(4) glm::mat4 projection;
			alignas(4) Vec4 textColor;
			alignas(4) float PixelRange;
		}pc;
		// Use ortho projection: Map [0, width] to [-1, 1] and [0, height] to [-1, 1]
		float width = (float)ActiveWindow->GetWidth();
		float height = (float)ActiveWindow->GetHeight();

		glm::mat4 projection = glm::mat4(1.0f);
		projection[0][0] = 2.0f / width;
		projection[1][1] = 2.0f / height;  // Positive because we'll handle the flip in the math
		projection[2][2] = 1.0f;
		projection[3][0] = -1.0f;          // Map 0 to -1 (Left)
		projection[3][1] = -1.0f;          // Map 0 to -1 (Top)
		projection[3][3] = 1.0f;
		pc.projection = projection;
		//pc.projection[1][1] *= -1; // Flip Y for Vulkan coordinate system
		//pc.projection = glm::mat4(1.0f);
		pc.textColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		pc.PixelRange = 4.0f;

		RenderService->PushInlineUniformData(ActiveShader.ValPtr->RawProgramHandle,
			SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, &pc, sizeof(pc), 0);

		RenderService->BindVertexBuffer({
			FlameResource->VertexBuffer.ValPtr->RawBufferHandle,
			});

		RenderService->DrawArray(6 * FlameResource->UICharacterCount, 1, 0, 0,
			6 * FlameResource->GeoCharacterCount);
	}

	std::vector<PipelineBarrier> GetFlamePrePassPipelineBarrier(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto FlameResource = Command.GetResource<Chilli::FlameResource>();

		PipelineBarrier VertexBarrier = FillBufferPipelineBarrier(
			FlameResource->VertexBuffer.ValPtr->RawBufferHandle,
			ResourceState::HostWrite, ResourceState::VertexRead,
			0);

		return { VertexBarrier };
	}

	std::vector<PipelineBarrier> GetFlamePostPassPipelineBarrier(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto FlameResource = Command.GetResource<Chilli::FlameResource>();

		PipelineBarrier VertexBarrier = FillBufferPipelineBarrier(
			FlameResource->VertexBuffer.ValPtr->RawBufferHandle,
			ResourceState::VertexRead, ResourceState::HostWrite,
			0);

		return { VertexBarrier };
	}

	void FlameExtension::Build(BackBone::App& App)
	{
		auto Command = Chilli::Command(App.Ctxt);

		App.AssetRegistry.RegisterStore<MSDFData>();
		App.AssetRegistry.RegisterStore<FlameFont>();

		App.Registry.AddResource<FlameResource>();
		App.Registry.AddResource<FlameExtensionConfig>();

		auto Config = App.Registry.GetResource<FlameExtensionConfig>();
		*Config = _Config;

		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::START_UP, OnFlameStartUp);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnFlameUpdate);
	}

#pragma endregion

#pragma region Pepper

#pragma region Pepper Material System

	PepperActionKey Pepper::RegisterAction(PepperActionKey Key,
		const PepperActionCallBackFn& CallBack)
	{
		auto Command = Chilli::Command(_Ctxt);
		auto Service = Command.GetService<PepperActionRegistry>();

		Service->RegisterAction(Key, CallBack);
		return Key;
	}

	bool Pepper::DoesActionExist(PepperActionKey Key)
	{
		auto Command = Chilli::Command(_Ctxt);
		auto Service = Command.GetService<PepperActionRegistry>();

		return Service->DoesActionExist(Key);
	}

	void Pepper::ExecuteAction(PepperActionKey Key, BackBone::SystemContext& Ctxt, Event& e, PepperEventTypes Type,
		BackBone::Entity Entity)
	{
		auto Command = Chilli::Command(_Ctxt);
		auto Service = Command.GetService<PepperActionRegistry>();

		Service->ExecuteAction(Key, Ctxt, e, Type, Entity);
	}

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

	BackBone::Entity Pepper::CreateSlider(Chilli::BackBone::SystemContext& Ctxt, PepperActionKey Key,
		const PepperActionCallBackFn& CallBack, BackBone::Entity ParentPanel)
	{
		auto Command = Chilli::Command(Ctxt);

		auto Pepper = Chilli::Pepper(Ctxt);
		auto Slider = Command.CreateEntity();
		auto Platform = Command.CreateEntity();

		Chilli::PepperTransform PlatformTransform;

		// Size: 20% width, 20% height (example)
		PlatformTransform.PercentageDimensions = { 0.1f, 0.01f };

		// Position: Top-right corner
		PlatformTransform.PercentagePosition = { 0.30f, 0.3f };

		PlatformTransform.AnchorX = Chilli::AnchorX::LEFT;  // Align right
		PlatformTransform.AnchorY = Chilli::AnchorY::TOP;    // Align top
		PlatformTransform.ZOrder = 1.0f;

		Chilli::PepperElement PlatformElement;
		PlatformElement.Flags = Chilli::PEPPER_ELEMENT_VISIBLE;
		strncpy(PlatformElement.Name, "Platform", 32);
		PlatformElement.Type = Chilli::PepperElementTypes::BUTTON;
		PlatformElement.Parent = ParentPanel;

		Command.AddComponent(Platform, PlatformTransform);
		Command.AddComponent(Platform, PlatformElement);

		Chilli::PepperTransform panelTransform;

		panelTransform.PercentageDimensions = { 0.02f, 0.04f };

		// Position: Top-right corner
		panelTransform.PercentagePosition = { 0.35f, 0.29f };

		panelTransform.AnchorX = Chilli::AnchorX::LEFT;  // Align right
		panelTransform.AnchorY = Chilli::AnchorY::TOP;    // Align top

		Chilli::PepperElement PepperElement;
		PepperElement.Flags = Chilli::PEPPER_ELEMENT_VISIBLE;
		strncpy(PepperElement.Name, "Slider", 32);
		PepperElement.Type = Chilli::PepperElementTypes::BUTTON;
		PepperElement.Parent = Platform;

		Command.AddComponent(Slider, panelTransform);
		Command.AddComponent(Slider, PepperElement);
		Command.AddComponent(Slider, Chilli::InteractionState());
		Command.AddComponent(Slider, Chilli::SliderComponent{
			.OnValueChanged = Pepper.RegisterAction(Key, CallBack),
			.Platform = Platform,
			});
		Pepper.SetMaterialColor(Slider, { 0.0f,0.0f, 0.0f, 1.0f });

		return Slider;
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

		VertexInputShaderLayout Layout;
		Layout.BeginBinding(0, false, BufferState::DYNAMIC_DRAW);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 1);
		Layout.AddAttribute(ShaderObjectTypes::INT1, "InPepperMaterialIndex", 2);

		MeshCreateInfo RenderMeshInfo;
		RenderMeshInfo.IndexCount = PepperResource->MeshQuadCount * 6;
		RenderMeshInfo.VertCount = PepperResource->MeshQuadCount * 4;
		RenderMeshInfo.IndexType = IndexBufferType::UINT32_T;
		RenderMeshInfo.IndexBufferState = BufferState::DYNAMIC_DRAW;
		RenderMeshInfo.MeshLayout = Layout;
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

		PepperResource->InputKeysToPepperInputText = [] {
			std::array<char, Input_key_Count> arr{};
			arr.fill(0);

			// Numbers
			arr[Input_key_0] = '0';
			arr[Input_key_1] = '1';
			arr[Input_key_2] = '2';
			arr[Input_key_3] = '3';
			arr[Input_key_4] = '4';
			arr[Input_key_5] = '5';
			arr[Input_key_6] = '6';
			arr[Input_key_7] = '7';
			arr[Input_key_8] = '8';
			arr[Input_key_9] = '9';

			// Letters (lowercase for text input)
			arr[Input_key_A] = 'a';
			arr[Input_key_B] = 'b';
			arr[Input_key_C] = 'c';
			arr[Input_key_D] = 'd';
			arr[Input_key_E] = 'e';
			arr[Input_key_F] = 'f';
			arr[Input_key_G] = 'g';
			arr[Input_key_H] = 'h';
			arr[Input_key_I] = 'i';
			arr[Input_key_J] = 'j';
			arr[Input_key_K] = 'k';
			arr[Input_key_L] = 'l';
			arr[Input_key_M] = 'm';
			arr[Input_key_N] = 'n';
			arr[Input_key_O] = 'o';
			arr[Input_key_P] = 'p';
			arr[Input_key_Q] = 'q';
			arr[Input_key_R] = 'r';
			arr[Input_key_S] = 's';
			arr[Input_key_T] = 't';
			arr[Input_key_U] = 'u';
			arr[Input_key_V] = 'v';
			arr[Input_key_W] = 'w';
			arr[Input_key_X] = 'x';
			arr[Input_key_Y] = 'y';
			arr[Input_key_Z] = 'z';

			// Space
			arr[Input_key_Space] = ' ';

			// Punctuation
			arr[Input_key_Apostrophe] = '\'';
			arr[Input_key_Comma] = ',';
			arr[Input_key_Minus] = '-';
			arr[Input_key_Period] = '.';
			arr[Input_key_Slash] = '/';
			arr[Input_key_Semicolon] = ';';
			arr[Input_key_Equal] = '=';
			arr[Input_key_LeftBracket] = '[';
			arr[Input_key_Backslash] = '\\';
			arr[Input_key_RightBracket] = ']';
			arr[Input_key_GraveAccent] = '`';

			return arr;
			}();

		Chilli::PepperTransform panelTransform;

		panelTransform.PercentageDimensions = { 0.0f, 0.0f };
		panelTransform.ActualDimensions = { 0.0f, 0.0f };

		// Position: Top-right corner
		panelTransform.PercentagePosition = { 0.55f, 0.5f };

		panelTransform.AnchorX = Chilli::AnchorX::CENTER;  // Align right
		panelTransform.AnchorY = Chilli::AnchorY::TOP;    // Align top
		panelTransform.ZOrder = 1000.0f;

		Chilli::PepperElement PepperElement;
		PepperElement.Flags = PEPPER_ELEMENT_VISIBLE;
		strncpy(PepperElement.Name, "CheckBox", 32);
		PepperElement.Type = Chilli::PepperElementTypes::PANEL;

		PepperResource->TextBoxCursorEntity = Command.CreateEntity();
		Command.AddComponent<PepperTransform>(PepperResource->TextBoxCursorEntity, panelTransform);
		Command.AddComponent(PepperResource->TextBoxCursorEntity, PepperElement);

		uint32_t* SliderCirclePixels = new uint32_t[32 * 32];
		Command.GenerateCircleImageData(SliderCirclePixels, 32, 32, 16, 16, 12, 0xFF00FFFF, 0x00000000);

		ImageSpec SliderCircleImageSpec;
		SliderCircleImageSpec.Resolution.Width = 32;
		SliderCircleImageSpec.Resolution.Height = 32;
		SliderCircleImageSpec.Resolution.Depth =1;
		SliderCircleImageSpec.Format = ImageFormat::RGBA8;
		SliderCircleImageSpec.MipLevel = 1;
		SliderCircleImageSpec.Sample = IMAGE_SAMPLE_COUNT_1_BIT;
		SliderCircleImageSpec.State = ResourceState::ShaderRead;
		SliderCircleImageSpec.Type = ImageType::IMAGE_TYPE_2D;
		SliderCircleImageSpec.Usage = IMAGE_USAGE_SAMPLED_IMAGE;
		PepperResource->SliderCircleImage = Command.AllocateImage(SliderCircleImageSpec);
		Command.MapImageData(PepperResource->SliderCircleImage, SliderCirclePixels, 32, 32);

		TextureSpec SliderCircleTextureSpec;
		SliderCircleTextureSpec.Format = ImageFormat::RGBA8;
		PepperResource->SliderCircleTexture = Command.CreateTexture(PepperResource->SliderCircleImage, SliderCircleTextureSpec);
	
		delete SliderCirclePixels;
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

	void OnPepperHandleLayout(BackBone::SystemContext& Ctxt)
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
		for (auto [Entity, TransformComp] : BackBone::QueryWithEntities<PepperTransform>(*Ctxt.Registry))
		{
			if (WindowSizeResized) {
				UpdateTransform(*TransformComp, PepperResource->WindowSize);
			}
			PepperResource->EntityZOrderSorted.push_back(SortedPepperEntity(Entity, TransformComp->ZOrder));
		}

		// 2. Sort: Highest Z-Order (closest to screen) comes first
		std::sort(PepperResource->EntityZOrderSorted.begin(), PepperResource->EntityZOrderSorted.end(), [](
			const SortedPepperEntity& a, const SortedPepperEntity& b) {
				return a.ZOrder > b.ZOrder;
			});
	}

	void OnPepperHandleInteraction(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

		auto PepperResource = Command.GetResource<Chilli::PepperResource>();
		auto Pepper = Chilli::Pepper(Ctxt);
		auto InputManager = Command.GetService<Chilli::Input>();
		auto EventHandler = Command.GetService<Chilli::EventHandler>();
		bool LMouseButtonDown = InputManager->IsMouseButtonDown(Input_mouse_Left);
		bool RMouseButtonDown = InputManager->IsMouseButtonDown(Input_mouse_Right);
		auto CursorDelta = InputManager->GetCursorDelta();

		bool MouseWasCaptured = false;
		bool GlobalDragging = (PepperResource->ActiveEntity != BackBone::npos);

		if (!LMouseButtonDown)
		{
			PepperResource->ActiveEntity = BackBone::npos;
		}

		for (auto& Entity : PepperResource->EntityZOrderSorted)
		{
			auto [TransformComp, State] = Command.GetComponents<PepperTransform, InteractionState>(Entity.Entity);

			if (TransformComp == nullptr)
				continue;
			if (State == nullptr)
				continue;

			bool IsMouseOver = IsCursorInside(*TransformComp, PepperResource->CursorPos, PepperResource->WindowSize);
			// An entity is ONLY hovered if the mouse is inside AND no one above it took the mouse
			State->IsHovered = IsMouseOver && !MouseWasCaptured;

			if (State->IsHovered) {
				MouseWasCaptured = true; // Block everyone behind this entity!

				if (LMouseButtonDown && State->MousePressed == Input_mouse_Unknown) {
					State->MousePressed = Input_mouse_Left;
					PepperResource->ActiveEntity = Entity.Entity;
				}

				if (!LMouseButtonDown) {
					// If it WAS pressed last frame but the button is NOW up... 
					// That is a "Click"!
					if (State->MousePressed == Input_mouse_Left) {
						// TRIGGER YOUR EVENT SYSTEM HERE
						EventHandler->Add< PepperClickEvent>(PepperClickEvent(Entity.Entity));
					}

					State->MousePressed = Input_mouse_Unknown;
					PepperResource->ActiveEntity = BackBone::npos;
				}
			}
		}

		if (PepperResource->ActiveEntity != BackBone::npos && LMouseButtonDown)
		{
			auto [TransformComp, State] = Command.GetComponents<PepperTransform, InteractionState>(PepperResource->ActiveEntity);
			auto Slider = Command.GetComponent<SliderComponent>(PepperResource->ActiveEntity);

			if (Slider) {
				auto Platform = Slider->Platform;
				auto PlatformTransform = Command.GetComponent<PepperTransform>(Platform);

				if (PlatformTransform) {
					// Calculate the absolute visual edges of the platform
					float platformLeft = PlatformTransform->ActualPosition.x - static_cast<float>(PlatformTransform->Pivot.x);
					float platformRight = platformLeft + PlatformTransform->ActualDimensions.x;

					// Boundary for the Knob's Pivot point
					// To keep the knob FULLY inside, we clamp its "visual" edges
					float knobWidth = TransformComp->ActualDimensions.x;
					float knobPivotOffset = static_cast<float>(TransformComp->Pivot.x);

					// The knob's Pivot.x is only allowed to move between these two pixel points:
					float minAllowedPivotX = platformLeft + knobPivotOffset;
					float maxAllowedPivotX = platformRight - (knobWidth - knobPivotOffset);

					// 1. Update Actual Position (Pixels) using the delta
					TransformComp->ActualPosition.x = std::clamp(TransformComp->ActualPosition.x + CursorDelta.x, minAllowedPivotX, maxAllowedPivotX);

					// 2. Sync PercentagePosition so UpdateTransform() doesn't overwrite our work
					TransformComp->PercentagePosition.x = TransformComp->ActualPosition.x / static_cast<float>(Command.GetActiveWindow()->GetWidth());

					// 3. Calculate Normalized Value (0.0 to 1.0) for the Event
					float range = maxAllowedPivotX - minAllowedPivotX;
					float pctX = 0.0f;
					if (range > 0.0f) {
						pctX = std::clamp((TransformComp->ActualPosition.x - minAllowedPivotX) / range, 0.0f, 1.0f);
					}

					EventHandler->Add<PepperSliderEvent>(PepperSliderEvent(PepperResource->ActiveEntity, pctX));
				}
			}
		}
	}

	void OnPepperHandleEvents(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

		auto PepperResource = Command.GetResource<Chilli::PepperResource>();
		auto Pepper = Chilli::Pepper(Ctxt);
		auto InputManager = Command.GetService<Chilli::Input>();
		auto EventHandler = Command.GetService<Chilli::EventHandler>();
		auto PepperActionRegistry = Command.GetService<Chilli::PepperActionRegistry>();

		for (auto& ClickEvent : *EventHandler->GetEventStorage<PepperClickEvent>())
		{
			if (ClickEvent.IsConsumed() == false)
				ClickEvent.Consume();
			auto Entity = ClickEvent.GetClickedEntity();

			bool Handled = false;

			if (Command.GetComponent<ButtonComponent>(Entity) != nullptr && !Handled)
			{
				auto Button = Command.GetComponent<ButtonComponent>(Entity);
				PepperActionRegistry->ExecuteAction(Button->OnClick, Ctxt,
					ClickEvent, PepperEventTypes::ON_CLICK, Entity);
				Handled = true;

				// if Any Text box is focused set it to false
				for (auto [TextBox] : BackBone::Query<TextBoxComponent>(*Ctxt.Registry))
				{
					TextBox->IsFocused = false;
				}
			}

			if (Command.GetComponent<CheckBoxComponent>(Entity) != nullptr && !Handled)
			{
				auto CheckBox = Command.GetComponent<CheckBoxComponent>(Entity);
				CheckBox->State = !CheckBox->State;
				PepperActionRegistry->ExecuteAction(CheckBox->OnStateChange, Ctxt,
					ClickEvent, PepperEventTypes::ON_CLICK, Entity);

				// if Any Text box is focused set it to false
				for (auto [TextBox] : BackBone::Query<TextBoxComponent>(*Ctxt.Registry))
				{
					TextBox->IsFocused = false;
				}

				Handled = true;
			}

			if (Command.GetComponent<TextBoxComponent>(Entity) != nullptr && !Handled)
			{
				auto TextBox = Command.GetComponent<TextBoxComponent>(Entity);
				auto TextRenderer = TextBox->TextRenderContext;

				// if Any Text box is focused set it to false
				for (auto [OtherEntity, TextBox] : BackBone::QueryWithEntities<TextBoxComponent>(*Ctxt.Registry))
				{
					TextBox->IsFocused = false;
				}

				TextBox->IsFocused = true;
				Handled = true;
			}
		}

		for (auto& Event : *EventHandler->GetEventStorage<PepperSliderEvent>())
		{
			if (Event.IsConsumed() == false)
				Event.Consume();
			auto Entity = Event.GetEntity();
			auto Delta = Event.GetDelta();

			if (Command.GetComponent<SliderComponent>(Entity) != nullptr)
			{
				auto Slider = Command.GetComponent<SliderComponent>(Entity);
				auto NewValue = Slider->Max * Event.GetDelta();
				Slider->Delta = NewValue - Slider->Value;
				Slider->Value = Chilli::Clamp(Slider->Value + Slider->Delta, Slider->Min, Slider->Max);

				PepperActionRegistry->ExecuteAction(Slider->OnValueChanged, Ctxt,
					Event, PepperEventTypes::ON_SLIDE, Entity);
			}
		}

		for (auto& Event : *EventHandler->GetEventStorage<PepperClickTextBoxEvent>())
		{
			if (Event.IsConsumed() == false)
				Event.Consume();
			auto Entity = Event.GetClickedEntity();

			if (Command.GetComponent<TextBoxComponent>(Entity) != nullptr)
			{
				auto TextBox = Command.GetComponent<TextBoxComponent>(Entity);
				PepperActionRegistry->ExecuteAction(TextBox->OnSubmit, Ctxt,
					Event, PepperEventTypes::ON_CLICK_TEXTBOX, Entity);
			}
		}

		EventHandler->Clear< PepperClickEvent>();
		EventHandler->Clear< PepperSliderEvent>();
		EventHandler->Clear< PepperClickTextBoxEvent>();
	}

	uint32_t WordsFitInTextBox(char* Content, uint32_t ContentOffset, uint32_t ContentLen, const FlameFont& font, float fontScale,
		uint32_t TextBoxWidth) {
		auto& msdf = font.RawFontData.ValPtr;
		float totalWidth = 0.0f;
		uint32_t WordCount = 0;

		for (int i = ContentOffset; i < ContentLen; i++)
		{
			char c = Content[i];
			// Optimization: Use a map for O(1) lookup instead of vector search
			const MSDFGlyphInfo* g = font.RawFontData.ValPtr->GlyphMap.Get((uint32_t)c);

			if (g) {
				auto Width = (totalWidth + g->advance) * fontScale;
				if (TextBoxWidth - Width <= 0)
				{
					WordCount = i;
					return WordCount;
				}
				totalWidth += g->advance;
			}
		}
		auto Width = (totalWidth)*fontScale;
		if (TextBoxWidth - Width > 0)
			return ContentLen;
	}

	void OnPepperHandleKeyBoard(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto InputManager = Command.GetService<Chilli::Input>();
		auto EventHandler = Command.GetService<Chilli::EventHandler>();
		auto PepperResource = Command.GetResource<Chilli::PepperResource>();
		auto Pepper = Chilli::Pepper(Ctxt);

		auto CursorTransform = Command.GetComponent<PepperTransform>(PepperResource->TextBoxCursorEntity);
		auto CursorElement = Command.GetComponent<PepperElement>(PepperResource->TextBoxCursorEntity);

		bool AnyFocused = false;

		for (auto [Entity, TextBox, Transform] : BackBone::QueryWithEntities<TextBoxComponent, PepperTransform>(*Ctxt.Registry))
		{
			if (TextBox->IsFocused == false)
				continue;
			AnyFocused = true;
			auto FlameText = Command.GetComponent<FlameTextComponent>(TextBox->TextRenderContext);

			if (PepperResource->CursorTimer.ElapsedMsl() > 500)
			{
				PepperResource->CursorTimer.Reset();

				if (PepperResource->CursorBlinkState == false)
				{
					CursorTransform->PercentageDimensions.x = 0;
					CursorTransform->PercentageDimensions.y = 0;

					CursorTransform->ActualDimensions.x = 0.0f;
					CursorTransform->ActualDimensions.y = 0;
					PepperResource->CursorBlinkState = true;
				}
				else
				{
					CursorTransform->ActualDimensions.x = 2.0f;
					CursorTransform->ActualDimensions.y = Transform->ActualDimensions.y * 0.85f;

					CursorTransform->PercentageDimensions.x = CursorTransform->ActualDimensions.x / Command.GetActiveWindow()->GetWidth();
					CursorTransform->PercentageDimensions.y = CursorTransform->ActualDimensions.y / Command.GetActiveWindow()->GetHeight();
					PepperResource->CursorBlinkState = false;
				}
			}

			for (auto& Key : *EventHandler->GetEventStorage<KeyPressedEvent>())
			{
				if (Key.GetKeyCode() == Input_key_Backspace && TextBox->CursorIndex > 0) {
					// CTROL + Backspace
					if (Key.GetMods() & Input_mod_Control)
					{
						uint32_t oldIndex = TextBox->CursorIndex;
						uint32_t newIndex = oldIndex;

						// 1. Skip trailing spaces (if the cursor is right after a space)
						while (newIndex > 0 && TextBox->Content[newIndex - 1] == ' ')
						{
							newIndex--;
						}

						// 2. Find the start of the word (keep going until we hit a space or start)
						while (newIndex > 0 && TextBox->Content[newIndex - 1] != ' ')
						{
							newIndex--;
						}

						uint32_t charsToDelete = oldIndex - newIndex;

						// 3. Shift the remaining content to the left
						// Move everything from oldIndex to the end of the string into the spot at newIndex
						for (uint32_t i = newIndex; i < TextBox->Len - charsToDelete; ++i)
						{
							TextBox->Content[i] = TextBox->Content[i + charsToDelete];
						}

						// 4. Update metrics
						TextBox->CursorIndex = newIndex;
						TextBox->Len -= charsToDelete;
						TextBox->Content[TextBox->Len] = '\0';

						continue;
					}
					else
					{
						// Shift everything after the cursor one to the left
						for (int i = TextBox->CursorIndex - 1; i < TextBox->Len - 1; ++i) {
							TextBox->Content[i] = TextBox->Content[i + 1];
						}

						TextBox->CursorIndex--;
						TextBox->Len--;
						TextBox->Content[TextBox->Len] = '\0';
						continue;
					}
				}
				// --- CTRL + RIGHT ARROW (Move to end of word) ---
				if (Key.GetKeyCode() == Input_key_Right)
				{
					if (Key.GetMods() & Input_mod_Control)
					{
						// 1. Skip current spaces
						while (TextBox->CursorIndex < TextBox->Len && TextBox->Content[TextBox->CursorIndex] == ' ')
							TextBox->CursorIndex++;

						// 2. Move to end of the word
						while (TextBox->CursorIndex < TextBox->Len && TextBox->Content[TextBox->CursorIndex] != ' ')
							TextBox->CursorIndex++;
					}
					else if (TextBox->Len > TextBox->CursorIndex) // Standard Right
					{
						TextBox->CursorIndex++;
					}
					continue;
				}
				if (Key.GetKeyCode() == Input_key_Left)
				{
					if (Key.GetMods() & Input_mod_Control)
					{
						// 1. Skip preceding spaces
						while (TextBox->CursorIndex > 0 && TextBox->Content[TextBox->CursorIndex - 1] == ' ')
							TextBox->CursorIndex--;

						// 2. Move to start of the word
						while (TextBox->CursorIndex > 0 && TextBox->Content[TextBox->CursorIndex - 1] != ' ')
							TextBox->CursorIndex--;
					}
					else if (TextBox->CursorIndex > 0) // Standard Left
					{
						TextBox->CursorIndex--;
					}
					continue;
				}
				if (Key.GetKeyCode() == Input_key_Enter)
				{
					EventHandler->Add< PepperClickTextBoxEvent>(PepperClickTextBoxEvent(Entity));
					continue;
				}

				char c = PepperResource->InputKeysToPepperInputText[Key.GetKeyCode()];

				if (c != 0 && TextBox->Len < TextBoxComponent::MAX_CONTENT_LEN - 1) {
					// Shift everything after the cursor one to the right
					for (int i = TextBox->Len; i > TextBox->CursorIndex; --i) {
						TextBox->Content[i] = TextBox->Content[i - 1];
					}

					// Insert the new char and move cursor
					TextBox->Content[TextBox->CursorIndex++] = c;
					TextBox->Len++;
					TextBox->Content[TextBox->Len] = '\0';
				}
			}

			auto CursorWidthX = 0.0f;
			float fontScale = Transform->ActualDimensions.y;

			auto WordCount = WordsFitInTextBox(TextBox->Content, 0, TextBox->Len,
				*FlameText->Font.ValPtr, Transform->ActualDimensions.y, Transform->ActualDimensions.x);
			// Calculate width UP TO the cursor index, not the word count
			for (int i = 0; i < TextBox->CursorIndex; i++)
			{
				char c = TextBox->Content[i];
				const MSDFGlyphInfo* g = FlameText->Font.ValPtr->RawFontData.ValPtr->GlyphMap.Get((uint32_t)c);
				if (g)
				{
					// Multiply by fontScale here or at the end for the pixel offset
					CursorWidthX += g->advance;
				}
			}

			auto MinX = Transform->ActualPosition.x - Transform->Pivot.x;
			// Apply the scale to the accumulated advances
			CursorTransform->ActualPosition.x = MinX + (CursorWidthX * fontScale);
			CursorTransform->ActualPosition.y = Transform->ActualPosition.y;

			// Update NDC/Percentage
			CursorTransform->PercentagePosition.x = CursorTransform->ActualPosition.x / Command.GetActiveWindow()->GetWidth();
			CursorTransform->PercentagePosition.y = CursorTransform->ActualPosition.y / Command.GetActiveWindow()->GetHeight();
			FlameText->Content.resize(WordCount, 0);
			memcpy(FlameText->Content.data(), (uint8_t*)TextBox->Content, WordCount);
		}

		if (!AnyFocused)
		{
			CursorTransform->PercentageDimensions.x = 0;
			CursorTransform->PercentageDimensions.y = 0;

			CursorTransform->ActualDimensions.x = 0.0f;
			CursorTransform->ActualDimensions.y = 0;
			PepperResource->CursorBlinkState = true;
		}
	}

	void OnPepperHandleRendering(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Command.GetService<Renderer>();
		auto MaterialSystem = Command.GetService<Chilli::MaterialSystem>();

		auto PepperResource = Command.GetResource<Chilli::PepperResource>();
		auto Pepper = Chilli::Pepper(Ctxt);

		for (auto [Entity, TransformComp, ElementComp] : BackBone::QueryWithEntities<PepperTransform, PepperElement>(*Ctxt.Registry))
		{
			if (ElementComp->Flags & PEPPER_ELEMENT_VISIBLE)
			{
				PepperResource->PepperMaterialData.push_back(Pepper.GetMaterialShaderData(Entity));
				GeneratePepperQuad(*TransformComp, PepperResource->WindowSize, PepperResource->QuadVertices,
					PepperResource->QuadIndicies, PepperResource->PepperMaterialData.size() - 1);
				PepperResource->QuadCount++;
			}
		}

		if (PepperResource->QuadCount == 0)
		{
			return;
		}

		RenderService->UpdateMaterialBufferData(MaterialSystem->GetRawMaterialHandle(PepperResource->ContextMaterial),
			PepperResource->PepperMaterialSSBO[RenderService->GetCurrentFrameIndex()].ValPtr->RawBufferHandle,
			"PepperMaterialSSBO",
			PepperResource->PepperMaterialSSBO[RenderService->GetCurrentFrameIndex()].ValPtr->CreateInfo.SizeInBytes,
			0);

		Command.MapMeshVertexBufferData(PepperResource->RenderMesh, 0,
			PepperResource->QuadVertices.data(),
			PepperResource->QuadVertices.size(), sizeof(PepperVertex) * PepperResource->QuadVertices.size(),
			0);
		Command.MapMeshIndexBufferData(PepperResource->RenderMesh, PepperResource->QuadIndicies.data(),
			PepperResource->QuadIndicies.size(), sizeof(uint32_t) * PepperResource->QuadIndicies.size(),
			0);
		Command.MapBufferData(PepperResource->PepperMaterialSSBO[RenderService->GetCurrentFrameIndex()],
			PepperResource->PepperMaterialData.data(),
			sizeof(PepperShaderMaterialData) * PepperResource->PepperMaterialData.size(), 0);
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

		// 1. Calculate Deltas
		IVec2 CursorDelta;
		CursorDelta.x = PepperResource->CursorPos.x - PepperResource->OldCursorPos.x;
		CursorDelta.y = PepperResource->CursorPos.y - PepperResource->OldCursorPos.y;
		Vec2 WindowSize = { (float)PepperResource->WindowSize.x, (float)PepperResource->WindowSize.y };
		bool LMouseButtonDown = InputManager->IsMouseButtonDown(Input_mouse_Left);

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
			if (activeResizeEntity == BackBone::npos && Edge != ResizeEdge::None && LMouseButtonDown &&
				(ElementComp->Flags & PEPPER_ELEMENT_RESIZEABLE))
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
			if (IsMouseOver && (ElementComp->Flags & PEPPER_ELEMENT_MOVEABLE) &&
				LMouseButtonDown && activeResizeEntity == BackBone::npos)
			{
				BackBone::Entity rootToMove = (ElementComp->Parent != BackBone::npos) ? ElementComp->Parent : Entity;
				ApplyMovementRecursive(rootToMove, &changePT, CursorDelta, PepperResource, *Ctxt.Registry);
			}

			*TransformComp = changePT;
		}

		wasLMouseDown = LMouseButtonDown;
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

	std::vector<PipelineBarrier> GetPepperPrePassPipelineBarrier(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto PepperResource = Command.GetResource<Chilli::PepperResource>();
		auto RenderService = Command.GetService<Renderer>();

		PipelineBarrier VertexBarrier = FillBufferPipelineBarrier(
			PepperResource->RenderMesh.ValPtr->VertexBufferHandles[0].ValPtr->RawBufferHandle,
			ResourceState::HostWrite, ResourceState::VertexRead, 0, CH_BUFFER_WHOLE_SIZE);

		PipelineBarrier IndexBarrier = FillBufferPipelineBarrier(
			PepperResource->RenderMesh.ValPtr->IBHandle.ValPtr->RawBufferHandle,
			ResourceState::HostWrite, ResourceState::IndexRead, 0, CH_BUFFER_WHOLE_SIZE);

		PipelineBarrier PrepareMaterialBarrier = FillBufferPipelineBarrier(PepperResource->PepperMaterialSSBO[RenderService->GetCurrentFrameIndex()].ValPtr->RawBufferHandle,
			ResourceState::HostWrite, ResourceState::ShaderRead, 0, CH_BUFFER_WHOLE_SIZE);

		return { VertexBarrier, IndexBarrier, PrepareMaterialBarrier };
	}

	std::vector<PipelineBarrier> GetPepperPostPassPipelineBarrier(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto PepperResource = Command.GetResource<Chilli::PepperResource>();
		auto RenderService = Command.GetService<Renderer>();

		PipelineBarrier VertexBarrier = FillBufferPipelineBarrier(
			PepperResource->RenderMesh.ValPtr->VertexBufferHandles[0].ValPtr->RawBufferHandle,
			ResourceState::VertexRead, ResourceState::HostWrite, 0, CH_BUFFER_WHOLE_SIZE);

		PipelineBarrier IndexBarrier = FillBufferPipelineBarrier(
			PepperResource->RenderMesh.ValPtr->IBHandle.ValPtr->RawBufferHandle,
			ResourceState::IndexRead, ResourceState::HostWrite, 0, CH_BUFFER_WHOLE_SIZE);

		PipelineBarrier PrepareMaterialBarrier = FillBufferPipelineBarrier(PepperResource->PepperMaterialSSBO[RenderService->GetCurrentFrameIndex()].ValPtr->RawBufferHandle,
			ResourceState::ShaderRead, ResourceState::HostWrite, 0, CH_BUFFER_WHOLE_SIZE);

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
		App.Registry.Register<Chilli::ButtonComponent>();
		App.Registry.Register<Chilli::SliderComponent>();
		App.Registry.Register<Chilli::CheckBoxComponent>();
		App.Registry.Register<Chilli::InteractionState>();

		auto EventHandler = App.ServiceRegistry.GetService<Chilli::EventHandler>();
		EventHandler->Register< PepperClickEvent>();
		EventHandler->Register< PepperSliderEvent>();
		EventHandler->Register< PepperClickTextBoxEvent>();

		App.ServiceRegistry.RegisterService< PepperActionRegistry>(std::make_shared< PepperActionRegistry>());

		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::START_UP, OnPepperStartUp);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnPepperHandleLayout);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnPepperHandleInteraction);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnPepperHandleEvents);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnPepperHandleKeyBoard);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnPepperUpdate);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnPepperHandleRendering);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::SHUTDOWN, OnPepperShutDown);

		FlameExtensionConfig FlameConfig;
		FlameConfig.MaxFrameInFlight = _Config.MaxFramesInFlight;

		App.Extensions.AddExtension(std::make_unique<FlameExtension>(FlameConfig), true, &App);
	}

#pragma endregion

}
