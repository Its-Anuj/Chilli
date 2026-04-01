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
		uint32_t Padding[2] = { 0,0 };
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

	struct RenderDeviceInfo
	{
		char DeviceName[256];
		uint32_t VendorID;     // PCI ID (0x10DE for NVIDIA, 0x1002 for AMD, etc.)
		uint32_t DeviceID;

		enum class DeviceType {
			Integrated,
			Discrete,
			Virtual,
			CPU,
			Unknown
		} Type;

		// API specific versioning (e.g., Vulkan 1.3, DX 12.2)
		uint32_t APIVersionMajor;
		uint32_t APIVersionMinor;

		uint32_t RawDeviceHandle = UINT32_MAX;
	};

	struct GraphicsMemoryStats
	{
		uint64_t GpuLocal = 0;
		uint64_t Upload = 0;
		uint64_t CpuReadback = 0;
		uint64_t CpuOnly = 0;
	};

	struct RenderDeviceLimits
	{
		// --- Multisampled Anti-Aliasing (MSAA) ---
		// A bitmask representing supported counts: 1, 2, 4, 8, 16, 32, 64
		uint32_t SupportedSampleCounts;
		uint32_t MaxColorSamples;       // Usually 8 or 16
		uint32_t MaxDepthSamples;       // Often matches Color, but sometimes lower

		// --- Texture & Buffer Constraints ---
		uint32_t MaxTextureDimension;   // Usually 8192 or 16384
		uint32_t MaxImageArrayLayers;   // Usually 2048 (Important for 2D Array Textures)
		uint32_t MaxViewports;          // For multi-monitor or VR (Usually 16)
		float    MaxAnisotropy;         // Usually 16.0f

		// --- Shader Constraints ---
		uint32_t MaxBoundDescriptorSets;// Vulkan specific (usually 4-32)
		uint32_t MaxPushConstantsSize;  // Vulkan (minimum guaranteed is 128 bytes)
		uint32_t MaxComputeWorkGroupSize[3]; // [X, Y, Z] for Compute Shaders
		uint32_t MaxComputeWorkGroupInvocations;

		// --- Features (Booleans for API capabilities) ---
		bool bSupportsRayTracing;
		bool bSupportsMeshShaders;
		bool bSupportsBindlessTextures; // Resource Heap in DX12 / Descriptor Indexing in VK
		bool bSupportsVariableRateShading;

		GraphicsMemoryStats MemoryLimits;
	};

	struct RenderDeviceStats
	{
		// --- Resource Counts ---
		uint32_t TotalImagesAllocated = 0;   // Current count of VkImage handles
		uint32_t TotalTexturesCreated = 0;  // Current count of VkImageView handles
		uint32_t TotalBuffersCreated = 0;   // Count of vertex/index/uniform buffers
		uint32_t ActivePipelines = 0;       // Number of unique shader/pso combinations

		// --- Frame Performance (Updated per frame) ---
		uint32_t IndiciesRendered = 0;
		uint32_t VerticesRendered = 0;
		uint32_t DrawCallsPerFrame = 0;
		uint32_t TrianglesPerFrame = 0;
		uint32_t DescriptorSetBinds = 0;

		GraphicsMemoryStats MemoryUsed;
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
		virtual void UpdateSceneShaderData(uint32_t Index, const SceneData& Data) = 0;

		virtual uint32_t PrepareMaterialData(uint32_t ShaderProgramHandle) = 0;
		virtual void ClearMaterialData(uint32_t RawMaterialHandle) = 0;

		virtual const GraphcisBackendCreateSpec& GetSpec() const = 0;
		virtual uint32_t GetCurrentFrameIndex() = 0;

		virtual uint32_t AllocateImage(ImageSpec& Spec) = 0;
		virtual void DestroyImage(uint32_t ImageHandle) = 0;
		virtual void MapImageData(uint32_t ImageHandle, void* Data, int Width, int Height) = 0;

		virtual uint32_t CreateTexture(uint32_t ImageHandle, TextureSpec& Spec) = 0;
		virtual void DestroyTexture(uint32_t TextureHandle) = 0;

		virtual uint32_t CreateSampler(const SamplerSpec& Spec) = 0;
		virtual void DestroySampler(uint32_t SamplerHandle) = 0;

		virtual uint32_t GetTextureShaderIndex(uint32_t RawTextureHandle) = 0;
		virtual uint32_t GetSamplerShaderIndex(uint32_t RawSamplerHandle) = 0;
		virtual uint32_t GetMaterialShaderIndex(uint32_t RawMaterialHandle) = 0;
		virtual uint32_t GetObjectShaderIndex(uint32_t Entity) = 0;

		virtual void UpdateMaterialShaderData(uint32_t MaterialHandle, const MaterialShaderData& Data) = 0;
		virtual void UpdateObjectShaderData(BackBone::Entity Entity, const ObjectShaderData& Data) = 0;

		virtual const std::vector<RenderDeviceInfo>& GetRenderDevices() = 0;
		// Takes in the raw render device handle that is inside the info
		virtual const RenderDeviceLimits& GetRenderDeviceLimit(uint32_t Handle) = 0;
		virtual const RenderDeviceLimits& GetActiveRenderDeviceLimit() = 0;
		virtual const RenderDeviceStats& GetRenderDeviceStats() = 0;
	private:
	};
}
