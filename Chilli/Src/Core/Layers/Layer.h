#pragma once

#include "Events/Events.h"

namespace Chilli
{
	class Layer
	{
	public:
		Layer(const char* Name);
		~Layer();

		virtual void Init() = 0;
		virtual void Terminate() = 0;
		virtual void Update() = 0;
		virtual void OnEvent(Event& e) = 0;
	private:
		std::string _Name;
	};
}
