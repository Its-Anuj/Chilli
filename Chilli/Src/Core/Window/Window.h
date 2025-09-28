#pragma once

struct GLFWwindow;

namespace Chilli
{
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
	private:
		GLFWwindow* _Window;
		WindowSpec _Spec;
	};
}
