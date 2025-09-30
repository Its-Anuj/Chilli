#pragma once

namespace Chilli
{
    struct GraphicsPipelineSpec
    {
        const char* Paths[2];
    };

    class GraphicsPipeline
    {
    public:
        ~GraphicsPipeline() {}

        virtual void Bind() = 0;
    private:
    };
} // namespace VEngine
