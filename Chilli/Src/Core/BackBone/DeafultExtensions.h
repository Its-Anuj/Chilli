#pragma once

#include "BackBone.h"
#include "Maths.h"
#include "Events/Events.h"
#include "Window\Window.h"
#include "Renderer/RenderClasses.h"
#include "Renderer/Mesh.h"
#include "Material.h"
#include "AssetLoader.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

inline glm::vec3 ToGlmVec3(Chilli::Vec3 Data) { return glm::vec3(Data.x, Data.y, Data.z); }
inline Chilli::Vec3 FromGlmVec3(glm::vec3 Data) { return Chilli::Vec3(Data.x, Data.y, Data.z); }

namespace Chilli
{
	using EventID = std::uint32_t;

#pragma region Event Manager
	struct __IPerEventStorage__
	{
		virtual void Clear() = 0;
		virtual uint32_t GetActiveSize() const = 0;
	};

	template<typename _EventType>
	struct PerEventStorage : __IPerEventStorage__
	{
	private:
		std::vector< _EventType> Events;
		uint32_t ActiveSize = 0;

	public:

		void Push(const _EventType& e) {
			Events.push_back(e);
			ActiveSize++;
		}

		virtual uint32_t GetActiveSize() const override { return ActiveSize; }
		virtual void Clear() override { Events.clear(); ActiveSize = 0; }

		_EventType* Data() { return Events.data(); }
		const _EventType* Data() const { return Events.data(); }

		std::vector< _EventType>::iterator begin() { return Events.begin(); }
		std::vector< _EventType>::iterator end() { return Events.end(); }

		std::vector< _EventType>::const_iterator begin()  const { return Events.begin(); }
		std::vector< _EventType>::const_iterator end() const { return Events.end(); }
	};

	uint32_t GetNewEventID();

	template<typename _Type>
	uint32_t GetEventID()
	{
		static uint32_t ID = GetNewEventID();
		return ID;
	}

	struct EventHandler
	{
	public:
		EventHandler() {}
		~EventHandler()
		{
			// free all allocated storages
			for (auto& storage : _Storage)
				delete storage;

			_Storage.clear();
		}

		template<typename _EventType>
		void Register()
		{
			EventID id = GetEventID<_EventType>();
			if (id < _Storage.size())
				return;

			_Storage.push_back(new PerEventStorage<_EventType>());
		}

		template<typename _EventType>
		PerEventStorage<_EventType>* GetEventStorage()
		{
			EventID id = GetEventID<_EventType>();
			if (id >= _Storage.size())
				return nullptr;

			return static_cast<PerEventStorage<_EventType>*>(_Storage[id]);
		}

		template<typename _EventType>
		void Add(const _EventType& e)
		{
			auto* storage = GetEventStorage<_EventType>();
			storage->Push(e);
		}

		template<typename _EventType>
		void Clear()
		{
			EventID id = GetEventID<_EventType>();
			if (id >= _Storage.size())
				return;

			auto Storage = static_cast<PerEventStorage<_EventType>*>(_Storage[id]);
			Storage->Clear();
		}

		void ClearAll()
		{
			for (auto& storage : _Storage)
				storage->Clear();
		}
	private:
		std::vector<__IPerEventStorage__*> _Storage;
	};

	template<typename _EventType>
	struct EventReader {
		PerEventStorage<_EventType>* storage = nullptr;

		EventReader() = default;
		EventReader(EventHandler* Handler) {
			storage = Handler->GetEventStorage<_EventType>();
		}

		// --- Range-for support using custom iterator ---
		struct Iterator {
			PerEventStorage<_EventType>* storage;
			uint32_t idx;

			using value_type = const _EventType;
			using reference = const _EventType&;
			using pointer = const _EventType*;

			Iterator(PerEventStorage<_EventType>* s, uint32_t i)
				: storage(s), idx(i) {
			}

			bool operator!=(const Iterator& other) const {
				return idx != other.idx;
			}

			reference operator*() const {
				return storage->Data()[idx];
			}

			Iterator& operator++() {
				idx++;
				return *this;
			}
		};

		// begin = first unread event
		Iterator begin() {
			return Iterator(storage, 0);
		}

		// end = storage->ActiveSize
		Iterator end() {
			return Iterator(storage, storage->GetActiveSize());
		}
	};

