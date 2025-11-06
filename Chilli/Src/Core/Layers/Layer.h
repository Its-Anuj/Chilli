#pragma once

#include "Events/Events.h"
#include "TimeStep.h"

namespace Chilli
{
	class Layer
	{
	public:
		Layer(const char* Name);
		~Layer();

		virtual void Init() = 0;
		virtual void Terminate() = 0;
		virtual void Update(TimeStep Ts) = 0;
		virtual void OnEvent(Event& e) {};

		virtual void OnKeyPressedEvent(KeyPressedEvent& e) {};
		virtual void OnKeyReleasedEvent(KeyReleasedEvent& e) {}
		virtual void OnKeyRepeatEvent(KeyRepeatEvent& e) {}
		;
		virtual void OnMouseButtonPressedEvent(MouseButtonPressedEvent& e) {};
		virtual void OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e) {};
		virtual void OnMouseButtonRepeatEvent(MouseButtonRepeatEvent& e) {};

		virtual void OnMouseScrollEvent(MouseScrollEvent& e) {};
		virtual void OnCursorPosEvent(CursorPosEvent& e) {};

		virtual void OnWindowCloseEvent(WindowCloseEvent& e) {};
		virtual void OnWindowResizeEvent(WindowResizeEvent& e) {};

		virtual void OnWindowMinimizedEvent(WindowMinimizedEvent& e) {};
		virtual void OnFrameBufferResizeEvent(FrameBufferResizeEvent& e) {};
	private:
		std::string _Name;
	};
}
