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
#include <glm/gtc/quaternion.hpp>
#include <FastNoise/FastNoise.h>

#include "BasicComponents.h"
#include "RenderExtension.h"
#include "UIExtensions.h"

inline glm::vec3 ToGlmVec3(Chilli::Vec3 Data) { return glm::vec3(Data.x, Data.y, Data.z); }
inline Chilli::Vec3 FromGlmVec3(glm::vec3 Data) { return Chilli::Vec3(Data.x, Data.y, Data.z); }

namespace Chilli
{
#pragma region ParentChildMapTable
	struct ParentChildMapTable
	{
		// 1 to 1 Map
		struct ParentChildMapStruct
		{
			bool Initiated = false;

			std::vector<BackBone::Entity> Map;
			ParentChildMapStruct(bool I = false) :Initiated(I) {}
		};

		struct ChildParentMapStruct
		{
			bool Initiated = false;
			BackBone::Entity Parent;
			ChildParentMapStruct(bool I = false) :Initiated(I) {}
		};

		SparseSet<ParentChildMapStruct> ParentChildMap;
		SparseSet<ChildParentMapStruct> ChildParentMap;

		void PushChild(BackBone::Entity Parent, BackBone::Entity Child) {
			// 1. Ensure the child isn't already attached elsewhere (Prevents logic leaks)
			RemoveFromCurrentParent(Child);

			auto Map = ParentChildMap.Get(Parent);
			if (!Map) {
				InsertParent(Parent);
				Map = ParentChildMap.Get(Parent);
			}

			Map->Map.push_back(Child);

			ChildParentMapStruct ChildStruct;
			ChildStruct.Initiated = true;
			ChildStruct.Parent = Parent;

			ChildParentMap.Insert(Child, Parent);
		}

		void EraseChild(BackBone::Entity Parent, BackBone::Entity Child) {
			auto Map = ParentChildMap.Get(Parent);
			if (!Map || !Map->Initiated) return;

			auto it = std::find(Map->Map.begin(), Map->Map.end(), Child);
			if (it != Map->Map.end()) {
				Map->Map.erase(it);
			}

			ChildParentMapStruct ChildStruct;
			ChildStruct.Initiated = false;
			ChildStruct.Parent = BackBone::npos;

			ChildParentMap.Insert(Child, ChildStruct);
		}

		bool DoesParentExist(BackBone::Entity Parent) {
			return ParentChildMap.Contains(Parent); // Assuming SparseSet has Contains()
		}

		// Crucial helper to keep both maps in sync
		void RemoveFromCurrentParent(BackBone::Entity Child) {
			auto CurrentParent = ChildParentMap.Get(Child);
			if (CurrentParent && CurrentParent->Initiated != false) {
				// Find the child in the old parent's list and remove it
				auto Map = ParentChildMap.Get(CurrentParent->Parent);
				if (Map) {
					auto& v = Map->Map;
					v.erase(std::remove(v.begin(), v.end(), Child), v.end());
				}
				ChildParentMap.Destroy(Child); // Fully remove the entry
			}
		}

		bool IsChildOf(BackBone::Entity Parent, BackBone::Entity Child) {
			auto Map = ChildParentMap.Get(Child);
			if (!Map || Map->Initiated == false) return false;

			if ((Map->Parent) == Parent)
				return true;
			return false;
		}

		const std::vector<BackBone::Entity>* GetChildMap(BackBone::Entity Parent) {
			auto Map = ParentChildMap.Get(Parent);
			if (!Map) return nullptr; // Safer than returning a reference to garbage
			return &Map->Map;
		}

		void InsertParent(BackBone::Entity Parent)
		{
			ParentChildMapStruct Map{ true };
			ParentChildMap.Insert(Parent, Map);
		}

