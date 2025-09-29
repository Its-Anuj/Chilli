#pragma once

namespace Chilli
{
	enum class RenderAPITypes
	{
		VULKAN1_3
	};

	class RenderAPI
	{
	public:
		virtual ~RenderAPI(){}

		virtual void Init(void* Spec) = 0;
		virtual void ShutDown() = 0;

		virtual RenderAPITypes GetType() = 0;
		virtual const char* GetName() = 0;
	protected:
	};
}
