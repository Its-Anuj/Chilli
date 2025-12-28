#pragma once

#include "BackBone.h"
#include "Image.h"
#include "Sampler.h"

using ChBool8 = uint8_t;
using ChBool16 = uint16_t;
using ChBool32 = uint32_t;

#define CH_TRUE true
#define CH_FALSE false
#define CH_NONE 0xFF

inline bool ChBoolToBool(ChBool8 chBool) {
	return chBool == CH_TRUE;
}

namespace Chilli
{
	enum class ShaderObjectTypes
	{
		FLOAT1,
		FLOAT2,
		FLOAT3,
		FLOAT4,
		INT1,
		INT2,
		INT3,
		INT4,
		UINT1,
		UINT2,
		UINT3,
		UINT4,
	};

	inline int ShaderTypeToSize(Chilli::ShaderObjectTypes Type)
	{
		switch (Type)
		{
		case Chilli::ShaderObjectTypes::FLOAT1:
			return sizeof(float) * 1;
		case Chilli::ShaderObjectTypes::FLOAT2:
			return sizeof(float) * 2;
		case Chilli::ShaderObjectTypes::FLOAT3:
			return sizeof(float) * 3;
		case Chilli::ShaderObjectTypes::FLOAT4:
			return sizeof(float) * 4;
		case Chilli::ShaderObjectTypes::INT1:
			return sizeof(int) * 1;
		case Chilli::ShaderObjectTypes::INT2:
			return sizeof(int) * 2;
		case Chilli::ShaderObjectTypes::INT3:
			return sizeof(int) * 3;
		case Chilli::ShaderObjectTypes::INT4:
			return sizeof(int) * 4;
		case Chilli::ShaderObjectTypes::UINT1:
			return sizeof(unsigned int) * 1;
		case Chilli::ShaderObjectTypes::UINT2:
			return sizeof(unsigned int) * 2;
		case Chilli::ShaderObjectTypes::UINT3:
			return sizeof(unsigned int) * 3;
		case Chilli::ShaderObjectTypes::UINT4:
			return sizeof(unsigned int) * 4;
		default:
			return -1;
		}
	}

	inline int ShaderTypeToElementCount(Chilli::ShaderObjectTypes Type)
	{
		switch (Type)
		{
		case Chilli::ShaderObjectTypes::FLOAT1:
			return 1;
		case Chilli::ShaderObjectTypes::FLOAT2:
			return 2;
		case Chilli::ShaderObjectTypes::FLOAT3:
			return 3;
		case Chilli::ShaderObjectTypes::FLOAT4:
			return 4;
		case Chilli::ShaderObjectTypes::INT1:
			return  1;
		case Chilli::ShaderObjectTypes::INT2:
			return  2;
		case Chilli::ShaderObjectTypes::INT3:
			return  3;
		case Chilli::ShaderObjectTypes::INT4:
			return  4;
		case Chilli::ShaderObjectTypes::UINT1:
			return  1;
		case Chilli::ShaderObjectTypes::UINT2:
			return  2;
		case Chilli::ShaderObjectTypes::UINT3:
			return  3;
		case Chilli::ShaderObjectTypes::UINT4:
			return  4;
		default:
			return -1;
		}
	}

	enum ShaderStageType : uint32_t
	{
		SHADER_STAGE_NONE,
		// Rasterization pipeline
		SHADER_STAGE_VERTEX = 1 << 0,   // 0x00000001
		SHADER_STAGE_TESSELLATION_CONTROL = 1 << 1,   // 0x00000002
		SHADER_STAGE_TESSELLATION_EVALUATION = 1 << 2,   // 0x00000004
		SHADER_STAGE_GEOMETRY = 1 << 3,   // 0x00000008
		SHADER_STAGE_FRAGMENT = 1 << 4,   // 0x00000010

		// Modern Mesh Shader pipeline
		SHADER_STAGE_TASK = 1 << 5,   // 0x00000020  (Amplification shader)
		SHADER_STAGE_MESH = 1 << 6,   // 0x00000040

		// Compute
		SHADER_STAGE_COMPUTE = 1 << 7,   // 0x00000080

