#pragma once

#include "UUID/UUID.h"
#include "TimeStep.h"
#include <any>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <optional>
#include <vector>
#include <tuple>
#include "SparseSet.h"

namespace Chilli
{
	namespace BackBone
	{
		using ComponentID = std::uint32_t;
		using ServiceID = std::uint32_t;
		using AssetID = std::uint32_t;
		using Entity = std::uint32_t;
		using SystemID = std::uint32_t;
		using ExtensionID = std::uint32_t;

		static constexpr Entity npos = static_cast<Entity>(-1);

		template<typename _T>
		struct AssetHandle
		{
			uint32_t Handle = npos;
			_T* ValPtr = nullptr;

			bool operator==(const AssetHandle& other) const noexcept
			{
				return Handle == other.Handle;
			}
		};

		class World;
		class System;
		class Extension;
		class ServiceTable;
		class AssetManager;
		struct App;

		template<typename T>
		class SingleAsset;

		uint32_t GetNewResourceID();
		uint32_t GetNewComponentID();
		uint32_t GetNewAssetID();
		uint32_t GetNewServiceID();

		template<typename _ResType>
		uint32_t GetResourceID()
		{
			static uint32_t ResourceID = GetNewResourceID();
			return ResourceID;
		}

		template<typename _CompType>
		uint32_t GetComponentID()
		{
			static uint32_t ComponentID = GetNewComponentID();
			return ComponentID;
		}

		template<typename _Type>
		uint32_t GetAssetID()
		{
			static uint32_t AssetID = GetNewAssetID();
			return AssetID;
		}

		template<typename _Type>
		uint32_t GetServiceID()
		{
			static uint32_t ServiceID = GetNewServiceID();
			return ServiceID;
		}

		struct __IPerComponentStorage__
		{
		public:
			virtual void RemoveEntity(Entity entity) = 0;
			virtual bool HasEntity(Entity entity) const = 0;
			virtual uint32_t Size() const = 0;
			virtual void ResizeSparse(size_t newSize) = 0;
		};

		template<typename _T>
		struct PerComponentStorage : public __IPerComponentStorage__
		{
			std::vector<Entity> Dense;
			std::vector<uint32_t> Sparse;
			std::vector<_T> Components;

			void Add(Entity id, _T component)
			{
				if (id >= Sparse.size()) Sparse.resize(id + 1, npos);
				if (HasEntity(id)) return;                  // already present
				Sparse[id] = Dense.size();
				Dense.push_back(id);
				Components.push_back(component);
			}

			void RemoveEntity(Entity entity) override
			{
				if (Dense.size() == 0)
					return;
				if (entity >= Sparse.size()) return;            // <- bounds check
				uint32_t index = Sparse[entity];
				if (!Contains(index, entity)) return;

				uint32_t lastIndex = Dense.size() - 1;
				Entity lastEntity = Dense[lastIndex];

				if (index != lastIndex)
				{
					// Swap with last element
					Dense[index] = lastEntity;
					Components[index] = std::move(Components[lastIndex]);
					Sparse[lastEntity] = index;
				}

				// Remove last element
				Dense.pop_back();
				Components.pop_back();
				Sparse[entity] = npos;
			}

			void ResizeSparse(size_t newSize) override {
				if (Sparse.size() < newSize) Sparse.resize(newSize, npos);
			}

			// ✅ Fix: Add missing methods
			bool HasEntity(Entity entity) const override
			{
				return entity < Sparse.size() && Sparse[entity] != npos;
			}

			uint32_t Size() const override { return Dense.size(); }

			_T* Get(Entity entity)
			{
				if (!HasEntity(entity)) return nullptr;
				return &Components[Sparse[entity]];
			}

			const _T* Get(Entity entity) const
			{
				if (!HasEntity(entity)) return nullptr;
				return &Components[Sparse[entity]];
			}

			bool Contains(uint32_t index, Entity id) const {
				if (index == npos) return false;
				return index < Dense.size() && Dense[index] == id;
			}
		};

		struct ComponentStorage
		{
			std::vector< __IPerComponentStorage__*> Storage;
			// change signature to accept current entity capacity
			template<typename _Type>
			void Register(size_t entityCapacity)
			{
				uint32_t ID = GetComponentID<_Type>();
				if (Storage.size() <= ID) Storage.resize(ID + 1, nullptr);

				if (Storage[ID] != nullptr) return;                    // keep existing
				auto* ps = new PerComponentStorage<_Type>();
				ps->ResizeSparse(entityCapacity);                      // initialize sparse size
				Storage[ID] = ps;
			}

