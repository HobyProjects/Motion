#include "CorePCH.hpp"

namespace Motion::Core
{
    static std::unordered_map<UUID, std::shared_ptr<Material>> s_MaterialsByID;
    static std::unordered_map<std::string, UUID> s_NameToUUID;
}