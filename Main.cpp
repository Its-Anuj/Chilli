#include <iostream>
#include "Chilli/Chilli.h"

int main()
{
	Chilli::ApplicationSpec Spec{};
	Spec.Name = "Chilli Editor";
	Spec.Dimensions = { 1280, 720 };
	Spec.VSync = true;

	Chilli::Application App;
	App.Init(Spec);
	App.Run();
	App.ShutDown();

	std::cin.get();
}