		bool IsParent(BackBone::Entity Parent)
		{
			auto Map = ParentChildMap.Get(Parent);
			if (!Map) return false;
			return Map->Initiated;
		}
	};
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
		bool Enable = true;
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
		glm::mat4 ViewProjMat{ 1.0f };
	};

	// Standard FPS/Flight style controls
	struct Deafult3DCameraController
	{
		float Move_Speed = 10.0f;
		float Look_Sensitivity = 0.1f;

		// Internal state to track rotation without Gimbal Lock
		float Pitch = 0.0f;
		float Yaw = 0.0f;

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

		void Update3DCamera(Chilli::BackBone::Entity Camera, Chilli::BackBone::SystemContext& Ctxt);
	}
#pragma endregion

#pragma region Noise Generator

	struct NoiseExtensionConfig
	{
		struct {
			bool Enable = true;
		} FastNoise2Config;
	};

	// The core types of noise your engine supports
	enum class NoiseType {
		Perlin,     // Classic, slightly grid-aligned (good for clouds)
		Simplex,    // Smoother, less directional (good for rolling hills)
		Cellular,   // Voronoi/Worley patterns (perfect for craters/crystals)
		Value       // Blocky, Minecraft-style base noise
	};

	// A data-oriented config struct
	struct NoiseConfig {
		NoiseType Type = NoiseType::Simplex;
		float Frequency = 0.01f;
		int Seed = 1337;

		// Fractal (fBm) Layering Settings
		bool UseFractal = true;
		bool UseWarp = true;
		int Octaves = 4;         // Number of detail layers
		float Gain = 0.5f;       // Amplitude multiplier per layer
		float Lacunarity = 2.0f; // Frequency multiplier per layer
		float WarpIntensity = 0.0f;
	};

	using NoiseHandle = uint32_t;
	constexpr NoiseHandle InvalidNoiseHandle = 0;

	class FastNoiseProvider {
	private:
		std::unordered_map<NoiseHandle, FastNoise::SmartNode<>> _generators;
		std::unordered_map<NoiseHandle, NoiseConfig> _configs;
		uint32_t _nextHandle = 1;

		// Internal helper to get a node safely
		FastNoise::SmartNode<> GetNode(NoiseHandle handle) {
			auto it = _generators.find(handle);
			return (it != _generators.end()) ? it->second : nullptr;
		}

	public:

		~FastNoiseProvider();

		NoiseHandle CreateGenerator(const NoiseConfig& config);

		float GetSingle3D(NoiseHandle handle, float x, float y, float z);
		float GetSingle2D(NoiseHandle handle, float x, float y);

		// Position-based sampling (Unity style)
		float GetAtPosition(NoiseHandle handle, const Vec3& pos) {
			return GetSingle3D(handle, pos.x, pos.y, pos.z);
		}

		// Position-based sampling (Unity style)
		float GetAtPosition(NoiseHandle handle, const Vec2& pos) {
			return GetSingle2D(handle, pos.x, pos.y);
		}

		// Like Unity's Mathf.Lerp or Remap functions
		float GetValue01(NoiseHandle handle, float x, float y) {
			float val = GetSingle2D(handle, x, y);
			return (val + 1.0f) * 0.5f; // FastNoise is -1 to 1, remap to 0 to 1
		}

		// Like Unity's Mathf.Lerp or Remap functions
		float GetValue01(NoiseHandle handle, float x, float y, float z) {
			float val = GetSingle3D(handle, x, y, z);
			return (val + 1.0f) * 0.5f; // FastNoise is -1 to 1, remap to 0 to 1
		}

		// Returns a value scaled to a custom range (like Unity's Remap)
		inline float GetValueRange(NoiseHandle h, float x, float y, float min, float max) {
			float val01 = GetValue01(h, x, y);
			return min + val01 * (max - min);
		}

		void GetGrid2D(NoiseHandle handle, std::vector<float>& outBuffer, int startX, int startY, int width, int height);
		// 3D Grid Generation (For Volumetric/Voxel data)
		void GetGrid3D(NoiseHandle handle, std::vector<float>& outBuffer,
			int startX, int startY, int startZ,
			int width, int height, int depth);
		void GetGrid2D(NoiseHandle handle, float* outBuffer, int startX, int startY, int width, int height);

		void DestroyGenerator(NoiseHandle handle) {
			_generators.erase(handle);
			_configs.erase(handle);
		}
	};
