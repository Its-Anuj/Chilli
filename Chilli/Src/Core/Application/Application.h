#pragma once

#include "Window/Window.h"
#include "Input/Input.h"
#include "Layers/LayerStack.h"
#include "Renderer.h"
#include "TimeStep.h"

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
		Application() {}
		~Application() {}

		void Init(const ApplicationSpec& Spec);
		void ShutDown();

		void Run();

		void PushLayer(const std::shared_ptr<Layer>& layer) { _Layers.PushLayer(layer); }
		void PushOverLay(const std::shared_ptr<Layer>& layer) { _Layers.PushOverlay(layer); }

		void OnEvent(Event& e);

		virtual void OnKeyPressedEvent(KeyPressedEvent& e);
		virtual void OnKeyReleasedEvent(KeyReleasedEvent& e);
		virtual void OnKeyRepeatEvent(KeyRepeatEvent& e);
		;
		virtual void OnMouseButtonPressedEvent(MouseButtonPressedEvent& e);
		virtual void OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e);
		virtual void OnMouseButtonRepeatEvent(MouseButtonRepeatEvent& e);
		virtual void OnMouseScrollEvent(MouseScrollEvent& e);
		virtual void OnCursorPosEvent(CursorPosEvent& e);
		virtual void OnWindowCloseEvent(WindowCloseEvent& e);
		virtual void OnWindowResizeEvent(WindowResizeEvent& e);
		virtual void OnWindowMinimizedEvent(WindowMinimizedEvent& e);
		virtual void OnFrameBufferResizeEvent(FrameBufferResizeEvent& e);
	private:
		Window _Window;
		LayerStack _Layers;
		float LastTime = 0.0f;
	};
}
