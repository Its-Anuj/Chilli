#pragma once

#include "BackBone.h"
#include "Maths.h"
#include "Events/Events.h"
#include "Window\Window.h"
#include "Renderer/RenderClasses.h"
#include "Renderer/Mesh.h"
#include "Material.h"

inline glm::vec3 ToGlmVec3(Chilli::Vec3 Data) { return glm::vec3(Data.x, Data.y, Data.z); }
inline Chilli::Vec3 FromGlmVec3(glm::vec3 Data) { return Chilli::Vec3(Data.x, Data.y, Data.z); }

namespace Chilli
{
	using EventID = std::uint32_t;

#pragma region Event Manager
	struct __IPerEventStorage__
	{
	public:
		virtual void Clear() = 0;
	};

	template<typename _EventType>
	//requires std::derived_from<Event, _EventType>
	struct PerEventStorage : __IPerEventStorage__
	{
		std::vector< _EventType> Events;
		uint32_t ActiveSize = 0;

		void Push(const _EventType& e) {
			Events.push_back(e);
			ActiveSize++;
		}

		virtual void Clear() override { Events.clear(); ActiveSize = 0; }

		std::vector< _EventType>::iterator begin() { return Events.begin(); }
		std::vector< _EventType>::iterator end() { return Events.end(); }

		std::vector< _EventType>::const_iterator begin()  const { return Events.begin(); }
		std::vector< _EventType>::const_iterator end() const { return Events.end(); }
	};

	struct EventHandler
	{
	public:

		EventHandler() {}
		~EventHandler()
		{
			// free all allocated storages
			for (auto& [id, storage] : Storage)
				delete storage;

			Storage.clear();
		}

		template<typename _EventType>
		//requires std::derived_from<Event, _EventType>
		void Register()
		{
			EventID id = typeid(_EventType).hash_code();
			if (Storage.contains(id)) return;

			Storage[id] = new PerEventStorage<_EventType>();
		}

		template<typename _EventType>
		PerEventStorage<_EventType>* GetEventStorage()
		{
			auto it = Storage.find(typeid(_EventType).hash_code());
			if (it == Storage.end()) return nullptr;
			return static_cast<PerEventStorage<_EventType>*>(it->second);
		}

		template<typename _EventType>
		//requires std::derived_from<Event, _EventType>
		void Add(const _EventType& e)
		{
			auto* storage = GetEventStorage<_EventType>();
			storage->Push(e);
		}

		void ClearAll()
		{
			for (auto& [id, storage] : Storage)
				storage->Clear();
		}
	private:
		std::unordered_map<EventID, __IPerEventStorage__*> Storage;
	};

	template<typename _EventType>
		requires std::derived_from<_EventType, Event>
	struct EventReader {
		PerEventStorage<_EventType>* storage = nullptr;
		uint32_t index = 0;  // how many events this reader has consumed

		EventReader() = default;
		EventReader(PerEventStorage<_EventType>* s) : storage(s) {}

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
				return storage->Events[idx];
			}

