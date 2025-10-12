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
			Spec.EnableValidation = false;

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
			float timestep = StartTime - LastTime;
			LastTime = StartTime;

			_Window.PollEvents();

			for (auto layer : _Layers)
				layer->Update();
		}
	}

	void Application::OnEvent(Event& e)
	{
		if (e.GetType() == FrameBufferResizeEvent::GetStaticType())
		{
			auto Fb = static_cast<FrameBufferResizeEvent&>(e);
		}
		if (e.GetType() == WindowCloseEvent::GetStaticType())
		{
			CH_CORE_INFO("Window Close!");
		}
		if (e.GetType() == KeyPressedEvent::GetStaticType())
		{
			auto keye = static_cast<KeyPressedEvent&>(e);
			CH_CORE_INFO("Key Pressed: {0}", (keye.GetKeyCode()));
		}

		for (auto layer : _Layers)
			layer->OnEvent(e);
	}
} // namespace Chilli
