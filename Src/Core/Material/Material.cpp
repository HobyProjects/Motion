#include "CorePCH.hpp"
#include "Material.hpp"

namespace Motion::Core
{
    void Material::SetUniform(const std::string& name, float value) 
    {
        m_FloatUniformsMaps[name] = value;
    }

    void Material::SetUniform(const std::string& name, const glm::vec3& value) 
    {
        m_Vec3UniformsMaps[name] = value;
    }

    void Material::SetUniform(const std::string& name, const glm::vec4& value) 
    {
        m_Vec4UniformsMaps[name] = value;
    }

    void Material::SetTexture(const std::string& name, const std::shared_ptr<ITexture>& texture) 
    {
        m_TexturesMaps[name] = std::move(texture);
    }

    Material::ShadingMethod Material::GetShadingMethod()
    {
        return DetectShadingMethod();
    }

    Material::ShadingMethod Material::DetectShadingMethod()
    {
        glm::vec3 baseColor = GetVec3Uniform(UniformCache::MaterialFactorsUniforms::BaseColor);
        float metallicFactor = GetFloatUniform(UniformCache::MaterialFactorsUniforms::MetallicFactor);
        float roughnessFactor = GetFloatUniform(UniformCache::MaterialFactorsUniforms::RoughnessFactor);

        bool usesPBR = baseColor != glm::vec3(1.0f) || metallicFactor > 0.0f || roughnessFactor < 1.0f ||
                   m_TexturesMaps.contains(UniformCache::PBRTextureUniforms::BaseColorTexture) ||
                   m_TexturesMaps.contains(UniformCache::PBRTextureUniforms::MetallicTexture);

        if(usesPBR)
            return ShadingMethod::PBR;

        glm::vec3 diffuseColor = GetVec3Uniform(UniformCache::SurfaceColorsUniforms::DiffuseColor);
        glm::vec3 specularColor = GetVec3Uniform(UniformCache::SurfaceColorsUniforms::SpecularColor);
        float shininess = GetFloatUniform(UniformCache::MaterialPropertiesUniforms::Shininess);

        bool usesLegacy = diffuseColor != glm::vec3(0.0f) || specularColor != glm::vec3(0.0f) || shininess > 0.0f ||
                    m_TexturesMaps.contains(UniformCache::LegacyTextureUniforms::DiffuseTexture) ||
                    m_TexturesMaps.contains(UniformCache::LegacyTextureUniforms::SpecularTexture);

        if(usesLegacy)
            return ShadingMethod::Phong;

        if (GetVec3Uniform(UniformCache::SurfaceColorsUniforms::EmissiveColor) != glm::vec3(0.0f))
            return ShadingMethod::Unlit;

        return ShadingMethod::PBR;
    }

    void Material::Bind() 
    {
        ShadingMethod shadingMethod = DetectShadingMethod();
        std::weak_ptr<IShader> shader; 

        if(shadingMethod == ShadingMethod::PBR)
            shader = AssetManager::GetShader("PBRShader");
        else if(shadingMethod == ShadingMethod::Phong)
            shader = AssetManager::GetShader("PhongShader");
        else if(shadingMethod == ShadingMethod::Unlit)
            shader = AssetManager::GetShader("UnlitShader");

        if(!shader.expired())
        {
            auto materialShader = shader.lock();
            for (const auto& [name, val] : m_FloatUniformsMaps)
                materialShader->SetUniform(name, val);

            for (const auto& [name, val] : m_Vec3UniformsMaps)
                materialShader->SetUniform(name, val);

            for (const auto& [name, val] : m_Vec4UniformsMaps)
                materialShader->SetUniform(name, val);

            uint32_t slot = 0;
            for (const auto& [name, tex] : m_TexturesMaps) 
            {
                tex->Bind(slot);
                materialShader->SetUniform(name, static_cast<float>(slot));
                ++slot;
            }
        }
    }

    void Material::Bind(const std::shared_ptr<IShader>& shader)
    {
        std::weak_ptr<IShader> matShader = shader;

        if(!matShader.expired())
        {
            auto materialShader = matShader.lock();
            for (const auto& [name, val] : m_FloatUniformsMaps)
                materialShader->SetUniform(name, val);

            for (const auto& [name, val] : m_Vec3UniformsMaps)
                materialShader->SetUniform(name, val);

            for (const auto& [name, val] : m_Vec4UniformsMaps)
                materialShader->SetUniform(name, val);

            uint32_t slot = 0;
            for (const auto& [name, tex] : m_TexturesMaps) 
            {
                tex->Bind(slot);
                materialShader->SetUniform(name, static_cast<float>(slot));
                ++slot;
            }
        }
    }

    void Material::Unbind() 
    {
        for (const auto& [name, tex] : m_TexturesMaps) 
        {
            tex->Unbind();
        }
    }
}