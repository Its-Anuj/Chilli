#pragma once

#include "Window/Window.h"
#include "Input/Input.h"
#include "Layers/LayerStack.h"
#include "Renderer.h"

namespace Chilli
{
	struct ApplicationSpec
	{
		const char* Name;
		struct {
			int x, y;
		} Dimensions;
		bool VSync;
	};

	class Application
	{
	public:
		Application(){}
		~Application(){}

		void Init(const ApplicationSpec& Spec);
		void ShutDown();

		void Run();

		void PushLayer(const std::shared_ptr<Layer>& layer) { _Layers.PushLayer(layer); }
		void PushOverLay(const std::shared_ptr<Layer>& layer) { _Layers.PushOverlay(layer); }

		void OnEvent(Event& e);
	private:
		Window _Window;
		LayerStack _Layers;
		float LastTime = 0.0f;
	};
}