	template<typename _EventType>
		requires std::derived_from<Event, _EventType>
	struct EventWriter {
		PerEventStorage<_EventType>* storage = nullptr;
		EventWriter(const EventHandler& Handler) :storage(Handler.GetEventStorage<_EventType>()) {}

		void Write(const _EventType& e) {
			storage->Push(e);
		}
	};
#pragma endregion 

	struct TransformComponent
	{
	public:
		TransformComponent(
			const Vec3& Pos = { 0, 0, 0 },
			const Vec3& Scle = { 1, 1, 1 },
			const Vec3& Rot = { 0, 0, 0 }
		)
			: _Position(Pos),
			_Scale(Scle),
			_Rotation(Rot),
			_Version(1),
			_WorldMatrix(1.0f)
		{
		}

		// --- Absolute Setters ---
		void SetPosition(const Vec3& Pos)
		{
			if (_Position != Pos) {
				_Position = Pos;
				IncrementVersion();
			}
		}

		void SetScale(const Vec3& Scle)
		{
			if (_Scale != Scle) {
				_Scale = Scle;
				IncrementVersion();
			}
		}

		void SetRotation(const Vec3& Rot)
		{
			if (_Rotation != Rot) {
				_Rotation = Rot;
				IncrementVersion();
			}
		}

		// --- Relative Movement ---
		void Move(const Vec3& Delta) { _Position += Delta; IncrementVersion(); }
		void MoveX(float Delta) { _Position.x += Delta; IncrementVersion(); }
		void MoveY(float Delta) { _Position.y += Delta; IncrementVersion(); }
		void MoveZ(float Delta) { _Position.z += Delta; IncrementVersion(); }

		// --- Relative Scaling ---
		void AddScale(const Vec3& Delta) { _Scale += Delta; IncrementVersion(); }
		void ScaleX(float Delta) { _Scale.x += Delta; IncrementVersion(); }
		void ScaleY(float Delta) { _Scale.y += Delta; IncrementVersion(); }
		void ScaleZ(float Delta) { _Scale.z += Delta; IncrementVersion(); }

		// --- Relative Rotation ---
		void Rotate(const Vec3& Delta) { _Rotation += Delta; IncrementVersion(); }
		void RotateX(float Delta) { _Rotation.x += Delta; IncrementVersion(); }
		void RotateY(float Delta) { _Rotation.y += Delta; IncrementVersion(); }
		void RotateZ(float Delta) { _Rotation.z += Delta; IncrementVersion(); }

		// --- Getters ---
		const Vec3& GetPosition() const { return _Position; }
		const Vec3& GetScale()    const { return _Scale; }
		const Vec3& GetRotation() const { return _Rotation; }
		uint32_t    GetVersion()  const { return _Version; }

		const glm::mat4& GetWorldMatrix() {
			if (_LastUpdatedVersion != _Version)
				CalculateWorldMatrix();
			return _WorldMatrix;
		}

		// --- System Internal ---
		void SetWorldMatrix(const glm::mat4& Matrix)
		{
			_WorldMatrix = Matrix;
			IncrementVersion();
		}

		bool IsDirty()
		{
			return _Version != _LastUpdatedVersion;
		}

	private:
		void CalculateWorldMatrix()
		{
			glm::mat4 Transform(1.0f);

			Transform = glm::translate(
				Transform,
				glm::vec3(_Position.x, _Position.y, _Position.z)
			);

			Transform = glm::rotate(Transform, glm::radians(_Rotation.x), glm::vec3(1, 0, 0));
			Transform = glm::rotate(Transform, glm::radians(_Rotation.y), glm::vec3(0, 1, 0));
			Transform = glm::rotate(Transform, glm::radians(_Rotation.z), glm::vec3(0, 0, 1));

			Transform = glm::scale(
				Transform,
				glm::vec3(_Scale.x, _Scale.y, _Scale.z)
			);

			_WorldMatrix = Transform;
			_LastUpdatedVersion = _Version;
		}

		void IncrementVersion()
		{
			++_Version;
			if (_Version == 0) _Version = 1;
		}

	private:
		Vec3 _Position;
		Vec3 _Scale;
		Vec3 _Rotation;

		uint32_t _Version = 1;
		uint32_t _LastUpdatedVersion = 0;
		glm::mat4 _WorldMatrix;
	};


	// Specifies that the entity with this component is visible
	struct VisibilityComponent;

