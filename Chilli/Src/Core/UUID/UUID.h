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
} // namespace std