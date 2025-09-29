#pragma once

#include "RenderAPI.h"

namespace Chilli
{
	class Renderer
	{
	public:
		static Renderer& Get()
		{
			static Renderer Instance;
			return Instance;
		}

		static void Init(RenderAPITypes Type);
		static void ShutDown();

		static const char* GetName() { return Get()._Api->GetName(); }
		static RenderAPITypes GetType() { return Get()._Api->GetType(); }
	private:
		RenderAPI* _Api;
	};
}