	struct RenderSurfaceComponent
	{
		// --- Surface / Swapchain handles ---
		void* NativeSurface = nullptr;             // Platform-specific surface (VkSurfaceKHR, etc.)
		void* SwapchainHandle = nullptr;           // Backend-managed swapchain (can be stored as a backend handle)

		// --- Dimensions / Viewport ---
		IVec2 Size = { 0, 0 };
		bool  Resized = false;

		// --- Synchronization / Frame info ---
		uint32_t CurrentFrame = 0;
		uint32_t MaxFramesInFlight = 2;

		// --- Optional: Callbacks or flags ---
		bool EnableVSync = true;
	};

	struct MeshComponent
	{
		BackBone::AssetHandle<Mesh> MeshHandle;
		BackBone::AssetHandle<Material> MaterialHandle;
	};

	struct SceneRenderSurfaceComponent
	{
		BackBone::AssetHandle<Mesh> ScreenMeshHandle;
		BackBone::AssetHandle<Material> ScreenMaterialHandle;
	};

	struct PresentRenderSurfaceComponent
	{
		BackBone::AssetHandle<Mesh> Mesh;
		BackBone::AssetHandle<Material> Mat;
	};

	struct MaterialComponent
	{
		Vec4 AlbedoColor;
	};

#pragma region Render Extension
	struct DefferedRenderingConfig
	{
		bool GeometryPass = true;
		bool ScenePass = true;
		bool UIPass = true;
	};

	struct DefferedRenderingResource
	{
		BackBone::AssetHandle<Image> GeometryColorMSAAImage;
		BackBone::AssetHandle<Image> GeometryColorImage;
		BackBone::AssetHandle<Image> GeometryDepthImage;
		BackBone::AssetHandle<Texture> GeometryColorMSAATexture;
		BackBone::AssetHandle<Texture> GeometryColorTexture;
		BackBone::AssetHandle<Texture> GeometryDepthTexture;
		BackBone::AssetHandle<Texture> GeometryDepthViewTexture;
		//BackBone::AssetHandle<GraphicsPipeline> ScenePipeline;
		BackBone::AssetHandle<Mesh> ScreenRenderMesh;
		BackBone::AssetHandle<ShaderProgram> ScreenShaderProgram;
		BackBone::AssetHandle<Material> ScreenMaterial;
	};

	enum class RendererType
	{
		DEFFERED,
		FOWARD_PLUS
	};

	struct RenderExtensionConfig
	{
		GraphcisBackendCreateSpec Spec;
		DefferedRenderingConfig DefferedConfig{};
		RendererType UsingType = RendererType::DEFFERED;

		RenderExtensionConfig(const GraphcisBackendCreateSpec& spec = GraphcisBackendCreateSpec())
		{
			Spec = spec;
		}
	};

	class RenderExtension : public BackBone::Extension
	{
	public:
		RenderExtension(const RenderExtensionConfig& Config)
			:_Config(Config)
		{
		}
		~RenderExtension() {}

		virtual void Build(BackBone::App& App) override;

		virtual const char* Name() const override { return "RenderExtension"; }
	private:
		RenderExtensionConfig _Config;
	};

	struct RenderResource
	{
		std::vector<CommandBufferAllocInfo> CommandBuffers;
		uint32_t CurrentFrameCount = 0;
		uint32_t MaxFrameInFlight = 0;
		uint32_t TotalFrames = 0;
		EventReader < WindowResizeEvent> ResizeEvent;
		FrameBufferResizeEvent	FrameBufferSize{ 0,0 };
		bool FrameBufferReSized = false;
		bool ContinueRender = false;
		CommandBufferAllocInfo ActiveCommandBuffer;

		struct {
			uint32_t  DrawCallsCount = 0;
			uint32_t VertexCount = 0;
			uint32_t IndexCount = 0;
			uint32_t PipelineCount = 0;
		} RenderDebugInfo;
		std::vector<TransformComponent> OldTransformComponents;
	};

	struct RenderGraphPass
	{
		const char* DebugName = nullptr;
		CompiledPass Pass;
		std::function<void(BackBone::SystemContext&, RenderPassInfo&)> RenderFn;
		PipelineStateInfo Info;
		VertexInputShaderLayout Layout;
		uint32_t SortOrder = 0;
	};

	struct RenderGraph
	{
		std::vector<RenderGraphPass> Passes;

		void PushGraphPass(const RenderGraphPass& Pass)
		{
			Passes.push_back(Pass);
		}

