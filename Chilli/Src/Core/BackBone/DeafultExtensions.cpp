#include "DeafultExtensions.h"
#include "Ch_PCH.h"
#include "DeafultExtensions.h"
#include "Window/Window.h"
#include "Profiling\Timer.h"

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

#if CHILLI_ENABLE_JOLT == true
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#endif

namespace Chilli
{
#pragma region Deafult Extension
	void OnTransformComponentParentChild(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Table = Command.GetService< ParentChildMapTable>();

		for (auto [Entity, Transform] : BackBone::QueryWithEntities<TransformComponent>(*Ctxt.Registry))
		{
			if (Transform->HasParent())
			{
				auto IsOurCurrentParent = Table->IsChildOf(Transform->GetParent(), Entity);
				if (!IsOurCurrentParent)
				{
					auto ChildMap = Table->ChildParentMap.Get(Entity);

					// First Time being a child
					if (ChildMap == nullptr)
						Table->PushChild(Transform->GetParent(), Entity);
					else {
						auto OldParent = *Table->ChildParentMap.Get(Entity);

						Table->PushChild(Transform->GetParent(), Entity);
						Table->EraseChild(OldParent.Parent, Entity);
					}
				}
			}
		}
	}

	void HandleParentChildTransformRecursive(BackBone::SystemContext& Ctxt, BackBone::Entity Entity, const glm::mat4& ParentWorldMatrix, bool IsParentDirty)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Table = Command.GetService< ParentChildMapTable>();

		auto TransformComp = Command.GetComponent<TransformComponent>(Entity);
		auto IsThisParentDirty = TransformComp->IsDirty();

		TransformComp->UpdateWorldMatrix(ParentWorldMatrix, IsParentDirty);

		if (Table->IsParent(Entity))
		{
			if (Table->GetChildMap(Entity) != nullptr)
				for (auto& Child : *Table->GetChildMap(Entity))
					HandleParentChildTransformRecursive(Ctxt, Child, ParentWorldMatrix, IsThisParentDirty);
		}
	}

	void HandleParentChildTransform(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Table = Command.GetService< ParentChildMapTable>();

		for (auto [Entity, Transform] : BackBone::QueryWithEntities<TransformComponent>(*Ctxt.Registry))
		{
			if (Transform->HasParent() == false)
			{
				auto ChildMap = Table->GetChildMap(Entity);
				bool ParentDirty = Transform->IsDirty();
				Transform->UpdateWorldMatrix(glm::mat4(1.0f), false);
				auto ParentWorldMatrix = Transform->GetWorldMatrix();

				if (ChildMap != nullptr)
					for (auto& Child : *ChildMap)
						HandleParentChildTransformRecursive(Ctxt, Child, ParentWorldMatrix, ParentDirty);
			}
		}
	}

	void DeafultExtension::Build(BackBone::App& App)
	{
		App.Registry.AddResource<ParentChildMapTable>();
		App.Registry.Register<Chilli::TransformComponent>();

		_Config.PepperConfig.MaxFramesInFlight = _Config.RenderConfig.Spec.MaxFrameInFlight;

		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::UPDATE, OnTransformComponentParentChild);
		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::UPDATE, HandleParentChildTransform);

		App.Extensions.AddExtension(std::make_unique<WindowExtension>(_Config.WindowConfig), true, &App);
		App.Extensions.AddExtension(std::make_unique<RenderExtension>(_Config.RenderConfig), true, &App);
		App.Extensions.AddExtension(std::make_unique<CameraExtension>(), true, &App);
		App.Extensions.AddExtension(std::make_unique<PepperExtension>(_Config.PepperConfig), true, &App);

		if (_Config.SimPhysicsConfig.Enabled)
		{
			App.Extensions.AddExtension(std::make_unique<JoltPhysicsExtension>(_Config.SimPhysicsConfig), true, &App);
		}
		if (_Config.NoiseExtensionConfig.EnableFastNoise2Provider)
		{
			//App.ServiceRegistry.RegisterService<FastNoiseProvider>(std::make_shared<FastNoiseProvider>());
		}
		// Services etc...
	}

#pragma endregion

#pragma region Window System
	void WindowManager::InitDefaultCursors(BackBone::AssetStore<Cursor>* CursorStore)
	{
		Window::Setup();
		for (int i = 0; i < int(DeafultCursorTypes::Count); i++)
		{
			Cursor DeafultCursor;
			DeafultCursor.RawCursorHandle = Window::CreateCursor(DeafultCursorTypes(i));
			_DeafultCursors[i] = CursorStore->Add(DeafultCursor);
		}
	}

	void OnWindowStartUp(BackBone::SystemContext& Registry)
	{
		auto InputService = Registry.ServiceRegistry->GetService<Input>();
		auto EventService = Registry.ServiceRegistry->GetService<EventHandler>();

		{
			EventService->Register<WindowResizeEvent>();
			EventService->Register<WindowMinimizedEvent>();
			EventService->Register<WindowCloseEvent>();
			EventService->Register<FrameBufferResizeEvent>();
			EventService->Register<WindowCloseEvent>();
			EventService->Register<KeyPressedEvent>();
			EventService->Register<KeyRepeatEvent>();
			EventService->Register<KeyReleasedEvent>();
			EventService->Register<MouseButtonPressedEvent>();
			EventService->Register<MouseButtonRepeatEvent>();
			EventService->Register<MouseButtonReleasedEvent>();
			EventService->Register<CursorPosEvent>();
			EventService->Register<MouseScrollEvent>();
			EventService->Register< SetCursorEvent>();
		}
	}

	void OnWindowRun(BackBone::SystemContext& Registry)
	{
		auto InputService = Registry.ServiceRegistry->GetService<Input>();
		auto EventService = Registry.ServiceRegistry->GetService<EventHandler>();
		auto WindowService = Registry.ServiceRegistry->GetService<WindowManager>();

		EventService->Clear<WindowResizeEvent>();
		EventService->Clear<WindowMinimizedEvent>();
		EventService->Clear<WindowCloseEvent>();
		EventService->Clear<FrameBufferResizeEvent>();
		EventService->Clear<WindowCloseEvent>();
		EventService->Clear<KeyPressedEvent>();
		EventService->Clear<KeyRepeatEvent>();
		EventService->Clear<KeyReleasedEvent>();
		EventService->Clear<MouseButtonPressedEvent>();
		EventService->Clear<MouseButtonRepeatEvent>();
		EventService->Clear<MouseButtonReleasedEvent>();
		EventService->Clear<CursorPosEvent>();
		EventService->Clear<MouseScrollEvent>();

		WindowService->Update();
		InputService->UpdateEvents(EventService);

		auto Read = EventService->GetEventStorage<WindowCloseEvent>();
		auto FrameData = Registry.Registry->GetResource<BackBone::GenericFrameData>();
		if (Read->GetActiveSize() > 0)
		{
			FrameData->IsRunning = false;
		}

		if (InputService->IsKeyDown(Input_key_W) && InputService->IsModActive(Input_mod_Shift))
			CH_CORE_TRACE("Pressed W");
		if (InputService->IsMouseButtonDown(Input_mouse_Left) && InputService->IsModActive(Input_mod_Shift))
			CH_CORE_TRACE("Mouse");
	}

	void OnWindowShutDown(BackBone::SystemContext& Registry)
	{
		auto WindowService = Registry.ServiceRegistry->GetService<WindowManager>();
		WindowService->Clear();

		auto InputService = Registry.ServiceRegistry->GetService<Input>();
		InputService->Terminate();
	}

	void WindowExtension::Build(BackBone::App& App)
	{
		App.AssetRegistry.RegisterStore<Cursor>();

		App.ServiceRegistry.RegisterService<Chilli::Input>(std::make_shared<Chilli::Input>());
		App.ServiceRegistry.RegisterService< EventHandler>(std::make_shared< EventHandler>());
		App.ServiceRegistry.RegisterService< WindowManager>(std::make_shared<WindowManager>(App.AssetRegistry.GetStore<Cursor>()));

		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::START_UP, OnWindowStartUp);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::INPUT, OnWindowRun);
		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::SHUTDOWN, OnWindowShutDown);
	}
#pragma endregion 

