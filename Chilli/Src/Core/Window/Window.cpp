#include "Window.h"
#include "Window.h"
#include "Ch_PCH.h"
#include "Window.h"

#include "Maths.h"
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "Input/Input.h"
#include "Input/InputCodes.h"
#include "Maths.h"
#include "BackBone\DeafultExtensions.h"

void Chilli_WindowIconifyCallBack(GLFWwindow* window, int iconified)
{
	if (iconified)
	{
		Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);
		Chilli::WindowMinimizedEvent e;
		Data->EventListener->Add<Chilli::WindowMinimizedEvent>(e);
	}
}

void Chilli_FrameBufferCallback(GLFWwindow* window, int width, int height)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);

	Chilli::FrameBufferResizeEvent e(width, height);
	Data->EventListener->Add<Chilli::FrameBufferResizeEvent>(e);
}

void Chilli_WindowCloseCallBack(GLFWwindow* window)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);

	Data->Close = true;
	Chilli::WindowCloseEvent e;
	Data->EventListener->Add<Chilli::WindowCloseEvent>(e);
}

void Chilli_WindowSizeCallBack(GLFWwindow* window, int width, int height)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);
	Data->Dimensions = { width, height };

	Chilli::WindowResizeEvent e(width, height);
	Data->EventListener->Add(e);
}

void Chilli_KeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);

	int MyMods = 0;	
	if (mods & GLFW_MOD_SHIFT) MyMods += Chilli::Input_mod_Shift;
	if (mods & GLFW_MOD_CONTROL) MyMods += Chilli::Input_mod_Control;
	if (mods & GLFW_MOD_ALT) MyMods += Chilli::Input_mod_Alt;
	if (mods & GLFW_MOD_SUPER) MyMods += Chilli::Input_mod_Super;
	if (mods & GLFW_MOD_CAPS_LOCK) MyMods += Chilli::Input_mod_CapsLock;
	if (mods & GLFW_MOD_NUM_LOCK) MyMods += Chilli::Input_mod_NumLock;

	if (action == GLFW_PRESS)
	{
		Chilli::KeyPressedEvent e(Chilli::GLFWToInputKeys(key), MyMods);
		Data->EventListener->Add(e);
	}
	else if (action == GLFW_REPEAT)
	{
		Chilli::KeyRepeatEvent e(Chilli::GLFWToInputKeys(key), MyMods);
		Data->EventListener->Add(e);
	}
	else if (action == GLFW_RELEASE)
	{
		Chilli::KeyReleasedEvent e(Chilli::GLFWToInputKeys(key), MyMods);
		Data->EventListener->Add(e);
	}
}

void Chilli_MouseButtonCallBack(GLFWwindow* window, int button, int action, int mods)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);

	if (action == GLFW_PRESS)
	{
		Chilli::MouseButtonPressedEvent e(Chilli::GLFWToInputMouse(button));
		Data->EventListener->Add(e);
	}
	else if (action == GLFW_REPEAT)
	{
		Chilli::MouseButtonRepeatEvent e(Chilli::GLFWToInputMouse(button));
		Data->EventListener->Add(e);
	}
	else if (action == GLFW_RELEASE)
	{
		Chilli::MouseButtonReleasedEvent e(Chilli::GLFWToInputMouse(button));
		Data->EventListener->Add(e);
	}
}

void Chilli_CursorPosCallBack(GLFWwindow* window, double xpos, double ypos)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);
	//Chilli::Input::SetCursorPos({ xpos, ypos });

	Chilli::CursorPosEvent e(xpos, ypos);
	Data->EventListener->Add(e);
}

void Chilli_MouseScrollCallBack(GLFWwindow* window, double xoffset, double yoffset)
{
	Chilli::WindowData* Data = (Chilli::WindowData*)glfwGetWindowUserPointer(window);

	Chilli::MouseScrollEvent e(xoffset, yoffset);
	Data->EventListener->Add(e);
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

		CH_CORE_INFO("Window Created: {0} of {1}x{2}", Spec.Title, Spec.Dimensions.x, Spec.Dimensions.y);

		if (glfwGetWindowAttrib(_Window, GLFW_MAXIMIZED)) {
			std::cout << "WINDOW IS MAXIMIZED - THIS LOCKS SURFACE SIZE!" << std::endl;
		}

		// Check if window is fullscreen  
		if (glfwGetWindowMonitor(_Window)) {
			std::cout << "WINDOW IS FULLSCREEN - THIS LOCKS SURFACE SIZE!" << std::endl;
		}

		glfwSetWindowUserPointer(_Window, &_Data);

		// Callbacks
		glfwSetWindowCloseCallback(_Window, Chilli_WindowCloseCallBack);
		glfwSetWindowSizeCallback(_Window, Chilli_WindowSizeCallBack);
		glfwSetCursorPosCallback(_Window, Chilli_CursorPosCallBack);
		glfwSetMouseButtonCallback(_Window, Chilli_MouseButtonCallBack);
		glfwSetKeyCallback(_Window, Chilli_KeyCallBack);
		glfwSetScrollCallback(_Window, Chilli_MouseScrollCallBack);
		glfwSetFramebufferSizeCallback(_Window, Chilli_FrameBufferCallback);
		glfwSetWindowIconifyCallback(_Window, Chilli_WindowIconifyCallBack);
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

	void* Chilli::Window::GetWin32Surface() const
	{
		return glfwGetWin32Window(_Window);
	}

	Vec2 Chilli::Window::GetFrameBufferSize() const
	{
		int w, h;
		glfwGetFramebufferSize(_Window, &w, &h);
		return Vec2((float)w, (float)h);
	}

	float GetWindowTime()
	{
		return glfwGetTime();
	}
}
