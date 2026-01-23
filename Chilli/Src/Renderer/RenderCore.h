#pragma once

#include "glm\glm.hpp"

namespace Chilli
{
	enum class RenderStreamTypes : uint8_t
	{
		GRAPHICS,
		TRANSFER,
		COMPUTE
	};

	enum class RenderOpCode : uint16_t {
		// --- Resource Management ---
		COPY_BUFFER,
		COPY_IMAGE,
		UPDATE_BUFFER,
		BLIT_IMAGE,
		RESOLVE_IMAGE,

		// Bindless Related
		UPDATE_GLOBAL_SHADER_DATA,
		UPDATE_SCENE_SHADER_DATA,

		// --- Synchronization ---
		PIPELINE_BARRIER,
		CROSS_RENDER_STREAM_BARRIER,

		// --- State Management (Shader Objects) ---
		BIND_SHADER_PROGRAM,
		SET_FULL_PIPELINE_STATE,
		PATCH_DYNAMIC_STATE,
		SET_VERTEX_LAYOUT,

		// --- Bindless / Resources ---
		BIND_MATERIAL_DATA,
		UPDATE_MATERIL_DATA,
		// Fast 128 byte data allowed per drwa call
		PUSH_SHADER_INLINE_UNIFORM_DATA,

		BIND_VERTEX_BUFFERS,
		BIND_INDEX_BUFFER,

		// --- Action Commands ---
		BEGIN_FRAME,
		BEGIN_RENDER_PASS,
		END_RENDER_PASS,
		END_FRAME,
		DRAW_INDEXED,
		DRAW_INDEXED_INDIRECT,
		DISPATCH,
		DISPATCH_INDIRECT,

		// --- Meta ---
		BEGIN_DEBUG_LABEL,
		END_DEBUG_LABEL,
		EXECUTE_CUSTOM_BUFFER
	};

	enum class ResourceState : uint8_t {
		Undefined,

		HostWrite,      // CPU is filling the buffer
		VertexRead,     // GPU is reading from Vertex Buffer
		IndexRead,      // GPU is reading from Index Buffer

		// Graphics
		RenderTarget,
		DepthWrite,

		// Shader access
		ShaderRead,
		ComputeRead,
		ComputeWrite,

		// Transfer
		CopySrc,
		CopyDst,

		// Presentation
		Present
	};

#define CH_MAX_COLOR_ATTACHMENT_COUNT 8

	// Global Set(0) Data Binding  0
	struct GlobalShaderData
	{
		Vec4 ResolutionTime;
	};

	struct SceneData
	{
		// --- Camera Data ---
		glm::mat4 ViewProjMatrix;
		Vec4 CameraPos;

		// --- Environment / Properties ---
		Vec4 AmbientColor;    // Global light color/intensity
		Vec4 FogProperties;   // x: density, y: start, z: end
		Vec4 ScreenData;      // x: gamma, y: exposure, etc.

		// --- Light Data (Example) ---
		Vec4 GlobalLightDir;  // Directional light for the whole scene
	};

#define MAX_SCENE_DATA_COUNT 16

	struct SceneSettings
	{
		// --- Environment / Properties ---
		Vec4 AmbientColor;    // Global light color/intensity
		Vec4 FogColor;   // x: density, y: start, z: end
		Vec4 FogProperties;   // x: density, y: start, z: end
		Vec4 ScreenData;      // x: gamma, y: exposure, etc.

		// --- Light Data (Example) ---
		Vec4 GlobalLightDir;  // Directional light for the whole scene
	};

	// Global Set(0) Data Binding  1
	struct SceneShaderData
	{
		SceneData Scenes[MAX_SCENE_DATA_COUNT];
	};

	struct Scene
	{
		std::string Name;
		BackBone::Entity MainCamera;
		SceneSettings Settings;
	};

	struct SceneManager
	{
	public:
		SceneManager(BackBone::World* Reg) :_World(Reg) {}
		~SceneManager() {}
		// Lifecycle
		void LoadScene(const std::string& path); // Clears and loads new
		void SaveScene(const std::string& path);
		void AppendScene(const std::string& path); // Adds entities without clearing

		SceneSettings& GetSettings() { return _CurrentSettings; }

		void PushUpdateShaderData();

		void SetActiveScene(Scene* Sc);
		const Scene* GetActiveScene()const;
	private:
		BackBone::World* _World;
		Scene* _ActiveScene;
		SceneSettings _CurrentSettings; // Ambient, Fog, etc.
	};

}
