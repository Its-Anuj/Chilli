#pragma once

#include "BackBone.h"
#include "Events/Events.h"
#include "Renderer/RenderClasses.h"
#include "Renderer/Mesh.h"
#include "Material.h"
#include "AssetLoader.h"
#include <cstdint>
#include "EventHandler.h"

namespace Chilli
{
	// Line Renderer
#pragma region Blaze

	enum class BlazeMode : uint32_t
	{
		// 1. SCREEN SPACE (Pepper UI)
	// No depth test, no depth write. Coordinates are usually 0->1 or Pixels.
	// Used for: UI Borders, selection boxes, crosshairs.
		OVERLAY = 0,

		// 2. INTERACTIVE DEPTH (Geometry/World)
		// Depth test ON, Depth write ON.
		// Used for: Laser beams, fences, actual 3D objects made of lines.
		WORLDSPACE = 1,

		// 3. OCCLUDED / X-RAY (Debug/Analysis)
		// Depth test ON (Compare), but Depth write OFF. 
		// Often rendered with a specific "Ghost" color or bias.
		// Used for: Seeing player skeletons through walls, debug paths.
		XRAY = 2,

		// 4. PERSISTENT OVERLAY (Gizmos)
		// Depth test OFF, Depth write OFF, but rendered in 3D space.
		// Used for: Transform gizmos (RGB axes) that must always be visible.
		GiIZMO = 3
	};

	struct BlazeVertex
	{
		Vec3 Vertices;
	};

	struct BlazeInlineUniformDataStruct
	{
		uint32_t ModeFlag;
		Vec4 Color;
	};

	struct BlazeMeshComponent
	{
		BackBone::AssetHandle<Buffer> VertexBuffer;

		uint32_t LineCount;
		BlazeMode Mode;
		Vec4 Color;
		float LineWidth = 1.0f;
	};

	struct BlazeResource
	{
		BackBone::AssetHandle<ShaderProgram> Shader;
	};

	struct BlazeExtensionConfig
	{
		bool Enable = true;
	};

	class BlazeExtension : public BackBone::Extension
	{
	public:
		BlazeExtension(const BlazeExtensionConfig& Config = BlazeExtensionConfig())
			:_Config(Config)
		{
		}
		~BlazeExtension() {}

		virtual void Build(BackBone::App& App) override;

		virtual const char* Name() const override { return "BlazeExtension"; }
	private:
		BlazeExtensionConfig _Config;
	};

	void OnBlazeSetup(BackBone::SystemContext& Ctxt);
	void OnBlazeUpdate(BackBone::SystemContext& Ctxt);
	void OnBlazeShutDown(BackBone::SystemContext& Ctxt);

	void OnBlazeRenderOverlay(BackBone::SystemContext& Ctxt, RenderPassDesc& Info);
	void OnBlazeRenderWorldSpace(BackBone::SystemContext& Ctxt, RenderPassDesc& Info);
	void OnBlazeRenderXRay(BackBone::SystemContext& Ctxt, RenderPassDesc& Info);
	void OnBlazeRenderGizmo(BackBone::SystemContext& Ctxt, RenderPassDesc& Info);


#pragma endregion 

#pragma region Render Extension
	struct DefferedRenderingConfig
	{
		bool GeometryPass = true;
		bool ScenePass = true;
		bool UIPass = true;
	};


	enum class RendererType
	{
		DEFFERED,
		FOWARD_PLUS
	};

	class RenderGraph;

	struct RenderExtensionConfig
	{
		bool Enable = true;
		GraphcisBackendCreateSpec Spec;
		DefferedRenderingConfig DefferedConfig{};
		RendererType UsingType = RendererType::DEFFERED;
		BlazeExtensionConfig BlazeConfig;

		RenderExtensionConfig(const GraphcisBackendCreateSpec& spec = GraphcisBackendCreateSpec())
		{
			Spec = spec;
		}
	};

	class RenderExtension : public BackBone::Extension
	{
	public:
		RenderExtension(const RenderExtensionConfig& Config)
			:_Config(Config)
		{
		}
		~RenderExtension() {}

		virtual void Build(BackBone::App& App) override;

		virtual const char* Name() const override { return "RenderExtension"; }
	private:
		RenderExtensionConfig _Config;
	};

	struct RenderResource
	{
		std::vector<CommandBufferAllocInfo> CommandBuffers;
		uint32_t CurrentFrameCount = 0;
		uint32_t MaxFrameInFlight = 0;
		uint32_t TotalFrames = 0;
		EventReader < WindowResizeEvent> ResizeEvent;
		FrameBufferResizeEvent	FrameBufferSize{ 0,0 };
		bool FrameBufferReSized = false;
		bool ContinueRender = false;
		CommandBufferAllocInfo ActiveCommandBuffer;