		std::vector<RenderGraphPass>::iterator begin() { return Passes.begin(); }
		std::vector<RenderGraphPass>::iterator end() { return Passes.end(); }
	};
	class Renderer;
	class RenderCommand;

	void OnPresentRender(BackBone::SystemContext& Ctxt, RenderPassInfo& Pass);
	void OnRenderExtensionsSetup(BackBone::SystemContext& Ctxt);

#pragma endregion

#pragma region Window Extensions
	class WindowManager
	{
	public:
		WindowManager(BackBone::AssetStore<Cursor>* CursorStore)
		{
			InitDefaultCursors(CursorStore);
		}
		~WindowManager() { Clear(); }

		uint32_t GetActiveWindowIndex() const { return _ActiveWindowID; }

		Window* GetActiveWindow()
		{
			return (_ActiveWindowID != BackBone::npos) ? &_Windows[_ActiveWindowID] : nullptr;
		}

		Window* GetWindow(uint32_t index)
		{
			return index < _Windows.size() ? &_Windows[index] : nullptr;
		}

		uint32_t GetCount() const { return (uint32_t)_Windows.size(); }

		uint32_t Create(const WindowSpec& spec)
		{
			_Windows.push_back(Window());
			_ActiveWindowID = (uint32_t)_Windows.size() - 1;
			_Windows[_ActiveWindowID].Init(spec);

			return _ActiveWindowID;
		}

		void DestroyWindow(uint32_t index)
		{
			if (index >= _Windows.size()) return;

			_Windows[index].Terminate();
			_Windows.erase(_Windows.begin() + index);

			if (_ActiveWindowID == index)
				_ActiveWindowID = _Windows.empty() ? BackBone::npos : 0;
		}

		void Update()
		{
			uint32_t Idx = 0;
			_ActiveWindowID = BackBone::npos;
			for (auto& Win : _Windows)
			{
				if (Win.IsActive()) {
					_ActiveWindowID = Idx;
					Win.PollEvents();
					return;
				}
				Idx++;
			}
		}

		void Clear()
		{
			for (auto& Win : _Windows)
				Win.Terminate();

			_Windows.clear();
			_ActiveWindowID = BackBone::npos;
		}

		BackBone::AssetHandle<Cursor> GetCursor(DeafultCursorTypes Type)
		{
			return _DeafultCursors[int(Type)];
		}

	private:
		void InitDefaultCursors(BackBone::AssetStore<Cursor>* CursorStore);
	private:
		std::vector<Window> _Windows;
		std::array<BackBone::AssetHandle<Cursor>, int(DeafultCursorTypes::Count) > _DeafultCursors;
		uint32_t _ActiveWindowID = BackBone::npos;
	};

	struct WindowExtensionConfig
	{

	};

	class WindowExtension : public BackBone::Extension
	{
	public:
		WindowExtension(const WindowExtensionConfig& Config = WindowExtensionConfig())
			:_Config(Config)
		{
		}
		~WindowExtension() {}

		virtual void Build(BackBone::App& App) override;

		virtual const char* Name() const override { return "WindowExtension"; }
	private:
		WindowExtensionConfig _Config;
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

		BackBone::AssetHandle<Texture> GetDeafultPepperTexture();

		bool ShouldMaterialShaderDataUpdate(BackBone::Entity Entity);
		PepperShaderMaterialData GetMaterialShaderData(BackBone::Entity Entity);

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
	void OnPepperRender(BackBone::SystemContext& Ctxt, RenderPassInfo& Pass);
	std::vector<PipelineBarrier> GetPepperPrePassPipelineBarrier(BackBone::SystemContext& Ctxt);
	std::vector<PipelineBarrier> GetPepperPostPassPipelineBarrier(BackBone::SystemContext& Ctxt);

	struct PepperExtensionConfig
	{
		uint32_t MaxFramesInFlight = 0;
	};

	struct PepperResource
	{
		static const uint32_t MeshQuadCount = 10000;

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
		SparseSet<std::vector<BackBone::Entity>> ParentChildMap;
		std::array<BackBone::AssetHandle<Cursor>, int(DeafultCursorTypes::Count) > DeafultCursors;

		IVec2 OldWindowSize{ 0,0 };
		IVec2 WindowSize{ 0,0 };
		IVec2 OldCursorPos{ 0,0 };
		IVec2 CursorPos{ 0,0 };

		uint32_t QuadCount = 0;
		std::vector<PepperVertex> QuadVertices;
		std::vector<uint32_t> QuadIndicies;

