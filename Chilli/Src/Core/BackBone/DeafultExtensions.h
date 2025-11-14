#pragma once

#include "BackBone.h"
#include "Maths.h"
#include "Events/Events.h"
#include "Window\Window.h"
#include "Input\Input.h"
#include "Renderer/RenderClasses.h"
#include "Renderer/Mesh.h"

namespace Chilli
{
	// ---- Basic Components ----

	struct TransformComponent
	{
		Vec3 Position, Scale, Rotation;
	};

	// Specifies that the entity with this component is visible
	struct VisibilityComponent;

	struct WindowComponent
	{
		WindowSpec Spec;
		BackBone::AssetHandle<Window> WindowHandle;
		std::function<void(Event&)> EventCallback;
	};

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
		BackBone::AssetHandle<GraphicsPipeline> ShaderHandle;
	};

	struct MaterialComponent
	{
		Vec4 AlbedoColor;
	};

	// ---- Basic Extensions ----
	class DeafultExtension : public BackBone::Extension
	{
	public:
		DeafultExtension() {}
		~DeafultExtension(){}

		virtual void Build(BackBone::App& App) override;

		virtual const char* Name() const override { return "DeafultExtension"; }
	private:
	};

	// ---- Basic Systems ----
	class WindowSystem : public BackBone::System
	{
	public:
		WindowSystem() {}
		~WindowSystem() {}

		virtual void OnCreate(BackBone::SystemContext& Registry) override;

		virtual void OnBeforeRun(BackBone::SystemContext& Registry) override;
		virtual void Run(BackBone::SystemContext& Registry) override;
		virtual void OnAfterRun(BackBone::SystemContext& Registry) override {}

		virtual void OnTerminate(BackBone::SystemContext& Registry) override;
	private:
	};

	class RenderSystem : public BackBone::System
	{
	public:
		RenderSystem() {}
		~RenderSystem() {}

		virtual void OnCreate(BackBone::SystemContext& Registry) override;

		virtual void OnBeforeRun(BackBone::SystemContext& Registry) override {}
		virtual void Run(BackBone::SystemContext& Registry) override;;
		virtual void OnAfterRun(BackBone::SystemContext& Registry) override {}

		virtual void OnTerminate(BackBone::SystemContext& Registry) override;
	private:
		std::vector<CommandBufferAllocInfo> _CommandBuffers;
		uint32_t _CurrentFrameCount = 0;
		uint32_t _MaxFrameInFlight = 0;
		uint32_t _TotalFrames = 0;
	};
}