			template<typename _Type>
			PerComponentStorage<_Type>* GetComponentStorage() {
				uint32_t ID = GetComponentID<_Type>();
				if (ID >= Storage.size()) return nullptr;
				return static_cast<PerComponentStorage<_Type>*>(Storage[ID]);
			}

			template<typename _Type>
			const PerComponentStorage<_Type>* GetComponentStorage() const {
				uint32_t ID = GetComponentID<_Type>();
				if (ID >= Storage.size()) return nullptr;
				return static_cast<const PerComponentStorage<_Type>*>(Storage[ID]);
			}

			void Free()
			{
				for (auto CompStorage : Storage)
				{
					delete CompStorage;
				}
				Storage.clear();
			}
		};

		class World
		{
		public:
			ComponentStorage Storage;
			std::vector<bool> ActiveEntities;
			std::vector<uint32_t> FreeList;
			Entity NextEntityId = 0;

			std::vector<std::shared_ptr<void>> Resources;

			template<typename _ResType>
			void AddResource()
			{
				uint32_t ID = GetResourceID<_ResType>();
				if (Resources.size() <= ID) Resources.resize(ID + 1);
				Resources[ID] = std::make_shared<_ResType>();
			}

			template<typename _ResType>
			_ResType* GetResource() {
				uint32_t ID = GetResourceID<_ResType>();
				if (ID >= Resources.size() || !Resources[ID]) return nullptr;
				return std::static_pointer_cast<_ResType>(Resources[ID]).get();
			}

			World()
			{
			}
			~World()
			{
			}

			Entity Create()
			{
				if (FreeList.size() > 0)
				{
					Entity Id = FreeList[FreeList.size() - 1];
					FreeList.pop_back();
					ActiveEntities[Id] = true;
					return Id;
				}

				Entity id = NextEntityId;
				if (id >= ActiveEntities.size())
				{
					ActiveEntities.resize(id + 1, false);

					// ensure all component sparse arrays match new size
					for (auto* s : Storage.Storage)
						if (s) s->ResizeSparse(ActiveEntities.size());
				}

				ActiveEntities[id] = true;
				NextEntityId++;
				return id;
			}

			void Destroy(Entity Id)
			{
				if (Id >= ActiveEntities.size() || !ActiveEntities[Id]) return;

				for (auto* storage : Storage.Storage)
					if (storage) storage->RemoveEntity(Id);

				ActiveEntities[Id] = false;
				FreeList.push_back(Id);
			}

			void Free()
			{
				Storage.Free();
			}

			template<typename _T>
			void Register()
			{
				Storage.Register<_T>(ActiveEntities.size());
			}

			template<typename _T>
			void AddComponent(Entity entity, _T component)
			{
				if (!IsEntityValid(entity)) return;

				auto* compStorage = Storage.GetComponentStorage<_T>();
				if (!compStorage)
				{
					Register<_T>();
					compStorage = Storage.GetComponentStorage<_T>();
				}
				compStorage->Add(entity, component);
			}

			template<typename _T>
			void RemoveComponent(Entity entity)
			{
				auto* compStorage = Storage.GetComponentStorage<_T>();
				if (compStorage)
				{
					compStorage->RemoveEntity(entity);
				}
			}

			bool IsEntityValid(Entity entity) const
			{
				return entity < ActiveEntities.size() && ActiveEntities[entity];
			}

			template<typename _T>
			_T* GetComponent(Entity entity)
			{
				auto compStorage = Storage.GetComponentStorage<_T>();
				if (!compStorage) return nullptr;
				return compStorage->Get(entity);
			}

			template<typename _T>
			const _T* GetComponent(Entity entity) const
			{
				const auto* compStorage = Storage.GetComponentStorage<_T>();
				if (!compStorage) return nullptr;
				return compStorage->Get(entity);
			}

			// Multiple components - returns tuple of raw pointers
			template<typename... Ts>
			std::tuple<Ts*...> GetComponents(Entity entity)
			{
				return std::tuple<Ts*...>{GetComponent<Ts>(entity)...};
			}

			template<typename... Ts>
			std::tuple<const Ts*...> GetComponents(Entity entity) const
			{
				return std::tuple<const Ts*...>{GetComponent<Ts>(entity)...};
			}

			// Base case
			template<typename T>
			bool HasAllComponents(Entity entity) const
			{
				return GetComponent<T>(entity) != nullptr;
			}

			// Recursive case
			template<typename T, typename T2, typename... Ts>
			bool HasAllComponents(Entity entity) const
			{
				return GetComponent<T>(entity) != nullptr && HasAllComponents<T2, Ts...>(entity);
			}

