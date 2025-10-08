#pragma once

#include <cstdint>
#include <random>
#include <functional>

namespace Motion
{
    using UUID = uint64_t;

    class UniqueIdentity
    {
        private:
            UniqueIdentity() = default;
            ~UniqueIdentity() = default;

            UniqueIdentity(const UniqueIdentity&) = delete;
            UniqueIdentity& operator=(const UniqueIdentity&) = delete;
            UniqueIdentity(const UniqueIdentity&&) = delete;
            UniqueIdentity&& operator=(const UniqueIdentity&&) = delete;

        public:
            static UUID GetUniqueID()
            {
                static std::mt19937_64 s_Range{ std::random_device{}() };
                static std::uniform_int_distribution<uint64_t> s_Distribution;
                return s_Distribution(s_Range);
            }

            static UUID GetHashUniqueID()
            {
                return std::hash<UUID>{}(GetUniqueID());
            }

            static UUID GetHashID(UUID uniqueID)
            {
                return std::hash<UUID>{}(uniqueID);
            }

            static std::string ToString(UUID uniqueID)
            {
                return std::to_string(uniqueID);
            }
    };
}