#pragma once

namespace Chilli
{
	struct TimeStep
	{
	public:
		TimeStep(float Time) :_Time(Time) {}
		~TimeStep() {}

		float GetSecond() const { return _Time; }
		float GetMilliSecond() const { return _Time * 1000.0f; }

		operator float() { return _Time; }
	private:
		float _Time;
	};
}