		struct LastTransformStruct
		{
			uint32_t GenerationVal = 0;
			uint32_t Version = UINT32_MAX;
			LastTransformStruct() :Version(UINT32_MAX) {}
		};

		SparseSet<LastTransformStruct> LastTransformVersion;

		BackBone::AssetHandle<ShaderProgram> DeafultShaderProgram;
		BackBone::AssetHandle<Material> DeafultMaterial;
		BackBone::AssetHandle<Image> DeafultImage;
		BackBone::AssetHandle<Texture> DeafultTexture;
		BackBone::AssetHandle<Sampler> DeafultSampler;
		std::shared_ptr<RenderGraph> RenderGraph;
		ResourceState SwapChainState = ResourceState::Present;
		BackBone::AssetHandle<Scene> ActiveSceneID;
	};

	class Command;

	struct ColorResources
	{
		bool IsSwapChain = false;
		BackBone::AssetHandle<Texture> Color;
		BackBone::AssetHandle<Texture> Resolve;
	};

	struct RenderGraphPassResources
	{
		std::array<ColorResources, CHILLI_MAX_COLOR_ATTACHMENT> ColorAttachments;
		std::array<BackBone::AssetHandle<Texture>, CHILLI_MAX_INPUT_ATTACHMENT > InputAttachments;
		std::array<BackBone::AssetHandle<Buffer>, CHILLI_MAX_COLOR_ATTACHMENT> BufferDependencies;
		BackBone::AssetHandle<Texture> DepthTexture;
		bool HasDepthStencil = false;
	};

	enum class RenderGraphErrorCodes
	{
		NONE,

		DESC_RESOURCES_DEPTH_MISMATCH,
		RESOURCES_DEPTH_TEXTURE_NULL,
		DESC_DEPTH_TEXTURE_NULL,

		DESC_RESOURCES_COLOR_SWAPCHAIN_MISMATCH,

		DESC_COLOR_TEXTURE_NULL,
		RESOURCES_COLOR_TEXTURE_NULL,
		DESC_RESOURCES_COLOR_TEXTURE_MISMATCH,
		DESC_RESOURCES_COLOR_TEXTURE_NOT_ONE_TO_ONE,

		DESC_INPUT_TEXTURE_NULL,
		DESC_RESOURCES_INPUT_TEXTURE_MISMATCH,
		DESC_RESOURCES_INPUT_TEXTURE_NOT_ONE_TO_ONE,
		RESOURCES_INPUT_TEXTURE_NULL,

		DESC_RENDERAREA_RESOURCES_COLOR_TEXTURE_MISMATCH,
		DESC_RENDERAREA_RESOURCES_DEPTH_TEXTURE_MISMATCH,
	};

	inline const char* RenderGraphErrorCodesToString(RenderGraphErrorCodes code)
	{
		switch (code)
		{
		case RenderGraphErrorCodes::NONE: return "NONE";

		case RenderGraphErrorCodes::DESC_RESOURCES_DEPTH_MISMATCH: return "DESC_RESOURCES_DEPTH_MISMATCH";
		case RenderGraphErrorCodes::RESOURCES_DEPTH_TEXTURE_NULL: return "RESOURCES_DEPTH_TEXTURE_NULL";
		case RenderGraphErrorCodes::DESC_DEPTH_TEXTURE_NULL: return "DESC_DEPTH_TEXTURE_NULL";

		case RenderGraphErrorCodes::DESC_RESOURCES_COLOR_SWAPCHAIN_MISMATCH: return "DESC_RESOURCES_COLOR_SWAPCHAIN_MISMATCH";

		case RenderGraphErrorCodes::DESC_COLOR_TEXTURE_NULL: return "DESC_COLOR_TEXTURE_NULL";
		case RenderGraphErrorCodes::RESOURCES_COLOR_TEXTURE_NULL: return "RESOURCES_COLOR_TEXTURE_NULL";
		case RenderGraphErrorCodes::DESC_RESOURCES_COLOR_TEXTURE_MISMATCH: return "DESC_RESOURCES_COLOR_TEXTURE_MISMATCH";
		case RenderGraphErrorCodes::DESC_RESOURCES_COLOR_TEXTURE_NOT_ONE_TO_ONE: return "DESC_RESOURCES_COLOR_TEXTURE_NOT_ONE_TO_ONE";

		case RenderGraphErrorCodes::DESC_INPUT_TEXTURE_NULL: return "DESC_INPUT_TEXTURE_NULL";
		case RenderGraphErrorCodes::DESC_RESOURCES_INPUT_TEXTURE_MISMATCH: return "DESC_RESOURCES_INPUT_TEXTURE_MISMATCH";
		case RenderGraphErrorCodes::DESC_RESOURCES_INPUT_TEXTURE_NOT_ONE_TO_ONE: return "DESC_RESOURCES_INPUT_TEXTURE_NOT_ONE_TO_ONE";
		case RenderGraphErrorCodes::RESOURCES_INPUT_TEXTURE_NULL: return "RESOURCES_INPUT_TEXTURE_NULL";

		case RenderGraphErrorCodes::DESC_RENDERAREA_RESOURCES_COLOR_TEXTURE_MISMATCH: return "DESC_RENDERAREA_RESOURCES_COLOR_TEXTURE_MISMATCH";
		case RenderGraphErrorCodes::DESC_RENDERAREA_RESOURCES_DEPTH_TEXTURE_MISMATCH: return "DESC_RENDERAREA_RESOURCES_DEPTH_TEXTURE_MISMATCH";

		default: return "UNKNOWN_ERROR";
		}
	}

