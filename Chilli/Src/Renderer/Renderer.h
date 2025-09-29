#pragma once

#include "RenderAPI.h"

namespace Chilli
{
	struct RendererSpec
	{
		Window* RenderWindow;
		bool EnableValidation = true;
	};

	class Renderer
	{
	public:
		static Renderer& Get()
		{
			static Renderer Instance;
			return Instance;
		}

		static void Init(RenderAPITypes Type, const RendererSpec& RenderSpec);
		static void ShutDown();

		static const char* GetName() { return Get()._Api->GetName(); }
		static RenderAPITypes GetType() { return Get()._Api->GetType(); }
	private:
		RenderAPI* _Api;
	};
}
