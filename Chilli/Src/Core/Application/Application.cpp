#include "Ch_PCH.h"
#include "Application.h"

namespace Chilli
{
#define CHILLI_EVENT_CALLBACK_FN(x) {std::bind(&Application::x, this, std::placeholders::_1)}

	void Application::Init(const ApplicationSpec& Spec)
	{
		Log::Init();

		CH_CORE_INFO("Welcome");

		WindowSpec WSpec{};
		WSpec.Title = Spec.Name;
		WSpec.Dimensions = { Spec.Dimensions.x, Spec.Dimensions.y };

		_Window.Init(WSpec);
		_Window.SetEventCallback(CHILLI_EVENT_CALLBACK_FN(OnEvent));

		Input::Init(_Window.GetRawHandle());
		{
			RendererInitSpec Spec{};
			
			Spec.Type = RenderAPIType::VULKAN1_3;
			Spec.GlfwWindow = _Window.GetRawHandle();
			Spec.InFrameFlightCount = 2;
			Spec.InitialWindowSize.x = _Window.GetWidth();
			Spec.InitialWindowSize.y = _Window.GetHeight();
			Spec.InitialFrameBufferSize.x = _Window.GetFrameBufferSize().x;
			Spec.InitialFrameBufferSize.y = _Window.GetFrameBufferSize().y;
			Spec.VSync = true;
			Spec.Name = "Chilli Editor";
			Spec.EnableValidation = true;

			Renderer::Init(Spec);
		}
		CH_CORE_INFO("Using: {0}", Renderer::GetName());
	}

	void Application::ShutDown()
	{
		_Layers.Flush();
		Renderer::ShutDown();
		_Window.Terminate();
		Input::ShutDown();
		// Clean up resources and shut down the application
	}

	void Application::Run()
	{
		while (!_Window.WindowShouldClose())
		{
			//_Window.SwapBuffers();
			auto StartTime = GetWindowTime();
			TimeStep timestep = StartTime - LastTime;
			LastTime = StartTime;
			_Window.PollEvents();

			for (auto layer : _Layers)
				layer->Update(timestep);
		}
	}

	void Application::OnEvent(Event& e)
	{
		// Dispatch events to all layers
		for (auto layer : _Layers)
			layer->OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(CHILLI_EVENT_CALLBACK_FN(OnKeyPressedEvent));
		dispatcher.Dispatch<KeyReleasedEvent>(CHILLI_EVENT_CALLBACK_FN(OnKeyReleasedEvent));
		dispatcher.Dispatch<KeyRepeatEvent>(CHILLI_EVENT_CALLBACK_FN(OnKeyRepeatEvent));

		dispatcher.Dispatch<MouseButtonPressedEvent>(CHILLI_EVENT_CALLBACK_FN(OnMouseButtonPressedEvent));
		dispatcher.Dispatch<MouseButtonReleasedEvent>(CHILLI_EVENT_CALLBACK_FN(OnMouseButtonReleasedEvent));
		dispatcher.Dispatch<MouseButtonRepeatEvent>(CHILLI_EVENT_CALLBACK_FN(OnMouseButtonRepeatEvent));
		dispatcher.Dispatch<MouseScrollEvent>(CHILLI_EVENT_CALLBACK_FN(OnMouseScrollEvent));
		dispatcher.Dispatch<CursorPosEvent>(CHILLI_EVENT_CALLBACK_FN(OnCursorPosEvent));

		dispatcher.Dispatch<WindowCloseEvent>(CHILLI_EVENT_CALLBACK_FN(OnWindowCloseEvent));
		dispatcher.Dispatch<WindowResizeEvent>(CHILLI_EVENT_CALLBACK_FN(OnWindowResizeEvent));
		dispatcher.Dispatch<WindowMinimizedEvent>(CHILLI_EVENT_CALLBACK_FN(OnWindowMinimizedEvent));
		dispatcher.Dispatch<FrameBufferResizeEvent>(CHILLI_EVENT_CALLBACK_FN(OnFrameBufferResizeEvent));

	}

	void Application::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		for (auto layer : _Layers)
			layer->OnKeyPressedEvent(e);
	}

	void Application::OnKeyReleasedEvent(KeyReleasedEvent& e)
	{
		for (auto layer : _Layers)
			layer->OnKeyReleasedEvent(e);
	}

	void Application::OnKeyRepeatEvent(KeyRepeatEvent& e)
	{
		for (auto layer : _Layers)
			layer->OnKeyRepeatEvent(e);
	}

	void Application::OnMouseButtonPressedEvent(MouseButtonPressedEvent& e)
	{
		for (auto layer : _Layers)
			layer->OnMouseButtonPressedEvent(e);
	}

	void Application::OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e)
	{
		for (auto layer : _Layers)
			layer->OnMouseButtonReleasedEvent(e);
	}

	void Application::OnMouseButtonRepeatEvent(MouseButtonRepeatEvent& e)
	{
		for (auto layer : _Layers)
			layer->OnMouseButtonRepeatEvent(e);
	}

	void Application::OnMouseScrollEvent(MouseScrollEvent& e)
	{
		for (auto layer : _Layers)
			layer->OnMouseScrollEvent(e);
	}

	void Application::OnCursorPosEvent(CursorPosEvent& e)
	{
		for (auto layer : _Layers)
			layer->OnCursorPosEvent(e);
	}

	void Application::OnWindowCloseEvent(WindowCloseEvent& e)
	{
		for (auto layer : _Layers)
			layer->OnWindowCloseEvent(e);
	}

	void Application::OnWindowResizeEvent(WindowResizeEvent& e)
	{
		for (auto layer : _Layers)
			layer->OnWindowResizeEvent(e);
	}

	void Application::OnWindowMinimizedEvent(WindowMinimizedEvent& e)
	{
		for (auto layer : _Layers)
			layer->OnWindowMinimizedEvent(e);
	}

	void Application::OnFrameBufferResizeEvent(FrameBufferResizeEvent& e)
	{
		for (auto layer : _Layers)
			layer->OnFrameBufferResizeEvent(e);
	}

} // namespace Chilli
