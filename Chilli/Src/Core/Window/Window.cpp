#include "PCH//Ch_PCH.h"
#include "Window.h"

#include "GLFW/glfw3.h"

namespace Chilli
{
	static bool GLFW_INIT = false;

	void Window::Init(const WindowSpec& Spec)
	{
		if (!GLFW_INIT)
		{
			int Success = glfwInit();
			if (Success == GLFW_FALSE)
				GLFW_INIT = false;
			else
				GLFW_INIT = true;
		}

		if (!GLFW_INIT)
			return;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		_Window = glfwCreateWindow(Spec.Dimensions.x, Spec.Dimensions.y, Spec.Title, NULL, NULL);
		glfwMakeContextCurrent(_Window);
	}

	void Window::Terminate()
	{
		if (GLFW_INIT)
		{
			glfwTerminate();
			GLFW_INIT = false;
		}
	}

	void Window::SwapBuffers()
	{
		glfwSwapBuffers(_Window);
	}

	void Window::PollEvents()
	{
		glfwPollEvents();
	}

	bool Window::WindowShouldClose()
	{
		return glfwWindowShouldClose(_Window);
	}
}
