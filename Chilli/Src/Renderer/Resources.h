#pragma once

#include "Material.h"

namespace Chilli
{
	class ResourceFactory
	{
	public:
		virtual std::shared_ptr <GraphicsPipeline> CreateGraphicsPipeline(const PipelineSpec& Spec) = 0;
		virtual void ReCreateGraphicsPipeline(std::shared_ptr <GraphicsPipeline>& Pipeline ,const PipelineSpec& Spec) = 0;
		virtual void DestroyGraphicsPipeline(std::shared_ptr<GraphicsPipeline>& Pipeline) = 0;

		virtual std::shared_ptr <VertexBuffer> CreateVertexBuffer(const VertexBufferSpec& Spec) = 0;
		virtual void DestroyVertexBuffer(std::shared_ptr<VertexBuffer>& VB) = 0;

		virtual std::shared_ptr <IndexBuffer> CreateIndexBuffer(const IndexBufferSpec& Spec) = 0;
		virtual void DestroyIndexBuffer(std::shared_ptr<IndexBuffer>& IB) = 0;

		virtual std::shared_ptr <UniformBuffer> CreateUniformBuffer(size_t Size) = 0;
		virtual void DestroyUniformBuffer(std::shared_ptr<UniformBuffer>& UB) = 0;

		virtual std::shared_ptr <Texture> CreateTexture(const TextureSpec& Spec) = 0;
		virtual void DestroyTexture(std::shared_ptr<Texture>& Tex) = 0;
	private:
	};
}
