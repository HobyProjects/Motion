#include "CorePCH.hpp"
#include "Texture.hpp"

namespace Motion::Core
{
    uint32_t TextureSpecification::GlobalAnisotropyLevel = 1; // Default anisotropy level
    static std::unordered_map<UUID, std::shared_ptr<ITexture>> s_TextureRegistry{};

    void TextureManager::InsertTexture(const UUID& uuid, const std::shared_ptr<ITexture>& texture)
    {
        if (s_TextureRegistry.find(uuid) != s_TextureRegistry.end())
        {
            MOTION_CORE_WARN("Texture with UUID {0} already exists!", uuid);
            return;
        }
        s_TextureRegistry[uuid] = texture;
    }

    std::shared_ptr<ITexture> TextureManager::GetTexture(const UUID& uuid)
    {
        if (s_TextureRegistry.find(uuid) == s_TextureRegistry.end())
        {
            MOTION_CORE_ERROR("Texture with UUID {0} does not exist!", uuid);
            return nullptr;
        }
        return s_TextureRegistry[uuid];
    }

    std::shared_ptr<ITexture> TextureManager::GetTexture(const std::string& name)
    {
        for (const auto& [uuid, texture] : s_TextureRegistry)
        {
            if (texture->GetMetaData().AssetName == name)
            {
                return texture;
            }
        }
        MOTION_CORE_ERROR("Texture with name {0} does not exist!", name);
        return nullptr;
    }

    std::unordered_map<UUID,std::shared_ptr<ITexture>>::const_iterator TextureManager::GetTextures()
    {
        return s_TextureRegistry.cbegin();
    }
}