#pragma region Command

	BackBone::AssetHandle<Mesh> Command::CreateSphere(int XSegments, int YSegments)
	{
		std::vector<Chilli::Vertex> Vertices;
		std::vector<uint32_t> Indices;

		GenerateSphere(XSegments, YSegments, Vertices, Indices);

		VertexInputShaderLayout Layout;
		Layout.BeginBinding(0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InNormal", 1);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 2);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InColor", 3);

		Chilli::MeshCreateInfo Info{};
		Info.VertCount = Vertices.size();
		Info.Vertices = Vertices.data();
		Info.IndexCount = Indices.size();
		Info.Indicies = Indices.data();
		Info.IndexType = Chilli::IndexBufferType::UINT32_T;
		Info.MeshLayout = Layout;
		Info.IndexBufferState = BufferState::STATIC_DRAW;

		// Use your existing CreateMesh(MeshCreateInfo) to talk to the GPU
		return CreateMesh(Info);
	}

	BackBone::AssetHandle<Mesh> Command::CreateCylinder(int Segments, float Radius, float Height)
	{
		std::vector<Chilli::Vertex>  Vertices;
		std::vector<uint32_t>        Indices;
		GenerateCylinder(Segments, Radius, Height, Vertices, Indices);

		VertexInputShaderLayout Layout;
		Layout.BeginBinding(0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InNormal", 1);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 2);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InColor", 3);

		MeshCreateInfo Info{};
		Info.VertCount = Vertices.size();
		Info.Vertices = Vertices.data();
		Info.IndexCount = Indices.size();
		Info.Indicies = Indices.data();
		Info.IndexType = IndexBufferType::UINT32_T;
		Info.MeshLayout = Layout;
		Info.IndexBufferState = BufferState::STATIC_DRAW;
		return CreateMesh(Info);
	}

	BackBone::AssetHandle<Mesh> Command::CreateCapsule(int Segments, float Radius, float Height)
	{
		std::vector<Chilli::Vertex>  Vertices;
		std::vector<uint32_t>        Indices;
		GenerateCapsule(Segments, Radius, Height, Vertices, Indices);

		VertexInputShaderLayout Layout;
		Layout.BeginBinding(0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InNormal", 1);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 2);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InColor", 3);

		MeshCreateInfo Info{};
		Info.VertCount = Vertices.size();
		Info.Vertices = Vertices.data();
		Info.IndexCount = Indices.size();
		Info.Indicies = Indices.data();
		Info.IndexType = IndexBufferType::UINT32_T;
		Info.MeshLayout = Layout;
		Info.IndexBufferState = BufferState::STATIC_DRAW;
		return CreateMesh(Info);
	}

	BackBone::AssetHandle<Mesh> Command::CreateTorus(int MajorSegments, int MinorSegments, float MajorRadius, float MinorRadius)
	{
		std::vector<Chilli::Vertex>  Vertices;
		std::vector<uint32_t>        Indices;
		GenerateTorus(MajorSegments, MinorSegments, MajorRadius, MinorRadius, Vertices, Indices);

		VertexInputShaderLayout Layout;
		Layout.BeginBinding(0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InNormal", 1);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 2);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InColor", 3);

		MeshCreateInfo Info{};
		Info.VertCount = Vertices.size();
		Info.Vertices = Vertices.data();
		Info.IndexCount = Indices.size();
		Info.Indicies = Indices.data();
		Info.IndexType = IndexBufferType::UINT32_T;
		Info.MeshLayout = Layout;
		Info.IndexBufferState = BufferState::STATIC_DRAW;
		return CreateMesh(Info);
	}

	BackBone::AssetHandle<Mesh> Command::CreateCone(int Segments, float Radius, float Height)
	{
		std::vector<Chilli::Vertex>  Vertices;
		std::vector<uint32_t>        Indices;
		GenerateCone(Segments, Radius, Height, Vertices, Indices);

		VertexInputShaderLayout Layout;
		Layout.BeginBinding(0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InNormal", 1);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 2);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InColor", 3);

		MeshCreateInfo Info{};
		Info.VertCount = Vertices.size();
		Info.Vertices = Vertices.data();
		Info.IndexCount = Indices.size();
		Info.Indicies = Indices.data();
		Info.IndexType = IndexBufferType::UINT32_T;
		Info.MeshLayout = Layout;
		Info.IndexBufferState = BufferState::STATIC_DRAW;
		return CreateMesh(Info);
	}

	BackBone::AssetHandle<Mesh> Command::CreateMesh(uint32_t VertexCount, uint32_t IndexCount
		, IndexBufferType Type, VertexInputShaderLayout Layout)
	{
		return BackBone::AssetHandle<Mesh>();
	}

	BackBone::AssetHandle<Mesh> Command::CreateMesh(const MeshCreateInfo& Info)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<Chilli::RenderCommand>();
		auto MeshStore = _Ctxt.AssetRegistry->GetStore<Chilli::Mesh>();
		CH_CORE_ASSERT(MeshStore != nullptr, "NO MESH STORE: NOPE");
		CH_CORE_ASSERT(RenderCommandService != nullptr, "NO RENDER COMMAAND SERVICE: NOPE");
		CH_CORE_ASSERT(Info.VertCount != 0, "A Vertex Count is Needed: NOPE");
		CH_CORE_ASSERT(Info.MeshLayout.Bindings.size() > 0, "Mesh layout Needs bindings");

		Chilli::Mesh NewMesh;
		NewMesh.IndexBufferState = Info.IndexBufferState;
		NewMesh.MeshLayout = Info.MeshLayout;
		// The "highest" binding index defines how many slots we actually need to bind in Vulkan
		uint32_t MaxBindingIndex = 0;

		// Iterate through layout bindings to create a buffer for EACH one
		for (const auto& Binding : NewMesh.MeshLayout.Bindings)
		{
			uint32_t BIdx = Binding.BindingIndex;
			CH_CORE_ASSERT(BIdx < 16, "Binding index exceeds limit of 16");

			if (BIdx > MaxBindingIndex) MaxBindingIndex = BIdx;

			// Determine if we multiply stride by VertexCount or InstanceCount
			uint32_t ElementCount = Binding.IsInstanced ? Info.InstanceCount : Info.VertCount;
			uint32_t TotalBufferSize = Binding.Stride * ElementCount;

			BufferCreateInfo VBInfo;
			VBInfo.State = Binding.State;
			VBInfo.Type = BufferType::BUFFER_TYPE_VERTEX;
			VBInfo.SizeInBytes = TotalBufferSize;

			// Pass initial data only if it's the primary binding and data exists
			// Otherwise, create an empty buffer for dynamic/manual filling
			VBInfo.Data = (BIdx == 0) ? Info.Vertices : nullptr;

			// Store handle at the exact index specified by the layout
			NewMesh.VertexBufferHandles[BIdx] = this->CreateBuffer(VBInfo,
				std::string("VB_Slot_" + std::to_string(BIdx)).c_str());
		}

		NewMesh.ActiveVBHandlesCount = MaxBindingIndex + 1;
		NewMesh.VertexCount = Info.VertCount;

		if (Info.IndexCount > 0)
		{
			CH_CORE_ASSERT(Info.IndexBufferState != BufferState::NONE, "Mesh layout Needs bindings");

			BufferCreateInfo IndexBufferInfo;
			IndexBufferInfo.State = Info.IndexBufferState;
			IndexBufferInfo.Data = Info.Indicies;
			IndexBufferInfo.Type = BufferType::BUFFER_TYPE_INDEX;

			if (Info.IndexType == IndexBufferType::UINT16_T)
				IndexBufferInfo.SizeInBytes = sizeof(uint16_t) * Info.IndexCount;
			if (Info.IndexType == IndexBufferType::UINT32_T)
				IndexBufferInfo.SizeInBytes = sizeof(uint32_t) * Info.IndexCount;

			NewMesh.IBHandle = this->CreateBuffer(IndexBufferInfo, "IndexBuffer");
			NewMesh.IBType = Info.IndexType;
			NewMesh.IndexCount = Info.IndexCount;
		}

		return MeshStore->Add(NewMesh);
	}

	void Command::MapMeshVertexBufferData(BackBone::AssetHandle<Mesh> Handle, uint32_t Binding, void* Data, uint32_t Count, size_t Size, uint32_t Offset)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<Chilli::RenderCommand>();
		auto MeshStore = _Ctxt.AssetRegistry->GetStore<Chilli::Mesh>();
		CH_CORE_ASSERT(MeshStore != nullptr, "NO MESH STORE: NOPE");
		CH_CORE_ASSERT(RenderCommandService != nullptr, "NO RENDER COMMAAND SERVICE: NOPE");

		auto Mesh = MeshStore->Get(Handle);

		CH_CORE_ASSERT(Mesh != nullptr, "Mesh Not Found");
		CH_CORE_ASSERT(Binding < 16, "Binding Over Limit");
		CH_CORE_ASSERT(Mesh->VertexBufferHandles[Binding].ValPtr->CreateInfo.SizeInBytes >= Size, "Given Size Exceeds Mesh Initialized Size");

		Mesh->VertexCount = Count;
		this->MapBufferData(Mesh->VertexBufferHandles[Binding], Data, Size, Offset);
	}

	void Command::MapMeshIndexBufferData(BackBone::AssetHandle<Mesh> Handle, void* Data, uint32_t Count,
		size_t Size, uint32_t Offset)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<Chilli::RenderCommand>();
		auto MeshStore = _Ctxt.AssetRegistry->GetStore<Chilli::Mesh>();
		CH_CORE_ASSERT(MeshStore != nullptr, "NO MESH STORE: NOPE");
		CH_CORE_ASSERT(RenderCommandService != nullptr, "NO RENDER COMMAAND SERVICE: NOPE");

		auto Mesh = MeshStore->Get(Handle);

		CH_CORE_ASSERT(Mesh != nullptr, "Mesh Not Found");
		CH_CORE_ASSERT(Mesh->IBHandle.ValPtr->CreateInfo.SizeInBytes >= Size, "Given Size Exceeds Mesh Initialized Size");

		Mesh->IndexCount = Count;
		this->MapBufferData(Mesh->IBHandle, Data, Size, Offset);
	}

	void Command::MakeNoiseTextureData(uint32_t Handle, uint8_t* Texture, uint32_t Width, uint32_t Height, bool ColorR, bool ColorG, bool ColorB)
	{
		auto NoiseProvider = GetService<Chilli::FastNoiseProvider>();
		NoiseProvider->GetGrid2D(Handle, (float*)Texture, 0, 0, Width, Height);

		for (int i = (Width * Height) - 1; i >= 0; i--)
		{
			float Color = ((float*)Texture)[i];
			uint8_t ActualColor = (uint8_t)((Color + 1.0f) * 0.5f * 255.0f);

			if (ColorR)
				Texture[i * sizeof(float) + 0] = ActualColor;
			else
				Texture[i * sizeof(float) + 0] = 0;

			if (ColorG)
				Texture[i * sizeof(float) + 1] = ActualColor;
			else
				Texture[i * sizeof(float) + 1] = 0;

			if (ColorB)
				Texture[i * sizeof(float) + 2] = ActualColor;
			else
				Texture[i * sizeof(float) + 2] = 0;

			Texture[i * sizeof(float) + 3] = 255;
		}
	}

	void Command::GeneratePlane(int ResolutionX, int ResolutionZ, std::vector<Chilli::Vertex>& OutVerts, std::vector<uint32_t>& OutIndices)
	{
		OutVerts.clear();
		OutIndices.clear();

		float StepX = 1.0f / ResolutionX;
		float StepZ = 1.0f / ResolutionZ;

		for (int z = 0; z <= ResolutionZ; z++)
		{
			for (int x = 0; x <= ResolutionX; x++)
			{
				Chilli::Vertex V;
				V.Position = { x * StepX, 0.0f, z * StepZ };
				V.Normal = { 0.0f, 1.0f, 0.0f };
				V.UV = { (float)x / ResolutionX, (float)z / ResolutionZ };
				OutVerts.push_back(V);
			}
		}

		for (int z = 0; z < ResolutionZ; z++)
		{
			for (int x = 0; x < ResolutionX; x++)
			{
				uint32_t TopLeft = z * (ResolutionX + 1) + x;
				uint32_t TopRight = TopLeft + 1;
				uint32_t BottomLeft = (z + 1) * (ResolutionX + 1) + x;
				uint32_t BottomRight = BottomLeft + 1;

				OutIndices.push_back(TopLeft);
				OutIndices.push_back(BottomLeft);
				OutIndices.push_back(TopRight);

				OutIndices.push_back(TopRight);
				OutIndices.push_back(BottomLeft);
				OutIndices.push_back(BottomRight);
			}
		}
	}

	void Command::GenerateSphere(int XSegments, int YSegments, std::vector<Chilli::Vertex>& Vertices, std::vector<uint32_t>& Indices)
	{
		const unsigned int X_SEGMENTS = XSegments;
		const unsigned int Y_SEGMENTS = YSegments;
		const float PI = 3.14159265359f;

		for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;

				// Calculate spherical coordinates
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI); // Top is 1, bottom is -1
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				Chilli::Vertex v;
				v.Position = { xPos * 0.5f, yPos * 0.5f, zPos * 0.5f }; // 0.5 radius
				v.Normal = { xPos, yPos, zPos }; // On a unit sphere, position = normal
				v.UV = { xSegment, ySegment };

				Vertices.push_back(v);
			}
		}

		// Generate Indices
		for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
		{
			for (unsigned int x = 0; x < X_SEGMENTS; ++x)
			{
				Indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				Indices.push_back(y * (X_SEGMENTS + 1) + x);
				Indices.push_back(y * (X_SEGMENTS + 1) + (x + 1));

				Indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				Indices.push_back(y * (X_SEGMENTS + 1) + (x + 1));
				Indices.push_back((y + 1) * (X_SEGMENTS + 1) + (x + 1));
			}
		}
	}

	void Command::GenerateCylinder(int Segments, float Radius, float Height, std::vector<Chilli::Vertex>& OutVerts, std::vector<uint32_t>& OutIndices)
	{
		OutVerts.clear();
		OutIndices.clear();

		float HalfHeight = Height * 0.5f;
		float Step = 2.0f * 3.14159265f / Segments;

		// Side vertices — two rings
		for (int i = 0; i <= Segments; i++)
		{
			float Angle = i * Step;
			float X = std::cos(Angle) * Radius;
			float Z = std::sin(Angle) * Radius;

			Chilli::Vec3 Normal = Chilli::Normalize(Chilli::Vec3{ X, 0, Z });

			// Bottom ring
			Chilli::Vertex Bottom;
			Bottom.Position = { X, -HalfHeight, Z };
			Bottom.Normal = Normal;
			Bottom.UV = { (float)i / Segments, 0.0f };
			OutVerts.push_back(Bottom);

			// Top ring
			Chilli::Vertex Top;
			Top.Position = { X, HalfHeight, Z };
			Top.Normal = Normal;
			Top.UV = { (float)i / Segments, 1.0f };
			OutVerts.push_back(Top);
		}

		// Side indices
		for (int i = 0; i < Segments; i++)
		{
			uint32_t B0 = i * 2;
			uint32_t T0 = i * 2 + 1;
			uint32_t B1 = (i + 1) * 2;
			uint32_t T1 = (i + 1) * 2 + 1;

			OutIndices.push_back(B0); OutIndices.push_back(B1); OutIndices.push_back(T0);
			OutIndices.push_back(T0); OutIndices.push_back(B1); OutIndices.push_back(T1);
		}

		// Cap centers
		uint32_t BottomCenter = OutVerts.size();
		Chilli::Vertex BC; BC.Position = { 0, -HalfHeight, 0 }; BC.Normal = { 0,-1,0 }; BC.UV = { 0.5f,0.5f };
		OutVerts.push_back(BC);

		uint32_t TopCenter = OutVerts.size();
		Chilli::Vertex TC; TC.Position = { 0, HalfHeight, 0 }; TC.Normal = { 0,1,0 }; TC.UV = { 0.5f,0.5f };
		OutVerts.push_back(TC);

		// Cap rim vertices — separate from side verts so normals are correct
		uint32_t BottomRimStart = OutVerts.size();
		for (int i = 0; i <= Segments; i++)
		{
			float Angle = i * Step;
			float X = std::cos(Angle) * Radius;
			float Z = std::sin(Angle) * Radius;

			Chilli::Vertex BV; BV.Position = { X,-HalfHeight,Z }; BV.Normal = { 0,-1,0 };
			BV.UV = { (std::cos(Angle) + 1) * 0.5f, (std::sin(Angle) + 1) * 0.5f };
			OutVerts.push_back(BV);
		}

		uint32_t TopRimStart = OutVerts.size();
		for (int i = 0; i <= Segments; i++)
		{
			float Angle = i * Step;
			float X = std::cos(Angle) * Radius;
			float Z = std::sin(Angle) * Radius;

			Chilli::Vertex TV; TV.Position = { X,HalfHeight,Z }; TV.Normal = { 0,1,0 };
			TV.UV = { (std::cos(Angle) + 1) * 0.5f, (std::sin(Angle) + 1) * 0.5f };
			OutVerts.push_back(TV);
		}

		// Cap indices
		for (int i = 0; i < Segments; i++)
		{
			// Bottom cap — winding flipped for downward normal
			OutIndices.push_back(BottomCenter);
			OutIndices.push_back(BottomRimStart + i + 1);
			OutIndices.push_back(BottomRimStart + i);

			// Top cap
			OutIndices.push_back(TopCenter);
			OutIndices.push_back(TopRimStart + i);
			OutIndices.push_back(TopRimStart + i + 1);
		}
	}

	void Command::GenerateCapsule(int Segments, float Radius, float Height, std::vector<Chilli::Vertex>& OutVerts, std::vector<uint32_t>& OutIndices)
	{
		OutVerts.clear();
		OutIndices.clear();

		// Ensure we don't have a height smaller than the diameter
		float CylinderHeight = std::max(0.0f, Height - (2.0f * Radius));
		float HalfCylHeight = CylinderHeight * 0.5f;
		int HalfSegments = Segments / 2; // Latitude steps for the caps
		float Step = 2.0f * 3.14159265f / Segments;

		// 1. Generate Vertices
		// We iterate through latitude (y-axis) then longitude (circular)
		for (int lat = 0; lat <= HalfSegments; lat++)
		{
			// Calculate vertical angle and Y position
			// Top cap goes from 0 to PI/2, Bottom cap from PI/2 to PI
			float theta = lat * (3.14159265f / 2.0f) / (HalfSegments * 0.5f);

			// We split the generation: 
			// lat 0 to HalfSegments/2 = Top Hemisphre
			// lat HalfSegments/2 to HalfSegments = Bottom Hemisphere

			float phi = lat * (3.14159265f / HalfSegments);
			float yOffset = (phi < 3.14159265f * 0.5f) ? HalfCylHeight : -HalfCylHeight;

			float currY = std::cos(phi) * Radius + yOffset;
			float ringRadius = std::sin(phi) * Radius;

			for (int lon = 0; lon <= Segments; lon++)
			{
				float angle = lon * Step;
				float x = std::cos(angle) * ringRadius;
				float z = std::sin(angle) * ringRadius;

				Chilli::Vertex v;
				v.Position = { x, currY, z };
				// Normal for a capsule is just the vector from the nearest 
				// point on the central line segment to the vertex
				v.Normal = Chilli::Normalize(Chilli::Vec3{ x, currY - yOffset, z });
				v.UV = { (float)lon / Segments, (float)lat / HalfSegments };
				OutVerts.push_back(v);
			}
		}

		// 2. Generate Indices (Grid-based stitching)
		for (int lat = 0; lat < HalfSegments; lat++)
		{
			for (int lon = 0; lon < Segments; lon++)
			{
				uint32_t first = (lat * (Segments + 1)) + lon;
				uint32_t second = first + Segments + 1;

				// Triangle 1
				OutIndices.push_back(first);
				OutIndices.push_back(first + 1);
				OutIndices.push_back(second);

				// Triangle 2
				OutIndices.push_back(second);
				OutIndices.push_back(first + 1);
				OutIndices.push_back(second + 1);
			}
		}
	}

	void Command::GenerateTorus(int MajorSegments, int MinorSegments, float MajorRadius, float MinorRadius, std::vector<Chilli::Vertex>& OutVerts, std::vector<uint32_t>& OutIndices)
	{
		OutVerts.clear();
		OutIndices.clear();

		float MajorStep = 2.0f * 3.14159265f / MajorSegments;
		float MinorStep = 2.0f * 3.14159265f / MinorSegments;

		for (int i = 0; i <= MajorSegments; i++)
		{
			float MajorAngle = i * MajorStep;
			float CosMajor = std::cos(MajorAngle);
			float SinMajor = std::sin(MajorAngle);

			for (int j = 0; j <= MinorSegments; j++)
			{
				float MinorAngle = j * MinorStep;
				float CosMinor = std::cos(MinorAngle);
				float SinMinor = std::sin(MinorAngle);

				// Position on the torus surface
				float X = (MajorRadius + MinorRadius * CosMinor) * CosMajor;
				float Y = MinorRadius * SinMinor;
				float Z = (MajorRadius + MinorRadius * CosMinor) * SinMajor;

				// Normal points from tube center to surface point
				float NX = CosMinor * CosMajor;
				float NY = SinMinor;
				float NZ = CosMinor * SinMajor;

				Chilli::Vertex V;
				V.Position = { X, Y, Z };
				V.Normal = Chilli::Normalize(Vec3{ NX, NY, NZ });
				V.UV = { (float)i / MajorSegments, (float)j / MinorSegments };
				OutVerts.push_back(V);
			}
		}

		// Indices
		for (int i = 0; i < MajorSegments; i++)
		{
			for (int j = 0; j < MinorSegments; j++)
			{
				uint32_t A = i * (MinorSegments + 1) + j;
				uint32_t B = A + 1;
				uint32_t C = A + (MinorSegments + 1);
				uint32_t D = C + 1;

				OutIndices.push_back(A); OutIndices.push_back(C); OutIndices.push_back(B);
				OutIndices.push_back(B); OutIndices.push_back(C); OutIndices.push_back(D);
			}
		}
	}

	void Command::GenerateCone(int Segments, float Radius, float Height, std::vector<Chilli::Vertex>& OutVerts, std::vector<uint32_t>& OutIndices)
	{
		OutVerts.clear();
		OutIndices.clear();

		float Step = 2.0f * 3.14159265f / Segments;
		float HalfHeight = Height * 0.5f;

		// Tip
		uint32_t TipIndex = 0;
		Chilli::Vertex Tip;
		Tip.Position = { 0, HalfHeight, 0 };
		Tip.Normal = { 0, 1, 0 };
		Tip.UV = { 0.5f, 1.0f };
		OutVerts.push_back(Tip);

		// Base rim — side normals point outward and slightly up
		float SlopNormal = Radius / std::sqrt(Radius * Radius + Height * Height);
		float UpNormal = Height / std::sqrt(Radius * Radius + Height * Height);

		for (int i = 0; i <= Segments; i++)
		{
			float Angle = i * Step;
			float X = std::cos(Angle) * Radius;
			float Z = std::sin(Angle) * Radius;

			Chilli::Vertex V;
			V.Position = { X, -HalfHeight, Z };
			V.Normal = Chilli::Normalize(Vec3{ X * SlopNormal, UpNormal, Z * SlopNormal });
			V.UV = { (float)i / Segments, 0.0f };
			OutVerts.push_back(V);
		}

		// Side indices
		for (int i = 0; i < Segments; i++)
		{
			OutIndices.push_back(TipIndex);
			OutIndices.push_back(1 + i);
			OutIndices.push_back(1 + i + 1);
		}

		// Base cap
		uint32_t BaseCenterIndex = OutVerts.size();
		Chilli::Vertex BaseCenter;
		BaseCenter.Position = { 0, -HalfHeight, 0 };
		BaseCenter.Normal = { 0, -1, 0 };
		BaseCenter.UV = { 0.5f, 0.5f };
		OutVerts.push_back(BaseCenter);

		uint32_t BaseRimStart = OutVerts.size();
		for (int i = 0; i <= Segments; i++)
		{
			float Angle = i * Step;
			float X = std::cos(Angle) * Radius;
			float Z = std::sin(Angle) * Radius;

			Chilli::Vertex V;
			V.Position = { X, -HalfHeight, Z };
			V.Normal = { 0, -1, 0 };
			V.UV = { (std::cos(Angle) + 1) * 0.5f, (std::sin(Angle) + 1) * 0.5f };
			OutVerts.push_back(V);
		}

		for (int i = 0; i < Segments; i++)
		{
			OutIndices.push_back(BaseCenterIndex);
			OutIndices.push_back(BaseRimStart + i + 1);
			OutIndices.push_back(BaseRimStart + i);
		}
	}

	void Command::Displace(std::vector<Chilli::Vertex>& ModelVerts, const std::vector<uint32_t>& Indices, float Strength, float Limit)
	{
		for (auto& Vertex : ModelVerts)
		{
			float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
			float Displacement = Chilli::Clamp(t * Strength, 0.0f, Limit);

			Vertex.Position.x += Vertex.Normal.x * Displacement;
			Vertex.Position.y += Vertex.Normal.y * Displacement;
			Vertex.Position.z += Vertex.Normal.z * Displacement;
		}

		for (auto& V : ModelVerts)
			V.Normal = Chilli::Vec3(0.0f);

		for (size_t i = 0; i < Indices.size(); i += 3)
		{
			auto& V0 = ModelVerts[Indices[i]];
			auto& V1 = ModelVerts[Indices[i + 1]];
			auto& V2 = ModelVerts[Indices[i + 2]];

			Chilli::Vec3 Edge1 = V1.Position - V0.Position;
			Chilli::Vec3 Edge2 = V2.Position - V0.Position;
			Chilli::Vec3 FaceNormal = Chilli::Cross(Edge1, Edge2);

			V0.Normal += FaceNormal;
			V1.Normal += FaceNormal;
			V2.Normal += FaceNormal;
		}

		for (auto& V : ModelVerts)
			V.Normal = Chilli::Normalize(V.Normal);
	}

	void Command::RecalculateNormals(std::vector<Chilli::Vertex>& Verts, const std::vector<uint32_t>& Indices)
	{
		for (auto& V : Verts)
			V.Normal = Chilli::Vec3(0.0f);

		for (size_t i = 0; i < Indices.size(); i += 3)
		{
			auto& V0 = Verts[Indices[i]];
			auto& V1 = Verts[Indices[i + 1]];
			auto& V2 = Verts[Indices[i + 2]];

			Chilli::Vec3 FaceNormal = Chilli::Cross(V1.Position - V0.Position, V2.Position - V0.Position);
			V0.Normal += FaceNormal;
			V1.Normal += FaceNormal;
			V2.Normal += FaceNormal;
		}

		for (auto& V : Verts)
			V.Normal = Chilli::Normalize(V.Normal);
	}

	void Command::DisplaceNoise(std::vector<Chilli::Vertex>& Verts, const std::vector<uint32_t>& Indices, uint32_t Handle, float Scale, float Strength, float OffsetX, float OffsetY, float OffsetZ, float FloorY, bool EnableX, bool EnableY, bool EnableZ)
	{
		auto NoiseProvider = GetService<Chilli::FastNoiseProvider>();

		for (size_t i = 0; i < Verts.size(); i++)
		{
			float Noise = NoiseProvider->GetValue01(
				Handle,
				Verts[i].Position.x * Scale + OffsetX,
				Verts[i].Position.y * Scale + OffsetY,
				Verts[i].Position.z * Scale + OffsetZ
			);

			float Displacement = (Noise - 0.5f) * Strength;

			if (EnableX) Verts[i].Position.x += Verts[i].Normal.x * Displacement;
			if (EnableY) Verts[i].Position.y += Verts[i].Normal.y * Displacement;
			if (EnableZ) Verts[i].Position.z += Verts[i].Normal.z * Displacement;

			Verts[i].Position.y = std::max(Verts[i].Position.y, FloorY);
		}

		RecalculateNormals(Verts, Indices);
	}

	std::vector<Chilli::Vertex> Command::RawVertexToVertex(const std::vector<uint8_t>& RawVertices, const Chilli::VertexInputShaderLayout& Layout)
	{
		uint32_t vertexStride = Layout.Bindings[0].Stride;
		std::vector<Chilli::Vertex> Vertices;
		Vertices.resize(RawVertices.size() / vertexStride);
		memcpy(Vertices.data(), RawVertices.data(), RawVertices.size());
		return Vertices;
	}

	// This is the "Bridge" function you need
	std::vector<uint8_t> RebuildDataVertexBuffer(
		void* Vertices, uint32_t InBytesVerticesSize, const Chilli::VertexInputShaderLayout& Current,
		const Chilli::VertexInputShaderLayout& Desired)
	{
		uint32_t fileStride = Current.Bindings[0].Stride;
		uint32_t desiredStride = Desired.Bindings[0].Stride;
		size_t vertCount = InBytesVerticesSize / fileStride;

		std::vector<uint8_t> NewBuffer(vertCount * desiredStride);

		for (size_t i = 0; i < vertCount; ++i) {
			uint8_t* destVert = NewBuffer.data() + (i * desiredStride);
			uint8_t* srcVert = (uint8_t*)Vertices + (i * fileStride);

			// For every attribute the USER wants...
			for (const auto& destAttr : Desired.Bindings[0].Attribs) {

				// 1. Try to find this attribute in the FILE
				bool foundInFile = false;
				for (const auto& srcAttr : Current.Bindings[0].Attribs) {
					if (destAttr.Location == srcAttr.Location) {
						// MATCH! Copy from File to New Buffer
						memcpy(destVert + destAttr.Offset, srcVert + srcAttr.Offset, Chilli::ShaderTypeToSize(destAttr.Type));
						foundInFile = true;
						break;
					}
				}

				// 2. If NOT found in file, Inject Defaults
				if (!foundInFile) {
					float* padPtr = (float*)(destVert + destAttr.Offset);
					if (destAttr.Location == (int)Chilli::MeshAttribute::COLOR) {
						padPtr[0] = 1.0f; padPtr[1] = 1.0f; padPtr[2] = 1.0f; // White
					}
					else {
						memset(padPtr, 0, Chilli::ShaderTypeToSize(destAttr.Type)); // Zero out (UVs, etc)
					}
				}
			}
		}
		return NewBuffer;
	}

	BackBone::AssetHandle<Mesh> Command::CreateMesh(BasicShapes Shape)
	{
		std::vector<Chilli::Vertex> Vertices;
		std::vector<uint32_t> Indices;

		switch (Shape)
		{
		case BasicShapes::TRIANGLE:
		{
			Vertices = {
				{ {  0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.5f, 0.0f }, {0,0,0} },
				{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, {0,0,0} },
				{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, {0,0,0} }
			};
			Indices = { 0, 1, 2 };
			break;
		}
		case BasicShapes::QUAD:
		{
			Vertices = {
				{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0,0,0 } },
				{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, {0,0,0} },
				{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, {0,0,0} },
				{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, {0,0,0} },
			};
			Indices = { 0, 1, 2, 2, 3, 0 };
			break;
		}
		case BasicShapes::CUBE:
		{
			// Using your provided cube data
			Vertices = {
				// Front, Back, Left, Right, Top, Bottom (24 vertices)
				{{-0.5f,-0.5f, 0.5f},{0,0, 1},{0,1}}, {{ 0.5f,-0.5f, 0.5f},{0,0, 1},{1,1}}, {{ 0.5f, 0.5f, 0.5f},{0,0, 1},{1,0}}, {{-0.5f, 0.5f, 0.5f},{0,0, 1},{0,0}},
				{{ 0.5f,-0.5f,-0.5f},{0,0,-1},{0,1}}, {{-0.5f,-0.5f,-0.5f},{0,0,-1},{1,1}}, {{-0.5f, 0.5f,-0.5f},{0,0,-1},{1,0}}, {{ 0.5f, 0.5f,-0.5f},{0,0,-1},{0,0}},
				{{-0.5f,-0.5f,-0.5f},{-1,0,0},{0,1}}, {{-0.5f,-0.5f, 0.5f},{-1,0,0},{1,1}}, {{-0.5f, 0.5f, 0.5f},{-1,0,0},{1,0}}, {{-0.5f, 0.5f,-0.5f},{-1,0,0},{0,0}},
				{{ 0.5f,-0.5f, 0.5f},{ 1,0,0},{0,1}}, {{ 0.5f,-0.5f,-0.5f},{ 1,0,0},{1,1}}, {{ 0.5f, 0.5f,-0.5f},{ 1,0,0},{1,0}}, {{ 0.5f, 0.5f, 0.5f},{ 1,0,0},{0,0}},
				{{-0.5f, 0.5f, 0.5f},{ 0,1,0},{0,1}}, {{ 0.5f, 0.5f, 0.5f},{ 0,1,0},{1,1}}, {{ 0.5f, 0.5f,-0.5f},{ 0,1,0},{1,0}}, {{-0.5f, 0.5f,-0.5f},{ 0,1,0},{0,0}},
				{{-0.5f,-0.5f,-0.5f},{ 0,-1,0},{0,1}}, {{ 0.5f,-0.5f,-0.5f},{ 0,-1,0},{1,1}}, {{ 0.5f,-0.5f, 0.5f},{ 0,-1,0},{1,0}}, {{-0.5f,-0.5f, 0.5f},{ 0,-1,0},{0,0}}
			};
			Indices = {
				0,1,2, 2,3,0, 4,5,6, 6,7,4, 8,9,10, 10,11,8,
				12,13,14, 14,15,12, 16,17,18, 18,19,16, 20,21,22, 22,23,20
			};
			break;
		}
		case BasicShapes::SPHERE:
		{
			const unsigned int X_SEGMENTS = 32;
			const unsigned int Y_SEGMENTS = 32;
			const float PI = 3.14159265359f;

			return CreateSphere(X_SEGMENTS, Y_SEGMENTS);
		}
		case BasicShapes::CYLINDER:
		{
			return CreateCylinder(32, 0.5f, 2.0f);
		}
		case BasicShapes::TORUS:
		{
			return CreateTorus(32, 16.0f, 1.0f, 0.3f);
		}
		case BasicShapes::CONE:
		{
			return CreateCone(32, 0.5f, 2.0f);
		}
		case BasicShapes::CAPSULE:
		{
			return CreateCapsule(32, 0.5f, 2.0f);
		}
		}

		VertexInputShaderLayout Layout;
		Layout.BeginBinding(0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InPosition", 0);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InNormal", 1);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT2, "InTexCoords", 2);
		Layout.AddAttribute(ShaderObjectTypes::FLOAT3, "InColor", 3);

		Chilli::MeshCreateInfo Info{};
		Info.VertCount = Vertices.size();
		Info.Vertices = Vertices.data();
		Info.IndexCount = Indices.size();
		Info.Indicies = Indices.data();
		Info.IndexType = Chilli::IndexBufferType::UINT32_T;
		Info.IndexBufferState = Chilli::BufferState::STATIC_DRAW;
		Info.MeshLayout = Layout;

		// Use your existing CreateMesh(MeshCreateInfo) to talk to the GPU
		return CreateMesh(Info);
	}

	// This is the "Bridge" function you need
	std::vector<uint8_t> Command::RebuildVertexBuffer(
		const Chilli::RawMeshData& Raw,
		const Chilli::VertexInputShaderLayout& Desired)
	{
		uint32_t fileStride = Raw.FileLayout.Bindings[0].Stride;
		uint32_t desiredStride = Desired.Bindings[0].Stride;
		size_t vertCount = Raw.Vertices.size() / fileStride;

		std::vector<uint8_t> NewBuffer(vertCount * desiredStride);

		for (size_t i = 0; i < vertCount; ++i) {
			uint8_t* destVert = NewBuffer.data() + (i * desiredStride);
			uint8_t* srcVert = (uint8_t*)Raw.Vertices.data() + (i * fileStride);

			// For every attribute the USER wants...
			for (const auto& destAttr : Desired.Bindings[0].Attribs) {

				// 1. Try to find this attribute in the FILE
				bool foundInFile = false;
				for (const auto& srcAttr : Raw.FileLayout.Bindings[0].Attribs) {
					if (destAttr.Location == srcAttr.Location) {
						// MATCH! Copy from File to New Buffer
						memcpy(destVert + destAttr.Offset, srcVert + srcAttr.Offset, Chilli::ShaderTypeToSize(destAttr.Type));
						foundInFile = true;
						break;
					}
				}

				// 2. If NOT found in file, Inject Defaults
				if (!foundInFile) {
					float* padPtr = (float*)(destVert + destAttr.Offset);
					if (destAttr.Location == (int)Chilli::MeshAttribute::COLOR) {
						padPtr[0] = 1.0f; padPtr[1] = 1.0f; padPtr[2] = 1.0f; // White
					}
					else {
						memset(padPtr, 0, Chilli::ShaderTypeToSize(destAttr.Type)); // Zero out (UVs, etc)
					}
				}
			}
		}
		return NewBuffer;
	}

	BackBone::AssetHandle<Mesh> Command::CreateMesh(BackBone::AssetHandle<RawMeshData> Data, const VertexInputShaderLayout& DesiredLayout)
	{
		Chilli::MeshCreateInfo Info;
		// 1. Map the Raw Data
		std::vector<uint8_t> FinalVertices = RebuildVertexBuffer(*Data.ValPtr, DesiredLayout);
		Info.Vertices = FinalVertices.data();
		Info.Indicies = Data.ValPtr->Indices.data();

		// 2. Calculate Counts
		// Total bytes / (8 floats * 4 bytes per float)
		uint32_t vertexStride = DesiredLayout.Bindings[0].Stride;
		Info.VertCount = static_cast<uint32_t>(FinalVertices.size() / vertexStride);

		// Total bytes / 4 bytes (size of uint32_t)
		Info.IndexCount = static_cast<uint32_t>(Data.ValPtr->Indices.size() / sizeof(uint32_t));

		// 3. Set Instance and Type Info
		Info.InstanceCount = 1; // Default to 1 unless doing instanced rendering
		Info.IndexType = Data.ValPtr->IBType;

		// 4. Buffer State (Usually starts as UNDEFINED or SHADER_READ depending on your backend)
		Info.IndexBufferState = Chilli::BufferState::STATIC_DRAW;

		Info.MeshLayout = DesiredLayout;
		return this->CreateMesh(Info);
	}

	BackBone::AssetHandle<MeshLoaderData> Command::LoadMesh(const std::string& Path)
	{
		return this->LoadAsset<Chilli::MeshLoaderData>(Path);
	}

	void Command::DestroyMesh(BackBone::AssetHandle<Mesh> mesh, bool Free)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<Chilli::RenderCommand>();
		auto MeshStore = _Ctxt.AssetRegistry->GetStore<Chilli::Mesh>();
		CH_CORE_ASSERT(MeshStore != nullptr, "NO MESH STORE: NOPE");
		CH_CORE_ASSERT(RenderCommandService != nullptr, "NO RENDER COMMAAND SERVICE: NOPE");

		auto Mesh = MeshStore->Get(mesh);

		CH_CORE_ASSERT(Mesh != nullptr, "Mesh Not Found");

		DestroyBuffer(Mesh->VertexBufferHandles[0]);
		if (Mesh->IndexCount != 0)
			DestroyBuffer(Mesh->IBHandle);

		if (Free)
			MeshStore->Remove(mesh);
	}

	void Command::FreeMesh(BackBone::AssetHandle<Mesh> mesh)
	{
		DestroyMesh(mesh, true);
	}

	std::vector<uint32_t> Command::RawIndicesToIndices(const std::vector<uint8_t>& RawIndices)
	{
		std::vector<uint32_t> Indices;
		Indices.resize(RawIndices.size() / sizeof(uint32_t));
		memcpy(Indices.data(), RawIndices.data(), RawIndices.size());
		return Indices;
	}

	uint32_t Command::CreateEntity()
	{
		auto RenderService = _Ctxt.ServiceRegistry->GetService<Renderer>();
		auto ReturnIndex = _Ctxt.Registry->Create();

		//if (RenderService != nullptr)
		//	RenderService->UpdateObjectSSBO(glm::mat4(1.0f), ReturnIndex);

		//auto RenderGlobalResource = GetResource<RenderResource>();
		//if (RenderGlobalResource)
		//	RenderGlobalResource->OldTransformComponents.push_back(TransformComponent(
		//		{ (float)UINT32_MAX,(float)UINT32_MAX,(float)UINT32_MAX },
		//		{ (float)UINT32_MAX,(float)UINT32_MAX,(float)UINT32_MAX },
		//		{ (float)UINT32_MAX,(float)UINT32_MAX,(float)UINT32_MAX }
		//	));

		return ReturnIndex;
	}

	uint32_t Command::GetEntityGeneration(uint32_t EntityID)
	{
		return _Ctxt.Registry->GetEntityGeneration(EntityID);
	}

	void Command::DestroyEntity(uint32_t EntityID)
	{
		_Ctxt.Registry->Destroy(EntityID);
	}

	bool Command::IsEntityValid(uint32_t EntityID)
	{
		return _Ctxt.Registry->IsEntityValid(EntityID);
	}

	void Command::GenerateCircleImageData(uint32_t* pixels, int width, int height, int cx, int cy, 
		int radius, uint32_t color, uint32_t bg_color)
	{
		int r2 = radius * radius;

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int dx = x - cx;
				int dy = y - cy;

				if (dx * dx + dy * dy <= r2) {
					pixels[y * width + x] = color;
				}
				else {
					pixels[y * width + x] = bg_color;
				}
			}
		}
	}

	BackBone::AssetHandle<ShaderModule> Command::CreateShaderModule(const char* FilePath, ShaderStageType Type)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto ShaderModuleStore = _Ctxt.AssetRegistry->GetStore<ShaderModule>();

		auto Module = RenderCommandService->CreateShaderModule(FilePath, Type);
		return ShaderModuleStore->Add(Module);
	}

	void Command::DestroyShaderModule(const BackBone::AssetHandle<ShaderModule>& Handle)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto ShaderModuleStore = _Ctxt.AssetRegistry->GetStore<ShaderModule>();

		auto Module = ShaderModuleStore->Get(Handle);
		CH_CORE_ASSERT(Module != nullptr, "Shader Module not Found is null");

		RenderCommandService->DestroyShaderModule(*Module);
		ShaderModuleStore->Remove(Handle);
	}

	BackBone::AssetHandle<ShaderProgram> Command::CreateShaderProgram()
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto ShaderProgramStore = _Ctxt.AssetRegistry->GetStore<ShaderProgram>();

		auto RawProgramID = RenderCommandService->MakeShaderProgram();
		ShaderProgram Program;
		Program.RawProgramHandle = RawProgramID;

		return ShaderProgramStore->Add(Program);
	}

	void Command::AttachShaderModule(const BackBone::AssetHandle<ShaderProgram>& Program, const BackBone::AssetHandle<ShaderModule>& Module)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto ShaderProgramStore = _Ctxt.AssetRegistry->GetStore<ShaderProgram>();
		auto ShaderModuleStore = _Ctxt.AssetRegistry->GetStore<ShaderModule>();

		auto RawProgram = ShaderProgramStore->Get(Program);
		CH_CORE_ASSERT(RawProgram != nullptr, "Shader Module not Found is null");

		auto RawModule = ShaderModuleStore->Get(Module);
		CH_CORE_ASSERT(RawModule != nullptr, "Shader Module not Found is null");

		RenderCommandService->AttachShader(RawProgram->RawProgramHandle, *RawModule);
	}

	void Command::LinkShaderProgram(const BackBone::AssetHandle<ShaderProgram>& Handle)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto ShaderProgramStore = _Ctxt.AssetRegistry->GetStore<ShaderProgram>();

		auto Program = ShaderProgramStore->Get(Handle);
		CH_CORE_ASSERT(Program != nullptr, "Shader Module not Found is null");

		RenderCommandService->LinkShaderProgram(Program->RawProgramHandle);
	}

	void Command::SetParentEntity(BackBone::Entity Child, BackBone::Entity Parent)
	{
		CH_CORE_ASSERT(this->GetComponent<TransformComponent>(Child) != nullptr, "Child Needs TransformComponent");
		CH_CORE_ASSERT(this->GetComponent<TransformComponent>(Parent) != nullptr, "Parent Needs TransformComponent");

		auto Transform = this->GetComponent<TransformComponent>(Child);
		Transform->SetParent(Parent);
	}

	void Command::DestroyShaderProgram(const BackBone::AssetHandle<ShaderProgram>& Handle)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto ShaderProgramStore = _Ctxt.AssetRegistry->GetStore<ShaderProgram>();

		auto Program = ShaderProgramStore->Get(Handle);
		CH_CORE_ASSERT(Program != nullptr, "Shader Module not Found is null");

		RenderCommandService->ClearShaderProgram(Program->RawProgramHandle);
		ShaderProgramStore->Remove(Handle);
	}

	BackBone::AssetHandle<Buffer> Command::CreateBuffer(const BufferCreateInfo& Info, const char* DebugName)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto BufferStore = _Ctxt.AssetRegistry->GetStore<Buffer>();

		Buffer NewBuffer;
		NewBuffer.RawBufferHandle = RenderCommandService->AllocateBuffer(Info);
		NewBuffer.CreateInfo = Info;
		strcpy(NewBuffer.DebugName, DebugName);

		return BufferStore->Add(NewBuffer);
	}

	void Command::MapBufferData(const BackBone::AssetHandle<Buffer>& Handle, void* Data, size_t Size
		, size_t Offset)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto BufferStore = _Ctxt.AssetRegistry->GetStore<Buffer>();

		auto Buffer = BufferStore->Get(Handle);
		CH_CORE_ASSERT(Buffer != nullptr, "Buffer not Found is null");

		RenderCommandService->MapBufferData(Buffer->RawBufferHandle, Data, Size, Offset);
	}

	BackBone::AssetHandle<FlameFont> Command::LoadFont_Msdf(const std::string& Path, const std::string& AtlasPath)
	{
		auto FontStore = GetStore<FlameFont>();

		FlameFont Font;
		Font.RawFontData = this->LoadAsset<MSDFData>(Path);

		auto AtlasImageData = this->LoadAsset<ImageData>(AtlasPath);
		auto AtlasRes = IVec2(Font.RawFontData.ValPtr->AtlasWidth, Font.RawFontData.ValPtr->AtlasHeight);

		auto Spec = Font.RawFontData.ValPtr->GetImageCreateInfo(AtlasRes.x, AtlasRes.y);
		Font.AtlasImage = this->AllocateImage(Spec);

		MapImageData(Font.AtlasImage, AtlasImageData.ValPtr->Pixels, AtlasRes.x,
			AtlasRes.y);

		TextureSpec TexSpec;
		TexSpec.Format = Font.AtlasImage.ValPtr->Spec.Format;

		Font.AtlasTexture = this->CreateTexture(Font.AtlasImage, TexSpec);

		auto ImageLoader = this->GetService<AssetLoader>()->GetLoader<Chilli::ImageLoader>();
		ImageLoader->Unload(_Ctxt, AtlasImageData);

		auto FlameResource = GetResource<Chilli::FlameResource>();
		FlameResource->UpdateFontAtlasTextureMap(GetService<Renderer>(), GetService<MaterialSystem>(), 
			Font.AtlasTexture);

		return FontStore->Add(Font);
	}

	void Command::UnLoadFont_Msdf(BackBone::AssetHandle<FlameFont> FontHandle)
	{
		auto FontStore = GetStore<FlameFont>();

		UnloadAsset(FontHandle.ValPtr->RawFontData.ValPtr->FilePath);
		this->DestroyTexture(FontHandle.ValPtr->AtlasTexture);
		this->DestroyImage(FontHandle.ValPtr->AtlasImage);

		FontStore->Remove(FontHandle);
	}

	void Command::DestroyBuffer(const BackBone::AssetHandle<Buffer>& Handle)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();
		auto BufferStore = _Ctxt.AssetRegistry->GetStore<Buffer>();

		auto Buffer = BufferStore->Get(Handle);
		CH_CORE_ASSERT(Buffer != nullptr, "Buffer not Found is null");

		RenderCommandService->FreeBuffer(Buffer->RawBufferHandle);
		BufferStore->Remove(Handle);
	}

	uint32_t Command::SpwanWindow(const WindowSpec& Spec)
	{
		auto WindowService = _Ctxt.ServiceRegistry->GetService<WindowManager>();
		auto EventService = _Ctxt.ServiceRegistry->GetService<EventHandler>();

		CH_CORE_ASSERT(WindowService != nullptr, "Window Service is null");
		CH_CORE_ASSERT(EventService != nullptr, "Event Handle Service is null");

		auto Idx = WindowService->Create(Spec);
		WindowService->GetWindow(Idx)->SetEventCallback(EventService);

		return Idx;
	}

	void Command::DestroyWindow(uint32_t Idx)
	{
		auto WindowService = _Ctxt.ServiceRegistry->GetService<WindowManager>();
		CH_CORE_ASSERT(WindowService != nullptr, "Window Service is null");
		WindowService->DestroyWindow(Idx);
	}

	Window* Command::GetActiveWindow()
	{
		auto WindowService = _Ctxt.ServiceRegistry->GetService<WindowManager>();
		CH_CORE_ASSERT(WindowService != nullptr, "Window Service is null");
		return WindowService->GetActiveWindow();
	}

	BackBone::AssetHandle<Material> Command::CreateMaterial(BackBone::AssetHandle<ShaderProgram> Program)
	{
		auto MaterialSystem = this->GetService<Chilli::MaterialSystem>();

		return MaterialSystem->CreateMaterial(Program);
	}

	BackBone::AssetHandle<Material> CreateMaterial()
	{
		return BackBone::AssetHandle<Material>();
	}

	void Command::DestroyMaterial(const BackBone::AssetHandle<Material>& Mat)
	{
		auto MaterialSystem = this->GetService<Chilli::MaterialSystem>();

		MaterialSystem->DestroyMaterial(Mat);
	}

	BackBone::AssetHandle<Scene> Command::CreateScene()
	{
		auto SceneManager = this->GetService<Chilli::SceneManager>();
		return SceneManager->CreateScene();
	}

	void Command::SetActiveScene(BackBone::AssetHandle<Scene> Scene)
	{
		auto RenderResource = this->GetResource<Chilli::RenderResource>();
		RenderResource->ActiveSceneID = Scene;
	}

	BackBone::AssetHandle<Sampler> Command::CreateSampler(const SamplerSpec& Spec)
	{
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();

		auto SamplerStore = GetStore<Chilli::Sampler>();

		// Check if sampler with same spec already exists
		for (auto SamplerHandle : SamplerStore->GetHandles())
		{
			auto Sampler = SamplerHandle.ValPtr;
			if (Sampler->Spec == Spec)
				return SamplerHandle;
		}

		Sampler NewSampler;
		NewSampler.SamplerHandle = RenderCommandService->CreateSampler(Spec);
		NewSampler.Spec = Spec;

		return SamplerStore->Add(NewSampler);

	}

	void Command::DestroySampler(const BackBone::AssetHandle<Sampler>& sampler)
	{
		auto SamplerStore = GetStore<Chilli::Sampler>();
		auto RenderCommandService = _Ctxt.ServiceRegistry->GetService<RenderCommand>();

		RenderCommandService->DestroySampler(SamplerStore->Get(sampler)->SamplerHandle);
		SamplerStore->Remove(sampler);
	}

	BackBone::AssetHandle<Image> Command::AllocateImage(ImageSpec& Spec)
	{
		auto ImageStore = GetStore<Image>();
		auto RenderCommandService = GetService<RenderCommand>();

		Image NewImage;
		NewImage.RawImageHandle = RenderCommandService->AllocateImage(Spec);
		NewImage.Spec = Spec;

		return ImageStore->Add(NewImage);
	}

	std::pair<BackBone::AssetHandle<Image>, BackBone::AssetHandle<ImageData>> Command::AllocateImage(const char* FilePath,
		ImageFormat Format, uint32_t Usage, ImageType Type, uint32_t MipLevel, bool YFlip)
	{
		auto ImageData = this->LoadAsset<Chilli::ImageData>(FilePath);

		Chilli::ImageSpec ImageSpec;
		ImageSpec.Resolution = { ImageData.ValPtr->Resolution.x,
			ImageData.ValPtr->Resolution.y,
			1 };
		ImageSpec.Format = Format;
		ImageSpec.Usage = Usage;
		ImageSpec.Type = Type;
		if (MipLevel == -1)
			ImageSpec.MipLevel = static_cast<uint32_t>(std::floor(std::log2(std::max(ImageData.ValPtr->Resolution.x, ImageData.ValPtr->Resolution.y)))) + 1;
		else
			ImageSpec.MipLevel = MipLevel;
		ImageSpec.YFlip = YFlip;
		ImageSpec.State = ResourceState::ShaderRead;
		auto Image = this->AllocateImage(ImageSpec);

		this->MapImageData(Image, (void*)ImageData.ValPtr->Pixels, ImageData.ValPtr->Resolution.x,
			ImageData.ValPtr->Resolution.y);

		return { Image, ImageData };
	}

	void Chilli::Command::DestroyImage(const BackBone::AssetHandle<Image>& ImageHandle)
	{
		auto ImageStore = GetStore<Image>();
		auto RenderCommandService = GetService<RenderCommand>();

		RenderCommandService->DestroyImage(ImageStore->Get(ImageHandle)->RawImageHandle);
		ImageStore->Remove(ImageHandle);
	}

	void Command::MapImageData(const BackBone::AssetHandle<Image>& ImageHandle, void* Data, int Width, int Height)
	{
		auto ImageStore = GetStore<Image>();
		auto RenderCommandService = GetService<RenderCommand>();

		RenderCommandService->MapImageData(ImageStore->Get(ImageHandle)->RawImageHandle,
			Data, Width, Height);
	}

	BackBone::AssetHandle<Texture> Command::CreateTexture(const BackBone::AssetHandle<Image>& ImageHandle, TextureSpec& Spec)
	{
		auto TextureStore = GetStore<Texture>();
		auto RenderCommandService = GetService<RenderCommand>();

		Texture NewTexture;
		NewTexture.ImageHandle = ImageHandle;
		NewTexture.RawTextureHandle = RenderCommandService->CreateTexture(
			ImageHandle.ValPtr->RawImageHandle,
			Spec
		);
		NewTexture.Spec = Spec;

		return TextureStore->Add(NewTexture);
	}

	void Command::DestroyTexture(const BackBone::AssetHandle<Texture>& TextureHandle)
	{
		auto TextureStore = GetStore<Texture>();
		auto RenderCommandService = GetService<RenderCommand>();

		RenderCommandService->DestroyTexture(TextureStore->Get(TextureHandle)->RawTextureHandle);
		TextureStore->Remove(TextureHandle);
	}

	inline BackBone::GenericFrameData* Command::GetGenericFrameData()
	{
		return GetResource<BackBone::GenericFrameData>();
	}