		// Ray Tracing pipeline
		SHADER_STAGE_RAYGEN = 1 << 8,   // 0x00000100
		SHADER_STAGE_ANY_HIT = 1 << 9,   // 0x00000200
		SHADER_STAGE_CLOSEST_HIT = 1 << 10,  // 0x00000400
		SHADER_STAGE_MISS = 1 << 11,  // 0x00000800
		SHADER_STAGE_INTERSECTION = 1 << 12,  // 0x00001000
		SHADER_STAGE_CALLABLE = 1 << 13,  // 0x00002000,
		SHADER_STAGE_LAST = SHADER_STAGE_CALLABLE,

		// Convenience masks
		SHADER_STAGE_ALL_GRAPHICS =
		SHADER_STAGE_VERTEX |
		SHADER_STAGE_TESSELLATION_CONTROL |
		SHADER_STAGE_TESSELLATION_EVALUATION |
		SHADER_STAGE_GEOMETRY |
		SHADER_STAGE_FRAGMENT |
		SHADER_STAGE_TASK |
		SHADER_STAGE_MESH,

		SHADER_STAGE_ALL_RAY_TRACING =
		SHADER_STAGE_RAYGEN |
		SHADER_STAGE_ANY_HIT |
		SHADER_STAGE_CLOSEST_HIT |
		SHADER_STAGE_MISS |
		SHADER_STAGE_INTERSECTION |
		SHADER_STAGE_CALLABLE,

		SHADER_STAGE_COUNT = std::countr_zero((uint32_t)SHADER_STAGE_LAST) + 1,
	};

	inline uint32_t GetShaderStageTypeToIndex(uint32_t Stage) {
		if (Stage == SHADER_STAGE_NONE)
			return UINT32_MAX; // or any sentinel you prefer

		return std::countr_zero((uint32_t)Stage);
	}

	enum class ShaderUniformTypes
	{
		UNIFORM_BUFFER,
		STORAGE_BUFFER,
		SAMPLED_IMAGE,
		SAMPLER,
		COMBINED_IMAGE_SAMPLER,
		UNKNOWN
	};

	enum class BindlessSetTypes
	{
		GLOBAL_SCENE = 0,
		TEX_MAP_USAGE = GLOBAL_SCENE,
		TEX_SAMPLERS = 1,
		SAMPLER_MAP_USAGE = TEX_SAMPLERS,
		// This is due to Scene UBO also needing a Uniform Buffer but we cant access 2 BUffers from GLOBAL_SCENE thus, utilzing TEX_SAMPLERS Index to serve as a Scene Buffer In Graphics Backend Bindless Rendering System
		SCENE_BUFFER_USAGE = TEX_SAMPLERS,
		MATERIAl = 2,
		OBJECT_MAP_USAGE = MATERIAl,
 		PER_OBJECT = 3,
		MAP_USAGE_COUNT = PER_OBJECT,
		COUNT_NON_USER,
		USER_0 = COUNT_NON_USER,
		USER_1 = 5,
		USER_2 = 6,
		USER_3 = 7,
		COUNT,
		COUNT_USER = USER_3 - COUNT_NON_USER + 1,
	};

	enum BindlessRenderingLimits
	{
		MAX_TEXTURES = 100000,
		MAX_SAMPLERS = 16,
		MAX_UNIFORM_BUFFERS = 500,
		MAX_STORAGE_BUFFERS = 500,
	};

	enum class CullMode : uint8_t { None, Front, Back };
	enum class PolygonMode : uint8_t { None, Fill, Wireframe };
	enum class InputTopologyMode : uint8_t { None, Triangle_List, Triangle_Strip };
	enum class FrontFaceMode : uint8_t { None, Clock_Wise, Counter_Clock_Wise };

	struct GraphicsPipelineCreateInfo
	{
		std::string VertPath;
		std::string FragPath;

		bool ColorBlend = true;
		InputTopologyMode TopologyMode = InputTopologyMode::Triangle_List;
		CullMode ShaderCullMode = CullMode::Back;
		PolygonMode  ShaderFillMode = PolygonMode::Fill;
		FrontFaceMode FrontFace = FrontFaceMode::Clock_Wise;

		bool EnableDepthStencil = false;
		bool EnableDepthTest = false;
		bool EnableDepthWrite = false;
		bool EnableStencilTest = false;
		ImageFormat DepthFormat;

		bool UseSwapChainColorFormat = true;
		ImageFormat ColorFormat;
	};

#define SHADER_UNIFORM_BINDING_NAME_SIZE 32

