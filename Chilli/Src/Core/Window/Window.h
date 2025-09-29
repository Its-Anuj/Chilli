#pragma once

#include "Events/Events.h"

struct GLFWwindow;

namespace Chilli
{
	struct WindowData
	{
		struct {
			int x, y;
		} Dimensions;
		std::string Name;

		std::function<void(Event&)> EventCallback;
	};

	struct WindowSpec
	{ 
		const char* Title = "Untitiled";
		struct {
			int x, y;
		} Dimensions;
	};

	class Window
	{
	public:
		Window() {}
		~Window() {}

		void Init(const WindowSpec& Spec);
		void Terminate();

		void SwapBuffers();
		void PollEvents();
		bool WindowShouldClose();

		void SetEventCallback(const std::function<void(Event&)>& Callback) { _Data.EventCallback = Callback; }
		GLFWwindow* GetRawHandle() const { return _Window; }
	private:
		GLFWwindow* _Window;
		WindowSpec _Spec;
		WindowData _Data;
	};
}
