#pragma once

#include "SpdLog/spdlog.h"

namespace Chilli
{
	class Log
	{
	public:
		static void Init();
		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

#if CHILLI_ENGINE_DEBUG == true
// Core log macros
#define CH_CORE_TRACE(...)    ::Chilli::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define CH_CORE_INFO(...)     ::Chilli::Log::GetCoreLogger()->info(__VA_ARGS__)
#define CH_CORE_WARN(...)     ::Chilli::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define CH_CORE_ERROR(...)    ::Chilli::Log::GetCoreLogger()->error(__VA_ARGS__)
#define CH_CORE_CRITICAL(...) ::Chilli::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define CH_TRACE(...)         ::Chilli::Log::GetClientLogger()->trace(__VA_ARGS__)
#define CH_INFO(...)          ::Chilli::Log::GetClientLogger()->info(__VA_ARGS__)
#define CH_WARN(...)          ::Chilli::Log::GetClientLogger()->warn(__VA_ARGS__)
#define CH_ERROR(...)         ::Chilli::Log::GetClientLogger()->error(__VA_ARGS__)
#define CH_CRITICAL(...)      ::Chilli::Log::GetClientLogger()->critical(__VA_ARGS__)

#define CH_CORE_ASSERT(Condition, Msg) { \
    if (!(Condition)) { \
        CH_CORE_ERROR(Msg); \
        assert((Condition) && "Assert Failed"); \
    } \
}

#else
#define CH_CORE_TRACE(...)    
#define CH_CORE_INFO(...)     
#define CH_CORE_WARN(...)     
#define CH_CORE_ERROR(...)    
#define CH_CORE_CRITICAL(...) 
#define CH_TRACE(...)         
#define CH_INFO(...)          
#define CH_WARN(...)          
#define CH_ERROR(...)         
#define CH_CRITICAL(...)      
#define CH_CORE_ASSERT(...)
#endif