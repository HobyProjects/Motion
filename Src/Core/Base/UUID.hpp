#pragma once

#include <cstdint>
#include <random>
#include <functional>

namespace Motion::Core
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
        /**
         * @brief Generates a unique 64-bit unsigned integer ID.
         *
         * This static function uses a Mersenne Twister 64-bit random number generator
         * seeded with a random device to produce a unique identifier each time it is called.
         * The distribution covers the entire range of uint64_t values.
         *
         * @return A randomly generated unique 64-bit unsigned integer.
         */
        static UUID GetUniqueID()
        {
            static std::mt19937_64 s_Range{ std::random_device{}() };
            static std::uniform_int_distribution<uint64_t> s_Distribution;
            return s_Distribution(s_Range);
        }

        /**
         * @brief Generates a hash value based on the unique identifier.
         *
         * This static method computes and returns a hashed version of the unique ID
         * obtained from GetUniqueID(). The hash is generated using std::hash on the UUID type.
         *
         * @return UUID The hashed value of the unique identifier.
         */
        static UUID GetHashUniqueID()
        {
            return std::hash<UUID>{}(GetUniqueID());
        }

        /**
         * @brief Generates a hash value for the given UUID.
         *
         * This static function takes a UUID as input and returns a hashed value
         * of the UUID using the standard library's hash function. The resulting
         * value can be used as a unique identifier for the given UUID in hash-based
         * containers or for quick comparisons.
         *
         * @param uniqueID The UUID to be hashed.
         * @return The hashed value of the input UUID.
         */
        static UUID GetHashID(UUID uniqueID)
        {
            return std::hash<UUID>{}(uniqueID);
        }
    };
}