	struct ReflectedVertexInput {
		std::string Name;
		uint32_t Location;
		ShaderObjectTypes Format; // VkFormat
		uint32_t Offset;
	};

	struct ReflectedBindingUniformInput
	{
		uint32_t Binding = 0;
		uint32_t Set = 0;
		uint32_t Count = 0;
		Chilli::ShaderUniformTypes Type;
		char Name[SHADER_UNIFORM_BINDING_NAME_SIZE] = { 0 };
		Chilli::ShaderStageType Stage;
	};

	struct ReflectedSetUniformInput
	{
		uint32_t Set = 0;
		std::vector< ReflectedBindingUniformInput> Bindings;
	};

	constexpr size_t MAX_INLINE_UNIFORM_DATA_SIZE = 128;

	struct ReflectedInlineUniformData
	{
		uint64_t Stage;
		uint32_t Offset = 0;
		uint32_t Size = 0;
	};

	struct ReflectedShaderInfo
	{
		std::vector< ReflectedVertexInput> VertexInputs;
		std::vector< ReflectedSetUniformInput> UniformInputs;
		std::vector< ReflectedInlineUniformData> PushConstants;
	};

	struct ShaderModule
	{
		uint32_t RawModuleHandle = BackBone::npos;
		std::string Path;
		std::string Name;
		ShaderStageType Stage;
		std::string EntryPoint = "main";
		uint64_t BinaryHash = 0;
	};

	struct ShaderProgram
	{
		uint32_t RawProgramHandle = -1;
	};

	enum ShaderDynamicStates {
		SHADER_DYNAMIC_STATE_VIEWPORT = (1 << 0),
		SHADER_DYNAMIC_STATE_SCISSOR = (1 << 1),
		SHADER_DYNAMIC_STATE_CULL_MODE = (1 << 2),
		SHADER_DYNAMIC_STATE_FRONT_FACE = (1 << 3),
		SHADER_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY = (1 << 4),
		SHADER_DYNAMIC_STATE_DEPTH_TEST_ENABLE = (1 << 5),
		SHADER_DYNAMIC_STATE_DEPTH_WRITE_ENABLE = (1 << 6),
		SHADER_DYNAMIC_STATE_DEPTH_COMPARE_OP = (1 << 7),
		SHADER_DYNAMIC_STATE_STENCIL_TEST_ENABLE = (1 << 8),
		SHADER_DYNAMIC_STATE_POLYGON_MODE = (1 << 9),
		SHADER_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE = (1 << 10),
		SHADER_DYNAMIC_STATE_COLOR_WRITE_ENABLE = (1 << 11),
		SHADER_DYNAMIC_STATE_COLOR_BLEND_ENABLE = (1 << 12),
		SHADER_DYNAMIC_STATE_LINE_WIDTH = (1 << 13),

		SHADER_DYNAMIC_ALL = SHADER_DYNAMIC_STATE_VIEWPORT | SHADER_DYNAMIC_STATE_SCISSOR |
		SHADER_DYNAMIC_STATE_CULL_MODE | SHADER_DYNAMIC_STATE_FRONT_FACE | SHADER_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY | SHADER_DYNAMIC_STATE_DEPTH_TEST_ENABLE | SHADER_DYNAMIC_STATE_DEPTH_WRITE_ENABLE | SHADER_DYNAMIC_STATE_DEPTH_COMPARE_OP | SHADER_DYNAMIC_STATE_STENCIL_TEST_ENABLE | SHADER_DYNAMIC_STATE_POLYGON_MODE | SHADER_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE | SHADER_DYNAMIC_STATE_COLOR_WRITE_ENABLE | SHADER_DYNAMIC_STATE_COLOR_BLEND_ENABLE | SHADER_DYNAMIC_STATE_LINE_WIDTH,
	};

	struct ShaderDescriptorBindingFlags
	{
		uint8_t Set = 0;
		uint8_t Binding = 0;

		// UPDATe After bind
		bool EnableBindless = false;

		bool EnableStreaming = false;
		bool EnablePartiallyBound = false;

		bool EnableVariableSize = false;
		uint32_t VariableSize = BackBone::npos;

		uint64_t StageFlags = 0;

