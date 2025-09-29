#include "Ch_PCH.h"
#include "LayerStack.h"

namespace Chilli
{
    LayerStack::~LayerStack()
    {
        CH_CORE_INFO("LayerStack Deleted!");
    }

    void LayerStack::PushLayer(const std::shared_ptr<Layer>& layer)
    {
        _Layers.push_back(layer);
        layer->Init();
    }

    void LayerStack::PushOverlay(std::shared_ptr<Layer> overlay)
    {
        _Layers.emplace_back(std::move(overlay));
    }

    void LayerStack::Flush()
    {
        for (auto& layer : _Layers)
            layer->Terminate();
        _Layers.clear();
    }
}
