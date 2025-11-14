#pragma once

#include "UUID/UUID.h"
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
		using ComponentID = std::size_t;
		using ServiceID = std::size_t;
		using AssetID = std::size_t;
		using Entity = std::size_t;
		using SystemID = std::size_t;
		using ExtensionID = std::size_t;

		template<typename _T>
		struct AssetHandle
		{
			UUID_32 Handle;
			_T* ValPtr;

			bool operator==(const AssetHandle& other) const noexcept
			{
				return Handle == other.Handle;
			}
		};

		static constexpr Entity npos = static_cast<Entity>(-1);

		class World;
		class System;
		class Extension;
		class ServiceTable;
		class AssetManager;
		struct App;

		template<typename T>
		class SingleAsset;

		struct __IPerComponentStorage__
		{
		public:
			virtual void RemoveEntity(Entity entity) = 0;
			virtual bool HasEntity(Entity entity) const = 0;
			virtual size_t Size() const = 0;
		};

		template<typename _T>
		struct PerComponentStorage : public __IPerComponentStorage__
		{
			std::vector<Entity> Dense;
			std::vector<size_t> Sparse;
			std::vector<_T> Components;

			void Add(Entity Id, _T Component)
			{
				if (Id >= Sparse.size())
					Sparse.resize(Id + 1, npos);

				Sparse[Id] = Dense.size();
				Dense.push_back(Id);
				Components.push_back(Component);
			}

			void RemoveEntity(Entity entity) override
			{
				if (Dense.size() == 0)
					return;
				if (!Contains(Sparse[entity], entity))
					return;

				size_t index = Sparse[entity];
				size_t lastIndex = Dense.size() - 1;
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

			// ✅ Fix: Add missing methods
			bool HasEntity(Entity entity) const override
			{
				return entity < Sparse.size() && Sparse[entity] != npos;
			}

			size_t Size() const override { return Dense.size(); }

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

			bool Contains(int Index, Entity Id) {
				return Index < Dense.size() && Dense[Index] == Id;
			};
		};

		struct ComponentStorage
		{
			std::unordered_map<ComponentID, __IPerComponentStorage__*> Storage;

			template<typename _T>
			PerComponentStorage<_T>* GetComponentStorage()
			{
				ComponentID id = typeid(_T).hash_code();
				auto it = Storage.find(id);
				if (it == Storage.end()) return nullptr;  // ✅ Fix: Check if exists
				return static_cast<PerComponentStorage<_T>*>(it->second);
			}

			template<typename _T>
			const PerComponentStorage<_T>* GetComponentStorage() const
			{
				ComponentID id = typeid(_T).hash_code();
				auto it = Storage.find(id);
				if (it == Storage.end()) return nullptr;
				return static_cast<const PerComponentStorage<_T>*>(it->second);
			}

			void Free()
			{
				for (auto [_, CompStorage] : Storage)
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
			std::vector<size_t> FreeList;
			Entity NextEntityId = 0;

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
				}
				ActiveEntities[id] = true;
				NextEntityId++;
				return id;
			}

			void Destroy(Entity Id)
			{
				if (Id >= ActiveEntities.size() || !ActiveEntities[Id]) return;

				// Remove all components from this entity
				for (auto& [componentId, storage] : Storage.Storage)
					storage->RemoveEntity(Id);

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
				ComponentID id = typeid(_T).hash_code();
				if (Storage.Storage.find(id) != Storage.Storage.end()) return;
				Storage.Storage[id] = new PerComponentStorage<_T>();
			}

			template<typename _T>
			void AddComponent(Entity entity, const _T& component)
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
				auto* compStorage = Storage.GetComponentStorage<_T>();
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

		struct SystemContext
		{
			World& Registry;
			AssetManager& AssetRegistry;
			ServiceTable& ServiceRegistry;
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
			UPDATE_BEGIN,
			UPDATE,
			UPDATE_END,
			RENDER_BEGIN,
			RENDER,
			RENDER_END,
			SHUTDOWN
		};

		class Schedule
		{
		public:
			void AddSystem(ScheduleTimer Stage, std::unique_ptr < System> S, App& App);
			void AddSystemFunction(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function);

			template<typename T, typename... Args>
			void AddSystem(ScheduleTimer Stage, App& App, Args... args)
			{
				AddSystem(Stage, std::unique_ptr<T>(args...), App);
			}

			// Runs Everything
			void Run(App& App);
			void Run(ScheduleTimer Stage, App& App);

			void Terminate(App& App);
		private:
			std::unordered_map < ScheduleTimer, std::vector<std::unique_ptr<System>>> _Systems;
			std::unordered_map < ScheduleTimer, std::vector< std::function<void(SystemContext&)>>> _SystemFunctions;
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
		class AssetStore : public IAssetStorage  // ✅ Inherit for type safety
		{
		public:
			SparseSet<T> Store;

			AssetHandle<T> Add(const T& Asset)
			{
				AssetHandle<T> NewHandle;
				NewHandle.Handle = Store.Create(Asset);
				NewHandle.ValPtr = Store.Get(NewHandle.Handle);
				return NewHandle;
			}

			AssetHandle<T> Add(T&& Asset)  // ✅ Add move version for efficiency
			{
				AssetHandle<T> NewHandle;
				NewHandle.Handle = Store.Create(std::move(Asset));
				NewHandle.ValPtr = Store.Get(NewHandle.Handle);
				return NewHandle;
			}

			T* Get(const AssetHandle<T>& Handle)
			{
				return Store.Get(Handle.Handle);
			}

			bool IsValid(const AssetHandle<T>& Handle) const  // ✅ Added const
			{
				return Store.HasVal(Handle.Handle);
			}

			// ✅ Remove function
			bool Remove(AssetHandle<T>& Handle)
			{
				if (!Store.HasVal(Handle.Handle))
					return false;
				Store.Destroy(Handle.Handle);
				Handle.ValPtr = nullptr; // safety 
				return true;
			}

			auto begin() { return Store.begin(); }
			auto end() { return Store.end(); }
			auto begin() const { return Store.begin(); }  // ✅ Add const versions
			auto end() const { return Store.end(); }
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
				AssetID ID = typeid(T).hash_code();
				if (_Storage.find(ID) != _Storage.end()) return;  // ✅ Avoid duplicates
				_Storage[ID] = std::make_unique<AssetStore<T>>();  // ✅ Use unique_ptr
			}

			template<typename T>
			inline void RegisterSingle()  // ✅ Separate method for single assets
			{
				AssetID ID = typeid(SingleAsset<T>).hash_code();
				if (_Storage.find(ID) != _Storage.end()) return;
				_Storage[ID] = std::make_unique<SingleAsset<T>>();
			}

			template<typename T>
			inline AssetStore<T>* GetStore()  // ✅ Get entire store
			{
				AssetID ID = typeid(T).hash_code();
				auto It = _Storage.find(ID);
				if (It == _Storage.end())
					return nullptr;
				return static_cast<AssetStore<T>*>(It->second.get());  // ✅ Correct cast
			}

			template<typename T>
			inline SingleAsset<T>* GetSingle()  // ✅ Get single asset
			{
				AssetID ID = typeid(T).hash_code();
				auto It = _Storage.find(ID);
				if (It == _Storage.end())
					return nullptr;
				return static_cast<SingleAsset<T>*>(It->second.get());
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
			std::unordered_map<AssetID, std::unique_ptr<IAssetStorage>> _Storage;  // ✅ Type-safe storage
		};

		class ServiceTable
		{
		public:
			ServiceTable() {}
			~ServiceTable() {}

			template<typename T>
			void RegisterService(std::shared_ptr<T> service)
			{
				ServiceID type = typeid(T).hash_code();
				_Services[type] = std::move(service);
			}

			template<typename T>
			T* GetService()
			{
				ServiceID type = typeid(T).hash_code();
				auto it = _Services.find(type);
				if (it != _Services.end())
					// cast back to the right service type
					return static_cast<T*>(it->second.get());
				return nullptr; // not found
			}

		private:
			std::unordered_map<ServiceID, std::shared_ptr<void>> _Services;
		};

		struct App
		{
			World Registry;
			Schedule SystemScheduler;
			ExtensionRegistry Extensions;
			AssetManager AssetRegistry;
			ServiceTable ServiceRegsitry;

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
		std::size_t operator()(const Chilli::BackBone::AssetHandle<T>& asset) const noexcept
		{
			return std::hash<uint32_t>{}(static_cast<uint32_t>(asset.Handle));
		}
	};
}
