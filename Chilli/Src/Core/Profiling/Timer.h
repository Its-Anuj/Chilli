/*
	Header storing a Timer class meant to measure execution time for debugging efficiency.
*/

#pragma once

#include <chrono>
#include <string>

namespace Chilli
{
	class Timer
	{
	public:
		Timer()
			:_Start(std::chrono::high_resolution_clock::now())
		{
		}

		~Timer()
		{
		}

		// Returns Time Elapsed In Ms
		std::chrono::milliseconds ElapsedMs()
		{
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - _Start);
			return duration;
		}

		// Returns Time Elapsed In Microsecond
		std::chrono::microseconds ElapsedMc()
		{
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - _Start);
			return duration;
		}

		// Returns Time Elapsed In Second
		std::chrono::seconds ElapsedS()
		{
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - _Start);
			return duration;
		}

		// Returns Time Elapsed In Milli seconds in long 
		long long ElapsedMsl()
		{
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - _Start);
			return duration.count();
		}

		// Returns Time Elapsed In Microsecond
		long long ElapsedMcl()
		{
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - _Start);
			return duration.count();
		}

		// Returns Time Elapsed In Second
		long long ElapsedSl()
		{
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - _Start);
			return duration.count();
		}

		void Reset()
		{
			_Start = std::chrono::high_resolution_clock::now();
		}

	private:
		std::chrono::high_resolution_clock::time_point _Start;
	};

	class DebugTimer
	{
	public:
		template<typename... Args>
		explicit DebugTimer(fmt::format_string<Args...> fmt, Args&&... args)
			: _Name(fmt::format(fmt, std::forward<Args>(args)...))
		{
		}

		~DebugTimer()
		{
			auto Duration = _Timer.ElapsedMsl();
			_Timer.Reset();
			CH_CORE_INFO("{0} took: {1} ms", _Name, Duration);
		}

	private:
		std::string _Name;
		Timer _Timer;
	};
}

#if CHILLI_ENGINE_DEBUG == true
#define CHILLI_DEBUG_TIMER(...) Chilli::DebugTimer _DebugTimer(__VA_ARGS__)
#else
#define CHILLI_DEBUG_TIMER(...)
#endif