#pragma endregion

#pragma region JoltPhysics

	// Layer that objects can be in, determines which other objects it can collide with
	// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
	// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
	// but only if you do collision testing).
	enum class Layers : uint16_t
	{
		DEFAULT = 0,
		STATIC = 1,
		DYNAMIC = 2,
		PLAYER = 3,
		ENEMIES = 4,
		PROJECTILE = 5,
		SENSOR = 6,
		DEBRIS = 7,
		CUSTOM_0,
		CUSTOM_1,
		CUSTOM_2,
		CUSTOM_3,
		CUSTOM_4,
		CUSTOM_5,
		CUSTOM_6,
		CUSTOM_7,
		NUM_LAYERS
	};

	enum class BroadPhaseLayers : uint16_t
	{
		STATIC,
		DYNAMIC,
		SENSOR,
		NUM_LAYERS
	};

	enum class MotionType
	{
		STATIC, DYNAMIC, KINEMATIC
	};

	struct JoltBackend
	{

	};

	struct RigidBody
	{
		float Mass;
		float GravityFactor = 0.0f;
		float Restitution = -1.0f;
		float Friction = -1.0f;
		float LinearDamping = -1.0f;

		Layers Layer = Layers::STATIC;
		MotionType MotionType;

		Vec3 Velocity{ 0,0,0 };
		Vec3 ForceAccumulator{ 0,0,0 };
		Vec3 Acceleration{ 0,0,0 };
		bool UseVelvert = false;

		void AddImpulse(const Vec3& Impulse)
		{
			ForceAccumulator += Impulse;
		}
	};

	enum class ColliderType
	{
		BOX,
		SPHERE,
		CAPSULE,
		CYLINDER,
		TORUS,
		CONE,
		TAPERED_CYLINDER,
		TAPERED_CAPSULE
	};

	struct CollisionEnterEvent : public Event
	{
	public:
		CollisionEnterEvent(BackBone::Entity Entity1, BackBone::Entity Entity2)
		{
			this->Entity1 = Entity1;
			this->Entity2 = Entity2;
		}

		const char* GetName() const override { return "CollisionEnterEvent"; }

		BackBone::Entity GetEntity1() { return Entity1; }
		BackBone::Entity GetEntity2() { return Entity2; }

	private:
		BackBone::Entity Entity1, Entity2;
	};

	struct CollisionStayEvent : public Event
	{
	public:
		CollisionStayEvent(BackBone::Entity Entity1, BackBone::Entity Entity2)
		{
			this->Entity1 = Entity1;
			this->Entity2 = Entity2;
		}

		const char* GetName() const override { return "CollisionStayEvent"; }

		BackBone::Entity GetEntity1() { return Entity1; }
		BackBone::Entity GetEntity2() { return Entity2; }

	private:
		BackBone::Entity Entity1, Entity2;
	};

	struct CollisionExitEvent : public Event
	{
	public:
		CollisionExitEvent(BackBone::Entity Entity1, BackBone::Entity Entity2)
		{
			this->Entity1 = Entity1;
			this->Entity2 = Entity2;
		}

		const char* GetName() const override { return "CollisionExitEvent "; }

		BackBone::Entity GetEntity1() { return Entity1; }
		BackBone::Entity GetEntity2() { return Entity2; }

	private:
		BackBone::Entity Entity1, Entity2;
	};

	struct Collider
	{
		ColliderType Type;
		bool IsTrigger = false;

		union ShapeUnion
		{
			struct { Vec3 HalfExtent; } AABB;
			struct { float Radius; } Sphere;

			struct
			{
				float Radius;
				float HalfHeight;  // half height of cylinder part only, not including end spheres
			} Capsule;

			struct
			{
				float Radius;
				float HalfHeight;
			} Cylinder;

			struct
			{
				float MajorRadius;  // distance from torus center to tube center
				float MinorRadius;  // radius of the tube itself
			} Torus;

			struct
			{
				float Radius;
				float HalfHeight;
			} Cone;

			struct
			{
				float TopRadius;     // radius at top — 0 makes a cone
				float BottomRadius;  // radius at bottom
				float HalfHeight;
			} TaperedCylinder;

			struct
			{
				float TopRadius;
				float BottomRadius;
				float HalfHeight;
			} TaperedCapsule;
			// Explicit default constructor to initialize the union
			ShapeUnion() : AABB{ {0,0,0} } {}

			~ShapeUnion() {}
		} Shape;

		// Collision STARTS - called once
		std::function<void(BackBone::Entity, BackBone::SystemContext&)> OnEnter = [](BackBone::Entity, BackBone::SystemContext&) {};

		// Collision CONTINUES - called every frame (usually unused)
		std::function<void(BackBone::Entity, BackBone::SystemContext&)> OnStay = [](BackBone::Entity, BackBone::SystemContext&) {};;

		// Collision ENDS - called once
		std::function<void(BackBone::Entity, BackBone::SystemContext&)> OnExit = [](BackBone::Entity, BackBone::SystemContext&) {};;

		// Helper method to handle Union Copying
		void CopyShapeFrom(const Collider& other)
		{
			Type = other.Type;
			switch (other.Type)
			{
			case ColliderType::BOX:              Shape.AABB = other.Shape.AABB; break;
			case ColliderType::SPHERE:           Shape.Sphere = other.Shape.Sphere; break;
			case ColliderType::CAPSULE:          Shape.Capsule = other.Shape.Capsule; break;
			case ColliderType::CYLINDER:         Shape.Cylinder = other.Shape.Cylinder; break;
			case ColliderType::TORUS:            Shape.Torus = other.Shape.Torus; break;
			case ColliderType::CONE:             Shape.Cone = other.Shape.Cone; break;
			case ColliderType::TAPERED_CYLINDER: Shape.TaperedCylinder = other.Shape.TaperedCylinder; break;
			case ColliderType::TAPERED_CAPSULE:  Shape.TaperedCapsule = other.Shape.TaperedCapsule; break;
			}
		}
		// Default constructor
		Collider() : Type(ColliderType::BOX), IsTrigger(false), Shape() {}
		~Collider() {}

		// Copy constructor
		Collider(const Collider& other) : IsTrigger(other.IsTrigger), OnEnter(other.OnEnter), OnStay(other.OnStay), OnExit(other.OnExit)
		{
			CopyShapeFrom(other);
		}

		// Move constructor
		Collider(Collider&& other) noexcept : IsTrigger(other.IsTrigger), OnEnter(std::move(other.OnEnter)), OnStay(std::move(other.OnStay)), OnExit(std::move(other.OnExit))
		{
			CopyShapeFrom(other);
		}

		// Copy assignment operator
		Collider& operator=(const Collider& other)
		{
			if (this != &other)
			{
				IsTrigger = other.IsTrigger;
				OnEnter = other.OnEnter;
				OnStay = other.OnStay;
				OnExit = other.OnExit;
				CopyShapeFrom(other);
			}
			return *this;
		}

		// Move assignment operator
		Collider& operator=(Collider&& other) noexcept
		{
			if (this != &other)
			{
				IsTrigger = other.IsTrigger;
				OnEnter = std::move(other.OnEnter);
				OnStay = std::move(other.OnStay);
				OnExit = std::move(other.OnExit);
				CopyShapeFrom(other);
			}
			return *this;
		}
	};

	struct JoltPhysicsExtensionConfig
	{
		bool Enabled = true;
		uint32_t UpFrontMemoryAllocated = 10 * 1024 * 1024;
		uint32_t MaxRigidBodies = 1024;
		uint32_t NumBodyMutexes = 0;
		uint32_t MaxBodyPairs = 1024;
		uint32_t MaxContactConstraints = 1024;
		uint32_t MaxPhysicsJobs = 2048;
		uint32_t MaxPhysicsBarriers = 8;
		uint32_t NumThreads = 1;
		float DeafultLinearDamping = 0.3f;
		float DeafultRestitution = 0.3f;
		float DeafultFriction = 0.3f;
		Vec3 Graivty{ 0.0f, -9.81f, 0.0f };

		uint8_t CollisionMatrix[int(Layers::NUM_LAYERS)][int(Layers::NUM_LAYERS)];
		BroadPhaseLayers LayerToBP[int(Layers::NUM_LAYERS)];

		JoltPhysicsExtensionConfig()
		{
			// Zero everything first
			memset(CollisionMatrix, 0, sizeof(CollisionMatrix));
			memset(LayerToBP, 0, sizeof(LayerToBP));

			SetupDefaultCollisionMatrix();
			SetupDefaultBPMap();
		}

		void SetupDefaultCollisionMatrix()
		{
			// Helper to set symmetric pair
			auto Enable = [&](Layers A, Layers B)
				{
					CollisionMatrix[int(A)][int(B)] = true;
					CollisionMatrix[int(B)][int(A)] = true;
				};

			// Static world collides with everything except itself and sensors
			Enable(Layers::STATIC, Layers::DYNAMIC);
			Enable(Layers::STATIC, Layers::PLAYER);
			Enable(Layers::STATIC, Layers::ENEMIES);
			Enable(Layers::STATIC, Layers::PROJECTILE);
			Enable(Layers::STATIC, Layers::DEBRIS);
			Enable(Layers::STATIC, Layers::DEFAULT);

			// Dynamic collides with dynamic and characters
			Enable(Layers::DYNAMIC, Layers::DYNAMIC);
			Enable(Layers::DYNAMIC, Layers::PLAYER);
			Enable(Layers::DYNAMIC, Layers::ENEMIES);
			Enable(Layers::DYNAMIC, Layers::PROJECTILE);
			Enable(Layers::DYNAMIC, Layers::DEBRIS);

			// Player collides with enemies and projectiles
			Enable(Layers::PLAYER, Layers::ENEMIES);
			Enable(Layers::PLAYER, Layers::PROJECTILE);

			// Enemies collide with projectiles
			Enable(Layers::ENEMIES, Layers::PROJECTILE);

			// Sensors collide with player and enemies only
			Enable(Layers::SENSOR, Layers::PLAYER);
			Enable(Layers::SENSOR, Layers::ENEMIES);

			// Debris only hits static world
			Enable(Layers::DEBRIS, Layers::STATIC);

			// Custom layers default to colliding with everything
			for (int i = int(Layers::CUSTOM_0); i < int(Layers::NUM_LAYERS); i++)
				for (int j = 0; j < int(Layers::NUM_LAYERS); j++)
				{
					CollisionMatrix[i][j] = true;
					CollisionMatrix[j][i] = true;
				}
		}

		void SetupDefaultBPMap()
		{
			// Static layers → BP STATIC bucket
			LayerToBP[int(Layers::STATIC)] = BroadPhaseLayers::STATIC;

			// Sensor → BP SENSOR bucket
			LayerToBP[int(Layers::SENSOR)] = BroadPhaseLayers::SENSOR;

			// Everything else → BP DYNAMIC bucket
			LayerToBP[int(Layers::DEFAULT)] = BroadPhaseLayers::DYNAMIC;
			LayerToBP[int(Layers::DYNAMIC)] = BroadPhaseLayers::DYNAMIC;
			LayerToBP[int(Layers::PLAYER)] = BroadPhaseLayers::DYNAMIC;
			LayerToBP[int(Layers::ENEMIES)] = BroadPhaseLayers::DYNAMIC;
			LayerToBP[int(Layers::PROJECTILE)] = BroadPhaseLayers::DYNAMIC;
			LayerToBP[int(Layers::DEBRIS)] = BroadPhaseLayers::DYNAMIC;

			for (int i = int(Layers::CUSTOM_0); i < int(Layers::NUM_LAYERS); i++)
				LayerToBP[i] = BroadPhaseLayers::DYNAMIC;
		}

		// Convenience — user calls this to override pairs
		void SetCollides(Layers A, Layers B, bool Collides)
		{
			CollisionMatrix[int(A)][int(B)] = Collides;
			CollisionMatrix[int(B)][int(A)] = Collides;
		}
	};

	struct JoltPhysicsResource
	{
		void* Data = nullptr;
	};

	// ---- Basic Extensions ----
	class JoltPhysicsExtension : public BackBone::Extension
	{
	public:
		JoltPhysicsExtension(const JoltPhysicsExtensionConfig& Config = JoltPhysicsExtensionConfig())
			:_Config(Config)
		{
		}
		~JoltPhysicsExtension() {}

		virtual void Build(BackBone::App& App) override;

		virtual const char* Name() const override { return "JoltPhysicsExtension"; }
	private:
		JoltPhysicsExtensionConfig _Config;
	};

	void OnJoltSetup(BackBone::SystemContext& Ctxt);
	void OnJoltTerminate(BackBone::SystemContext& Ctxt);

