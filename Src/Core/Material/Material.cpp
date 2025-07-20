#include "CorePCH.hpp"

namespace Motion::Core
{
    /**
     * @brief Constructs a Material object with the specified UUID, name, and shading method.
     *
     * @param uuid The universally unique identifier for the material.
     * @param name The name of the material.
     * @param shadingMethod The shading method used by the material.
     */
    Material::Material(const UUID& uuid, const std::string& name) :
        AssetBase(uuid, name, AssetType::Material, "Undefined")
    {
        AssetInfo.IsAssetInitialized = true;
        m_ShadingMethod = MaterialShadingMethod::Auto;
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
    Material::Material(const std::string& name) :
        AssetBase(UniqueIdentity::GetUniqueID(), name, AssetType::Material, "Undefined")
    {
        AssetInfo.IsAssetInitialized = true;
        m_ShadingMethod = MaterialShadingMethod::Auto;
    }

    /**
     * @brief Sets the value of a float uniform parameter for the material.
     *
     * This function assigns the given float value to the uniform parameter specified by its name.
     * If the parameter already exists, its value is updated; otherwise, a new parameter is created.
     *
     * @param uniformName The name of the uniform parameter to set.
     * @param value The float value to assign to the uniform parameter.
     */
    void Material::SetUniform(const std::string_view uniformName, float value)
    {
        m_FloatParameters[uniformName] = value;
    }

    /**
     * @brief Sets a vec3 uniform parameter for the material.
     *
     * Stores the given glm::vec3 value in the material's uniform parameter map,
     * associated with the specified uniform name. If the uniform already exists,
     * its value will be updated.
     *
     * @param uniformName The name of the uniform parameter to set.
     * @param value The glm::vec3 value to assign to the uniform parameter.
     */
    void Material::SetUniform(const std::string_view uniformName, const glm::vec3& value)
    {
        m_Vec3Parameters[uniformName] = value;
    }

    /**
     * @brief Sets a vec4 uniform parameter for the material.
     *
     * Updates or adds a vec4 parameter associated with the given uniform name.
     *
     * @param uniformName The name of the uniform parameter to set.
     * @param value The glm::vec4 value to assign to the uniform parameter.
     */
    void Material::SetUniform(const std::string_view uniformName, const glm::vec4& value)
    {
        m_Vec4Parameters[uniformName] = value;
    }

    /**
     * @brief Associates a texture with a specified uniform name in the material.
     *
     * This function sets or updates the texture corresponding to the given uniform name.
     * The texture is stored internally and can be used during rendering to bind the appropriate
     * texture to the shader uniform.
     *
     * @param uniformName The name of the shader uniform to associate with the texture.
     * @param texture A shared pointer to the texture object to be set.
     */
    void Material::SetTexture(const std::string_view uniformName, const std::shared_ptr<ITexture>& texture)
    {
        m_Textures[uniformName] = texture;
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
    void Material::Bind(const std::shared_ptr<IShader>& shader) noexcept
    {
        if (shader->IsAssetInitialized())
        {
            for (const auto& [uniformName, value] : m_FloatParameters)
            {
                shader->SetUniform(uniformName, value);
            }

            for (const auto& [uniformName, value] : m_Vec3Parameters)
            {
                shader->SetUniform(uniformName, value);
            }

            for (const auto& [uniformName, value] : m_Vec4Parameters)
            {
                shader->SetUniform(uniformName, value);
            }

            if (!m_Textures.empty())
            {
                if (!m_Textures.size() > Renderer::GetMaxTextureSlots())
                {
                    std::uint32_t samples[32] = { 0,  1,  2,  3,  4,  5,  6,  7,
                                                   8,  9, 10, 11, 12, 13, 14, 15,
                                                  16, 17, 18, 19, 20, 21, 22, 23,
                                                  24, 25, 26, 27, 28, 29, 30, 31 };

                    shader->SetUniform(UniformCache::GlobalAttri_Sampler2DArray, 32, samples);

                    std::uint32_t slot = 0;
                    for (const auto& [uniformName, texture] : m_Textures)
                    {
                        if (texture->IsAssetInitialized())
                        {
                            texture->Bind(slot++);
                        }
                        else
                        {
                            MOTION_CORE_ERROR("Texture {0} is not initialized in material {1}", uniformName, GetName());
                        }
                    }
                }
                else
                {
                    MOTION_CORE_ERROR("Too many textures bound to the material {0}", GetName());
                    return;
                }
            }
            else
            {
                MOTION_CORE_ERROR("No textures bound to the material {0}", GetName());
                return;
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
        for (const auto& [uniformName, texture] : m_Textures)
        {
            if (texture->IsAssetInitialized())
            {
                texture->Unbind();
            }
        }
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
    void Material::DetermineShadingMethod() noexcept
    {
        if (m_ShadingMethod == MaterialShadingMethod::Auto)
        {
            glm::vec3 baseColor = m_Vec3Parameters[UniformCache::Factor_BaseColorFactor];
            float metallicFactor = m_FloatParameters[UniformCache::Factor_MetallicFactor];
            float roughnessFactor = m_FloatParameters[UniformCache::Factor_RoughnessFactor];

            bool usePBR = baseColor != glm::vec3(0.0f) && metallicFactor > 0.0f && roughnessFactor < 1.0f &&
                m_Textures.contains(UniformCache::Texture_BaseColorTexture) &&
                m_Textures.contains(UniformCache::Texture_MetallicTexture) &&
                m_Textures.contains(UniformCache::Texture_RoughnessTexture) &&
                m_Textures.contains(UniformCache::Texture_AOMapTexture);

            if (usePBR)
            {
                m_ShadingMethod = MaterialShadingMethod::PBR;
                return;
            }

            glm::vec3 diffuseColor = m_Vec3Parameters[UniformCache::Color_DiffuseColor];
            glm::vec3 specularColor = m_Vec3Parameters[UniformCache::Color_SpecularColor];
            float shininess = m_FloatParameters[UniformCache::Property_Shininess];

            bool usePhong = diffuseColor != glm::vec3(0.0f) && specularColor != glm::vec3(0.0f) && shininess > 0.0f ||
                m_Textures.contains(UniformCache::Texture_DiffuseTexture) &&
                m_Textures.contains(UniformCache::Texture_SpecularTexture) &&
                m_Textures.contains(UniformCache::Texture_ShininessTexture);

            if (usePhong)
            {
                m_ShadingMethod = MaterialShadingMethod::Phong;
                return;
            }

            glm::vec3 emissiveColor = m_Vec3Parameters[UniformCache::Color_EmissiveColor];
            if (emissiveColor != glm::vec3(0.0f) && m_Textures.contains(UniformCache::Texture_EmissiveTexture))
            {
                m_ShadingMethod = MaterialShadingMethod::Unlit;
                return;
            }

            MOTION_CORE_WARN("Material shading method could not be determined, defaulting to Unlit.");
            m_ShadingMethod = MaterialShadingMethod::Unlit;
        }
    }
}