			template<typename _T>
			bool HasComponent(Entity entity) const
			{
				const auto* compStorage = Storage.GetComponentStorage<_T>();
				return compStorage && compStorage->HasEntity(entity);
			}
		};

		template<typename... Components>
		class Query
		{
		private:
			World& registry;

		public:
			Query(World& reg) : registry(reg) {}

			class Iterator
			{
			private:
				World& registry;
				Entity currentEntity;
				Entity endEntity;

				void advance_to_valid()
				{
					while (currentEntity < endEntity &&
						(!registry.IsEntityValid(currentEntity) ||
							!registry.HasAllComponents<Components...>(currentEntity)))
					{
						++currentEntity;
					}
				}

			public:
				Iterator(World& reg, Entity start, Entity end)
					: registry(reg), currentEntity(start), endEntity(end)
				{
					advance_to_valid();
				}

				// Returns tuple of raw pointers - maximum performance!
				auto operator*() const
				{
					return registry.GetComponents<Components...>(currentEntity);
				}

				Iterator& operator++()
				{
					++currentEntity;
					advance_to_valid();
					return *this;
				}

				bool operator!=(const Iterator& other) const
				{
					return currentEntity != other.currentEntity;
				}
			};

			Iterator begin() { return Iterator(registry, 0, registry.ActiveEntities.size()); }
			Iterator end() { return Iterator(registry, registry.ActiveEntities.size(), registry.ActiveEntities.size()); }
		};

		template<typename... Components>
		class QueryWithEntities
		{
		private:
			World& registry;

		public:
			QueryWithEntities(World& reg) : registry(reg) {}

			class Iterator
			{
			private:
				World& registry;
				Entity currentEntity;
				Entity endEntity;

				void advance_to_valid()
				{
					while (currentEntity < endEntity &&
						(!registry.IsEntityValid(currentEntity) ||
							!registry.HasAllComponents<Components...>(currentEntity)))
					{
						++currentEntity;
					}
				}

			public:
				Iterator(World& reg, Entity start, Entity end)
					: registry(reg), currentEntity(start), endEntity(end)
				{
					advance_to_valid();
				}

				// Return entity + tuple of raw pointers to components
				auto operator*() const
				{
					return std::tuple_cat(std::make_tuple(currentEntity),
						registry.GetComponents<Components...>(currentEntity));
				}

				Iterator& operator++()
				{
					++currentEntity;
					advance_to_valid();
					return *this;
				}

				bool operator!=(const Iterator& other) const
				{
					return currentEntity != other.currentEntity;
				}
			};

			Iterator begin() { return Iterator(registry, 0, registry.ActiveEntities.size()); }
			Iterator end() { return Iterator(registry, registry.ActiveEntities.size(), registry.ActiveEntities.size()); }
		};

		struct GenericFrameData
		{
			TimeStep Ts{ 0.0f };
			bool IsRunning = true;
		};

		struct SystemContext
		{
			World* Registry;
			AssetManager* AssetRegistry;
			ServiceTable* ServiceRegistry;
			GenericFrameData FrameData;
		};

		class System
		{
		public:
			virtual ~System() = default;

			virtual void OnCreate(SystemContext& Registry) {}

			virtual void OnBeforeRun(SystemContext& Registry) {}
			virtual void Run(SystemContext& Registry) = 0;
			virtual void OnAfterRun(SystemContext& Registry) {}

			virtual void OnTerminate(SystemContext& Registry) {}
		};

		enum class ScheduleTimer
		{
			START_UP,
			UPDATE,
			RENDER,
			SHUTDOWN,
			COUNT
		};

		class Schedule
		{
		public:
			void AddSystem(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function);
			void AddSystemOverLayBefore(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function);
			void AddSystemOverLayAfter(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function);

			// Runs Everything
			void Run(ScheduleTimer Stage, SystemContext& Ctxt);
		private:
			std::array<std::vector< std::function<void(SystemContext&)>>, int(ScheduleTimer::COUNT)> _SystemFunctions;
			std::array<std::vector< std::function<void(SystemContext&)>>, int(ScheduleTimer::COUNT)> _SystemOverLayBefore;
			std::array<std::vector< std::function<void(SystemContext&)>>, int(ScheduleTimer::COUNT)> _SystemOverLayAfter;
		};

		class Extension
		{
		public:
			virtual ~Extension() = default;

			virtual void Build(App& App) = 0;
			virtual const char* Name() const = 0;

			ExtensionID ID() const { return _ID; }
		private:
			ExtensionID _ID;
		};

		class ExtensionRegistry
		{
		public:
			std::unordered_map<ExtensionID, std::unique_ptr<Extension>> Extensions;

