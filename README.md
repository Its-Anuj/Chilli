# Chilli Engine

A data driven, ecs-based game engine inspired by ![Bevy](https://bevy.org/)

# BackBone

The ECS Framework used, As it name says(BackBone) everything in this engine is based on it

Like bevy we have a App class overseeing and owning everything

```cpp
Chilli::BackBone::App App;
```

```cpp
struct App{
    World Registry;
    Schedule SystemScheduler;
    ExtensionRegistry Extensions;
    AssetManager AssetRegistry;
    ServiceTable ServiceRegistry;
    SystemContext Ctxt;
};
```
As shown in the struct App contains:

# World (Registry)

```cpp
    class World
    {
    public:
        ComponentStorage Storage;
        std::vector<bool> ActiveEntities;
        std::vector<uint32_t> FreeList;
        Entity NextEntityId = 0;

        std::vector<std::shared_ptr<void>> Resources;
    };
```
World or called as Registry holds all the entities, components and resources in the App.

Uses Basic Sparse Set Principle to hold entities

## Entity
 
A Simple uint32_t ID that has Components Associated with it

```cpp
    using Entity = std::uint32_t;
```

## Component Storage

Simply Stores all the individual components storage registered
```cpp
    std::vector< __IPerComponentStorage__*> Storage;
```

where  __IPerComponentStorage__ is a parent class used for ease of reading

```cpp
    struct __IPerComponentStorage__
    {
    public:
        virtual void RemoveEntity(Entity entity) = 0;
        virtual bool HasEntity(Entity entity) const = 0;
        virtual uint32_t Size() const = 0;
        virtual void ResizeSparse(size_t newSize) = 0;
    };
```

### PerComponentStorage

The class that holds the storage of each component (TransformComponent, MeshComponent, etc...)

Uses Sparse Set for Cache Locality, Performance, Fast Look ups, Fast Iteration

```cpp
    template<typename _T>
    struct PerComponentStorage : public __IPerComponentStorage__
    {
        std::vector<Entity> Dense;
        std::vector<uint32_t> Sparse;
        std::vector<_T> Components;
    };
```

## Resources

```cpp
    std::vector<std::shared_ptr<void>> Resources;
```


Fast, Single Instance, Accessible, Modifiable data to hold game or application specific resources


## Example 

```cpp
    struct TransformComponent
    {
        float x,y z;
    }

    Chilli::BackBone::World Registry;
    Registry.Register<TransformComponent>();
    auto Practise = Registry.Create();
    Registry.AddComponent<TransformComponent>(Practise, TransformComponent{0,0,0});

```

# Schedule(Sytem Scheduler)

Component that is responsible for executing the systems with specific flags decidiing on when to be executed 

Systems modify the components for their use like (MoveMent System Modifies the Transform Component) 

```cpp
    enum class ScheduleTimer
    {
        START_UP, // Functions With This Timer are called right as App.Run is called
        UPDATE_BEGIN, // Functions are called Before Update(Window Poll Events)
        UPDATE, // (Phyics Update, Player Movements, Camerra, etc...)
        UPDATE_END, // 
        RENDER_BEGIN, // (TOOD: Prepare Render Graphs, All Objects to be rendererd), Begin Command buffer
        RENDER, // Actually Renders them and records to a Command Buffer
        RENDER_END, // Submits the Command Buffer then presents the Image to the SwapChain
        SHUTDOWN, // Systems handled to delete assets, services shutdown etc
        COUNT
    };

```
```cpp
    struct SystemContext
    {
        World* Registry;
        AssetManager* AssetRegistry;
        ServiceTable* ServiceRegistry;
        GenericFrameData FrameData;
    };

    class Schedule
    {
    public:
        void AddSystem(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function);

        // Runs Everything
        void Run(ScheduleTimer Stage, SystemContext& Ctxt);
    private:
        std::array<std::vector< std::function<void(SystemContext&)>>, int(ScheduleTimer::COUNT)> _SystemFunctions;
    };

```

## Examples 

```cpp
    void OnGameShutDown(Chilli::BackBone::SystemContext& Ctxt)
    {
        auto GameData = Ctxt.Registry->GetResource<GameResource>();
        auto Command = Chilli::Command(Ctxt);

        Command.DestroyEntity(GameData->Player);
    }

    BackBone::Schedule SystemScheduler;
    SystemScheduler.AddSystem(SHUT_DOWN, OnGameShutDown);
    SystemScheduler.Run(SHUT_DOWN);
```

# ExtensionRegistry(Extensions)

Holds all the Extensions 

```cpp
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
```

Extensions dont have any functionalty instead they Register and AddSystems to the World and SystemScheduler to allow more functionality in the Application.


```cpp
    class ExtensionRegistry
    {
        std::unordered_map<ExtensionID, std::unique_ptr<Extension>> Extensions;
        void AddExtension(std::unique_ptr<Extension> Ext, bool BuildNow = false, App* app = nullptr);
    };
```

The Extenion Build function is the only functionm that is ever called on AddFunction To Extension Registry 
    
```cpp
    App App;
    ExtensionRegistry Registry
    Registry.AddExtension(std::make_unique<Chilli::DeafultExtension>(), true, &App);
```

# AssetManager(AssetRegistry)
```cpp
    class AssetManager
		{
            // TOOD: USE VECTOR
			std::unordered_map<AssetID, std::unique_ptr<IAssetStorage>> _Storage;   
        };
```

Service that holds all Assets in our Application

Assets Types:

## SingleAsset

```cpp
    template<typename T>
    struct SingleAsset : public IAssetStorage  // ✅ Inherit for type safety
    {
    public:
        T Asset;
    };
```

Single Instance of a Asset 

## Asset Store

Use Sparse Set To hold various Assets in a single object (Sparse Set for cache locality, Performance)

```cpp
    template<typename T>
    class AssetStore : public IAssetStorage
    {
        uint32_t NextId = 0;
        std::vector<uint32_t> _Sparse;
        std::vector<uint32_t> _Dense;
        std::vector<std::unique_ptr<T>> _Data;
        std::vector<uint32_t> _FreeList;
    };
```
## AssetHandle
```cpp
    template<typename _T>
    struct AssetHandle
    {
        uint32_t Handle;
        _T* ValPtr;

        bool operator==(const AssetHandle& other) const noexcept
        {
            return Handle == other.Handle;
        }
    };
```

Handle that stores the actual SparseSet handle also a raw pointer to the Asset for Ease of Access

## Example

```cpp
    class Window
    {
        // Stuffs
    };

    AssetManager AssetRegistry;
    AssetRegistry.RegisterStore<Window>();
    
    // For Asset Store
    auto WindowStore =     AssetRegistry.GetStore<Window>();
    WindwoStore->Add(Window{});

    struct InputState{
    //STuffs
     };

    AssetRegistry.RegisterStingle<InputState>();
    auto InputAsset =     AssetRegistry.GetSingle<Window>();
    InputAsset;//Stuffs

```

# ServiceTable(Service Registry)

Holds all the Services of our Application
A Service is bascially something that is of only a single instance, global to application


```cpp
    class ServiceTable
    {
        std::unordered_map<ServiceID, std::shared_ptr<void>> _Services;

        template<typename T>
        void RegisterService(std::shared_ptr<T> service);

			template<typename T>
			T* GetService();
    };
```

## Example


```cpp
    class Renderer
    {
        public:
        Render() {}
    };

    ServiceTable ServiceRegsitry;
    ServiceRegsitry.RegisterService<Renderer>(std::make_shared<Renderer>());
    auto RenderService =     ServiceRegsitry.GetService<Renderer>();
    RenderService->Render();
```
