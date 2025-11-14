#pragma once

#include "UUID/UUID.h"
#include <any>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <optional>
#include <vector>
#include <tuple>

namespace Chilli |
{
	namespace OldBackBone
	{
		using ComponentID = std::size_t;
		using ServiceID = std::size_t;
		using AssetID = std::size_t;
		using SystemID = UUID;
		using ExtensionID = UUID;
		using AssetHandle = UUID;

		typedef UUID Entity;

		class World;
		class System;
		class Extension;
		class ComponentStorage;
		class ServiceTable;
		class AssetManager;
		struct App;

		template<typename T>
		class Asset;

		struct __IPerComponentStorage__
		{
			virtual ~__IPerComponentStorage__() = default;
			virtual size_t Size() const = 0;
		};

		template<typename T>
		struct PerComponentStorage : public __IPerComponentStorage__ {
			std::unordered_map<Entity, T> Components;
			virtual size_t Size() const override { return Components.size(); }
		};

		class ComponentStorage
		{
		public:
			std::unordered_map< ComponentID, std::shared_ptr<__IPerComponentStorage__>> Storages;

			template<typename T>
			inline void Register()
			{
				ComponentID ID = typeid(T).hash_code();
				Storages[ID] = std::make_shared< PerComponentStorage<T>>();
			}

			template<typename T>
			inline void Add(Entity entity, const T& Component)
			{
				ComponentID ID = typeid(T).hash_code();
				auto It = Storages.find(ID);

				if (It != Storages.end())
				{
					auto ComponentStorage = std::static_pointer_cast<PerComponentStorage<T>>(It->second);
					ComponentStorage->Components[entity] = Component;
				}
			}

			template<typename T>
			inline  std::optional<std::reference_wrapper<T>> GetComponent(Entity entity)
			{
				return __Impl__GetComponent__<T>(entity);
			}

			template<typename T>
			inline  const std::optional<std::reference_wrapper<T>> GetComponents(Entity entity) const {
				return __Impl__GetComponent__<T>(entity);
			}

			// For multiple Components Return
			template<typename... Ts>
			std::optional<std::tuple<std::reference_wrapper<Ts>...>> GetComponents(Entity entity)
			{
				return __Impl__GetComponents__<Ts...>(entity);
			}

			// For multiple Components Return
			template<typename... Ts>
			std::optional<std::tuple<std::reference_wrapper<Ts>...>> GetComponent(Entity entity) const {
				return __Impl__GetComponents__<Ts...>(entity);
			}

			template<typename T>
			inline bool DoesComponentExist(Entity entity)
			{
				return GetComponent<T>(entity) != std::nullopt;
			}

			template<typename T>
			void RemoveComponent(Entity Entity)
			{
				ComponentID ID = typeid(T).hash_code();
				auto It = Storages.find(ID);

				if (It == Storages.end())
					return;
				auto CompStorage = std::static_pointer_cast<PerComponentStorage<T>>(It->second);
				auto CompIt = CompStorage->Components.find(Entity);

				if (CompIt == CompStorage->Components.end())
					return;
				CompStorage->Components.erase(CompIt);
			}

			std::unordered_map< ComponentID, std::shared_ptr<__IPerComponentStorage__>>::iterator  begin() { return Storages.begin(); }
			std::unordered_map< ComponentID, std::shared_ptr<__IPerComponentStorage__>>::iterator  end() { return Storages.end(); }

			std::unordered_map< ComponentID, std::shared_ptr<__IPerComponentStorage__>>::const_iterator   begin() const { return Storages.begin(); }
			std::unordered_map< ComponentID, std::shared_ptr<__IPerComponentStorage__>>::const_iterator  end() const { return Storages.end(); }
		private:

			template<typename T>
			inline  std::optional<std::reference_wrapper<T>> __Impl__GetComponent__(Entity entity) const
			{
				ComponentID ID = typeid(T).hash_code();
				auto It = Storages.find(ID);

				if (It == Storages.end())
					return std::nullopt;

				auto CompStorage = std::static_pointer_cast<PerComponentStorage<T>>(It->second);
				auto CompIt = CompStorage->Components.find(entity);

				if (CompIt == CompStorage->Components.end())
					return std::nullopt;

				return std::ref(CompIt->second);
			}
			// For multiple Components Return
			template<typename... Ts>
			std::optional<std::tuple<std::reference_wrapper<Ts>...>> __Impl__GetComponents__(Entity entity) const
			{
				// Pack all __Impl__GetComponent__ calls into a tuple of optionals
				std::tuple<std::optional<std::reference_wrapper<Ts>>...> optionals{
					__Impl__GetComponent__<Ts>(entity)...
				};

				// If any component is missing, return nullopt
				bool all_exist = (... && std::get<std::optional<std::reference_wrapper<Ts>>>(optionals).has_value());
				if (!all_exist)
					return std::nullopt;

				// Construct optional in-place to wrap tuple of references
				return std::optional<std::tuple<std::reference_wrapper<Ts>...>>(
					std::in_place,
					std::get<std::optional<std::reference_wrapper<Ts>>>(optionals).value().get()...
				);
			}
		};

		class World
		{
		public:
			template<typename _T>
			inline void RegisterComponent() { Components.Register<_T>(); }

			template<typename _T>
			inline void AddComponent(Entity Entity, const _T& Component) {
				if (DoesEntityExist(Entity)) Components.Add<_T>(Entity, Component);
			}

			template<typename _T>
			inline const std::optional<std::reference_wrapper<_T>> GetComponent(Entity Entity) const {
				if (DoesEntityExist(Entity)) return Components.GetComponent<_T>(Entity);
				return std::nullopt;
			}