	class RenderGraphRegistry;

	class RenderGraphPass
	{
	public:
		virtual ~RenderGraphPass() = default;

		// Called once at startup or when pass is registered
		// User declares their attachments here
		virtual RenderPassDesc Setup(BackBone::SystemContext& Ctxt, RenderGraphRegistry& Registry) = 0;

		// Called every frame at the declared stage
		// User issues draw calls here
		virtual void Execute(BackBone::SystemContext& Ctxt, const RenderPassDesc& Desc) = 0;

		// Called when window resizes — user recreates size-dependent resources
		virtual void OnResize(BackBone::SystemContext& Ctxt, uint32_t Width, uint32_t Height) {}

		// Called when pass is removed
		virtual void Teardown(BackBone::SystemContext& Ctxt) = 0;

		const RenderPassDesc& GetDesc() const { return _Desc; }
		RenderPassDesc& GetDesc() { return _Desc; }

		const RenderGraphPassResources& GetResources() const { return _Resources; }
		RenderGraphPassResources& GetResources() { return _Resources; }

	protected:
		RenderPassDesc _Desc;
		RenderGraphPassResources _Resources;
	};

	struct ResourceStateChange
	{
		std::vector<BackBone::AssetHandle<Buffer>> ResourceChangeBufferHandles;
		std::vector<ResourceState> ResourceChangeBufferStates;

		struct ImageResource
		{
			bool IsSwapChain = false;
			BackBone::AssetHandle<Texture> Texture;
		};

		std::vector<ImageResource> ResourceChangeImageHandles;
		std::vector<ResourceState> ResourceChangeImageStates;
	};

#define CHILLI_MAX_RENDER_GRAPH_KEY_LENGTH 32
#define CHILLI_MAX_RENDER_GRAPH_KEY_LENGTH 32

	struct RGKey {
		char Data[CHILLI_MAX_RENDER_GRAPH_KEY_LENGTH] = { 0 };

		RGKey(const char* name) {
			// Clear buffer first to ensure stable hashing (important for padding bytes)
			std::memset(Data, 0, CHILLI_MAX_RENDER_GRAPH_KEY_LENGTH);
			if (name) {
				std::strncpy(Data, name, CHILLI_MAX_RENDER_GRAPH_KEY_LENGTH - 1);
			}
		}

		bool operator==(const RGKey& other) const {
			// Modern compilers optimize this into 2-4 SIMD instructions
			return std::memcmp(Data, other.Data, CHILLI_MAX_RENDER_GRAPH_KEY_LENGTH) == 0;
		}
	};

	struct RGKeyHasher {
		size_t operator()(const RGKey& k) const {
			// Fast 64-bit hashing (process 8 bytes at a time)
			const uint64_t* p = reinterpret_cast<const uint64_t*>(k.Data);
			size_t h = 14695981039346656037ULL; // FNV offset basis for 64-bit

			// Unrolled loop for 32 bytes (4 iterations)
			h = (h ^ p[0]) * 1099511628211ULL;
			h = (h ^ p[1]) * 1099511628211ULL;
			h = (h ^ p[2]) * 1099511628211ULL;
			h = (h ^ p[3]) * 1099511628211ULL;

			return h;
		}
	};

	struct RenderGraphResource {
		enum class Type { None, Image, Buffer } ResType;

		// Using a variant is safer for AssetHandles which have non-trivial destructors
		union  ResourceUnion {
			BackBone::AssetHandle<Texture>Texture;
			BackBone::AssetHandle<Buffer> Buffer;

			ResourceUnion() {}
		} Resource;

		RenderGraphResource() :Resource()
		{

		}
	};

	class RenderGraphRegistry {
	public:
		RenderGraphRegistry()
		{

		}

