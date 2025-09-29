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
        explicit DebugTimer(const char* Name)
            : _Name(Name), _Start(std::chrono::high_resolution_clock::now())
        {
        }

        ~DebugTimer()
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - _Start);

            CH_CORE_INFO("{0} took: {1} ms", _Name.c_str(), duration.count);
        }

    private:
        std::chrono::high_resolution_clock::time_point _Start;
        std::string _Name;
    };
}

#if CHILLI_ENGINE_DEBUG == true
#define CHILLI_DEBUG_TIMER(Name) Chilli::DebugTimer _DebugTimer(Name);
#else
#define CHILLI_DEBUG_TIMER(Name)
#endif