		struct {
			glm::mat4 OthroMat = glm::mat4{ 1.0f };
		} CameraManager;

		// Which Canvas the mouse is on
		BackBone::Entity ActiveMouseElement;

		// Add these to PepperResource struct
		BackBone::Entity ResizingEntity = BackBone::npos;
		ResizeEdge ActiveEdge = ResizeEdge::None;
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

	struct DeafultExtensionConfig
	{
		RenderExtensionConfig RenderConfig{};
		WindowExtensionConfig WindowConfig{};
		PepperExtensionConfig PepperConfig{};
	};

	// ---- Basic Extensions ----
	class DeafultExtension : public BackBone::Extension
	{
	public:
		DeafultExtension(const DeafultExtensionConfig& Config = DeafultExtensionConfig())
			:_Config(Config)
		{
		}
		~DeafultExtension() {}

		virtual void Build(BackBone::App& App) override;

		virtual const char* Name() const override { return "DeafultExtension"; }
	private:
		DeafultExtensionConfig _Config;
	};

#pragma region Camera Extension
	struct MainCameraTag {};
	struct UICameraTag {};

	// Main camera component (like Bevy's Camera3d)
	struct CameraComponent {
		float Fov = 60.0f;
		float Near_Clip = 0.1f;
		float Far_Clip = 1000.0f;
		bool Is_Orthro = false;
		Vec2 Orthro_Size = { 1.0f, 1.0f };
	};

	// Standard FPS/Flight style controls
	struct Deafult3DCameraController
	{
		float Move_Speed = 10.0f;
		float Look_Sensitivity = 0.1f;

		// Internal state to track rotation without Gimbal Lock
		float Pitch = 0.0f;
		float Yaw = -90.0f;

		bool Invert_Y = false;
	};

	// Standard Pan/Zoom style controls
	struct Deafult2DCameraController
	{
		float Pan_Speed = 5.0f;
		float Zoom_Speed = 0.5f;
		float Min_Zoom = 0.1f;
		float Max_Zoom = 10.0f;
	};

	// ---- Basic Extensions ----
	class CameraExtension : public BackBone::Extension
	{
	public:
		CameraExtension() {}
		~CameraExtension() {}

		virtual void Build(BackBone::App& App) override;

		virtual const char* Name() const override { return "CameraExtension"; }
	private:
	};

	namespace CameraBundle
	{
		Chilli::BackBone::Entity Create3D(Chilli::BackBone::SystemContext& Ctxt,
			glm::vec3 Pos = { 0, 0, 5.0f },
			bool Main_Cam = true);

		// If Resolution is not provided it will use window coordinates
		Chilli::BackBone::Entity Create2D(Chilli::BackBone::SystemContext& Ctxt,
			bool Main_Cam = true, const IVec2& Resolution = { 0, 0 });
	}
#pragma endregion

#pragma region Ember Extension

	struct STBITTF_BakedChar
	{
		uint16_t x0, y0, x1, y1; // Coordinates in the atlas
		float xoff, yoff, xadvance;
		float xoff2, yoff2;
	};

	struct STBITTF_Data
	{
		void* Pixels = nullptr;
		uint32_t Width = 0;
		uint32_t Height = 0;

		STBITTF_BakedChar BakedChars[96];

		float FontSize = 32.0f;
		float Ascent = 0.0f;
		float Descent = 0.0f;
		float LineGap = 0.0f;
		std::string FileName;
		std::string FilePath;

		~STBITTF_Data() {
			if (Pixels) {
				free(Pixels);
				Pixels = nullptr;
			}
		}

		// --- Helper Functions ---

		// Safely get the character data, defaulting to '?' if out of range
		const STBITTF_BakedChar& GetChar(char c) const {
			if (c >= 32 && c <= 126) {
				return BakedChars[c - 32];
			}
			return BakedChars['?' - 32]; // Fallback character
		}

		// Returns the normalized UV coordinates for a specific glyph
		// Useful for building the Instance Data
		void GetUVs(char c, Vec2& outOffset, Vec2& outRange) const {
			const auto& bc = GetChar(c);
			float invW = 1.0f / (float)Width;
			float invH = 1.0f / (float)Height;

			outOffset = { (float)bc.x0 * invW, (float)bc.y0 * invH };
			outRange = { (float)(bc.x1 - bc.x0) * invW, (float)(bc.y1 - bc.y0) * invH };
		}

