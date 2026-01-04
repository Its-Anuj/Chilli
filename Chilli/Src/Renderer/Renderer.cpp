#include "Ch_PCH.h"
#include "RenderClasses.h"

namespace Chilli
{

	void Renderer::Init(const GraphcisBackendCreateSpec& Spec)
	{
		_Api = std::shared_ptr<GraphicsBackendApi>(GraphicsBackendApi::Create(Spec));
		_RenderPerFrameArena.Prepare(5 * 1024 * 1024);
		_InlineUniformDataAllocator.Ref(_RenderPerFrameArena, 1 * 1024 * 128);

		_MaxFramesInFlight = Spec.MaxFrameInFlight;
		_FrameIndex = 0;
		_FramePackets.resize(_MaxFramesInFlight);
	}

	void Renderer::Terminate()
	{
		GraphicsBackendApi::Terminate(_Api.get(), false);
	}

	void Renderer::BeginFrame()
	{
		_FramePackets[_FrameIndex].Graphics_Stream.BeginFrame(_FrameIndex);
	}

	void Renderer::Clear()
	{
		_FramePackets[_FrameIndex].Graphics_Stream.Clear();
		_FramePackets[_FrameIndex].Compute_Stream.Clear();
		_FramePackets[_FrameIndex].Transfer_Stream.Clear();
	}

	void Renderer::EndFrame()
	{
		_FramePackets[_FrameIndex].Graphics_Stream.EndFrame();
		_Api->EndFrame(_FramePackets[_FrameIndex]);
		this->Clear();
		_FrameIndex = (_FrameIndex + 1) % _MaxFramesInFlight;
	}

	std::shared_ptr<RenderCommand> Renderer::CreateRenderCommand()
	{
		return std::make_shared<RenderCommand>(_Api);
	}

	static inline void HashCombine(uint64_t& seed, uint64_t value)
	{
		const uint64_t fnv_offset = 1469598103934665603ull;
		const uint64_t fnv_prime = 1099511628211ull;

		seed ^= value;
		seed *= fnv_prime;
	}

	uint64_t ShaderDescriptorBindingFlags::HashFlag(const ShaderDescriptorBindingFlags& Flag)
	{
		uint64_t hash = 1469598103934665603ull; // FNV64 Offset Basis

		// ---- Simple booleans ----
		HashCombine(hash, Flag.Set);
		HashCombine(hash, Flag.Binding);

		HashCombine(hash, Flag.EnableBindless);
		HashCombine(hash, Flag.EnableStreaming);
		HashCombine(hash, Flag.EnablePartiallyBound);
		HashCombine(hash, Flag.EnableVariableSize);
		HashCombine(hash, Flag.VariableSize);

		HashCombine(hash, Flag.StageFlags);
		return hash;
	}
}