			void AddExtension(std::unique_ptr<Extension> Ext, bool BuildNow = false, App* app = nullptr);
			void BuildAll(App& app);
		};
		// Base interface for type-safe storage
		struct IAssetStorage {
			virtual ~IAssetStorage() = default;
		};

		template<typename T>
		class AssetStore : public IAssetStorage
		{
		public:
			static constexpr uint32_t npos = static_cast<uint32_t>(-1);

			AssetStore() = default;
			~AssetStore()
			{
			}

			AssetHandle<T> Add(const T& val)
			{
				AssetHandle<T> Handle;
				uint32_t id;
				if (!_FreeList.empty()) {
					id = _FreeList.back();
					_FreeList.pop_back();
				}
				else {
					id = NextId++;
					if (id >= _Sparse.size())
						_Sparse.resize(id + 1, npos);
				}

				_Sparse[id] = _Dense.size();
				_Dense.push_back(id);
				_Data.push_back(std::make_unique<T>(val));
				Handle.Handle = id;
				Handle.ValPtr = Get(Handle);

				return Handle;
			}

			void Remove(const AssetHandle<T>& Handle)
			{
				auto id = Handle.Handle;
				if (!HasVal(Handle.Handle)) return;

				uint32_t index = _Sparse[id];
				uint32_t lastIndex = _Dense.size() - 1;
				uint32_t lastVal = _Dense[lastIndex];

				if (index != lastIndex)
				{
					_Dense[index] = lastVal;
					_Data[index] = std::move(_Data[lastIndex]);
					_Sparse[lastVal] = index;
				}

				_Dense.pop_back();
				_Data.pop_back();
				_Sparse[id] = npos;
				_FreeList.push_back(id);
			}

			T* Get(const AssetHandle<T>& Handle)
			{
				return HasVal(Handle) ? _Data[_Sparse[Handle.Handle]].get() : nullptr;
			}

			const T* Get(const AssetHandle<T>& Handle) const
			{
				return HasVal(Handle) ? _Data[_Sparse[Handle.Handle]].get() : nullptr;
			}

			bool HasVal(uint32_t id) const {
				if (id >= _Sparse.size())
					return false;

				return _Sparse[id] != npos;
			}

			bool HasVal(const AssetHandle<T>& Handle) const {
				return HasVal(Handle.Handle);
			}

			uint32_t size() const { return _Dense.size(); }
			bool empty() const { return _Dense.empty(); }

			bool Contains(uint32_t Id) const {
				if (Id >= _Sparse.size())         // >= because valid indices are 0..size-1
					return false;
				if (_Sparse[Id] >= _Dense.size()) // >= same reason
					return false;

				return _Dense[_Sparse[Id]] == Id;
			}

			bool Contains(const AssetHandle<T>& Handle) const {
				return Contains(Handle.Handle);
			}

			std::vector<std::unique_ptr<T>>::iterator begin() { return _Data.begin(); }
			std::vector<std::unique_ptr<T>>::iterator end() { return _Data.end(); }

			std::vector<std::unique_ptr<T>>::const_iterator begin() const { return _Data.begin(); }
			std::vector<std::unique_ptr<T>>::const_iterator end() const { return _Data.end(); }

			const uint32_t GetActiveCount() const { return _Dense.size(); }
			const uint32_t GetSparseCount() const { return _Sparse.size(); }

			// For range-based for loops:
			template<typename T>
			class HandleView {
			public:
				struct Iterator {
					const AssetStore<T>* _Store;
					size_t _Index;

					Iterator& operator++() {
						++_Index;
						return *this;
					}

					bool operator!=(const Iterator& other) const {
						return _Index != other._Index;
					}

					AssetHandle<T> operator*() const {
						// No bounds check needed - assume _Index is always valid when dereferenced
						AssetHandle<T> handle;
						handle.Handle = _Store->_Dense[_Index];
						handle.ValPtr = _Store->_Data[_Index].get();
						return handle;
					}
				};
				const AssetStore<T>* _Store;

				HandleView<T> GetHandles() const {
					return HandleView(this);
				}

				Iterator begin() const {
					return Iterator{ _Store, 0 };
				}

				Iterator end() const {
					return Iterator{ _Store, _Store->_Dense.size() };  // Points ONE PAST the end
				}
			};

			HandleView<T> GetHandles() const { return HandleView(this); }

			// For range-based for loops:
			template<typename T>
			class RefView {
			public:
				struct Iterator {
					const AssetStore<T>* _Store;
					size_t _Index;

					Iterator& operator++() {
						++_Index;
						return *this;
					}

					bool operator!=(const Iterator& other) const {
						return _Index != other._Index;
					}