		// Calculate the total pixel width of a string
		float GetStringWidth(const std::string& text) const {
			float totalWidth = 0.0f;
			for (char c : text) {
				totalWidth += GetChar(c).xadvance;
			}
			return totalWidth;
		}

		// Get the recommended height between lines (Baseline to Baseline)
		float GetLineHeight() const {
			return (Ascent - Descent) + LineGap;
		}

		// Calculate a full bounding box for a string (Width and Height)
		Vec2 GetTextSize(const std::string& text) const {
			if (text.empty()) return { 0.0f, 0.0f };

			float x = GetStringWidth(text);
			// Height is typically the Max Ascent to Min Descent for the font
			float y = Ascent - Descent;
			return { x, y };
		}

		ImageSpec GetImageCreateInfo()
		{
			ImageSpec Spec;
			Spec.Resolution.Width = this->Width;
			Spec.Resolution.Height = this->Height;
			Spec.Resolution.Depth = 1;
			Spec.Format = ImageFormat::R8;
			Spec.Type = ImageType::IMAGE_TYPE_2D;
			Spec.Usage = IMAGE_USAGE_SAMPLED_IMAGE;
			Spec.State = ResourceState::ShaderRead;
			return Spec;
		}
	};

	// 1. The Interface (The Contract)
	class STBITTF_Loader : public BaseLoader<STBITTF_Data> {
	public:
		STBITTF_Loader() {}
		~STBITTF_Loader() {}

		// Every loader must implement these
		virtual BackBone::AssetHandle<STBITTF_Data> LoadTyped(BackBone::SystemContext& Ctxt, const std::string& Path) override;
		virtual void Unload(BackBone::SystemContext& Ctxt, const std::string& Path) override;
		virtual void Unload(BackBone::SystemContext& Ctxt, const BackBone::AssetHandle<STBITTF_Data>& Data) override;
		virtual std::vector<std::string> GetExtensions() override { return { ".ttf" }; }
		virtual bool DoesExist(const std::string& Path) const override;
		virtual int __FindIndex(const std::string& Path) const override { return 0; }

	private:
		std::vector<IndexEntry> _IndexMaps;
		std::vector<BackBone::AssetHandle<STBITTF_Data>> _STBITTF_Handles;   // Maps Hash index to _ImageDatas index
	};

	struct EmberFont
	{
		BackBone::AssetHandle<STBITTF_Data> FontSource; // This has the pixels so functions as the image data
		BackBone::AssetHandle<Image> FontImage; // the raw gpu image
		BackBone::AssetHandle<Texture> FontTexture; // the texture
	};

	struct EmberExtensionConfig
	{
		uint32_t MaxFrameInFlight = 0;
	};

	struct EmberFontVertex {
		Vec2 pos; // Normalized coordinates (0,0) to (1,1)
	};

	struct EmberFontInstance {
		Vec2 instPos;      // Position on screen (Pixels or NDC)
		Vec2 instSize;     // Width and Height of the glyph
		Vec2 instUVOffset; // Top-left coordinate in the Atlas (0.0 to 1.0)
		Vec2 instUVRange;  // Width and Height of the glyph in the Atlas (0.0 to 1.0)
		uint32_t FontIndex = 0;
	};

#define CH_EMBER_MAX_UNIQUE_FONT_COUNT 32

	struct EmberResource
	{
		static const uint32_t MaxCharacterCount = 1000;

		BackBone::AssetHandle<ShaderProgram> EmberShaderProgram;
		BackBone::AssetHandle<Material> ContextMaterial;

		std::vector<BackBone::AssetHandle<Buffer>> EmberFontVBs;
		BackBone::AssetHandle<Buffer> EmberVertexVB;
		BackBone::AssetHandle<Buffer> EmberVertexIB;

		std::vector<EmberFontInstance> EmberInstanceData;
		uint32_t GeoCharacterCount = 0;
		uint32_t UICharacterCount = 0;

		struct ShaderArrayFontAtlasMetaData
		{
			BackBone::AssetHandle<Texture> Tex;
			uint32_t ShaderIndex;
		};

		SparseSet<ShaderArrayFontAtlasMetaData> FontAtlasTextureMap;

		uint32_t _FontAtlasTextureMapCount = 0;

		void UpdateFontAtlasTextureMap(Renderer* RenderService, MaterialSystem* MaterialSystem, BackBone::AssetHandle<Texture> Tex);