#pragma endregion

#pragma region Camera Extension
	void OnCameraSystem(Chilli::BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto RenderService = Ctxt.ServiceRegistry->GetService<Renderer>();
		auto Window = Command.GetService<WindowManager>()->GetActiveWindow();

		for (auto [Entity, Camera, Transform] :
			BackBone::QueryWithEntities<CameraComponent, TransformComponent>(*Ctxt.Registry))
		{
			// 1. Get the pre-calculated World Matrix (which includes your Pitch/Yaw)
			glm::mat4 Model = Transform->GetWorldMatrix();

			// 2. Extract Position and Directions from the Matrix
			// Column 3 is Position, Column 2 is Z-Axis (Forward), Column 1 is Y-Axis (Up)
			glm::vec3 CameraPos = glm::vec3(Transform->GetPosition().x, Transform->GetPosition().y,
				Transform->GetPosition().z);

			glm::vec3 Forward = glm::vec3(Transform->GetForward().x, Transform->GetForward().y, Transform->GetForward().z);
			glm::vec3 Up = glm::vec3(Transform->GetUp().x, Transform->GetUp().y, Transform->GetUp().z);

			// 3. Create the View Matrix looking at (Pos + Forward)
			glm::mat4 view = glm::lookAtRH(CameraPos, CameraPos + Forward, Up);
			// 4. Calculate Projection
			glm::mat4 projection;
			if (Camera->Is_Orthro)
			{
				projection = glm::ortho(-Camera->Orthro_Size.x, Camera->Orthro_Size.x,
					-Camera->Orthro_Size.y, Camera->Orthro_Size.y,
					Camera->Near_Clip, Camera->Far_Clip);
			}
			else {
				projection = glm::perspective(glm::radians(Camera->Fov),
					Window->GetAspectRatio(),
					Camera->Near_Clip, Camera->Far_Clip);
			}
			projection[1][1] *= -1;
			// 5. Build and Push Scene Data
			SceneData SceneData{};
			SceneData.CameraPos = { CameraPos.x, CameraPos.y, CameraPos.z, 1.0f };

			// Final Matrix: Projection * View
			SceneData.ViewProjMatrix = projection * view;

			Camera->ViewProjMat = SceneData.ViewProjMatrix;
		}
	}

	void CameraExtension::Build(BackBone::App& App)
	{
		auto Command = Chilli::Command(App.Ctxt);
		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::UPDATE, OnCameraSystem);
	}

	namespace CameraBundle
	{
		Chilli::BackBone::Entity Create3D(Chilli::BackBone::SystemContext& Ctxt, glm::vec3 Pos, bool Main_Cam)
		{
			auto Command = Chilli::Command(Ctxt);
			auto camera = Command.CreateEntity();

			Command.AddComponent<CameraComponent>(camera, CameraComponent{
				.Fov = 60.0f,
				.Near_Clip = 0.1f,
				.Far_Clip = 1000.0f
				});

			Command.AddComponent<TransformComponent>(camera, TransformComponent(
				FromGlmVec3(Pos),
				{ 1.0f, 1.0f, 1.0f }
			));

			Command.GetComponent<TransformComponent>(camera)->SetEulerRotation({ 0,0,0 });

			return camera;
		}

		Chilli::BackBone::Entity Create2D(Chilli::BackBone::SystemContext& Ctxt, bool Main_Cam, const IVec2& Resolution)
		{
			auto Command = Chilli::Command(Ctxt);
			auto camera = Command.CreateEntity();
			Command.AddComponent<CameraComponent>(camera, CameraComponent{
				.Near_Clip = 0.1f,
				.Far_Clip = 1000.0f,
				.Is_Orthro = true,
				.Orthro_Size = {(float)Resolution.x, (float)Resolution.y},
				});

			Command.AddComponent<TransformComponent>(camera, TransformComponent(
				{ 0.0f, 0.0f, 0.0f },
				{ 1.0f, 1.0f, 1.0f },
				{ 0.0f, 0.0f, 0.0f }
			));

			return camera;
		}
		void Update3DCamera(Chilli::BackBone::Entity Camera, Chilli::BackBone::SystemContext& Ctxt)
		{
			auto Command = Chilli::Command(Ctxt);
			auto Input = Ctxt.ServiceRegistry->GetService<Chilli::Input>();
			auto FrameData = Command.GetResource<Chilli::BackBone::GenericFrameData>();
			float DT = FrameData->Ts.GetSecond();

			auto Transform = Command.GetComponent<Chilli::TransformComponent>(Camera);
			auto Control = Command.GetComponent<Chilli::Deafult3DCameraController>(Camera);

			if (!Control || !Transform) return;

			// 1. Handle Rotation via Quaternions (Euler-to-Quat conversion)
			if (Input->IsMouseButtonDown(Chilli::Input_mouse_Left))
			{
				Chilli::IVec2 Mouse_Delta = Input->GetCursorDelta();

				Control->Yaw += Mouse_Delta.x * Control->Look_Sensitivity;

				float Y_Mult = Control->Invert_Y ? 1.0f : -1.0f;
				Control->Pitch += Mouse_Delta.y * Control->Look_Sensitivity * Y_Mult;

				// Constrain Pitch to avoid flipping (still useful with Quats for FPS style)
				Control->Pitch = glm::clamp(Control->Pitch, -89.0f, 89.0f);

				// Re-apply rotation to the quaternion
				Transform->SetEulerRotation({ Control->Pitch, Control->Yaw, 0.0f });
			}

			// 2. Handle Movement using Transform's direction vectors
			// Your TransformComponent now provides these via Quaternion math
			Chilli::Vec3 Forward = Transform->GetForward();
			Chilli::Vec3 Right = Transform->GetRight();

			float S = Control->Move_Speed * DT;

			// W moves toward Forward, S moves away
			if (Input->IsKeyDown(Chilli::Input_key_W)) Transform->Move(Forward * S);
			if (Input->IsKeyDown(Chilli::Input_key_S)) Transform->Move(Forward * -S);

			// A moves Left (-Right), D moves Right
			if (Input->IsKeyDown(Chilli::Input_key_A)) Transform->Move(Right * -S);
			if (Input->IsKeyDown(Chilli::Input_key_D)) Transform->Move(Right * S);

			// Space moves UP (+Y), Control moves DOWN (-Y)
			if (Input->IsKeyDown(Chilli::Input_key_Space))       Transform->MoveY(S);
			if (Input->IsKeyDown(Chilli::Input_key_LeftControl)) Transform->MoveY(-S);
		}

	}
