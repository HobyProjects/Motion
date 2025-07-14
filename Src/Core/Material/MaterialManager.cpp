#include "CorePCH.hpp"
#include "MaterialManager.hpp"

namespace Motion::Core
{
    static std::unordered_map<UUID, std::shared_ptr<Material>> s_MaterialsByID;
    static std::unordered_map<std::string, UUID> s_NameToUUID;

    void MaterialManager::Rgister(const std::string& name, const std::shared_ptr<Material>& material)
    {
        if (s_MaterialsByID.find(material->GetMetaData().AssetID) != s_MaterialsByID.end())
        {
            MOTION_CORE_WARN("Material with UUID {0} already exists!", material->GetMetaData().AssetID);
            return;
        }

        s_MaterialsByID[material->GetMetaData().AssetID] = std::move(material);
        s_NameToUUID[name] = material->GetMetaData().AssetID;
    }

    std::shared_ptr<Material> MaterialManager::Get(const UUID& id)
    {
        if (s_MaterialsByID.find(id) == s_MaterialsByID.end())
        {
            MOTION_CORE_WARN("Material with UUID {0} does not exist!", id);
            return nullptr;
        }

        return s_MaterialsByID[id];
    }

    std::shared_ptr<Material> MaterialManager::Get(const std::string& name)
    {
        if (s_NameToUUID.find(name) == s_NameToUUID.end())
        {
            MOTION_CORE_WARN("Material with name {0} does not exist!", name);
            return nullptr;
        }

        return s_MaterialsByID[s_NameToUUID[name]];
    }

    bool MaterialManager::HasMaterial(const std::string& name)
    {
        return s_NameToUUID.find(name) != s_NameToUUID.end();
    }

    bool MaterialManager::HasMaterial(const UUID& name)
    {
        return s_MaterialsByID.find(name) != s_MaterialsByID.end();
    }

    void MaterialManager::Unregister(const UUID& id)
    {
        if (s_MaterialsByID.find(id) == s_MaterialsByID.end())
        {
            MOTION_CORE_WARN("Material with UUID {0} does not exist!", id);
            return;
        }

        s_MaterialsByID.erase(id);
    }

    void MaterialManager::Unregister(const std::string& name)
    {
        if (s_NameToUUID.find(name) == s_NameToUUID.end())
        {
            MOTION_CORE_WARN("Material with name {0} does not exist!", name);
            return;
        }

        s_MaterialsByID.erase(s_NameToUUID[name]);
        s_NameToUUID.erase(name);
    }
}