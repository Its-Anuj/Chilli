#pragma once

//#include "UUID\UUID.h"
#include "Maths.h"
#include "Buffers.h"
#include "Pipeline.h"
#include "Texture.h"
#include "glm/glm.hpp"
#include "Material.h"
#include "RenderPass.h"

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

	enum class CommandType : uint8_t {
		Draw,
		Dispatch,
	};

	struct RenderStateInfo {
		uint32_t ShaderProgramID = UINT32_MAX;
		uint32_t PipelineStateID = UINT32_MAX;
		uint32_t VertexBufferID = UINT32_MAX;
		uint32_t IndexBufferID = UINT32_MAX;
		uint32_t RenderPassIndex = UINT32_MAX;
		uint32_t MaterialID = UINT32_MAX;
		IndexBufferType IndexType = IndexBufferType::NONE;
	};

	struct RenderCommandInfo
	{
		CommandType Type;
		RenderStateInfo RenderState;

		InputTopologyMode TopologyMode;

		CullMode ShaderCullMode;
		PolygonMode ShaderFillMode;
		FrontFaceMode FrontFace;
		float LineWidth = 1.0f;
		ChBool8 RasterizerDiscardEnable = CH_FALSE;

		// Depth Bias (dynamic in most modern APIs)
		ChBool8 DepthBiasEnable = CH_FALSE;
		float DepthBiasConstantFactor = 0.0f;
		float DepthBiasClamp = 0.0f;
		float DepthBiasSlopeFactor = 0.0f;

		// Draw-specific
		// Specifis vertex or index count based on what is bounded
		uint32_t ElementCount = 0;
		uint32_t InstanceCount = 0;
		uint32_t FirstElement = 0;
		uint32_t FirstInstance = 0;

		// Compute-specific
		uint32_t GroupX = 0;
		uint32_t GroupY = 0;
		uint32_t GroupZ = 0;
		// Data references (not owned!)
		struct {
			void* Block = nullptr;
			size_t Offst = 0;
			size_t TotalSize = 0;
			uint32_t Stages = 0;
		} InlineUniformData;
	};

	struct RenderCommandInfoInlineUniformData
	{
		void* Data;
		uint32_t State;
		uint32_t Size;
		uint32_t Offset;
	};

	struct RenderShaderDataUpdateInfo
	{
		uint32_t MaterialHandle = UINT32_MAX;
		char Name[SHADER_UNIFORM_BINDING_NAME_SIZE] = { 0 };
		uint32_t DstArrayIndex = UINT32_MAX;

		bool Persistent = false;

		struct {
			uint32_t Handle = UINT32_MAX;
			uint32_t Offset = 0;
			uint32_t Range = 0;
		} BufferInfo;

		struct {
			uint32_t Handle = UINT32_MAX;
			uint32_t Sampler = UINT32_MAX;
		} ImageInfo;
	};

	// Submits Info To Gpu
	struct GraphicsCommandBuffer
	{
		std::vector<RenderCommandInfo> _RenderCommandInfos;
		std::vector<CompiledPass> _RenderPasses;
		std::vector< RenderShaderDataUpdateInfo> _ShaderUpdateInfos;
		std::vector<PipelineStateInfo> _PipelineStates;
		uint32_t _PipelineOffset = 0;
		RenderCommandInfo _CurrentInfo;

		void Clear()
		{
			_RenderCommandInfos.clear();
			_RenderPasses.clear();
			_PipelineStates.clear();
			_ShaderUpdateInfos.clear();
			_PipelineOffset = 0;
		}
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
		virtual void EndFrame(const std::vector<GraphicsCommandBuffer>& CmdBuffers) = 0;

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
