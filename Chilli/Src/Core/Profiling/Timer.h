/*
    Header storing a Timer class meant to measure execution time for debugging efficiency.
*/

#include <chrono>
#include <string>

namespace Chilli
{
    class DebugTimer
    {
    public:
        template<typename... Args>
        explicit DebugTimer(fmt::format_string<Args...> fmt, Args&&... args)
            : _Name(fmt::format(fmt, std::forward<Args>(args)...)),
            _Start(std::chrono::high_resolution_clock::now())
        {
        }

        ~DebugTimer()
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - _Start);

            CH_CORE_INFO("{0} took: {1} ms", _Name, duration.count());
        }

    private:
        std::string _Name;
        std::chrono::high_resolution_clock::time_point _Start;
    };
}

#if CHILLI_ENGINE_DEBUG == true
#define CHILLI_DEBUG_TIMER(...) Chilli::DebugTimer _DebugTimer(__VA_ARGS__)
#else
#define CHILLI_DEBUG_TIMER(...)
#endif