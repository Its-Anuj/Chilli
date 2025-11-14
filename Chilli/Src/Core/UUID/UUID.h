#pragma once

namespace Chilli
{
    class UUID
    {
    public:
        UUID();
        UUID(uint64_t id) : ID(id) {}
        UUID(const UUID&) = default;

        operator uint64_t() const { return ID; }

    private:
        uint64_t ID;
    };

    class UUID_32
    {
    public:
        UUID_32();
        UUID_32(uint32_t id) : ID(id) {}
        UUID_32(const UUID_32&) = default;

        operator uint32_t() const { return ID; }

    private:
        uint32_t ID;
    };
} // namespace VEngine

namespace std
{
    template <>
    struct hash<Chilli::UUID>
    {
        std::size_t operator()(const Chilli::UUID& uuid) const
        {
            return hash<uint64_t>()((uint64_t)uuid);
        }
    };

    template <>
    struct hash<Chilli::UUID_32>
    {
        std::size_t operator()(const Chilli::UUID_32& uuid) const
        {
            return hash<uint32_t>()((uint32_t)uuid);
        }
    };
} // namespace std