#pragma endregion

#pragma region Noise Generator Service

	FastNoiseProvider::~FastNoiseProvider()
	{
		if (_configs.size() > 0)
		{

		}
	}

	NoiseHandle FastNoiseProvider::CreateGenerator(const NoiseConfig& config)
	{
		// 1. Base Pattern
		FastNoise::SmartNode<> node;
		switch (config.Type) {
		case NoiseType::Perlin:   node = FastNoise::New<FastNoise::Perlin>(); break;
		case NoiseType::Simplex:  node = FastNoise::New<FastNoise::Simplex>(); break;
		case NoiseType::Value:    node = FastNoise::New<FastNoise::Value>(); break;
		case NoiseType::Cellular: {
			auto cell = FastNoise::New<FastNoise::CellularDistance>();
			cell->SetDistanceFunction(FastNoise::DistanceFunction::Euclidean);
			node = cell;
			break;
		}
		}

		// 2. Fractal Layering
		if (config.UseFractal) {
			auto fractal = FastNoise::New<FastNoise::FractalFBm>();
			fractal->SetSource(node);
			fractal->SetOctaveCount(config.Octaves);
			fractal->SetGain(config.Gain);
			fractal->SetLacunarity(config.Lacunarity);
			node = fractal;
		}

		// 3. Domain Warping (The "Swirl" Logic)
		if (config.UseWarp) {
			auto warper = FastNoise::New<FastNoise::DomainWarpGradient>();
			warper->SetSource(node);
			warper->SetWarpAmplitude(config.WarpIntensity);
			node = warper;
		}

		NoiseHandle handle = _nextHandle++;
		_generators[handle] = node;
		_configs[handle] = config;
		return handle;
	}

	float FastNoiseProvider::GetSingle3D(NoiseHandle handle, float x, float y, float z)
	{
		auto it = _generators.find(handle);
		if (it != _generators.end()) {
			const auto& config = _configs[handle];

			// FastNoise2 scales coordinates by frequency
			return it->second->GenSingle3D(
				x * config.Frequency,
				y * config.Frequency,
				z * config.Frequency,
				config.Seed
			);
		}
		return 0.0f; // Safe fallback for invalid handles
	}

	float FastNoiseProvider::GetSingle2D(NoiseHandle handle, float x, float y)
	{
		auto it = _generators.find(handle);
		if (it != _generators.end()) {
			const auto& config = _configs[handle];

			// FastNoise2 scales coordinates by frequency
			return it->second->GenSingle2D(
				x * config.Frequency,
				y * config.Frequency,
				config.Seed
			);
		}
		return 0.0f; // Safe fallback for invalid handles
	}

	void FastNoiseProvider::GetGrid2D(NoiseHandle handle, std::vector<float>& outBuffer, int startX, int startY, int width, int height)
	{
		auto it = _generators.find(handle);
		if (it != _generators.end()) {
			const auto& config = _configs[handle];

			// Safety check: ensure buffer won't overflow
			size_t requiredSize = static_cast<size_t>(width * height);
			if (outBuffer.size() < requiredSize) {
				outBuffer.resize(requiredSize);
			}

			// SIMD Generation
			it->second->GenUniformGrid2D(
				outBuffer.data(),
				startX, startY,
				width, height,
				config.Frequency,
				config.Frequency,
				config.Seed
			);
		}
	}

	void FastNoiseProvider::GetGrid2D(NoiseHandle handle, float* outBuffer, int startX, int startY, int width, int height)
	{
		auto it = _generators.find(handle);
		if (it != _generators.end()) {
			const auto& config = _configs[handle];

			// Safety check: ensure buffer won't overflow
			size_t requiredSize = static_cast<size_t>(width * height);

			// SIMD Generation
			it->second->GenUniformGrid2D(
				outBuffer,
				startX, startY,
				width, height,
				config.Frequency,
				config.Frequency,
				config.Seed
			);
		}
	}

	void FastNoiseProvider::GetGrid3D(NoiseHandle handle, std::vector<float>& outBuffer, int startX, int startY, int startZ, int width, int height, int depth)
	{
		if (auto node = GetNode(handle)) {
			const auto& cfg = _configs[handle];
			size_t totalSize = (size_t)width * height * depth;
			if (outBuffer.size() < totalSize) outBuffer.resize(totalSize);

			// GenUniformGrid3D uses SIMD to fill a volume of data
			node->GenUniformGrid3D(outBuffer.data(),
				(float)startX, (float)startY, (float)startZ,
				width, height, depth,
				cfg.Frequency, cfg.Frequency, cfg.Frequency,
				cfg.Seed);
		}
	}
