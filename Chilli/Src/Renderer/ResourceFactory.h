#pragma once

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
    private:
    };
} // namespace VEngine
