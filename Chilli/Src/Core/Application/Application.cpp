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
			Renderer::Init(RenderAPITypes::VULKAN1_3);
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
			_Window.SwapBuffers();

			for (auto layer : _Layers)
				layer->Update();

			_Window.PollEvents();
		}
	}

	void Application::OnEvent(Event& e)
	{
		if (e.GetType() == WindowCloseEvent::GetStaticType())
		{
			CH_CORE_INFO("Window Close!");
		}
		if (e.GetType() == KeyPressedEvent::GetStaticType())
		{
			auto keye = static_cast<KeyPressedEvent&>(e);
			CH_CORE_INFO("Key Pressed: {0}", (keye.GetKeyCode()));
		}
	}
} // namespace Chilli
