#pragma once

#include "Layer.h"

namespace Chilli
{
	class LayerStack
	{
	public:
		LayerStack() {}
		~LayerStack();

		void PushLayer(const std::shared_ptr<Layer>& layer);
		void PushOverlay(std::shared_ptr<Layer> overlay);

		void Flush();

		using Iterator = std::vector<std::shared_ptr<Layer>>::iterator;
		using Const_Iterator = std::vector<std::shared_ptr<Layer>>::const_iterator;

		Iterator begin() { return _Layers.begin(); }
		Iterator end() { return _Layers.end(); }

		Const_Iterator begin() const { return _Layers.begin(); }
		Const_Iterator end() const { return _Layers.end(); }
	private:
		std::vector<std::shared_ptr<Layer>> _Layers;
	};
}
