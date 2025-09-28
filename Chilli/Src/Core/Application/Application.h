#pragma once

#include "Window/Window.h"

namespace Chilli
{
	struct ApplicationSpec
	{
		const char* Name;
		struct {
			int x, y;
		} Dimensions;
		bool VSync;
	};

	class Application
	{
	public:
		Application(){}
		~Application(){}

		void Init(const ApplicationSpec& Spec);
		void ShutDown();

		void Run();
	private:
		Window _Window;
	};
}