		// Equality operator
		bool operator==(const ShaderDescriptorBindingFlags& other) const
		{
			return Set == other.Set
				&& Binding == other.Binding
				&& EnableBindless == other.EnableBindless
				&& EnableStreaming == other.EnableStreaming
				&& EnablePartiallyBound == other.EnablePartiallyBound
				&& EnableVariableSize == other.EnableVariableSize
				&& VariableSize == other.VariableSize
				&& StageFlags == other.StageFlags;
		}
		// Equality operator
		bool operator!=(const ShaderDescriptorBindingFlags& other) const
		{
			return Set != other.Set
				&& Binding != other.Binding
				&& EnableBindless != other.EnableBindless
				&& EnableStreaming != other.EnableStreaming
				&& EnablePartiallyBound != other.EnablePartiallyBound
				&& EnableVariableSize != other.EnableVariableSize
				&& VariableSize != other.VariableSize
				&& StageFlags != other.StageFlags;
		}

		static uint64_t HashFlag(const ShaderDescriptorBindingFlags& Flag);
	};

	enum class BlendFactor : uint32_t
	{
		ZERO,
		ONE,
		SRC_COLOR,
		ONE_MINUS_SRC_COLOR,
		DST_COLOR,
		ONE_MINUS_DST_COLOR,
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		DST_ALPHA,
		ONE_MINUS_DST_ALPHA
		// ... more factors as needed
	};

	enum class BlendOp : uint32_t
	{
		ADD,
		SUBTRACT,
		REVERSE_SUBTRACT,
		MIN,
		MAX
	};

	enum class StencilOp : uint32_t
	{
		KEEP,
		ZERO,
		REPLACE,
		INCREMENT_AND_CLAMP,
		DECREMENT_AND_CLAMP,
		INVERT,
		INCREMENT_AND_WRAP,
		DECREMENT_AND_WRAP
	};

	// --- Helper Structs for Complex States ---

	struct StencilOpState
	{
		StencilOp FailOp = StencilOp::KEEP;
		StencilOp PassOp = StencilOp::KEEP;
		StencilOp DepthFailOp = StencilOp::KEEP;
		CompareOp CompareFunction = CompareOp::ALWAYS;
		uint32_t CompareMask = 0xFFFFFFFF;
		uint32_t WriteMask = 0xFFFFFFFF;
		uint32_t Reference = 0;

		bool operator!=(const StencilOpState& Other) const
		{
			return
				FailOp != Other.FailOp ||
				PassOp != Other.PassOp ||
				DepthFailOp != Other.DepthFailOp ||
				CompareFunction != Other.CompareFunction ||
				CompareMask != Other.CompareMask ||
				WriteMask != Other.WriteMask ||
				Reference != Other.Reference;
		}
	};

	struct ColorBlendAttachmentState
	{
		ChBool8 BlendEnable = CH_NONE;
		BlendFactor SrcColorFactor = BlendFactor::ONE;
		BlendFactor DstColorFactor = BlendFactor::ZERO;
		BlendOp ColorBlendOp = BlendOp::ADD;
		BlendFactor SrcAlphaFactor = BlendFactor::ONE;
		BlendFactor DstAlphaFactor = BlendFactor::ZERO;
		BlendOp AlphaBlendOp = BlendOp::ADD;
		uint32_t ColorWriteMask = 0xF; // 0xF is R|G|B|A

		inline static ColorBlendAttachmentState OpaquePass()
		{
			return 	ColorBlendAttachmentState{
				.BlendEnable = CH_FALSE,
				.SrcColorFactor = BlendFactor::ONE,
				.DstColorFactor = BlendFactor::ZERO,
				.ColorBlendOp = BlendOp::ADD,
				.SrcAlphaFactor = BlendFactor::ONE,
				.DstAlphaFactor = BlendFactor::ZERO,
				.AlphaBlendOp = BlendOp::ADD,
				.ColorWriteMask = 0xF // R|G|B|A
			};
		}
	};

	struct VertexInputShaderAttribute
	{
		ShaderObjectTypes Type;
		uint32_t Offset = 0;
		std::string Name; // Changed to std::string for safety, or keep const char* if managing memory carefully
		uint32_t Location = 0; // 'Binding' usually refers to the Buffer index, 'Location' to the shader input index

