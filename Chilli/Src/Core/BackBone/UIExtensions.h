#pragma once

#include "BackBone.h"
#include "Profiling\Timer.h"

namespace Chilli
{
#pragma region Flame

	struct MSDFGlyphInfo
	{
		int unicode;
		float advance;
		// Plane Bounds: Where the quad sits relative to the cursor (in Font Units)
		float pl, pb, pr, pt;
		// Atlas Bounds: Where the pixels are in the PNG (in Pixels)
		float al, ab, ar, at;
	};

	struct MSDFData
	{
		float PixelRange;
		float AtlasWidth, AtlasHeight;
		SparseSet<MSDFGlyphInfo> GlyphMap;
		std::string FilePath;

		ImageSpec GetImageCreateInfo(uint32_t Width, uint32_t Height)
		{
			ImageSpec Spec;
			Spec.Resolution.Width = Width;
			Spec.Resolution.Height = Height;
			Spec.Resolution.Depth = 1;
			Spec.Format = ImageFormat::RGBA8;
			Spec.Type = ImageType::IMAGE_TYPE_2D;
			Spec.Usage = IMAGE_USAGE_SAMPLED_IMAGE;
			Spec.State = ResourceState::ShaderRead;
			return Spec;
		}
	};

#define CH_FLAME_MAX_UNIQUE_FONT_COUNT 32

	struct FlameMaterial
	{
		Vec4 Color{ 1.0f, 1.0f, 1.0f ,1.0f };
	};

	struct FlameFont
	{
		BackBone::AssetHandle< MSDFData> RawFontData;
		BackBone::AssetHandle<Texture> AtlasTexture;
		BackBone::AssetHandle<Image> AtlasImage;
	};

	struct FlameVertex
	{
		Vec2 Pos;
		Vec2 TexCoords;
		uint32_t FontIndex;
		uint32_t MaterialIndex;
	};

	struct FlameTextComponent {
		std::string Content;
		BackBone::AssetHandle<FlameFont> Font;
		Vec4 Color = { 1, 1, 1, 1 };
		bool IsDirty = true; // Flag to tell the system to rebuild the instances
		bool IsRender = true;
	};

	struct FlameResource
	{
		BackBone::AssetHandle<ShaderProgram> ShaderProgram;
		BackBone::AssetHandle<Material> Material;

		uint32_t GeoCharacterCount = 0;
		uint32_t UICharacterCount = 0;

		BackBone::AssetHandle<Buffer> VertexBuffer;
		std::vector<BackBone::AssetHandle<Buffer>> MaterialSSBOs;

		std::vector<FlameVertex> Vertices;
		std::vector<uint32_t> Indicies;
		uint32_t MaterialCount = 0;

		struct ShaderArrayFontAtlasMetaData
		{
			BackBone::AssetHandle<Texture> Tex;
			uint32_t ShaderIndex;
		};

		SparseSet<ShaderArrayFontAtlasMetaData> FontAtlasTextureMap;

		uint32_t _FontAtlasTextureMapCount = 0;

		void UpdateFontAtlasTextureMap(Renderer* RenderService, MaterialSystem* MaterialSystem,
			BackBone::AssetHandle<Texture> Tex);

		uint32_t GetFontAtlasTextureShaderIndex(BackBone::AssetHandle<Texture> Tex)
		{
			return FontAtlasTextureMap.Get(Tex.Handle)->ShaderIndex;
		}
	};

	// 1. The Interface (The Contract)
	class Msdfgen_Loader : public BaseLoader<MSDFData> {
	public:
		Msdfgen_Loader() {}
		~Msdfgen_Loader();

		// Every loader must implement these
		virtual BackBone::AssetHandle<MSDFData> LoadTyped(BackBone::SystemContext& Ctxt, const std::string& Path) override;
		virtual void Unload(BackBone::SystemContext& Ctxt, const std::string& Path) override;
		virtual void Unload(BackBone::SystemContext& Ctxt, const BackBone::AssetHandle<MSDFData>& Data) override;
		virtual std::vector<std::string> GetExtensions() override { return { ".msdf" }; }
		virtual bool DoesExist(const std::string& Path) const override;
		virtual int __FindIndex(const std::string& Path) const override { return 0; }