			Iterator& operator++() {
				idx++;
				return *this;
			}
		};

		// begin = first unread event
		Iterator begin() {
			return Iterator(storage, index);
		}

		// end = storage->ActiveSize
		Iterator end() {
			return Iterator(storage, storage->ActiveSize);
		}

		// Called after range-for loop to update reader position
		void sync() {
			index = storage->ActiveSize;
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

	// ---- Basic Components ----
	struct TransformComponent
	{
		Vec3 Position, Scale, Rotation;

		TransformComponent(const Vec3& Pos, const Vec3& Scle, const Vec3& Rot)
			:Position(Pos), Scale(Scle), Rotation(Rot)
		{

		}

		bool operator==(const TransformComponent& other)
		{
			if (this->Scale == other.Scale && this->Position == other.Position && this->Rotation == other.Rotation)
				return true;
			return false;
		}

		TransformComponent& operator=(const TransformComponent& other)
		{
			Position = other.Position;
			Scale = other.Scale;
			Rotation = other.Rotation;
			return *this;
		}
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
		BackBone::AssetHandle<Texture> GeometryColorOutput;
		BackBone::AssetHandle<GraphicsPipeline> ScenePipeline;
		BackBone::AssetHandle<Mesh> SceneRenderMesh;
		BackBone::Entity SceneEntity;
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
		RenderPassInfo Pass;
		std::function<void(BackBone::SystemContext&, RenderPassInfo&)> RenderFn;
		uint32_t SortOrder = 0;
	};

	struct RenderGraph
	{
		std::vector<RenderGraphPass> Passes;
	};
	class Renderer;
	class RenderCommand;
	
	void OnPresentRender(BackBone::SystemContext& Ctxt, RenderPassInfo& Pass);

#pragma endregion

#pragma region Window Extensions
	class WindowManager
	{
	public:
		WindowManager() = default;
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

	private:
		std::vector<Window> _Windows;
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
	};

	struct PepperElement
	{
		PepperElementTypes Type = PepperElementTypes::PANEL;
		BackBone::AssetHandle<Material> Material;
		bool Visible = true;
		bool Interactive = true;
	};

	// Systems
	void OnPepperStartUp(BackBone::SystemContext& Ctxt);
	void OnPepperRender(BackBone::SystemContext& Ctxt, RenderPassInfo& Pass);

	struct PepperExtensionConfig
	{

	};
	

	struct PepperResource
	{
		static const uint32_t MeshQuadCount = 10000;
	
		BackBone::AssetHandle<Mesh> SquareMesh;
		BackBone::AssetHandle<Texture> PepperDeafultTexture;
		BackBone::AssetHandle<GraphicsPipeline> PepperPipeline;
		BackBone::AssetHandle<Material> BasicMaterial;
		IVec2 InitialWindowSize{ 0,0 };
		IVec2 CurrentWindowSize{ 0,0 };
		IVec2 CursorPos{ 0,0 };

		std::vector<BackBone::AssetHandle<Mesh>> BatchMesh;
		uint32_t QuadCount = 0;
		std::vector<std::vector<float>> QuadVertices;
		std::vector<std::vector<uint32_t>> QuadIndicies;

		struct {
			BackBone::Entity Camera;
			bool ReCreate = true;
			glm::mat4 OthroMat = glm::mat4{ 1.0f };
		} CameraManager;;
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

#pragma region Command
	class Command
	{
	public:
		Command() {}
		Command(const BackBone::SystemContext& Ctxt) :_Ctxt(Ctxt) {}
		~Command() {}

		// Render Services Related
		BackBone::AssetHandle<Mesh> CreateMesh(const BetterMeshCreateInfo& Info);
		BackBone::AssetHandle<Mesh> CreateMesh(BasicShapes Shape, float Scale = 0.5f);
		void DestroyMesh(BackBone::AssetHandle<Mesh> mesh, bool Free = true);
		void FreeMesh(Mesh& mesh);

		uint32_t CreateEntity();
		void DestroyEntity(uint32_t EntityID);

		BackBone::AssetHandle<Sampler> CreateSampler(const Sampler& Spec);
		void FreeSampler(Sampler& sampler);
		void DestroySampler(const BackBone::AssetHandle<Sampler>& sampler, bool Free = true);

		BackBone::AssetHandle<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& Spec);
		void FreeGraphicsPipeline(GraphicsPipeline& Pipeline);
		void DestroyGraphicsPipeline(const BackBone::AssetHandle<GraphicsPipeline>& Pipeline, bool Free = true);

		BackBone::AssetHandle<Texture> CreateTexture(const ImageSpec& ImgSpec, const char* FilePath);
		void LoadImageData(const BackBone::AssetHandle<Texture>& Tex, void* Data, IVec2 Resolution);
		void DestroyTexture(const BackBone::AssetHandle<Texture>& Tex, bool Free = true);
		void FreeTexture(const Texture& Tex);

		BackBone::AssetHandle<Material> AddMaterial(const Material& Mat);;

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

		template<typename _ServiceType>
		_ServiceType* GetService() { return _Ctxt.ServiceRegistry->GetService<_ServiceType>(); }

		Window* GetActiveWindow();
	private:
		void _Setup(const BackBone::SystemContext& Ctxt);
	private:
		BackBone::SystemContext _Ctxt;
		// Common Services
		Renderer* _RenderService = nullptr;
		RenderCommand* _RenderCommandService = nullptr;
		WindowManager* _WindowService = nullptr;
		EventHandler* _EventService = nullptr;
		// Common Asset Stores
		BackBone::AssetStore<Sampler>* _SamplerStore = nullptr;
		BackBone::AssetStore<Texture>* _TextureStore = nullptr;
		BackBone::AssetStore<Material>* _MaterialStore = nullptr;
		BackBone::AssetStore<Mesh>* _MeshStore = nullptr;
		BackBone::AssetStore<GraphicsPipeline>* _GraphicsPipelineStore = nullptr;
	};
#pragma endregion 
}
