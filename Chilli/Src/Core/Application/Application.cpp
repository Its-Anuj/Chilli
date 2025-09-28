#include "PCH//Ch_PCH.h"
#include "Application.h"

namespace Chilli
{
	void Application::Init(const ApplicationSpec& Spec)
	{
		Log::Init();

		CH_CORE_INFO("Welcome");

		WindowSpec WSpec{};
		WSpec.Title = Spec.Name;
		WSpec.Dimensions = { Spec.Dimensions.x, Spec.Dimensions.y };
		
		_Window.Init(WSpec);
	}

	void Application::ShutDown()
	{
		_Window.Terminate();
		// Clean up resources and shut down the application
	}

	void Application::Run()
	{
		while (!_Window.WindowShouldClose())
		{
			_Window.SwapBuffers();
			_Window.PollEvents();
		}
	}
} // namespace Chilli