	private:
		std::vector<IndexEntry> _IndexMaps;
		std::vector<BackBone::AssetHandle<MSDFData>> _Msdf_Handles;   // Maps Hash index to _ImageDatas index
	};

	void OnFlameRenderUI(BackBone::SystemContext& Ctxt);
	std::vector<PipelineBarrier> GetFlamePrePassPipelineBarrier(BackBone::SystemContext& Ctxt);
	std::vector<PipelineBarrier> GetFlamePostPassPipelineBarrier(BackBone::SystemContext& Ctxt);

	struct FlameExtensionConfig
	{
		uint32_t MaxFrameInFlight = 0;
	};

	// ---- Basic Extensions ----
	class FlameExtension : public BackBone::Extension
	{
	public:
		FlameExtension(FlameExtensionConfig Config) : _Config(Config) {}
		~FlameExtension() {}

		virtual void Build(BackBone::App& App) override;

		virtual const char* Name() const override { return "FlameExtension"; }
	private:
		FlameExtensionConfig _Config;
	};

#pragma endregion

	// The Base UI Library
#pragma region Pepper

	enum class AnchorX
	{
		LEFT,
		RIGHT,
		CENTER,
		STRETCH
	};

	enum class AnchorY
	{
		TOP,
		BOTTOM,
		CENTER,
		STRETCH
	};

	enum class ResizeEdge { None, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight };

	inline const char* ResizeEdgeToString(ResizeEdge edge) {
		switch (edge) {
		case ResizeEdge::None:        return "None";
		case ResizeEdge::Left:        return "Left";
		case ResizeEdge::Right:       return "Right";
		case ResizeEdge::Top:         return "Top";
		case ResizeEdge::Bottom:      return "Bottom";
		case ResizeEdge::TopLeft:     return "TopLeft";
		case ResizeEdge::TopRight:    return "TopRight";
		case ResizeEdge::BottomLeft:  return "BottomLeft";
		case ResizeEdge::BottomRight: return "BottomRight";
		default:                      return "Unknown";
		}
	}

	struct ResizeFactors {
		float moveX, moveY; // Position shift weight
		float sizeX, sizeY; // Dimension scale weight
	};

	// Index this using your ResizeEdge enum
	static const ResizeFactors Factors[] = {
		{0, 0,  0, 0}, // None
		{1, 0, -1, 0}, // Left:   Shift X, Inverse Scale Width
		{0, 0,  1, 0}, // Right:  No Shift, Direct Scale Width
		{0, 0,  0, 1}, // Top:    No Shift, Direct Scale Height
		{0, 1,  0,-1}, // Bottom: Shift Y, Inverse Scale Height
		{1, 0, -1, 1}, // TopLeft
		{0, 0,  1, 1}, // TopRight
		{1, 1, -1,-1}, // BottomLeft
		{0, 1,  1,-1}  // BottomRight
	};

#define CHILLI_MAX_PEPPER_ACTION_KEY_LENGTH 32

	struct PepperActionKey
	{
		char Data[CHILLI_MAX_PEPPER_ACTION_KEY_LENGTH];

		PepperActionKey() {
			std::memset(Data, 0, CHILLI_MAX_PEPPER_ACTION_KEY_LENGTH);
		}

		PepperActionKey(const char* name) {
			std::memset(Data, 0, CHILLI_MAX_PEPPER_ACTION_KEY_LENGTH);
			if (name) {
				std::strncpy(Data, name, CHILLI_MAX_PEPPER_ACTION_KEY_LENGTH - 1);
			}
		}

		bool operator==(const PepperActionKey& other) const {
			return std::memcmp(Data, other.Data, CHILLI_MAX_PEPPER_ACTION_KEY_LENGTH) == 0;
		}

		PepperActionKey& operator=(const std::string& other) {
			std::memset(Data, 0, CHILLI_MAX_PEPPER_ACTION_KEY_LENGTH);
			size_t len = std::min(other.size(), (size_t)CHILLI_MAX_PEPPER_ACTION_KEY_LENGTH - 1);
			std::memcpy(Data, other.data(), len);
			return *this;
		}
	};

