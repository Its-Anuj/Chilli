#include <random>
#include "UUID.h"

namespace Chilli
{
    static std::random_device RandomDevice;
    static std::mt19937_64 Engine(RandomDevice());
    static std::uniform_int_distribution<uint64_t> UniformDistritbuion;

    UUID::UUID()
        :ID(UniformDistritbuion(Engine))
    {
    }
} // namespace VEngine