		// Helper to calculate size based on type
		static uint32_t GetSize(ShaderObjectTypes type) {
			return ShaderTypeToSize(type);
		}
	};

	struct VertexInputShaderBinding
	{
		std::vector<VertexInputShaderAttribute> Attribs;
		uint32_t Stride = 0;
		uint32_t BindingIndex = 0; // Which slot (0, 1, etc.) this buffer binds to
	};

	struct VertexInputShaderLayout
	{
		std::vector<VertexInputShaderBinding> Bindings;

		// Helper to start a new buffer binding
		void BeginBinding(uint32_t bindingIndex)
		{
			VertexInputShaderBinding newBinding;
			newBinding.BindingIndex = bindingIndex;
			Bindings.push_back(newBinding);
		}

		/// <summary>
		/// Adds a vertex attribute to the shader layout configuration.
		/// </summary>
		/// <param name="type">The data type of the attribute (e.g., Float3, Vec2).</param>
		/// <param name="name">The name of the attribute as defined in the shader.</param>
		/// <param name="location">The layout location index.</param>
		/// <param name="Hide">
		/// If false (default), the attribute is added normally. 
		/// If true, the attribute is not registered in the shader, but its size is still 
		/// added to the global stride, effectively "skipping" these bytes in the vertex 
		/// buffer and adding them as an offset to the next attribute.
		/// </param>
		void AddAttribute(ShaderObjectTypes type, const std::string& name, uint32_t location, bool Hide = false)
		{
			if (Bindings.empty()) BeginBinding(0);

			auto& currentBinding = Bindings.back();

			VertexInputShaderAttribute attrib;
			attrib.Type = type;
			attrib.Name = name;
			attrib.Location = location;

			// Auto-calculate offset based on current stride
			attrib.Offset = currentBinding.Stride;

			if (Hide == false)
				// Push attribute
				currentBinding.Attribs.push_back(attrib);

			// Update stride
			currentBinding.Stride += VertexInputShaderAttribute::GetSize(attrib.Type);
		}
	};

	struct PipelineStateInfo
	{
		// --- 1. Input Assembly ---
		InputTopologyMode TopologyMode = InputTopologyMode::None;
		ChBool8 PrimitiveRestartEnable = CH_NONE; // Often static, but can be dynamic

		// --- 2. Rasterization ---
		CullMode ShaderCullMode = CullMode::None;
		PolygonMode ShaderFillMode = PolygonMode::None;
		FrontFaceMode FrontFace = FrontFaceMode::None;
		float LineWidth = -1.0f;
		ChBool8 RasterizerDiscardEnable = CH_NONE;

		// Depth Bias (dynamic in most modern APIs)
		ChBool8 DepthBiasEnable = CH_NONE;
		float DepthBiasConstantFactor = 0.0f;
		float DepthBiasClamp = 0.0f;
		float DepthBiasSlopeFactor = 0.0f;

		// --- 3. Depth & Stencil ---
		ChBool8 DepthTestEnable = CH_NONE;
		ChBool8 DepthWriteEnable = CH_NONE;
		CompareOp DepthCompareOp = CompareOp::NEVER;

		// Stencil operations are also dynamic if VK_EXT_extended_dynamic_state is used
		ChBool8 StencilTestEnable = CH_NONE;
		StencilOpState StencilFront;
		StencilOpState StencilBack;

		// === Multisampling (MSAA) State ===
		uint32_t SampleCount = UINT32_MAX; // 1 = VK_SAMPLE_COUNT_1_BIT
		uint32_t SampleMask = 0x0;
		ChBool8 AlphaToCoverageEnable = CH_NONE;

		// --- 4. Blending/Color Write ---
		// If blend equations are dynamic (rare, but possible)
		std::vector<ColorBlendAttachmentState> ColorBlendAttachments;

		VertexInputShaderLayout VertexInputLayout;
	};

	class PipelineBuilder {
	public:
		PipelineBuilder() = default;