	struct PepperActionKeyHasher {
		size_t operator()(const PepperActionKey& k) const {
			// Fast 64-bit hashing (process 8 bytes at a time)
			const uint64_t* p = reinterpret_cast<const uint64_t*>(k.Data);
			size_t h = 14695981039346656037ULL; // FNV offset basis for 64-bit

			// Unrolled loop for 32 bytes (4 iterations)
			h = (h ^ p[0]) * 1099511628211ULL;
			h = (h ^ p[1]) * 1099511628211ULL;
			h = (h ^ p[2]) * 1099511628211ULL;
			h = (h ^ p[3]) * 1099511628211ULL;

			return h;
		}
	};

	struct PepperClickEvent : public Chilli::Event
	{
	public:
		PepperClickEvent(BackBone::Entity ClickedEntity)
			:_ClickedEntity(ClickedEntity)
		{
		}

		const char* GetName() const override { return "PepperClickEvent"; }

		BackBone::Entity GetClickedEntity() const { return _ClickedEntity; }

	private:
		BackBone::Entity _ClickedEntity;
	};

	struct PepperClickTextBoxEvent : public Chilli::Event
	{
	public:
		PepperClickTextBoxEvent(BackBone::Entity ClickedEntity)
			:_ClickedEntity(ClickedEntity)
		{
		}

		const char* GetName() const override { return "PepperClickTextBoxEvent"; }

		BackBone::Entity GetClickedEntity() const { return _ClickedEntity; }

	private:
		BackBone::Entity _ClickedEntity;
	};

	struct PepperSliderEvent : public Chilli::Event
	{
	public:
		PepperSliderEvent(BackBone::Entity Entity, float Delta)
			:_Entity(Entity), _Delta(Delta)
		{
		}

		const char* GetName() const override { return "PepperSliderEvent "; }

		BackBone::Entity GetEntity() const { return _Entity; }
		float GetDelta() const { return _Delta; }

	private:
		BackBone::Entity _Entity;
		float _Delta = 0;
	};

	enum class PepperEventTypes
	{
		ON_CLICK,
		ON_CLICK_TEXTBOX,
		ON_SLIDE,
	};

	struct InteractionState {
		bool IsHovered = false;
		Input_mouse MousePressed = Input_mouse_Unknown;
		bool WasPressedLastFrame = false; // Important for "OnRelease" logic
	};

	class PepperInteractionSystem
	{

	};

	using PepperActionCallBackFn = std::function<void(BackBone::SystemContext&, Event&, PepperEventTypes,
		BackBone::Entity)>;

	static const auto DeafultPepperActionCallBackFn = [](BackBone::SystemContext&, Event&, PepperEventTypes,
		BackBone::Entity) {};
	static const std::string DeafultPepperActionKey = "DefaultPepperActionKey";

	struct PepperAction
	{
		uint64_t ActionKeyHash = 0;
		PepperActionCallBackFn CallBack;
	};

	struct ButtonComponent
	{
		PepperActionKey OnClick;
	};

	struct SliderComponent
	{
		float Delta = 0.0f;
		float Value = 0.0f;
		float Min = 0.0f;
		float Max = 100.0f;
		// Action Called on Slide
		PepperActionKey OnValueChanged;
		// Sliders Parent Platform
		BackBone::Entity Platform;
	};

	struct CheckBoxComponent
	{
		bool State = false;
		PepperActionKey OnStateChange;
	};

	struct TextBoxComponent
	{
		static const int MAX_CONTENT_LEN = 256;
		char Content[MAX_CONTENT_LEN] = { 0 };
		uint32_t Len = 0;
		uint32_t CursorIndex = 0;
		uint32_t ScrollLeftIndex = 0;
		uint32_t ScrollRightIndex = 0;
		bool IsFocused = false;

		PepperActionKey OnSubmit = PepperActionKey(DeafultPepperActionKey.data());
		// Stores the ember text to render our text box contents
		BackBone::Entity TextRenderContext;
		//PepperActionKey OnLetterChanged = ;
	};
	// Components
	struct PepperTransform
	{
		Vec2 PercentageDimensions{ 0,0 };
		Vec2 PercentagePosition{ 0,0 };
		Vec2 ActualDimensions{ 0,0 };
		Vec2 ActualPosition{ 0,0 };

		Chilli::AnchorX AnchorX;
		Chilli::AnchorY AnchorY;
		IVec2 Anchor{ 0,0 };

		IVec2 Pivot{ 0,0 };
		int ZOrder = 0;
	};

	enum class PepperElementTypes
	{
		PANEL,
		BUTTON,
		HEADER
	};

