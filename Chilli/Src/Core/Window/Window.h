#pragma once

#include "Events/Events.h"
#include "Maths.h"

struct GLFWwindow;

namespace Chilli
{
	struct WindowData
	{
		struct {
			int x, y;
		} Dimensions;
		std::string Name;
		bool Close = false;
		bool IsActive = true;

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

		int GetWidth() const { return _Data.Dimensions.x; }
		int GetHeight() const { return _Data.Dimensions.y; }
		const char* GetName() const { return _Data.Name.c_str(); }

		void* GetWin32Surface() const;
		Vec2 GetFrameBufferSize() const;

		bool IsClose() const { return _Data.Close; }
		bool IsActive() const { return _Data.IsActive; }
	private:
		GLFWwindow* _Window;
		WindowSpec _Spec;
		WindowData _Data;
	};

	float GetWindowTime();
}