		void AddImageResource(const RGKey& Name, BackBone::AssetHandle<Texture> Texture) {
			RenderGraphResource Resource;
			Resource.Resource.Texture = Texture;
			Resource.ResType = RenderGraphResource::Type::Image;
			_ResourcesMap[Name] = Resource;
		}

		void AddBufferResource(const RGKey& Name, BackBone::AssetHandle<Buffer> Buffer) {
			RenderGraphResource Resource;
			Resource.Resource.Buffer = Buffer;
			Resource.ResType = RenderGraphResource::Type::Buffer;
			_ResourcesMap[Name] = Resource;
		}

		bool DoesResourceExist(const RGKey& Name) const {
			return _ResourcesMap.find(Name) != _ResourcesMap.end();
		}

		RenderGraphResource::Type GetType(const RGKey& Name) {
			auto it = _ResourcesMap.find(Name);
			if (it != _ResourcesMap.end())
			{
				// true
				return it->second.ResType;
			}
			return RenderGraphResource::Type::None;
		}

		BackBone::AssetHandle<Texture> GetImageResource(const RGKey& Name) {
			auto it = _ResourcesMap.find(Name);
			if (it != _ResourcesMap.end() && it->second.ResType == RenderGraphResource::Type::Image) {
				return it->second.Resource.Texture;
			}
			return {}; // Return invalid handle
		}

		BackBone::AssetHandle<Buffer> GetBufferResource(const RGKey& Name) {
			auto it = _ResourcesMap.find(Name);
			if (it != _ResourcesMap.end() && it->second.ResType == RenderGraphResource::Type::Buffer) {
				return it->second.Resource.Buffer;
			}
			return {};
		}

	private:
		std::unordered_map<RGKey, RenderGraphResource, RGKeyHasher> _ResourcesMap;
	};

	class RenderGraph
	{
	public:

		RenderGraph()
		{

		}
		// Add an existing shared_ptr directly
		RenderGraph& AddPass(std::shared_ptr<RenderGraphPass> pass)
		{
			_Passes.push_back(pass);
			return *this;
		}
		// Register a pass — auto sorted by stage
		template<typename T, typename... Args>
		RenderGraph& AddPass(Args&&... args)
		{
			_Passes.push_back(std::make_shared<T>(std::forward<Args>(args)...));
			return *this;
		}
		void AddBarrier(std::array<PipelineBarrier, CHILLI_MAX_PASS_BARRIERS>& container, uint32_t& counter, uint32_t handle,
			ResourceState oldState, ResourceState newState, bool IsSwapChain);

		// Insert before/after a named pass — like Bevy's add_render_graph_edges
		RenderGraph& AddPassBefore(const std::string& BeforePass,
			std::shared_ptr<RenderGraphPass> Pass)
		{
			_OrderOverrides.push_back({ BeforePass, Pass, true });
			return *this;
		}

		RenderGraph& AddPassAfter(const std::string& AfterPass,
			std::shared_ptr<RenderGraphPass> Pass)
		{
			_OrderOverrides.push_back({ AfterPass, Pass, false });
			return *this;
		}

		void Build(BackBone::SystemContext& Ctxt);

		std::vector<std::shared_ptr<RenderGraphPass>>& GetPasses()
		{
			return _Passes;
		}

		const std::vector<std::shared_ptr<RenderGraphPass>>& GetPasses() const
		{
			return _Passes;
		}

		const std::vector<ResourceStateChange>& GetPrePassChanges()
		{
			return _PrePassChanges;
		}

		const std::vector<ResourceStateChange>& GetPostPassChanges()
		{
			return _PostPassChanges;
		}

		std::vector<std::shared_ptr<RenderGraphPass>>::iterator begin() {
			return _Passes.begin();
		};
		std::vector<std::shared_ptr<RenderGraphPass>>::iterator end() {
			return _Passes.end();
		}

	private:
		RenderGraphErrorCodes _IsPassValid(const RenderGraphPass* Pass);
	private:
		struct OrderOverride
		{
			std::string               RelativeTo;
			std::shared_ptr<RenderGraphPass> Pass;
			bool                      Before;
		};

		RenderGraphRegistry _Registry;
		std::vector<std::shared_ptr<RenderGraphPass>> _Passes;
		std::vector<ResourceStateChange> _PrePassChanges;
		std::vector<ResourceStateChange> _PostPassChanges;
		std::vector<OrderOverride>               _OrderOverrides;
	};

	class Renderer;
	class RenderCommand;

	void OnPresentRender(BackBone::SystemContext& Ctxt, RenderPassDesc& Pass);
	void OnRenderExtensionsSetup(BackBone::SystemContext& Ctxt);

#pragma endregion
}
