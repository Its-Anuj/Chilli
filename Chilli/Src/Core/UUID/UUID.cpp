//#include <random>
//#include "UUID.h"
//
//namespace Chilli
//{
//    static std::random_device RandomDevice;
//    static std::mt19937_64 Engine(RandomDevice());
//    static std::uniform_int_distribution<uint64_t> UniformDistritbuion;
//    static std::uniform_int_distribution<uint32_t> UUID_32_UniformDistritbuion;
//
//    UUID::UUID()
//        :ID(UniformDistritbuion(Engine))
//    {
//    }
//
//    UUID_32::UUID_32()
//        :ID(UUID_32_UniformDistritbuion(Engine))
//    {
//    }
//} // namespace VEngine