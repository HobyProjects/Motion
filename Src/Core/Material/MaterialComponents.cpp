#include "CorePCH.hpp"
#include "MaterialComponents.hpp"

namespace Motion
{
    void ApplyAlphaState(const AlphaProperties& a, StageStatus& rs)
    {
        switch (a.Mode) 
        {
            case AlphaMode::Opaque:
                rs.BlendEnabled = false;
                rs.DepthWrite   = true;
                break;

            case AlphaMode::Mask:
                rs.BlendEnabled = false;   
                rs.DepthWrite   = true;      
                break;

            case AlphaMode::Blend: 
                rs.BlendEnabled = true;
                rs.DepthWrite   = false;     
                if (a.BlendMode == AlphaBlendMode::Premultiplied) 
                {
                    rs.SrcRGB = BlendFactor::One;
                    rs.DstRGB = BlendFactor::OneMinusSrcAlpha;
                    rs.SrcA   = BlendFactor::One;
                    rs.DstA   = BlendFactor::OneMinusSrcAlpha;
                } 
                else 
                { 
                    rs.SrcRGB = BlendFactor::SrcAlpha;
                    rs.DstRGB = BlendFactor::OneMinusSrcAlpha;
                    rs.SrcA   = BlendFactor::One;
                    rs.DstA   = BlendFactor::OneMinusSrcAlpha;
                }
                break;
        }
    }

    ResolvedMaterials GetResolvedMaterials(const CorePBR& c, const PackedMaps& p, const AlphaProperties& a)
    {
        ResolvedMaterials rm;

        auto applyTexture = 
        [&](ITexture* tex, ITexture* apply, TexturesBitMask mask) 
        {
            if (tex) 
            {
                apply = tex;
                rm.TMask |= mask;

                if(mask == TexturesBitMask::HasORM)
                    rm.UseORMTextures = true;
            }
        };

        applyTexture(c.BaseColorTexture.get(), rm.BaseColor, TexturesBitMask::HasBaseColor);
        applyTexture(c.NormalTexture.get(), rm.Normal, TexturesBitMask::HasNormal);
        applyTexture(c.MetallicTexture.get(), rm.Metallic, TexturesBitMask::HasMetallic);
        applyTexture(c.RoughnessTexture.get(), rm.Roughness, TexturesBitMask::HasRoughness);
        applyTexture(c.OcclusionTexture.get(), rm.AO, TexturesBitMask::HasOcclusion);
        applyTexture(c.EmissiveTexture.get(), rm.Emissive, TexturesBitMask::HasEmissive);
        applyTexture(c.DisplacementTexture.get(), rm.Displacement, TexturesBitMask::HasDisplacement);
        applyTexture(a.OpacityTexture.get(), rm.Opacity, TexturesBitMask::HasOpacity);
        applyTexture(p.ORMTexture.get(), rm.ORM, TexturesBitMask::HasORM);

        rm.BaseColorFactor      = c.BaseColorFactor;
        rm.NormalScale          = c.NormalScale;
        rm.MetallicFactor       = c.MetallicFactor;
        rm.RoughnessFactor      = c.RoughnessFactor;
        rm.AOStrength           = c.OcclusionStrength;
        rm.EmissiveStrength     = c.EmissiveStrength;
        rm.EmissiveFactor       = c.EmissiveFactor;
        rm.OpacityFactor        = a.OpacityFactor;

        rm.Mode                 = a.Mode;
        rm.BlendMode            = a.BlendMode;
        rm.AlphaCutoff          = a.AlphaCutoff;

        rm.DispScale            = c.DisplacementScale;
        rm.DispBias             = c.DisplacementBias;
        rm.AdvDisplacement      = c.AdvDisplacement;

        return rm;
    }
}