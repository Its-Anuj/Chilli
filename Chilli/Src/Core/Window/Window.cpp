#include "Ch_PCH.h"
#include "Window.h"

#include "Maths.h"
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "Input/Input.h"
#include "Input/InputCodes.h"
#include "Maths.h"

void Chilli_FrameBufferCallback(GLFWwindow* window, int width, int height)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);

	Chilli::FrameBufferResizeEvent e(width, height);
	Data->EventCallback(e);
}

void Chilli_WindowCloseCallBack(GLFWwindow* window)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);

	Chilli::WindowCloseEvent e;
	Data->EventCallback(e);
}

void Chilli_WindowSizeCallBack(GLFWwindow* window, int width, int height)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);
	Data->Dimensions = { width, height };

	Chilli::WindowResizeEvent e(width, height);
	Data->EventCallback(e);
}

void Chilli_KeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);

	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
			Chilli::Input::SetModState(Chilli::Input_mod_Shift, true);
		else if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
			Chilli::Input::SetModState(Chilli::Input_mod_Control, true);
		else if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
			Chilli::Input::SetModState(Chilli::Input_mod_Alt, true);
		else if (key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER)
			Chilli::Input::SetModState(Chilli::Input_mod_Super, true);
		else if (key == GLFW_KEY_CAPS_LOCK)
			Chilli::Input::SetModState(Chilli::Input_mod_CapsLock, true);
		else if (key == GLFW_KEY_NUM_LOCK)
			Chilli::Input::SetModState(Chilli::Input_mod_NumLock, true);

		Chilli::KeyPressedEvent e(key);
		Data->EventCallback(e);
	}
	else if (action == GLFW_REPEAT)
	{
		Chilli::KeyRepeatEvent e(key);
		Data->EventCallback(e);
	}
	else if (action == GLFW_RELEASE)
	{
		if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
			Chilli::Input::SetModState(Chilli::Input_mod_Shift, false);
		else if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
			Chilli::Input::SetModState(Chilli::Input_mod_Control, false);
		else if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
			Chilli::Input::SetModState(Chilli::Input_mod_Alt, false);
		else if (key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER)
			Chilli::Input::SetModState(Chilli::Input_mod_Super, false);
		else if (key == GLFW_KEY_CAPS_LOCK)
			Chilli::Input::SetModState(Chilli::Input_mod_CapsLock, false);
		else if (key == GLFW_KEY_NUM_LOCK)
			Chilli::Input::SetModState(Chilli::Input_mod_NumLock, false);

		Chilli::KeyReleasedEvent e(key);
		Data->EventCallback(e);
	}
}

void Chilli_MouseButtonCallBack(GLFWwindow* window, int button, int action, int mods)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);

	if (action == GLFW_PRESS)
	{
		Chilli::MouseButtonPressedEvent e(button);
		Data->EventCallback(e);
	}
	else if (action == GLFW_REPEAT)
	{
		Chilli::MouseButtonRepeatEvent e(button);
		Data->EventCallback(e);
	}
	else if (action == GLFW_RELEASE)
	{
		Chilli::MouseButtonReleasedEvent e(button);
		Data->EventCallback(e);
	}
}

void Chilli_CursorPosCallBack(GLFWwindow* window, double xpos, double ypos)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);
	Chilli::Input::SetCursorPos({ xpos, ypos });

	Chilli::CursorPosEvent e(xpos, ypos);
	Data->EventCallback(e);
}

void Chilli_MouseScrollCallBack(GLFWwindow* window, double xoffset, double yoffset)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);

	Chilli::MouseScrollEvent e(xoffset, yoffset);
	Data->EventCallback(e);
}

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

		_Spec = Spec;
		_Data.Name = Spec.Title;
		_Data.Dimensions = { Spec.Dimensions.x,Spec.Dimensions.y };

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		_Window = glfwCreateWindow(Spec.Dimensions.x, Spec.Dimensions.y, Spec.Title, NULL, NULL);
		glfwMakeContextCurrent(_Window);

		CH_CORE_INFO("Window Created: {0} of {1}x{2}", Spec.Title, Spec.Dimensions.x, Spec.Dimensions.y);

		glfwSetWindowUserPointer(_Window, &_Data);

		// Callbacks
		glfwSetWindowCloseCallback(_Window, Chilli_WindowCloseCallBack);
		glfwSetWindowSizeCallback(_Window, Chilli_WindowSizeCallBack);
		glfwSetCursorPosCallback(_Window, Chilli_CursorPosCallBack);
		glfwSetMouseButtonCallback(_Window, Chilli_MouseButtonCallBack);
		glfwSetKeyCallback(_Window, Chilli_KeyCallBack);
		glfwSetScrollCallback(_Window, Chilli_MouseScrollCallBack);
		glfwSetFramebufferSizeCallback(_Window, Chilli_FrameBufferCallback);
	}

	void Window::Terminate()
	{
		glfwDestroyWindow(_Window);
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
