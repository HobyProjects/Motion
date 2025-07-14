#include "CorePCH.hpp"
#include "Material.hpp"

namespace Motion::Core
{
    /**
     * @brief Constructs a Material object with the specified UUID, name, and shading method.
     *
     * @param uuid The universally unique identifier for the material.
     * @param name The name of the material.
     * @param shadingMethod The shading method used by the material.
     */
    Material::Material(const UUID& uuid, const std::string& name, MaterialShadingMethod shadingMethod) :
        AssetBase(uuid, name, AssetType::Material, "Undefined"), m_ShadingMethod(shadingMethod)
    {
    }

    /**
     * @brief Constructs a Material object with the specified name and shading method.
     *
     * Initializes the Material by assigning a unique ID, setting its name,
     * specifying its asset type as Material, and setting the shading method.
     *
     * @param name The name of the material.
     * @param shadingMethod The shading method to be used by the material.
     */
    Material::Material(const std::string& name, MaterialShadingMethod shadingMethod) :
        AssetBase(UniqueIdentity::GetUniqueID(), name, AssetType::Material, "Undefined"), m_ShadingMethod(shadingMethod)
    {
    }

    /**
     * @brief Binds the material parameters and textures to the given shader.
     *
     * This function ensures that the shading method is determined only once per material instance.
     * If the provided shader is valid and currently in use, it iterates over all parameter bindings,
     * setting each parameter as a uniform in the shader. It also binds all associated textures.
     *
     * @param shader A shared pointer to the shader to which the material parameters and textures will be bound.
     * @note This function is noexcept and thread-safe for the shading method determination.
     */
    void Material::Bind(const std::shared_ptr<IShader>& shader) const noexcept
    {
        static std::once_flag flag;
        std::call_once(flag, [this]() { DetermineShadingMethod(); });

        if (shader && shader->InUse())
        {
            for (const auto& [name, binding] : m_ParameterBindings)
            {
                const auto& value = binding.ParameterValue;
                std::visit([&](auto&& arg) { shader->SetUniform(name.data(), arg); }, value);
            }

            std::uint32_t bindingPoint = 0;
            for (const auto& [name, binding] : m_TextureBindings)
            {
                if (binding.Texture)
                {
                    shader->SetUniform(std::format("{}_{}", name.data(), bindingPoint).c_str(), static_cast<float>(bindingPoint));
                    binding.Texture->Bind(bindingPoint);
                    bindingPoint++;
                }
            }
        }
    }

    /**
     * @brief Unbinds all textures associated with this material.
     *
     * Iterates through all texture bindings and calls the Unbind method
     * on each bound texture, if present. This is typically used to
     * release texture resources from the rendering pipeline after use.
     *
     * @note This method does not throw exceptions.
     */
    void Material::Unbind() const noexcept
    {
        for (const auto& [name, binding] : m_TextureBindings)
        {
            if (binding.Texture)
                binding.Texture->Unbind();
        }
    }

    /**
     * @brief Associates a texture with a given name in the material.
     *
     * This function binds a texture to the material using the specified name as the key.
     * If the provided texture is valid (non-null), it is stored in the material's texture bindings.
     *
     * @param name The identifier for the texture binding.
     * @param texture A shared pointer to the texture to be associated with the name.
     */
    void Material::Set(std::string_view name, std::shared_ptr<ITexture> texture)
    {
        if (texture)
            m_TextureBindings[name] = { name, std::move(texture) };
    }

    /**
     * @brief Sets a material parameter by name.
     *
     * Associates the given parameter name with the specified MaterialParameters value.
     * If the parameter already exists, its value will be updated.
     *
     * @param name The name of the material parameter to set.
     * @param value The MaterialParameters value to associate with the parameter name.
     */
    void Material::Set(std::string_view name, const MaterialParameters& value)
    {
        m_ParameterBindings[name] = { name, value };
    }

    /**
     * @brief Checks if a texture with the specified name is bound to the material.
     *
     * @param name The name of the texture to check for.
     * @return true if the texture is bound; false otherwise.
     *
     * @note This function does not throw exceptions.
     */
    bool Material::HasTexture(std::string_view name) const noexcept
    {
        return m_TextureBindings.find(name) != m_TextureBindings.end();
    }

    /**
     * @brief Retrieves a texture bound to the material by its name.
     *
     * Searches for a texture with the specified name in the material's texture bindings.
     * If found, returns a shared pointer to the texture. If not found, logs an error
     * and returns nullptr.
     *
     * @param name The name of the texture to retrieve.
     * @return std::shared_ptr<ITexture> Shared pointer to the texture if found, nullptr otherwise.
     */
    std::shared_ptr<ITexture> Material::GetTexture(std::string_view name) const noexcept
    {
        auto it = m_TextureBindings.find(name);
        if (it != m_TextureBindings.end())
            return it->second.Texture;

        MOTION_CORE_ERROR("Material does not have a texture with name: {}", name);
        return nullptr;
    }