		/**
		 * Creates a builder initialized with your specific "builder" defaults:
		 * Triangle List, Back Culling, Depth Disabled, No MSAA, Opaque Blend.
		 */
		static PipelineBuilder Default() {
			PipelineBuilder builder;

			// === Input Assembly State ===
			builder._state.TopologyMode = InputTopologyMode::Triangle_List;
			builder._state.PrimitiveRestartEnable = false;

			// === Rasterization State ===
			builder._state.ShaderCullMode = CullMode::Back;          // Typical for 2D UI/Quads
			builder._state.FrontFace = FrontFaceMode::Counter_Clock_Wise;
			builder._state.ShaderFillMode = PolygonMode::Fill;
			builder._state.LineWidth = 1.0f;
			builder._state.RasterizerDiscardEnable = false;

			// Depth Bias (Disabled)
			builder._state.DepthBiasEnable = false;
			builder._state.DepthBiasConstantFactor = 0.0f;
			builder._state.DepthBiasClamp = 0.0f;
			builder._state.DepthBiasSlopeFactor = 0.0f;

			// === Depth State (Disabled) ===
			builder._state.DepthTestEnable = false;
			builder._state.DepthWriteEnable = false;
			builder._state.DepthCompareOp = CompareOp::LESS_OR_EQUAL;

			// === Stencil State (Disabled) ===
			builder._state.StencilTestEnable = false;
			builder._state.StencilFront = StencilOpState{};          // Default initialized to keep/always
			builder._state.StencilBack = StencilOpState{};           // Default initialized to keep/always

			// === Multisampling (MSAA) State ===
			builder._state.SampleCount = 1;                          // No MSAA
			builder._state.SampleMask = 0xFFFFFFFF;
			builder._state.AlphaToCoverageEnable = false;


			// The struct defaults cover most of it, we just add the default attachment
			builder._state.ColorBlendAttachments.push_back(ColorBlendAttachmentState::OpaquePass());
			return builder;
		}

		// --- Input Assembly ---
		PipelineBuilder& SetTopology(InputTopologyMode mode, bool restart = false) {
			_state.TopologyMode = mode;
			_state.PrimitiveRestartEnable = restart;
			return *this;
		}

		// --- Rasterization (Granular) ---
		PipelineBuilder& SetRasterizer(CullMode cull, FrontFaceMode front = FrontFaceMode::Counter_Clock_Wise, PolygonMode fill = PolygonMode::Fill) {
			_state.ShaderCullMode = cull;
			_state.FrontFace = front;
			_state.ShaderFillMode = fill;
			return *this;
		}

		PipelineBuilder& SetLineWidth(float width) { _state.LineWidth = width; return *this; }
		PipelineBuilder& SetRasterizerDiscard(bool discard) { _state.RasterizerDiscardEnable = discard; return *this; }

		PipelineBuilder& SetDepthBias(bool enable, float constant = 0.0f, float clamp = 0.0f, float slope = 0.0f) {
			_state.DepthBiasEnable = enable;
			_state.DepthBiasConstantFactor = constant;
			_state.DepthBiasClamp = clamp;
			_state.DepthBiasSlopeFactor = slope;
			return *this;
		}

		// --- Depth & Stencil (Granular) ---
		PipelineBuilder& SetDepth(bool test, bool write, CompareOp op = CompareOp::LESS_OR_EQUAL) {
			_state.DepthTestEnable = test;
			_state.DepthWriteEnable = write;
			_state.DepthCompareOp = op;
			return *this;
		}

		PipelineBuilder& SetStencil(bool enable, StencilOpState front = {}, StencilOpState back = {}) {
			_state.StencilTestEnable = enable;
			_state.StencilFront = front;
			_state.StencilBack = back;
			return *this;
		}

		// --- Multisampling ---
		PipelineBuilder& SetMSAA(uint32_t samples, uint32_t mask = 0xFFFFFFFF, bool alphaToCoverage = false) {
			_state.SampleCount = samples;
			_state.SampleMask = mask;
			_state.AlphaToCoverageEnable = alphaToCoverage;
			return *this;
		}

		// --- Blending & Layout ---
		PipelineBuilder& AddColorBlend(const ColorBlendAttachmentState& blend) {
			_state.ColorBlendAttachments.push_back(blend);
			return *this;
		}

		PipelineBuilder& ClearColorBlends() {
			_state.ColorBlendAttachments.clear();
			return *this;
		}

		PipelineBuilder& SetVertexLayout(const VertexInputShaderLayout& layout) {
			_state.VertexInputLayout = layout;
			return *this;
		}

		// --- Finalize ---
		PipelineStateInfo Build() {
			return _state;
		}

	private:
		PipelineStateInfo _state;
	};
}
