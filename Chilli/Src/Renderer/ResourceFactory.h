#pragma once

#include "Buffers.h"
#include "Shader.h"

namespace Chilli
{
    template<typename _T>
    using Ref = std::shared_ptr<_T>;

    class ResourceFactory
    {
    public:
        virtual Ref<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineSpec& Spec) = 0;
        virtual void DestroyGraphicsPipeline(Ref<GraphicsPipeline>& Pipeline) = 0;

        virtual Ref<VertexBuffer> CreateVertexBuffer(const VertexBufferSpec& Spec) = 0;
        virtual void DestroyVertexBuffer(Ref<VertexBuffer>& VB) = 0;

        virtual Ref<IndexBuffer> CreateIndexBuffer(const IndexBufferSpec& Spec) = 0;
        virtual void DestroyIndexBuffer(Ref<IndexBuffer>& IB) = 0;
    private:
    };
} // namespace VEngine