	enum PepperElementFlags
	{
		PEPPER_ELEMENT_RESIZEABLE = (1 << 0),
		PEPPER_ELEMENT_MOVEABLE = (1 << 1),
		PEPPER_ELEMENT_VISIBLE = (1 << 2),
	};

	class Pepper;
	struct PepperElement;
	struct PepperResource;

	struct PepperMaterial
	{
	private:
		BackBone::AssetHandle <Texture> TextureHandle;
		BackBone::AssetHandle <Sampler> SamplerHandle;
		Vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };

		uint32_t Version = 0;
		uint32_t LastUploadedVersion = 0;

		friend class Pepper;
	};

	struct PepperShaderMaterialData
	{
		// x for AlbedoTexture, y for AlbedoSampler
		int TextureIndex;
		int SamplerIndex;
		int Padding[2];
		Vec4 Color;
	};

	struct PepperElement
	{
		PepperElementTypes Type = PepperElementTypes::PANEL;
		uint64_t Flags = PEPPER_ELEMENT_VISIBLE | PEPPER_ELEMENT_MOVEABLE | PEPPER_ELEMENT_RESIZEABLE;
		char Name[32] = { 0 };
		BackBone::Entity Parent = BackBone::npos;
		PepperMaterial Material;
	};

	class PepperActionRegistry
	{
	public:
		PepperActionRegistry()
		{

		}

		PepperActionKey RegisterAction(PepperActionKey Key,
			const PepperActionCallBackFn& CallBack)
		{
			PepperAction NewAction;
			PepperActionKeyHasher Hasher;
			NewAction.ActionKeyHash = Hasher(Key);
			NewAction.CallBack = CallBack;
			_ActionMap[Key] = NewAction;
			return Key;
		}

		bool DoesActionExist(PepperActionKey Key)
		{
			return _ActionMap.find(Key) != _ActionMap.end();
		}

		void ExecuteAction(PepperActionKey Key, BackBone::SystemContext& Ctxt, Event& e, PepperEventTypes Type,
			BackBone::Entity Entity)
		{
			if (DoesActionExist(Key))
			{
				auto it = _ActionMap.find(Key);
				if (it != _ActionMap.end()) {
					(it->second.CallBack)(Ctxt, e, Type, Entity);
				}
				return;
			}
		}

	private:
		std::unordered_map<PepperActionKey, PepperAction, PepperActionKeyHasher> _ActionMap;
	};

	class Pepper
	{
	public:
		Pepper(BackBone::SystemContext& Ctxt) :_Ctxt(Ctxt) {}

		void SetMaterialColor(BackBone::Entity Entity, const Vec4& Color)
		{
			auto Mat = GetMaterial(Entity);
			Mat->Color = Color;
			Mat->Version++;
		}

		void SetMaterialTexture(BackBone::Entity Entity, BackBone::AssetHandle <Texture> AlbedoTextureHandle)
		{
			auto Mat = GetMaterial(Entity);
			Mat->TextureHandle = AlbedoTextureHandle;
			Mat->Version++;
		}

		void SetMaterialSampler(BackBone::Entity Entity, BackBone::AssetHandle <Sampler> SamplerHandle)
		{
			auto Mat = GetMaterial(Entity);
			Mat->SamplerHandle = SamplerHandle;
			Mat->Version++;
		}

		void SetElementPercentagePosition(BackBone::Entity Entity, const Vec2& Position)
		{

		}

		void RemoveFlag(BackBone::Entity Entity, PepperElementFlags Flag)
		{
			auto Element = GetElement(Entity);
			if (Element->Flags & Flag)
				Element->Flags -= Flag;
		}

		void AddFlag(BackBone::Entity Entity, PepperElementFlags Flag)
		{
			auto Element = GetElement(Entity);
			if (!(Element->Flags & Flag))
				Element->Flags += Flag;
		}

		PepperActionKey RegisterAction(PepperActionKey Key,
			const PepperActionCallBackFn& CallBack);

		bool DoesActionExist(PepperActionKey Key);

		void ExecuteAction(PepperActionKey Key, BackBone::SystemContext& Ctxt, Event& e, PepperEventTypes Type,
			BackBone::Entity Entity);

		BackBone::AssetHandle<Texture> GetDeafultPepperTexture();

		bool ShouldMaterialShaderDataUpdate(BackBone::Entity Entity);
		PepperShaderMaterialData GetMaterialShaderData(BackBone::Entity Entity);

		BackBone::Entity CreateSlider(Chilli::BackBone::SystemContext& Ctxt, PepperActionKey Key,
			const PepperActionCallBackFn& CallBack, BackBone::Entity ParentPanel);

	private:
		PepperMaterial* GetMaterial(BackBone::Entity Entity);
		PepperElement* GetElement(BackBone::Entity Entity);
		PepperTransform* GetTransform(BackBone::Entity Entity);
		PepperResource* GetPepperResource();

	private:
		BackBone::SystemContext _Ctxt;
	};

	struct PepperVertex
	{
		Vec3 Position;
		Vec2 TexCoords;
		uint32_t PepperMaterialIndex = 0;
	};

	// Systems
	void OnPepperStartUp(BackBone::SystemContext& Ctxt);
	void OnPepperUpdate(BackBone::SystemContext& Ctxt);
	void OnPepperRender(BackBone::SystemContext& Ctxt, RenderPassDesc& Pass);
	std::vector<PipelineBarrier> GetPepperPrePassPipelineBarrier(BackBone::SystemContext& Ctxt);
	std::vector<PipelineBarrier> GetPepperPostPassPipelineBarrier(BackBone::SystemContext& Ctxt);

	struct PepperExtensionConfig
	{
		bool Enable = true;
		uint32_t MaxFramesInFlight = 0;
		FlameExtensionConfig EmberConfig;
	};

	struct SortedPepperEntity
	{
		BackBone::Entity Entity;
		int ZOrder = 0;
	};

	struct PepperResource
	{
		static const uint32_t MeshQuadCount = 10000;

		BackBone::Entity TextBoxCursorEntity;

		BackBone::AssetHandle<Image> SliderCircleImage;
		BackBone::AssetHandle<Texture> SliderCircleTexture;
		BackBone::AssetHandle<ImageData> ResizeCursorImageData;
		BackBone::AssetHandle<Cursor> ResizeCursor;
		BackBone::AssetHandle<Mesh> RenderMesh;
		BackBone::AssetHandle<Texture> PepperDeafultTexture;
		BackBone::AssetHandle<Sampler> PepperDeafultSampler;
		BackBone::AssetHandle<Image> PepperDeafultImage;
		BackBone::AssetHandle<ShaderProgram> PepperShaderProgram;
		BackBone::AssetHandle<Material> ContextMaterial;
		std::vector< BackBone::AssetHandle<Buffer>> PepperMaterialSSBO;
		std::vector< PepperShaderMaterialData> PepperMaterialData;
		std::vector<uint32_t> EntityToPCMap;

		bool CursorBlinkState = false;
		long long CursorBlinkTime = 0;

		std::vector<SortedPepperEntity> EntityZOrderSorted;
		SparseSet<std::vector<BackBone::Entity>> ParentChildMap;
		std::array<BackBone::AssetHandle<Cursor>, int(DeafultCursorTypes::Count) > DeafultCursors;

		IVec2 OldWindowSize{ 0,0 };
		IVec2 WindowSize{ 0,0 };
		IVec2 OldCursorPos{ 0,0 };
		IVec2 CursorPos{ 0,0 };

		uint32_t QuadCount = 0;
		std::vector<PepperVertex> QuadVertices;
		std::vector<uint32_t> QuadIndicies;
		Timer CursorTimer;

		struct {
			glm::mat4 OthroMat = glm::mat4{ 1.0f };
		} CameraManager;

		// Which Canvas the mouse is on
		BackBone::Entity ActiveMouseElement;
		BackBone::Entity ActiveEntity;

		// Add these to PepperResource struct
		BackBone::Entity ResizingEntity = BackBone::npos;
		ResizeEdge ActiveEdge = ResizeEdge::None;
		std::array<char, Input_key_Count> InputKeysToPepperInputText;
	};

	class PepperExtension : public BackBone::Extension
	{
	public:
		PepperExtension(const PepperExtensionConfig& Config = PepperExtensionConfig())
			:_Config(Config)
		{
		}
		~PepperExtension() {}

		virtual void Build(BackBone::App& App) override;

		virtual const char* Name() const override { return "PepperExtension"; }
	private:
		PepperExtensionConfig _Config;
	};
#pragma endregion

}