					T* operator*() const {
						// No bounds check needed - assume _Index is always valid when dereferenced
						return _Store->_Data[_Index].get();
					}
				};
				const AssetStore<T>* _Store;

				RefView<T> GetHandles() const {
					return RefView(this);
				}

				Iterator begin() const {
					return Iterator{ _Store, 0 };
				}

				Iterator end() const {
					return Iterator{ _Store, _Store->_Dense.size() };  // Points ONE PAST the end
				}
			};

			RefView<T> GetRefs() const { return RefView(this); }
		private:
			uint32_t NextId = 0;

			std::vector<uint32_t> _Sparse;
			std::vector<uint32_t> _Dense;
			std::vector<std::unique_ptr<T>> _Data;
			std::vector<uint32_t> _FreeList;
		};

		template<typename T>
		struct SingleAsset : public IAssetStorage  // ✅ Inherit for type safety
		{
		public:
			T Asset;
		};

		class AssetManager
		{
		public:
			AssetManager() = default;
			~AssetManager() { Free(); }  // ✅ Auto-cleanup

			template<typename T>
			inline void RegisterStore()  // ✅ Renamed for clarity
			{
				AssetID ID = GetAssetID<T>();
				if (_Storage.size() <= ID) _Storage.resize(ID + 1);
				_Storage[ID] = std::make_unique<AssetStore<T>>();  // ✅ Use unique_ptr
			}

			template<typename T>
			inline void RegisterSingle()  // ✅ Separate method for single assets
			{
				AssetID ID = GetAssetID<T>();
				if (_Storage.size() <= ID) _Storage.resize(ID + 1);
				_Storage[ID] = std::make_unique<SingleAsset<T>>();
			}

			template<typename T>
			inline AssetStore<T>* GetStore()  // ✅ Get entire store
			{
				AssetID ID = GetAssetID<T>();
				if (ID >= _Storage.size() || !_Storage[ID]) return nullptr;
				return static_cast<AssetStore<T>*>(_Storage[ID].get());
			}

			template<typename T>
			inline SingleAsset<T>* GetSingle()  // ✅ Get single asset
			{
				AssetID ID = GetAssetID<T>();
				if (ID >= _Storage.size() || !_Storage[ID]) return nullptr;
				return static_cast<SingleAsset<T>*>(_Storage[ID].get());
			}

			template<typename T>
			inline T* GetAsset(const AssetHandle<T>& Handle)  // ✅ Convenience method
			{
				auto* store = GetStore<T>();
				if (!store) return nullptr;
				return store->Get(Handle);
			}

			template<typename T>
			inline AssetHandle<T> AddAsset(const T& asset)  // ✅ Convenience method
			{
				auto* store = GetStore<T>();
				if (!store) {
					RegisterStore<T>();
					store = GetStore<T>();
				}
				return store->Add(asset);
			}

			inline void Free()
			{
				_Storage.clear();  // ✅ unique_ptr auto-deletes
			}

		private:
			std::vector< std::unique_ptr<IAssetStorage>> _Storage;
		};

		class ServiceTable
		{
		public:
			ServiceTable() {}
			~ServiceTable() {}

			template<typename T>
			void RegisterService(std::shared_ptr<T> service)
			{
				ServiceID ID = GetServiceID<T>();
				if (_Services.size() <= ID) _Services.resize(ID + 1);
				_Services[ID] = std::move(service);  // ✅ Use unique_ptr
			}

			template<typename T>
			T* GetService()
			{
				ServiceID ID = GetServiceID<T>();
				if (ID >= _Services.size() || !_Services[ID]) return nullptr;
				return static_cast<T*>(_Services[ID].get());
			}

		private:
			std::vector<std::shared_ptr<void>> _Services;
		};

		struct App
		{
			World Registry;
			Schedule SystemScheduler;
			ExtensionRegistry Extensions;
			AssetManager AssetRegistry;
			ServiceTable ServiceRegistry;
			SystemContext Ctxt;

			App()
			{

				this->Ctxt.Registry = &Registry;
				this->Ctxt.AssetRegistry = &AssetRegistry;
				this->Ctxt.ServiceRegistry = &ServiceRegistry;
			}

			void Run();
		};
	}
}

// ✅ hash specialization must come *after* the type definition
namespace std
{
	template<typename T>
	struct hash<Chilli::BackBone::AssetHandle<T>>
	{
		std::uint32_t operator()(const Chilli::BackBone::AssetHandle<T>& asset) const noexcept
		{
			return std::hash<uint32_t>{}(static_cast<uint32_t>(asset.Handle));
		}
	};
}
