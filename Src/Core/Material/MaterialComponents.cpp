#include "CorePCH.hpp"
#include "MaterialComponents.hpp"

namespace Motion
{
    ResolvedMaterials GetResolvedMaterials(Material* material)
    {
        ResolvedMaterials rm;
        if(!material) return rm;

        auto applyTexture =  [&](const std::shared_ptr<ITexture>& tex, std::weak_ptr<ITexture>& apply, TextureType type, TexturesBitMask mask) 
        {
            if (tex) 
            {
                apply = tex;
                rm.TMask |= mask;
            }
            else
            {
                if(material->GetBaseMaterial())
                {
                    std::shared_ptr<BaseMaterial> base = material->GetBaseMaterial();
                    if(base->Textures.contains(type))
                    {
                        apply = base->Textures[type];
                        rm.TMask |= mask;
                    }
                }
                else
                {
                    apply.reset();
                }
            }
        };

        if(!material->Has<CoreMaterialComponents>())
        {
            MOTION_ASSERT(false, "Material does not have required textures!");
            return rm;
        }

        const auto& c = material->Get<CoreMaterialComponents>();

        applyTexture(c.BaseColorTexture,      rm.BaseColor,       TextureType::BaseColorTexture,          TexturesBitMask::HasBaseColor);
        applyTexture(c.NormalTexture,         rm.Normal,          TextureType::NormalTexture,             TexturesBitMask::HasNormal);
        applyTexture(c.MetallicTexture,       rm.Metallic,        TextureType::MetallicTexture,           TexturesBitMask::HasMetallic);
        applyTexture(c.RoughnessTexture,      rm.Roughness,       TextureType::RoughnessTexture,          TexturesBitMask::HasRoughness);
        applyTexture(c.OcclusionTexture,      rm.AO,              TextureType::AmbientOcclusionTexture,   TexturesBitMask::HasOcclusion);
        applyTexture(c.EmissiveTexture,       rm.Emissive,        TextureType::EmissiveTexture,           TexturesBitMask::HasEmissive);

        if(material->Has<PackedMaterialComponents>())
        {
            const auto& p = material->Get<PackedMaterialComponents>();
            applyTexture(p.ORMTexture, rm.ORM, TextureType::ORMTexture, TexturesBitMask::HasORM);
        }

        rm.BaseColorFactor      = c.BaseColorFactor;
        rm.NormalScale          = c.NormalScale;
        rm.MetallicFactor       = c.MetallicFactor;
        rm.RoughnessFactor      = c.RoughnessFactor;
        rm.AOStrength           = c.OcclusionStrength;
        rm.EmissiveStrength     = c.EmissiveStrength;
        rm.EmissiveFactor       = c.EmissiveFactor;
        rm.OpacityFactor        = c.OpacityFactor;

        return rm;
    }
}