#pragma endregion JoltPhysics

	struct DeafultExtensionConfig
	{
		RenderExtensionConfig RenderConfig{};
		WindowExtensionConfig WindowConfig{};
		PepperExtensionConfig PepperConfig{};
		JoltPhysicsExtensionConfig SimPhysicsConfig;

		struct {
			bool EnableFastNoise2Provider = true;
		} NoiseExtensionConfig;

		DeafultExtensionConfig()
		{
			SimPhysicsConfig.Enabled = true;
		}
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

#pragma region Command
	class Command
	{
	public:
		Command() {}
		Command(const BackBone::SystemContext& Ctxt) :_Ctxt(Ctxt) {}
		~Command() {}

		// Render Services Related
		BackBone::AssetHandle<Mesh> CreateSphere(int XSegments, int YSegments);
		BackBone::AssetHandle<Mesh> CreateCylinder(int Segments, float Radius, float Height);
		BackBone::AssetHandle<Mesh> CreateCapsule(int Segments, float Radius, float Height);
		BackBone::AssetHandle<Mesh> CreateTorus(int MajorSegments, int MinorSegments,
			float MajorRadius, float MinorRadius);
		BackBone::AssetHandle<Mesh> CreateCone(int Segments, float Radius, float Height);

		// Dynamic Mesh Creation
		BackBone::AssetHandle<Mesh> CreateMesh(uint32_t VertexCount,
			uint32_t IndexCount, IndexBufferType  Type, VertexInputShaderLayout Layout);

		std::vector<uint8_t> RebuildVertexBuffer(
			const Chilli::RawMeshData& Raw,
			const Chilli::VertexInputShaderLayout& Desired);

		BackBone::AssetHandle<Mesh> CreateMesh(const MeshCreateInfo& Info);
		BackBone::AssetHandle<Mesh> CreateMesh(BasicShapes Shape);
		BackBone::AssetHandle<Mesh> CreateMesh(BackBone::AssetHandle<RawMeshData> Data, const VertexInputShaderLayout& DesiredLayout);
		BackBone::AssetHandle<MeshLoaderData> LoadMesh(const std::string& Path);

		void DestroyMesh(BackBone::AssetHandle<Mesh> mesh, bool Free = false);
		void FreeMesh(BackBone::AssetHandle<Mesh> mesh);

		void MapMeshVertexBufferData(BackBone::AssetHandle<Mesh> Handle, uint32_t Binding, void* Data, uint32_t Count, size_t Size, uint32_t Offset);
		void MapMeshIndexBufferData(BackBone::AssetHandle<Mesh> Handle, void* Data, uint32_t Count, size_t Size, uint32_t Offset);

		void MakeNoiseTextureData(uint32_t Handle, uint8_t* Texture, uint32_t Width, uint32_t Height, bool ColorR = true, bool ColorG = true, bool ColorB = true);

		void GeneratePlane(int ResolutionX, int ResolutionZ, std::vector<Chilli::Vertex>& OutVerts, std::vector<uint32_t>& OutIndices);
		void GenerateSphere(int XSegments, int YSegments, std::vector<Chilli::Vertex>& OutVerts, std::vector<uint32_t>& OutIndices);
		void GenerateCylinder(int Segments, float Radius, float Height,
			std::vector<Chilli::Vertex>& OutVerts,
			std::vector<uint32_t>& OutIndices);
		void GenerateTorus(int MajorSegments, int MinorSegments,
			float MajorRadius, float MinorRadius,
			std::vector<Chilli::Vertex>& OutVerts,
			std::vector<uint32_t>& OutIndices);
		void GenerateCone(int Segments, float Radius, float Height,
			std::vector<Chilli::Vertex>& OutVerts,
			std::vector<uint32_t>& OutIndices);
		void GenerateCapsule(int Segments, float Radius, float Height, std::vector<Chilli::Vertex>& OutVerts, std::vector<uint32_t>& OutIndices);

		void Displace(std::vector<Chilli::Vertex>& ModelVerts, const std::vector<uint32_t>& Indices, float Strength, float Limit);
		void RecalculateNormals(std::vector<Chilli::Vertex>& Verts, const std::vector<uint32_t>& Indices);

		void DisplaceNoise(
			std::vector<Chilli::Vertex>& Verts,
			const std::vector<uint32_t>& Indices,
			uint32_t                        Handle,
			float                           Scale,
			float                           Strength,
			float                           OffsetX,
			float                           OffsetY,
			float                           OffsetZ,
			float                           FloorY,
			bool                            EnableX,
			bool                            EnableY,
			bool                            EnableZ);

		std::vector<Chilli::Vertex> RawVertexToVertex(const std::vector<uint8_t>& RawVertices, const Chilli::VertexInputShaderLayout& Layout);

		std::vector<uint32_t> RawIndicesToIndices(const std::vector<uint8_t>& RawIndices);

		void UpdateDynamicMesh(uint8_t* VertexData);

		InputResult GetKeyState(Input_key KeyCode) {
			return GetService<Input>()->GetKeyState(KeyCode);
		}

		InputResult GetMouseButtonState(Input_mouse ButtonCode) {
			return GetService<Input>()->GetMouseButtonState(ButtonCode);
		}
		bool GetModState(Input_mod mod) {
			return GetService<Input>()->GetModState(mod);
		}

		bool IsModActive(Input_mod mod) {
			return GetService<Input>()->IsModActive(mod);
		}

		IVec2 GetCursorPos() {
			return GetService<Input>()->GetCursorPos();
		}

		IVec2 GetCursorDelta() {
			return GetService<Input>()->GetCursorDelta();
		}

		bool IsKeyPressed(Input_key key) {
			return GetService<Input>()->IsKeyPressed(key);
		}

		bool IsKeyDown(Input_key key) {
			return GetService<Input>()->IsKeyDown(key);
		}

		bool IsKeyReleased(Input_key key) {
			return GetService<Input>()->IsKeyReleased(key);
		}

		bool IsMouseButtonPressed(Input_mouse button) {
			return GetService<Input>()->IsMouseButtonPressed(button);
		}

		bool IsMouseButtonDown(Input_mouse button) {
			return GetService<Input>()->IsMouseButtonDown(button);
		}

		bool IsMouseButtonRelease(Input_mouse button) {
			return GetService<Input>()->IsMouseButtonRelease(button);
		}

		const char* InputKeyToString(Input_key Key)
		{
			return GetService<Input>()->KeyToString(Key);
		}

		const char* InputMouseToString(Input_mouse Mouse)
		{
			return GetService<Input>()->MouseToString(Mouse);
		}

		template<typename _EventType>
			requires std::derived_from<_EventType, Event>
		void RegisterEvent()
		{
			auto EventService = GetService<EventHandler>();
			EventService->Register<_EventType>();
		}

		template<typename _EventType>
			requires std::derived_from<_EventType, Event>
		void ClearEvent()
		{
			auto EventService = GetService<EventHandler>();
			EventService->Clear<_EventType>();
		}

		template<typename _EventType>
			requires std::derived_from<_EventType, Event>
		PerEventStorage<_EventType>* GetEventStorage()
		{
			auto EventService = GetService<EventHandler>();
			return static_cast<PerEventStorage<_EventType>*>(EventService->GetEventStorage<_EventType>());
		}

		template<typename _EventType>
			requires std::derived_from<_EventType, Event>
		void AddEvent(const _EventType& e)
		{
			auto EventService = GetService<EventHandler>();
			EventService->Add(e);
		}

		uint32_t CreateEntity();
		uint32_t GetEntityGeneration(uint32_t EntityID);
		void DestroyEntity(uint32_t EntityID);
		bool IsEntityValid(uint32_t EntityID);

		void GenerateCircleImageData(
			uint32_t* pixels,
			int width,
			int height,
			int cx, int cy,       // center of circle
			int radius,
			uint32_t color,       // circle color (RGBA)
			uint32_t bg_color     // background color (RGBA)
		);

		BackBone::AssetHandle<Scene> CreateScene();
		void SetActiveScene(BackBone::AssetHandle<Scene> Scene);

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
		BackBone::AssetHandle<Material> CreateMaterial();
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

		BackBone::AssetHandle<FlameFont> LoadFont_Msdf(const std::string& Path, const std::string& AtlasPath);
		void UnLoadFont_Msdf(BackBone::AssetHandle<FlameFont> FontHandle);
		// Window Services Related
		uint32_t SpwanWindow(const WindowSpec& Spec);
		void DestroyWindow(uint32_t Idx);

		void SetParentEntity(BackBone::Entity Child, BackBone::Entity Parent);

		BackBone::AssetHandle<ShaderProgram> GetDeafultShaderProgram() {
			return GetResource<RenderResource>()->DeafultShaderProgram;
		}

		BackBone::GenericFrameData* GetGenericFrameData();

		float GetPhysicsDeltaTime() {
			return GetGenericFrameData()->FixedPhysicsData.Ticks;
		}

		float GetNetWorkDeltaTime() {
			return GetGenericFrameData()->FixedNetWorkData.Ticks;
		}

		float GetTriggerDeltaTime() {
			return GetGenericFrameData()->FixedTriggerData.Ticks;
		}

		float GetAIDeltaTime() {
			return GetGenericFrameData()->FixedAIData.Ticks;
		}

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
			return _Ctxt.Registry->GetComponents<Ts...>(entity);
		}

		template<typename... Ts>
		std::tuple<const Ts*...> GetComponents(BackBone::Entity  entity) const
		{
			return _Ctxt.Registry->GetComponents<Ts...>(entity);
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

		BackBone::SystemContext& Ctxt() { return _Ctxt; }

		Window* GetActiveWindow();
	private:
		void _Setup(const BackBone::SystemContext& Ctxt);
	private:
		BackBone::SystemContext _Ctxt;
	};
#pragma endregion 
}