		uint32_t GetFontAtlasTextureShaderIndex(BackBone::AssetHandle<Texture> Tex)
		{
			return FontAtlasTextureMap.Get(Tex.Handle)->ShaderIndex;
		}
	};

	struct EmberTextComponent {
		std::string Content;
		BackBone::AssetHandle<EmberFont> Font;
		float FontSize = 32.0f;
		Vec4 Color = { 1, 1, 1, 1 };
		bool IsDirty = true; // Flag to tell the system to rebuild the instances
	};

	// ---- Basic Extensions ----
	class EmberExtension : public BackBone::Extension
	{
	public:
		EmberExtension(EmberExtensionConfig Config) : _Config(Config) {}
		~EmberExtension() {}

		virtual void Build(BackBone::App& App) override;

		virtual const char* Name() const override { return "EmberExtension "; }
	private:
		EmberExtensionConfig _Config;
	};

	std::vector<PipelineBarrier> GetEmberPrePassPipelineBarrier(BackBone::SystemContext& Ctxt);
	std::vector<PipelineBarrier> GetEmberPostPassPipelineBarrier(BackBone::SystemContext& Ctxt);
#pragma endregion

#pragma region Command
	class Command
	{
	public:
		Command() {}
		Command(const BackBone::SystemContext& Ctxt) :_Ctxt(Ctxt) {}
		~Command() {}

		// Render Services Related
		BackBone::AssetHandle<Mesh> CreateMesh(const MeshCreateInfo& Info);
		void MapMeshVertexBufferData(BackBone::AssetHandle<Mesh> Handle, void* Data, uint32_t Count, size_t Size, uint32_t Offset);
		void MapMeshIndexBufferData(BackBone::AssetHandle<Mesh> Handle, void* Data, uint32_t Count, size_t Size, uint32_t Offset);
		BackBone::AssetHandle<Mesh> CreateMesh(BasicShapes Shape);
		void DestroyMesh(BackBone::AssetHandle<Mesh> mesh, bool Free = false);
		void FreeMesh(BackBone::AssetHandle<Mesh> mesh);

		uint32_t CreateEntity();
		void DestroyEntity(uint32_t EntityID);

		BackBone::AssetHandle<Sampler> CreateSampler(const SamplerSpec& Spec);
		void DestroySampler(const BackBone::AssetHandle<Sampler>& sampler);

		BackBone::AssetHandle<Image> AllocateImage(ImageSpec& Spec);
		// -1 means the function will estimate the miplevel
		std::pair<BackBone::AssetHandle<Image>, BackBone::AssetHandle<ImageData>> AllocateImage(const char* FilePath, ImageFormat Format, uint32_t Usage,
			ImageType Type, uint32_t MipLevel = -1, bool YFlip = false);
		void DestroyImage(const BackBone::AssetHandle<Image>& ImageHandle);
		void MapImageData(const BackBone::AssetHandle<Image>& ImageHandle, void* Data, int Width, int Height);

		BackBone::AssetHandle<Texture> CreateTexture(const BackBone::AssetHandle<Image>& ImageHandle, TextureSpec& Spec);
		void DestroyTexture(const BackBone::AssetHandle<Texture>& TextureHandle);

		BackBone::AssetHandle<Material> CreateMaterial(BackBone::AssetHandle<ShaderProgram> Program);
		void DestroyMaterial(const BackBone::AssetHandle<Material>& Mat);

		BackBone::AssetHandle<ShaderModule> CreateShaderModule(const char* FilePath, ShaderStageType Type);
		void DestroyShaderModule(const BackBone::AssetHandle<ShaderModule>& Handle);

		BackBone::AssetHandle<ShaderProgram> CreateShaderProgram();
		void AttachShaderModule(const BackBone::AssetHandle<ShaderProgram>& Program,
			const BackBone::AssetHandle<ShaderModule>& Module);
		void LinkShaderProgram(const BackBone::AssetHandle<ShaderProgram>& Handle);
		void DestroyShaderProgram(const BackBone::AssetHandle<ShaderProgram>& Handle);

		BackBone::AssetHandle<Buffer> CreateBuffer(const BufferCreateInfo& Info, const char* DebugName = "");
		void DestroyBuffer(const BackBone::AssetHandle<Buffer>& Handle);
		void MapBufferData(const BackBone::AssetHandle<Buffer>& Handle, void* Data, size_t Size
			, size_t Offset = 0);