    /**
     * @brief Checks if a material property with the given name exists.
     *
     * This function determines whether a property identified by the specified
     * name is present in the material's parameter bindings.
     *
     * @param name The name of the property to check for existence.
     * @return true if the property exists; false otherwise.
     */
    bool Material::HasProperty(std::string_view name) const noexcept
    {
        return m_ParameterBindings.find(name) != m_ParameterBindings.end();
    }

    /**
     * @brief Retrieves the value of a material property by its name.
     *
     * Searches for the specified property name in the material's parameter bindings.
     * If the property exists, its value is returned. Otherwise, an error is logged
     * and a default-constructed MaterialParameters object is returned.
     *
     * @param name The name of the property to retrieve.
     * @return MaterialParameters The value of the property if found; otherwise, a default value.
     */
    MaterialParameters Material::GetProperty(std::string_view name) const noexcept
    {
        auto it = m_ParameterBindings.find(name);
        if (it != m_ParameterBindings.end())
            return it->second.ParameterValue;

        MOTION_CORE_ERROR("Material does not have a property with name: {}", name);
        return {};
    }

    /**
     * @brief Determines the appropriate shading method for the material based on its properties and textures.
     *
     * This method inspects the material's properties and associated textures to automatically select
     * a shading method if the current shading method is set to MaterialShadingMethod::Auto.
     * The selection is made in the following order:
     *   1. PBR (Physically Based Rendering): Chosen if base color, metallic, and roughness factors are valid and
     *      all required PBR textures are present.
     *   2. Phong: Chosen if diffuse and specular colors, and shininess are valid, or if all legacy Phong textures are present.
     *   3. Unlit: Chosen if the emissive color is valid and the emissive texture is present.
     * If none of the above conditions are met, the method defaults to Phong shading and logs a warning.
     *
     * @note This method modifies the m_ShadingMethod member variable if the shading method is determined.
     * @note The method is noexcept and does not throw exceptions.
     */
    void Material::DetermineShadingMethod() const noexcept
    {
        if (m_ShadingMethod == MaterialShadingMethod::Auto)
        {
            glm::vec3 baseColor = GetPropertyValue<glm::vec3>(UniformCache::MaterialFactorsUniforms::BaseColor);
            float metallicFactor = GetPropertyValue<float>(UniformCache::MaterialFactorsUniforms::MetallicFactor);
            float roughnessFactor = GetPropertyValue<float>(UniformCache::MaterialFactorsUniforms::RoughnessFactor);

            bool usePBR = baseColor != glm::vec3(0.0f) && metallicFactor > 0.0f && roughnessFactor < 1.0f &&
                HasTexture(UniformCache::PBRTextureUniforms::BaseColorTexture) &&
                HasTexture(UniformCache::PBRTextureUniforms::MetallicTexture) &&
                HasTexture(UniformCache::PBRTextureUniforms::RoughnessTexture) &&
                HasTexture(UniformCache::PBRTextureUniforms::AOMapTexture);

            if (usePBR)
            {
                m_ShadingMethod = MaterialShadingMethod::PBR;
                return;
            }

            glm::vec3 diffuseColor = GetPropertyValue<glm::vec3>(UniformCache::SurfaceColorsUniforms::DiffuseColor);
            glm::vec3 specularColor = GetPropertyValue<glm::vec3>(UniformCache::SurfaceColorsUniforms::SpecularColor);
            float shininess = GetPropertyValue<float>(UniformCache::MaterialPropertiesUniforms::Shininess);

            bool usePhong = diffuseColor != glm::vec3(0.0f) && specularColor != glm::vec3(0.0f) && shininess > 0.0f ||
                HasTexture(UniformCache::LegacyTextureUniforms::DiffuseTexture) &&
                HasTexture(UniformCache::LegacyTextureUniforms::SpecularTexture) &&
                HasTexture(UniformCache::LegacyTextureUniforms::ShininessTexture);

            if (usePhong)
            {
                m_ShadingMethod = MaterialShadingMethod::Phong;
                return;
            }

            glm::vec3 emissiveColor = GetPropertyValue<glm::vec3>(UniformCache::SurfaceColorsUniforms::EmissiveColor);
            if (emissiveColor != glm::vec3(0.0f) && HasTexture(UniformCache::LegacyTextureUniforms::EmissiveTexture))
            {
                m_ShadingMethod = MaterialShadingMethod::Unlit;
                return;
            }

            MOTION_CORE_WARN("Material shading method could not be determined, defaulting to Unlit.");
            m_ShadingMethod = MaterialShadingMethod::Unlit;
        }
    }


}