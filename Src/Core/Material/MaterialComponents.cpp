#include "CorePCH.hpp"
#include "MaterialComponents.hpp"

namespace Motion
{
    ResolvedMaterials GetResolvedMaterials(Material* material)
    {
        ResolvedMaterials rm;
        if(!material) 
            return rm;

        auto applyTexture = 
        [&](ITexture* tex, ITexture* apply, TextureType type, TexturesBitMask mask) 
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
                        apply = base->Textures[type].get();
                        rm.TMask |= mask;
                    }
                }
            }

            if(rm.TMask & TexturesBitMask::HasORM)             rm.UseORMTextures = true;
            if(rm.TMask & TexturesBitMask::HasDisplacement)    rm.UseDisplacement = true;
        };

        if(!material->HasTexture<CorePBR>() || !material->HasTexture<AlphaProperties>())
        {
            MOTION_ASSERT(false, "Material does not have required textures!");
            return rm;
        }

        const auto& c = material->GetTexture<CorePBR>();
        const auto& a = material->GetTexture<AlphaProperties>();

        applyTexture(c.BaseColorTexture.get(),      rm.BaseColor,       TextureType::BaseColorTexture,          TexturesBitMask::HasBaseColor);
        applyTexture(c.NormalTexture.get(),         rm.Normal,          TextureType::NormalTexture,             TexturesBitMask::HasNormal);
        applyTexture(c.MetallicTexture.get(),       rm.Metallic,        TextureType::MetallicTexture,           TexturesBitMask::HasMetallic);
        applyTexture(c.RoughnessTexture.get(),      rm.Roughness,       TextureType::RoughnessTexture,          TexturesBitMask::HasRoughness);
        applyTexture(c.OcclusionTexture.get(),      rm.AO,              TextureType::AmbientOcclusionTexture,   TexturesBitMask::HasOcclusion);
        applyTexture(c.EmissiveTexture.get(),       rm.Emissive,        TextureType::EmissiveTexture,           TexturesBitMask::HasEmissive);
        applyTexture(c.DisplacementTexture.get(),   rm.Displacement,    TextureType::DisplacementTexture,       TexturesBitMask::HasDisplacement);
        applyTexture(a.OpacityTexture.get(),        rm.Opacity,         TextureType::OpacityTexture,            TexturesBitMask::HasOpacity);

        if(material->HasTexture<PackedMaps>())
        {
            const auto& p = material->GetTexture<PackedMaps>();
            applyTexture(p.ORMTexture.get(), rm.ORM, TextureType::ORMTexture, TexturesBitMask::HasORM);
        }

        rm.BaseColorFactor      = c.BaseColorFactor;
        rm.NormalScale          = c.NormalScale;
        rm.MetallicFactor       = c.MetallicFactor;
        rm.RoughnessFactor      = c.RoughnessFactor;
        rm.AOStrength           = c.OcclusionStrength;
        rm.EmissiveStrength     = c.EmissiveStrength;
        rm.EmissiveFactor       = c.EmissiveFactor;
        rm.OpacityFactor        = a.OpacityFactor;
        rm.SpecularStrength     = c.SpecularStrength;

        rm.Mode                 = a.Mode;
        rm.BlendMode            = a.BlendMode;
        rm.AlphaCutoff          = a.AlphaCutoff;

        rm.DispScale            = c.DisplacementScale;
        rm.DispBias             = c.DisplacementBias;
        rm.AdvDisplacement      = c.AdvDisplacement;

        return rm;
    }
}