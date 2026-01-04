#pragma once

//#include "UUID\UUID.h"
#include "Maths.h"
#include "Buffers.h"
#include "Pipeline.h"
#include "Texture.h"
#include "glm/glm.hpp"
#include "Material.h"
#include "CommandBuffer.h"

namespace Chilli
{
	enum class GraphicsBackendType
	{
		VULKAN_1_3,
		DIRECTX_12
	};

	struct GraphicsDeviceFeatures
	{
		bool EnableSwapChain;
		bool EnableDescriptorIndexing;
	};

	enum class CommandBufferPurpose
	{
		NONE,
		GRAPHICS,
		PRESENT,
		TRANSFER,
		COMPUTE,
		COUNT
	};

	enum CommandBufferSubmitState
	{
		COMMAND_BUFFER_SUBMIT_STATE_ONE_TIME = 1, COMMAND_BUFFER_SUBMIT_STATE_SIMULTANEOUS_USE = 2, COMMAND_BUFFER_SUBMIT_STATE_RENDER_PASS_CONTINUE = 4, COMMAND_BUFFER_SUBMIT_STATE_NONE = 8,
		COMMAND_BUFFER_SUBMIT_STATE_LAST = COMMAND_BUFFER_SUBMIT_STATE_NONE,
		COMMAND_BUFFER_SUBMIT_STATE_COUNT = std::countr_zero((uint32_t)COMMAND_BUFFER_SUBMIT_STATE_LAST) + 1,
	};

	inline uint32_t GetCommandBufferSubmitTypeToIndex(uint32_t Stage) {
		return std::countr_zero((uint32_t)Stage);
	}

	struct CommandBufferAllocInfo
	{
		uint32_t CommandBufferHandle;
		CommandBufferPurpose Purpose = CommandBufferPurpose::NONE;
		int State = CommandBufferSubmitState::COMMAND_BUFFER_SUBMIT_STATE_NONE;
	};

	struct Semaphore
	{

	};

	struct GraphcisBackendCreateSpec
	{
		GraphicsBackendType Type = GraphicsBackendType::VULKAN_1_3;
		const char* Name = "UnDefined";
		bool EnableValidation = true;
		GraphicsDeviceFeatures DeviceFeatures{
			.EnableSwapChain = true,
			.EnableDescriptorIndexing = true,
		};

		void* WindowSurfaceHandle = nullptr;

		bool VSync = true;;
		IVec2 ViewPortSize = { 800, 600 };
		bool ViewPortResized = true;
		uint32_t MaxFrameInFlight = 3;
	};

	struct BufferCopyInfo
	{
		uint32_t SrcOffset, DstOffset, Size;
	};

	struct Fence
	{
		uint32_t Handle;
	};

	const uint32_t CH_MATERIAL_SHADER_DATA_AMOUNT = 1000;

	struct MaterialShaderData
	{
		uint32_t AlbedoTextureIndex;
		uint32_t AlbedoSamplerIndex;
		uint32_t Padding[2];
		Vec4 AlbedoColor;
	};

	const uint32_t CH_OBJECT_SHADER_DATA_AMOUNT = 1000;

	struct ObjectShaderData
	{
		glm::mat4 TransformationMat;
		glm::mat4 InverseTransformationMat;
	};

	struct RenderFramePacket
	{
		GraphicsCommandBuffer Graphics_Stream;
		RenderCommandBuffer Compute_Stream;
		RenderCommandBuffer Transfer_Stream;	
	};

	struct RenderCommandInfoInlineUniformData
	{
		void* Data;
		uint32_t State;
		uint32_t Size;
		uint32_t Offset;
	};

	struct RenderFrameData
	{
		uint32_t CurrentFrameIndex = 0;
	};

	class GraphicsBackendApi
	{
	public:
		virtual ~GraphicsBackendApi() {}

		virtual void Init(const GraphcisBackendCreateSpec& Spec) = 0;
		virtual void Terminate() = 0;

		virtual const char* GetName() const = 0;
		virtual GraphicsBackendType GetType() const = 0;

		static GraphicsBackendApi* Create(const GraphcisBackendCreateSpec& Spec);
		static void Terminate(GraphicsBackendApi* Api, bool Delete = false);

		virtual void FrameBufferResize(int Width, int Height) = 0;

		virtual void BeginFrame(uint32_t Index) = 0;
		virtual void EndFrame(const RenderFramePacket& Packet) = 0;

		virtual uint32_t AllocateBuffer(const BufferCreateInfo& Info) = 0;
		virtual void MapBufferData(uint32_t BufferHandle, void* Data, uint32_t Size, uint32_t Offset = 0) = 0;
		virtual void FreeBuffer(uint32_t BufferHandle) = 0;

		virtual void PrepareForShutDown() = 0;

		virtual ShaderModule CreateShaderModule(const char* FilePath, ShaderStageType Type) = 0;
		virtual void DestroyShaderModule(const ShaderModule& Module) = 0;

		virtual uint32_t MakeShaderProgram() = 0;
		virtual void AttachShader(uint32_t ProgramHandle, const ShaderModule& Shader) = 0;
		virtual void ClearShaderProgram(uint32_t ProgramHandle) = 0;
		virtual void LinkShaderProgram(uint32_t ProgramHandle) = 0;

		virtual void UpdateGlobalShaderData(const GlobalShaderData& Data) = 0;
		virtual void UpdateSceneShaderData(const SceneShaderData& Data) = 0;

		virtual uint32_t PrepareMaterialData(uint32_t ShaderProgramHandle) = 0;
		virtual void ClearMaterialData(uint32_t RawMaterialHandle) = 0;

		virtual const GraphcisBackendCreateSpec& GetSpec() const = 0;
		virtual uint32_t GetCurrentFrameIndex() = 0;

		virtual uint32_t AllocateImage(const ImageSpec& Spec) = 0;
		virtual void DestroyImage(uint32_t ImageHandle) = 0;
		virtual void MapImageData(uint32_t ImageHandle, void* Data, int Width, int Height) = 0;

		virtual uint32_t CreateTexture(uint32_t ImageHandle, const TextureSpec& Spec) = 0;
		virtual void DestroyTexture(uint32_t TextureHandle) = 0;

		virtual uint32_t CreateSampler(const SamplerSpec& Spec) = 0;
		virtual void DestroySampler(uint32_t SamplerHandle) = 0;

		virtual uint32_t GetTextureShaderIndex(uint32_t RawTextureHandle) = 0;
		virtual uint32_t GetSamplerShaderIndex(uint32_t RawSamplerHandle) = 0;
		virtual uint32_t GetMaterialShaderIndex(uint32_t RawMaterialHandle) = 0;

		virtual void UpdateMaterialShaderData(uint32_t MaterialHandle, const MaterialShaderData& Data) = 0;
		virtual void UpdateObjectShaderData(const ObjectShaderData& Data) = 0;
	private:
	};
}
