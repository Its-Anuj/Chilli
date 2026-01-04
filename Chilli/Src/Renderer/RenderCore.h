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
	// Global Set(0) Data Binding  1
	struct SceneShaderData
	{
		glm::mat4 ViewProjMatrix{ 1.0f };
		Vec4 CameraPos;
	};

}