		BackBone::AssetHandle<EmberFont> LoadFont_TTF(const std::string& Path);
		void UnLoadFont_TTF(BackBone::AssetHandle<EmberFont> FontHandle);
		// Window Services Related
		uint32_t SpwanWindow(const WindowSpec& Spec);
		void DestroyWindow(uint32_t Idx);

		template<typename _ResType>
		void AddResource() { _Ctxt.Registry->AddResource<_ResType>(); }

		template<typename _ResType>
		_ResType* GetResource() { return _Ctxt.Registry->GetResource<_ResType>(); }

		template<typename _ServiceType, typename... Args>
		void RegisterService(Args&&... args) {
			_Ctxt.ServiceRegistry->RegisterService<_ServiceType>(
				std::make_shared<_ServiceType>(std::forward<Args>(args)...)
			);
		}

		template<typename _Type>
		void RegisterComponent() {
			_Ctxt.Registry->Register<_Type>();
		}

		template<typename _Type>
		void AddComponent(uint32_t Entity, _Type Component) {
			_Ctxt.Registry->AddComponent<_Type>(Entity, Component);
		}

		template<typename _Type>
		void RemoveComponent(BackBone::Entity entity)
		{
			_Ctxt.Registry->RemoveComponent<_Type>(entity);
		}

		template<typename _Type>
		_Type* GetComponent(BackBone::Entity entity)
		{
			return _Ctxt.Registry->GetComponent<_Type>(entity);
		}

		template<typename _T>
		const _T* GetComponent(BackBone::Entity entity) const
		{
			return _Ctxt.Registry->GetComponent<_T>(entity);
		}

		// Multiple components - returns tuple of raw pointers
		template<typename... Ts>
		std::tuple<Ts*...> GetComponents(BackBone::Entity  entity)
		{
			return _Ctxt.Registry->GetComponents<Ts>(entity);
		}

		template<typename... Ts>
		std::tuple<const Ts*...> GetComponents(BackBone::Entity  entity) const
		{
			return _Ctxt.Registry->GetComponents<Ts>(entity);
		}

		template<typename T>
		inline void RegisterStore() {
			_Ctxt.AssetRegistry->RegisterStore<T>();
		}

		template<typename T>
		inline void RegisterSingle()  // ✅ Separate method for single assets
		{
			_Ctxt.AssetRegistry->RegisterSingle<T>();
		}

		template<typename T>
		inline BackBone::AssetStore<T>* GetStore() {
			return _Ctxt.AssetRegistry->GetStore<T>();
		}

		template<typename T>
		inline BackBone::SingleAsset<T>* GetSingle() {
			return _Ctxt.AssetRegistry->GetSingle<T>();
		}

		template<typename T>
		inline T* GetAsset(const BackBone::AssetHandle<T>& Handle)  // ✅ Convenience method
		{
			return _Ctxt.AssetRegistry->GetAsset<T>(Handle);
		}

		template<typename T>
		inline BackBone::AssetHandle<T> AddAsset(const T& asset)  // ✅ Convenience method
		{
			return _Ctxt.AssetRegistry->AddAsset<T>(asset);
		}

		template<DerivedFromIAssetLoader _T>
		void AddLoader()
		{
			auto AssetLoader = this->GetService<Chilli::AssetLoader>();
			AssetLoader->AddLoader<_T>();
		}

		template<DerivedFromIAssetLoader _T>
		_T* GetLoader()
		{
			auto AssetLoader = this->GetService<Chilli::AssetLoader>();
			return AssetLoader->GetLoader<_T>();
		}

		template<typename _AssetType>
		BackBone::AssetHandle<_AssetType> LoadAsset(const std::string& Path)
		{
			auto AssetLoader = this->GetService<Chilli::AssetLoader>();
			return AssetLoader->Load<_AssetType>(Path);
		}

		void UnloadAsset(const std::string& Path)
		{
			auto AssetLoader = this->GetService<Chilli::AssetLoader>();
			AssetLoader->Unload(Path);
		}

		template<typename _ServiceType>
		_ServiceType* GetService() { return _Ctxt.ServiceRegistry->GetService<_ServiceType>(); }

		Window* GetActiveWindow();
	private:
		void _Setup(const BackBone::SystemContext& Ctxt);
	private:
		BackBone::SystemContext _Ctxt;
	};
#pragma endregion 
}
