#pragma once

namespace Chilli
{
    class GraphicsPipeline
    {
    public:
        ~GraphicsPipeline() {}

        virtual void Bind() = 0;
    private:
    };
} // namespace VEngine