			template<typename _T>
			inline std::optional<std::reference_wrapper<_T>> GetComponent(Entity Entity) {
				if (DoesEntityExist(Entity)) return Components.GetComponent<_T>(Entity);
				return std::nullopt;
			}

			// For multiple Components Return
			template<typename... Ts>
			std::optional<std::tuple<std::reference_wrapper<Ts>...>> GetComponents(Entity Entity) const {
				if (DoesEntityExist(Entity)) return Components.GetComponents<Ts...>(Entity);
			}

			// For multiple Components Return
			template<typename... Ts>
			std::optional<std::tuple<std::reference_wrapper<Ts>...>> GetComponents(Entity Entity) {
				if (DoesEntityExist(Entity)) return Components.GetComponents<Ts...>(Entity);
			}

			template<typename _T>
			inline bool DoesComponentExist(Entity Entity) { return Components.DoesComponentExist<_T>(Entity); }

			Entity Create();
			bool DoesEntityExist(Entity Entity) const;

			size_t EntityCount() const { return Entities.size(); }

			std::vector<Entity> Entities;
			ComponentStorage Components;
		};

		struct SystemContext
		{
			World& Regsitry;
			AssetManager& AssetRegistry;
			ServiceTable& ServiceRegsitry;
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
			void AddSystem(ScheduleTimer Stage, const std::shared_ptr<System>& S, App& App);
			void AddSystemFunction(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function);

			template<typename T, typename... Args>
			void AddSystem(ScheduleTimer Stage, App& App, Args... args)
			{
				AddSystem(Stage, std::make_shared<T>(args...), App);
			}

			// Runs Everything
			void Run(App& App);
			void Run(ScheduleTimer Stage, App& App);

			void Terminate(App& App);
		private:
			std::vector<std::pair<ScheduleTimer, std::shared_ptr<System>>> _Systems;
			std::vector<std::pair<ScheduleTimer, std::function<void(SystemContext&)>>> _SystemFunctions;
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

		template<typename... Ts>
		struct Query
		{
		private:
			World& Registry;
		public:

			Query(World& Registry) :Registry(Registry) {}

			class Iterator
			{
			public:
				World& Registry;
				size_t _Index = 0;
				Entity _ActiveEntity = 0;

				Iterator(World& Registry, size_t Index) :Registry(Registry), _Index(Index) {
					advance_to_valid();
				}

				void advance_to_valid() {
					while (_Index < Registry.EntityCount()) {
						_ActiveEntity = Registry.Entities[_Index];
						// Check if ALL components exist for this entity
						if ((Registry.DoesComponentExist<Ts>(_ActiveEntity) && ...))
							break;
						++_Index;
					}
				}

				// Return a tuple of references to all components
				auto operator*() const {
					auto opt_tuple = Registry.GetComponents<Ts...>(_ActiveEntity);
					// opt_tuple must exist here, because we checked in advance_to_valid()
					return *opt_tuple;
				}

				Iterator& operator++() {
					++_Index;
					advance_to_valid();
					return *this;
				}

				bool operator!=(const Iterator& other) const { return _Index != other._Index; }
			};

			Iterator begin() { return Iterator(Registry, 0); }
			Iterator end() { return Iterator(Registry, Registry.EntityCount()); }
		};

		struct __IPerAssetStorage__
		{
			virtual ~__IPerAssetStorage__() = default;
		};

		template<typename T>
		class AssetStore : public __IPerAssetStorage__
		{
		public:
			std::unordered_map<AssetHandle, std::shared_ptr<T>> Store;

			AssetHandle Add(const T& Asset)
			{
				AssetHandle NewHandle;
				Store[NewHandle] = std::make_shared<T>(Asset);
				return NewHandle;
			}

			std::optional<std::reference_wrapper<T>> Get(const AssetHandle& Handle)
			{
				auto It = Store.find(Handle);
				if (It == Store.end())
					return std::nullopt;

				return std::ref(*It->second);
			}

			bool IsValid(const AssetHandle& Handle)
			{
				Store.find(Handle) != Store.end();
			}

			std::unordered_map<AssetHandle, std::shared_ptr<T>>::iterator begin() { return Store.begin(); }
			std::unordered_map<AssetHandle, std::shared_ptr<T>>::iterator end() { return Store.end(); }
		};

		template<typename T>
		struct SingleAsset : public __IPerAssetStorage__
		{
		public:
			T Asset;
		};

		class AssetManager
		{
		public:
			AssetManager() {}
			~AssetManager() {}

			template<typename T>
			inline void Register()
			{
				AssetID ID = typeid(T).hash_code();
				_Storage[ID] = std::make_shared< T>();
			}

			template<typename T>
			inline std::optional<std::reference_wrapper<T>> Get()
			{
				AssetID ID = typeid(T).hash_code();
				auto It = _Storage.find(ID);

				if (It == _Storage.end())
					return std::nullopt;
				auto Assets = std::static_pointer_cast<T>(It->second);
				return std::ref(*Assets);
			}
		private:
			std::unordered_map< AssetID, std::shared_ptr<__IPerAssetStorage__>> _Storage;
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
			std::optional<std::reference_wrapper<T>> GetService()
			{
				ServiceID type = typeid(T).hash_code();
				auto it = _Services.find(type);
				if (it != _Services.end())
					return std::ref(*static_cast<T*>(it->second.get()));
				return std::nullopt;
			}

		private:
			std::unordered_map<ServiceID, std::shared_ptr<void>> _Services;
		};

		struct App
		{
			World Registry;
			Schedule Scheduler;
			ExtensionRegistry Extensions;
			AssetManager AssetRegistry;
			ServiceTable ServiceRegsitry;

			void Run();
		};
	}

}