#pragma endregion

#pragma region JoltPhysics

	/// Class that determines if two object layers can collide
	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
	{
	public:
		ObjectLayerPairFilterImpl(const JoltPhysicsExtensionConfig* Config)
			:_Config(Config)
		{

		}

		virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
		{
			if (inObject1 >= int(Layers::NUM_LAYERS) || inObject2 >= int(Layers::NUM_LAYERS))
				return false;

			return _Config->CollisionMatrix[(uint16_t)inObject1][(uint16_t)inObject2];
		}
	private:
		const JoltPhysicsExtensionConfig* _Config;
	};

	namespace JoltBroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer STATIC(0);
		static constexpr JPH::BroadPhaseLayer DYNAMIC(1);
		static constexpr JPH::BroadPhaseLayer SENSOR(2);
		static constexpr JPH::uint NUM_LAYERS(3);
	};

	class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
	{
	public:
		BPLayerInterfaceImpl(const JoltPhysicsExtensionConfig* Config)
			: _Config(Config) {
		}  // no array to init — config already has the map

		JPH::uint GetNumBroadPhaseLayers() const override
		{
			return JoltBroadPhaseLayers::NUM_LAYERS;
		}

		JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
		{
			if (inLayer >= (JPH::ObjectLayer)Layers::NUM_LAYERS)
				return JoltBroadPhaseLayers::DYNAMIC;

			return (JPH::BroadPhaseLayer)((uint16_t)_Config->LayerToBP[inLayer]);
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
		{
			switch ((JPH::BroadPhaseLayer::Type)inLayer)
			{
			case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayers::STATIC:  return "STATIC";
			case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayers::DYNAMIC: return "DYNAMIC";
			case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayers::SENSOR:  return "SENSOR";
			default: JPH_ASSERT(false); return "INVALID";
			}
		}
#endif

	private:
		const JoltPhysicsExtensionConfig* _Config;
		// no _ObjectToBroadPhase array needed — _Config->LayerToBP already does this
	};

	/// Class that determines if an object layer can collide with a broadphase layer
	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		ObjectVsBroadPhaseLayerFilterImpl(const JoltPhysicsExtensionConfig* Config)
			: _Config(Config) {
		}

		bool ShouldCollide(JPH::ObjectLayer Layer, JPH::BroadPhaseLayer BPLayer) const override
		{
			// Check if this object layer collides with ANY layer that maps to this BP bucket
			for (int OtherLayer = 0; OtherLayer < int(Layers::NUM_LAYERS); OtherLayer++)
			{
				if ((JPH::BroadPhaseLayer)((uint16_t)_Config->LayerToBP[OtherLayer]) == BPLayer &&
					_Config->CollisionMatrix[Layer][OtherLayer])
					return true;
			}
			return false;
		}

	private:
		const JoltPhysicsExtensionConfig* _Config;
	};

	struct JoltContactListenerParamter
	{
		BackBone::SystemContext& Ctxt;
		JPH::PhysicsSystem* PhysicsSystem;

		JoltContactListenerParamter(BackBone::SystemContext& ctxt) : Ctxt(ctxt) {}
	};

	struct JoltPhysicsResourceImpl;

	// An example contact listener
	class JoltContactListenerImpl : public JPH::ContactListener
	{
	public:
		JoltContactListenerImpl(JoltContactListenerParamter Impl)
			:_Parameter(Impl)
		{

		}

		~JoltContactListenerImpl() {
		}

		// See: ContactListener
		virtual JPH::ValidateResult	OnContactValidate(const JPH::Body& inBody1,
			const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override;

		virtual void			OnContactAdded(const JPH::Body& inBody1,
			const JPH::Body& inBody2, const JPH::ContactManifold& inManifold,
			JPH::ContactSettings& ioSettings) override;

		virtual void			OnContactPersisted(const JPH::Body& inBody1,
			const JPH::Body& inBody2, const JPH::ContactManifold& inManifold,
			JPH::ContactSettings& ioSettings) override;

		virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

	private:
		JoltContactListenerParamter _Parameter;
	};

	struct JoltBodyShapeMetaData
	{
		bool Active = false;
		JPH::BodyID BodyID;
		uint32_t GenerationVersion = 1;
		uint32_t RigidBodyVersion = 0;
		uint32_t ColliderVersion = 0;
	};

	struct JoltPhysicsResourceImpl
	{
		//std::unique_ptr<JPH::Factory> Factory;
		JPH::PhysicsSystem PhysicsSystem;
		JPH::BodyInterface* BodyInterFace;
		SparseSet<JoltBodyShapeMetaData> BodiesMetaData;
		std::vector<uint32_t> BodiesMetaDataEntitiesList;
		std::unique_ptr<JPH::TempAllocatorImpl> TempAllocator;
		std::unique_ptr<JPH::JobSystemThreadPool> JobSystem;
		std::unique_ptr<BPLayerInterfaceImpl >Broad_Phase_Layer_Interface;
		std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl >Object_Vs_BroadPhase_Layer_Filter;
		std::unique_ptr<ObjectLayerPairFilterImpl >Object_Vs_Object_Layer_Filter;
		std::unique_ptr<JoltContactListenerImpl > ContactListener;
		BackBone::SystemContext& Ctxt;

		JoltPhysicsResourceImpl(BackBone::SystemContext& ctxt) : Ctxt(ctxt) {}
	};

	void OnJoltSetup(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Resource = Command.GetResource< JoltPhysicsResource>();
		auto Config = Command.GetResource<JoltPhysicsExtensionConfig>();
		Resource->Data = new JoltPhysicsResourceImpl(Ctxt);
		auto JoltData = (JoltPhysicsResourceImpl*)Resource->Data;

		JPH::RegisterDefaultAllocator();
		JPH::Factory::sInstance = new JPH::Factory();

		JPH::RegisterTypes();
		JoltData->TempAllocator = std::make_unique<JPH::TempAllocatorImpl>(Config->UpFrontMemoryAllocated);
		JoltData->JobSystem = std::make_unique<JPH::JobSystemThreadPool>(Config->MaxPhysicsJobs, Config->MaxPhysicsBarriers,
			Config->NumThreads);

		JoltData->Broad_Phase_Layer_Interface = std::make_unique< BPLayerInterfaceImpl>(Config);
		JoltData->Object_Vs_BroadPhase_Layer_Filter = std::make_unique< ObjectVsBroadPhaseLayerFilterImpl>(Config);
		JoltData->Object_Vs_Object_Layer_Filter = std::make_unique< ObjectLayerPairFilterImpl>(Config);

		JoltData->PhysicsSystem.Init(Config->MaxRigidBodies, Config->NumBodyMutexes, Config->MaxBodyPairs,
			Config->MaxContactConstraints,
			(const BPLayerInterfaceImpl&)*JoltData->Broad_Phase_Layer_Interface.get(),
			(const ObjectVsBroadPhaseLayerFilterImpl&)*JoltData->Object_Vs_BroadPhase_Layer_Filter.get(),
			(const ObjectLayerPairFilterImpl&)*JoltData->Object_Vs_Object_Layer_Filter.get());
		JoltData->PhysicsSystem.SetGravity({ 0.0f, -9.81f, 0.0f });
		JoltData->BodyInterFace = &JoltData->PhysicsSystem.GetBodyInterface();

		JoltContactListenerParamter Parameter{ Ctxt };
		Parameter.PhysicsSystem = &JoltData->PhysicsSystem;

		JoltData->ContactListener = std::make_unique< JoltContactListenerImpl>(Parameter);

		JoltData->PhysicsSystem.SetContactListener(JoltData->ContactListener.get());
		JoltData->PhysicsSystem.OptimizeBroadPhase();

		CH_CORE_INFO("Jolt Physics Extension Setup!");
	}

	void OnJoltClearEvents(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);

		Command.ClearEvent<CollisionEnterEvent>();
		Command.ClearEvent<CollisionExitEvent>();
		Command.ClearEvent<CollisionStayEvent>();
	}

	void OnJoltHandleConversions(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Resource = Command.GetResource< JoltPhysicsResource>();
		auto Config = Command.GetResource<JoltPhysicsExtensionConfig>();
		auto JoltData = (JoltPhysicsResourceImpl*)Resource->Data;

		auto JoltGravity = JoltData->PhysicsSystem.GetGravity();
		if (JoltGravity.GetX() != Config->Graivty.x ||
			JoltGravity.GetY() != Config->Graivty.y ||
			JoltGravity.GetZ() != Config->Graivty.z)
		{
			JoltGravity = { Config->Graivty.x, Config->Graivty.y, Config->Graivty.z };
			JoltData->PhysicsSystem.SetGravity(JoltGravity);
		}

		for (auto [Entity, Transform, RigidBody, Collider] : BackBone::QueryWithEntities<TransformComponent,
			RigidBody, Collider>(*Ctxt.Registry))
		{
			auto MetaDataPtr = JoltData->BodiesMetaData.Get(Entity);

			// FIX #1: Handle first-time creation
			if (MetaDataPtr == nullptr)
			{
				// Entity doesn't exist in metadata - create it
				JoltBodyShapeMetaData NewMetaData;
				NewMetaData.Active = false;
				NewMetaData.RigidBodyVersion = 0;
				NewMetaData.ColliderVersion = 0;

				JoltData->BodiesMetaData.Insert(Entity, NewMetaData);
				JoltData->BodiesMetaDataEntitiesList.push_back(Entity);
				MetaDataPtr = JoltData->BodiesMetaData.Get(Entity);

				// FIX #2: Check insert didn't fail
				if (MetaDataPtr == nullptr)
				{
					CH_CORE_ERROR("Failed to allocate metadata for entity!");
					continue;
				}
			}

			bool Create = false;
			if (MetaDataPtr->Active == false)
				Create = true;

			if (Create)
			{
				JPH::Shape::ShapeResult ShapeResult;

				switch (Collider->Type)
				{
				case ColliderType::BOX:
				{
					// Jolt uses HalfExtents for boxes
					JPH::BoxShapeSettings BoxShape(JPH::Vec3(
						Collider->Shape.AABB.HalfExtent.x,
						Collider->Shape.AABB.HalfExtent.y,
						Collider->Shape.AABB.HalfExtent.z));
					BoxShape.SetEmbedded();
					ShapeResult = BoxShape.Create();
					break;
				}

				case ColliderType::SPHERE:
				{
					JPH::SphereShapeSettings SphereShape(Collider->Shape.Sphere.Radius);
					SphereShape.SetEmbedded();
					ShapeResult = SphereShape.Create();
					break;
				}

				case ColliderType::CAPSULE:
				{
					// Jolt Capsule takes HalfHeight of the cylinder part
					JPH::CapsuleShapeSettings CapsuleShape(Collider->Shape.Capsule.HalfHeight, Collider->Shape.Capsule.Radius);
					CapsuleShape.SetEmbedded();
					ShapeResult = CapsuleShape.Create();
					break;
				}
				}
				JPH::ShapeRefC ShapeRef = ShapeResult.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

				if (ShapeResult.HasError())
				{
					CH_CORE_INFO("ERORR: {}", ShapeResult.GetError().c_str());
				}

				auto InPosition = JPH::RVec3(Transform->GetPosition().x, Transform->GetPosition().y,
					Transform->GetPosition().z);
				JPH::EMotionType MotionType = JPH::EMotionType::Static;

				if (RigidBody->MotionType == MotionType::DYNAMIC)
					MotionType = JPH::EMotionType::Dynamic;
				if (RigidBody->MotionType == MotionType::KINEMATIC)
					MotionType = JPH::EMotionType::Kinematic;

				// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
				JPH::BodyCreationSettings BodySettings(ShapeRef, InPosition,
					JPH::Quat::sIdentity(), MotionType, (JPH::ObjectLayer)RigidBody->Layer);

				// FIX #9: Set mass if dynamic
				if (MotionType == JPH::EMotionType::Dynamic && RigidBody->Mass > 0)
				{
					// Override mass calculation with custom mass
					BodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
					BodySettings.mMassPropertiesOverride.mMass = RigidBody->Mass;
				}

				if (RigidBody->Restitution == -1.0f)
					RigidBody->Restitution = Config->DeafultRestitution;
				if (RigidBody->Friction == -1.0f)
					RigidBody->Friction = Config->DeafultFriction;
				if (RigidBody->LinearDamping == -1.0f)
					RigidBody->LinearDamping = Config->DeafultLinearDamping;

				// FIX #10: Set other properties
				BodySettings.mIsSensor = Collider->IsTrigger;
				BodySettings.mFriction = RigidBody->Friction;
				BodySettings.mRestitution = RigidBody->Restitution;
				BodySettings.mGravityFactor = RigidBody->GravityFactor;
				BodySettings.mLinearDamping = RigidBody->LinearDamping;
				BodySettings.mUserData = (uint64_t)Entity;

				if (RigidBody->UseVelvert)
				{
					BodySettings.mLinearDamping = 0.0f;
					BodySettings.mAngularDamping = 0.0f;
					BodySettings.mGravityFactor = 0.0f;
				}

				// Create the actual rigid body
				JPH::Body* Body = JoltData->BodyInterFace->CreateBody(BodySettings); // Note that if we run out of bodies this can return nullptr

				// FIX #3: Check if body creation failed
				if (Body == nullptr)
				{
					CH_CORE_ERROR("Failed to create body - max bodies reached!");
					MetaDataPtr->Active = false;
					continue;
				}

				// Add it to the world
				JoltData->BodyInterFace->AddBody(Body->GetID(),
					MotionType == JPH::EMotionType::Dynamic
					? JPH::EActivation::Activate
					: JPH::EActivation::DontActivate);

				if (RigidBody->Velocity != Chilli::Vec3(0, 0, 0))
				{
					auto Velocity = JPH::Vec3(RigidBody->Velocity.x, RigidBody->Velocity.y,
						RigidBody->Velocity.z);
					JoltData->BodyInterFace->SetLinearVelocity(Body->GetID(), Velocity);
				}

				MetaDataPtr->BodyID = Body->GetID();
				MetaDataPtr->Active = true;
				MetaDataPtr->GenerationVersion = Command.GetEntityGeneration(Entity);
			}
		}
	}

	void OnJoltVelvertIntegrate(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Resource = Command.GetResource< JoltPhysicsResource>();
		auto FrameData = Command.GetResource< BackBone::GenericFrameData>();
		auto Config = Command.GetResource<JoltPhysicsExtensionConfig>();
		auto JoltData = (JoltPhysicsResourceImpl*)Resource->Data;

		for (auto [Entity, Transform, RigidBody, Collider] : BackBone::QueryWithEntities<TransformComponent,
			RigidBody, Collider>(*Ctxt.Registry))
		{
			if (RigidBody->UseVelvert == false)
			{
				if (RigidBody->MotionType != MotionType::DYNAMIC)
					continue;
				// Even if a non velvert user might add a force and rather than waste 2 queries might as well do it here
				auto MetaDataPtr = JoltData->BodiesMetaData.Get(Entity);

				if (RigidBody->ForceAccumulator != Chilli::Vec3(0, 0, 0))
				{
					auto Impulse = JPH::Vec3(RigidBody->ForceAccumulator.x, RigidBody->ForceAccumulator.y,
						RigidBody->ForceAccumulator.z);
					JoltData->BodyInterFace->AddImpulse(MetaDataPtr->BodyID, Impulse);
				}
				RigidBody->ForceAccumulator = Vec3(0, 0, 0);

				continue;

			}
			if (RigidBody->MotionType != MotionType::DYNAMIC)
				continue;

			auto MetaDataPtr = JoltData->BodiesMetaData.Get(Entity);

			// Standard gravity
			Chilli::Vec3 GravityForce = Config->Graivty * RigidBody->Mass * RigidBody->GravityFactor;
			RigidBody->ForceAccumulator += GravityForce;

			// Linear drag — opposes velocity
			JPH::BodyID ID = MetaDataPtr->BodyID;
			JPH::Vec3 JoltVel = JoltData->BodyInterFace->GetLinearVelocity(ID);
			Chilli::Vec3 Vel = { JoltVel.GetX(), JoltVel.GetY(), JoltVel.GetZ() };

			Chilli::Vec3 DragForce = Vel * -RigidBody->LinearDamping * RigidBody->Mass;
			RigidBody->ForceAccumulator += DragForce;

			Chilli::Vec3 NewAcceleration = RigidBody->ForceAccumulator / RigidBody->Mass;

			// Average old and new acceleration → new velocity
			Chilli::Vec3 NewVelocity = Vel +
				(RigidBody->Acceleration + NewAcceleration) * (0.5f * FrameData->FixedPhysicsData.Ticks);

			JoltData->BodyInterFace->SetLinearVelocity(ID,
				JPH::Vec3(NewVelocity.x, NewVelocity.y, NewVelocity.z));

			RigidBody->Acceleration = NewAcceleration;
			RigidBody->ForceAccumulator = Vec3(0, 0, 0);
		}
	}

	void OnJoltUpdate(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Resource = Command.GetResource< JoltPhysicsResource>();
		auto FrameData = Command.GetResource< BackBone::GenericFrameData>();
		auto Config = Command.GetResource<JoltPhysicsExtensionConfig>();
		auto JoltData = (JoltPhysicsResourceImpl*)Resource->Data;

		for (auto& Entity : JoltData->BodiesMetaDataEntitiesList)
		{
			bool Destroy = false;
			auto MetaDataPtr = JoltData->BodiesMetaData.Get(Entity);

			if (Command.IsEntityValid(Entity) == false && MetaDataPtr->Active)
				Destroy = true;
			if (Command.GetEntityGeneration(Entity) != MetaDataPtr->GenerationVersion
				&& MetaDataPtr->Active)
				Destroy = true;
			if (Ctxt.Registry->HasComponent<RigidBody>(Entity) == false
				&& MetaDataPtr->Active)
				Destroy = true;

			if (Destroy)
			{
				MetaDataPtr->Active = false;

				// Remove and destroy the floor
				JoltData->BodyInterFace->RemoveBody(MetaDataPtr->BodyID);
				JoltData->BodyInterFace->DestroyBody(MetaDataPtr->BodyID);
			}
		}

		// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
		const int cCollisionSteps = 1;

		{
			// Step the world
			JoltData->PhysicsSystem.Update(FrameData->FixedPhysicsData.Ticks, cCollisionSteps, JoltData->TempAllocator.get(), JoltData->JobSystem.get());
		}
	}

	void OnJoltSyncBack(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Resource = Command.GetResource< JoltPhysicsResource>();
		auto Config = Command.GetResource<JoltPhysicsExtensionConfig>();
		auto JoltData = (JoltPhysicsResourceImpl*)Resource->Data;

		for (auto [Entity, Transform, RigidBody, Collider] : BackBone::QueryWithEntities<TransformComponent,
			RigidBody, Collider>(*Ctxt.Registry))
		{
			auto MetaDataPtr = JoltData->BodiesMetaData.Get(Entity);
			if (MetaDataPtr != nullptr)
			{
				if (MetaDataPtr->Active == true)
				{
					auto JoltPosition = JoltData->BodyInterFace->GetCenterOfMassPosition(MetaDataPtr->BodyID);
					auto JoltVelocity = JoltData->BodyInterFace->GetLinearVelocity(MetaDataPtr->BodyID);

					RigidBody->Velocity = { JoltVelocity.GetX(), JoltVelocity.GetY(), JoltVelocity.GetZ() };
					Transform->SetPosition({ JoltPosition.GetX(), JoltPosition.GetY(), JoltPosition.GetZ() });
				}
			}
		}
	}

	void OnJoltTerminate(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto Resource = Command.GetResource< JoltPhysicsResource>();
		auto JoltData = (JoltPhysicsResourceImpl*)Resource->Data;

		for (auto& MetaData : JoltData->BodiesMetaData)
		{
			if (MetaData.Active == true)
			{
				JoltData->BodyInterFace->RemoveBody(MetaData.BodyID);
				JoltData->BodyInterFace->DestroyBody(MetaData.BodyID);
			}
			MetaData.Active = false;
			// Remove and destroy the floor
		}

		JoltData->BodiesMetaData.Clear();

		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;

		delete Resource->Data;
	}

	void OnJoltHandleEvents(BackBone::SystemContext& Ctxt)
	{
		auto Command = Chilli::Command(Ctxt);
		auto EventService = Command.GetService<EventHandler>();

		for (auto& Event : *EventService->GetEventStorage<CollisionEnterEvent>())
		{
			auto* ColliderA = Command.GetComponent<Collider>(Event.GetEntity1());
			auto* ColliderB = Command.GetComponent<Collider>(Event.GetEntity2());

			// A gets told it hit B
			if (ColliderA)
				ColliderA->OnEnter(Event.GetEntity2(), Ctxt);

			// B gets told it hit A
			if (ColliderB)
				ColliderB->OnEnter(Event.GetEntity1(), Ctxt);
		}

		for (auto& Event : *EventService->GetEventStorage<CollisionStayEvent>())
		{
			auto* ColliderA = Command.GetComponent<Collider>(Event.GetEntity1());
			auto* ColliderB = Command.GetComponent<Collider>(Event.GetEntity2());

			if (ColliderA && ColliderA->OnStay)
				ColliderA->OnStay(Event.GetEntity2(), Ctxt);

			if (ColliderB && ColliderB->OnStay)
				ColliderB->OnStay(Event.GetEntity1(), Ctxt);
		}

		for (auto& Event : *EventService->GetEventStorage<CollisionExitEvent>())
		{
			auto* ColliderA = Command.GetComponent<Collider>(Event.GetEntity1());
			auto* ColliderB = Command.GetComponent<Collider>(Event.GetEntity2());

			if (ColliderA && ColliderA->OnExit)
				ColliderA->OnExit(Event.GetEntity2(), Ctxt);

			if (ColliderB && ColliderB->OnExit)
				ColliderB->OnExit(Event.GetEntity1(), Ctxt);
		}
	}

	void JoltPhysicsExtension::Build(BackBone::App& App)
	{
		App.Registry.AddResource<JoltPhysicsExtensionConfig>();
		App.Registry.AddResource<JoltPhysicsResource>();
		App.Registry.Register<Collider>();
		App.Registry.Register<RigidBody>();
		auto Command = Chilli::Command(App.Ctxt);

		Command.RegisterEvent<CollisionEnterEvent>();
		Command.RegisterEvent<CollisionStayEvent>();
		Command.RegisterEvent<CollisionExitEvent>();

		auto Config = App.Registry.GetResource<JoltPhysicsExtensionConfig>();
		*Config = _Config;

		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::START_UP, OnJoltSetup);

		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::FIXED_PHYSICS, OnJoltHandleConversions);
		App.SystemScheduler.AddSystemOverLayBefore(BackBone::ScheduleTimer::FIXED_PHYSICS, OnJoltClearEvents);

		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::FIXED_PHYSICS, OnJoltVelvertIntegrate);
		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::FIXED_PHYSICS, OnJoltUpdate);
		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::FIXED_PHYSICS, OnJoltSyncBack);
		App.SystemScheduler.AddSystemOverLayAfter(BackBone::ScheduleTimer::FIXED_PHYSICS, OnJoltHandleEvents);

		App.SystemScheduler.AddSystem(BackBone::ScheduleTimer::SHUTDOWN, OnJoltTerminate);
	}

	JPH::ValidateResult JoltContactListenerImpl::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult)
	{
		auto Entity1 = (BackBone::Entity)inBody1.GetUserData();
		auto Entity2 = (BackBone::Entity)inBody2.GetUserData();

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	void JoltContactListenerImpl::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		auto Entity1 = (BackBone::Entity)inBody1.GetUserData();
		auto Entity2 = (BackBone::Entity)inBody2.GetUserData();

		auto Command = Chilli::Command(_Parameter.Ctxt);
		CollisionEnterEvent Enter(Entity1, Entity2);
		Command.AddEvent< CollisionEnterEvent>(Enter);
	}

	void JoltContactListenerImpl::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		auto Entity1 = (BackBone::Entity)inBody1.GetUserData();
		auto Entity2 = (BackBone::Entity)inBody2.GetUserData();

		auto Command = Chilli::Command(_Parameter.Ctxt);
		CollisionStayEvent Enter(Entity1, Entity2);
		Command.AddEvent< CollisionStayEvent>(Enter);
	}

	void JoltContactListenerImpl::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
	{
		auto& Interface = _Parameter.PhysicsSystem->GetBodyLockInterfaceNoLock();

		JPH::BodyLockRead lock1(Interface, inSubShapePair.GetBody1ID());
		JPH::BodyLockRead lock2(Interface, inSubShapePair.GetBody2ID());

		if (lock1.Succeeded() && lock2.Succeeded())
		{
			const JPH::Body& body1 = lock1.GetBody();
			const JPH::Body& body2 = lock2.GetBody();

			auto Entity1 = (BackBone::Entity)body1.GetUserData();
			auto Entity2 = (BackBone::Entity)body2.GetUserData();

			auto Command = Chilli::Command(_Parameter.Ctxt);
			CollisionExitEvent Enter(Entity1, Entity2);
			Command.AddEvent< CollisionExitEvent>(Enter);
		}
	}

#pragma endregion
}

