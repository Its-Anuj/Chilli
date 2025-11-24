#pragma once

#include "BackBone.h"
#include "Maths.h"
#include "Events/Events.h"
#include "Window\Window.h"
#include "Input\Input.h"
#include "Renderer/RenderClasses.h"
#include "Renderer/Mesh.h"
#include "Events/Events.h"
#include "glm/glm.hpp"
#include "Material.h"

inline glm::vec3 ToGlmVec3(Chilli::Vec3 Data) { return glm::vec3(Data.x, Data.y, Data.z); }

namespace Chilli
{
	using EventID = std::uint32_t;

	// ---- Basic Components ----
	struct TransformComponent
	{
		Vec3 Position, Scale, Rotation;
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

	struct MaterialComponent
	{
		Vec4 AlbedoColor;
	};

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
	// ---- Basic Extensions ----
	class DeafultExtension : public BackBone::Extension
	{
	public:
		DeafultExtension() {}
		~DeafultExtension() {}

		virtual void Build(BackBone::App& App) override;

		virtual const char* Name() const override { return "DeafultExtension"; }
	private:
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
	};

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
	
	class Renderer;
	class RenderCommand;

	class Command
	{
	public:
		Command() {}
		Command(const BackBone::SystemContext& Ctxt) :_Ctxt(Ctxt) {}
		~Command() {}

		// Render Services Related
		Mesh CreateMesh(const MeshCreateInfo& Info);
		void DestroyMesh(Mesh& mesh);

		uint32_t CreateEntity();
		void DestroyEntity(uint32_t EntityID);

		BackBone::AssetHandle<Sampler> CreateSampler(const Sampler& Spec);
		void DestroySampler(const BackBone::AssetHandle<Sampler>& sampler);

		BackBone::AssetHandle<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& Spec);
		void DestroyGraphicsPipeline(const BackBone::AssetHandle<GraphicsPipeline>& Pipeline);

		BackBone::AssetHandle<Texture> CreateTexture(const ImageSpec& ImgSpec, const char* FilePath);;
		void DestroyTexture(const BackBone::AssetHandle<Texture>& Tex);

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
		BackBone::AssetStore<Material>* _MaterialStore= nullptr;
		BackBone::AssetStore<Mesh>* _MeshStore = nullptr;
		BackBone::AssetStore<GraphicsPipeline>* _GraphicsPipelineStore = nullptr;		
	};

}
