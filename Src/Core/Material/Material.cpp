#include "CorePCH.hpp"
#include "Material.hpp"

namespace Motion::Core
{
    /**
     * @brief Initializes the static fallback textures for materials.
     *
     * This function creates and assigns default textures used as fallbacks for materials.
     * These textures are typically used when no specific texture is provided for a material.
     */
    std::shared_ptr<ITexture> MaterialFallbackTextures::White = nullptr;

    /**
     * @brief Initializes the static fallback textures for materials.
     *
     * This function creates and assigns default textures used as fallbacks for materials.
     * These textures are typically used when no specific texture is provided for a material.
     */
    std::shared_ptr<ITexture> MaterialFallbackTextures::Black = nullptr;

    /**
     * @brief Initializes the static fallback textures for materials.
     *
     * This function creates and assigns default textures used as fallbacks for materials.
     * These textures are typically used when no specific texture is provided for a material.
     */
    std::shared_ptr<ITexture> MaterialFallbackTextures::Grey = nullptr;

    /**
     * @brief Initializes the static fallback normal texture for materials.
     *
     * This function creates and assigns a default normal texture used as a fallback for materials.
     * The normal texture is typically used to provide surface detail without additional geometry.
     */
    std::shared_ptr<ITexture> MaterialFallbackTextures::Normal = nullptr;

    /**
     * @brief Initializes the static fallback textures for materials.
     *
     * This function creates and assigns default textures used as fallbacks for materials.
     * These textures are typically used when no specific texture is provided for a material.
     */
    void MaterialFallbackTextures::Initialize()
    {
        White = GL_CreateUnregisteredPlainTexture(10, 10, { 1.0f, 1.0f, 1.0f });
        Black = GL_CreateUnregisteredPlainTexture(10, 10, { 0.0f, 0.0f, 0.0f });
        Grey = GL_CreateUnregisteredPlainTexture(10, 10, { 0.8f, 0.8f, 0.8f });
        Normal = GL_CreateUnregisteredPlainTexture(10, 10, { 0.5f, 0.5f, 1.0f });
    }

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
        AssetInfo.IsInitialized = true;
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
        if (shader->IsInitialized())
        {
            if (m_ShadingMethod == MaterialShadingMethod::Auto)
            {
                DetermineShadingMethod();
            }

            if (m_ShadingMethod & MaterialShadingMethod::PBR)
            {
                shader->SetUniform(UniformCache::Factor_BaseColorFactor, m_Vec4Parameters[UniformCache::Factor_BaseColorFactor]);
                shader->SetUniform(UniformCache::Factor_MetallicFactor, m_FloatParameters[UniformCache::Factor_MetallicFactor]);
                shader->SetUniform(UniformCache::Factor_RoughnessFactor, m_FloatParameters[UniformCache::Factor_RoughnessFactor]);
                shader->SetUniform(UniformCache::Factor_AmbientOcclusionFactor, m_FloatParameters[UniformCache::Factor_AmbientOcclusionFactor]);
                shader->SetUniform(UniformCache::Factor_TransmissionFactor, m_FloatParameters[UniformCache::Factor_TransmissionFactor]);
                shader->SetUniform(UniformCache::Factor_ClearCoatFactor, m_FloatParameters[UniformCache::Factor_ClearCoatFactor]);
                shader->SetUniform(UniformCache::Factor_ClearCoatRoughnessFactor, m_FloatParameters[UniformCache::Factor_ClearCoatRoughnessFactor]);
                shader->SetUniform(UniformCache::Factor_SheenFactor, m_FloatParameters[UniformCache::Factor_SheenFactor]);
                shader->SetUniform(UniformCache::Factor_SheenRoughnessFactor, m_FloatParameters[UniformCache::Factor_SheenRoughnessFactor]);
                shader->SetUniform(UniformCache::Factor_IndexOfRefraction, m_FloatParameters[UniformCache::Factor_IndexOfRefraction]);

                Renderer::BindTextureUnit(0, m_Textures[UniformCache::Texture_BaseColorTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_BaseColorTexture, 0);

                Renderer::BindTextureUnit(1, m_Textures[UniformCache::Texture_MetallicTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_MetallicTexture, 1);

                Renderer::BindTextureUnit(2, m_Textures[UniformCache::Texture_RoughnessTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_RoughnessTexture, 2);

                Renderer::BindTextureUnit(3, m_Textures[UniformCache::Texture_AmbientOcclusionTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_AmbientOcclusionTexture, 3);

                Renderer::BindTextureUnit(3, m_Textures[UniformCache::Texture_NormalMapTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_NormalMapTexture, 3);

                Renderer::BindTextureUnit(4, m_Textures[UniformCache::Texture_EmissiveTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_EmissiveTexture, 4);

                Renderer::BindTextureUnit(5, m_Textures[UniformCache::Texture_ClearCoatTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_ClearCoatTexture, 5);

                Renderer::BindTextureUnit(6, m_Textures[UniformCache::Texture_SheenTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_SheenTexture, 6);

                Renderer::BindTextureUnit(7, m_Textures[UniformCache::Texture_TransmissionTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_TransmissionTexture, 7);
            }

            if (m_ShadingMethod & MaterialShadingMethod::Phong)
            {
                shader->SetUniform(UniformCache::Color_AmbientColor, m_Vec3Parameters[UniformCache::Color_AmbientColor]);
                shader->SetUniform(UniformCache::Color_DiffuseColor, m_Vec3Parameters[UniformCache::Color_DiffuseColor]);
                shader->SetUniform(UniformCache::Color_SpecularColor, m_Vec3Parameters[UniformCache::Color_SpecularColor]);
                shader->SetUniform(UniformCache::Color_EmissiveColor, m_Vec3Parameters[UniformCache::Color_EmissiveColor]);

                shader->SetUniform(UniformCache::Property_Shininess, m_FloatParameters[UniformCache::Property_Shininess]);
                shader->SetUniform(UniformCache::Property_ShininessStrength, m_FloatParameters[UniformCache::Property_ShininessStrength]);
                shader->SetUniform(UniformCache::Property_Opacity, m_FloatParameters[UniformCache::Property_Opacity]);
                shader->SetUniform(UniformCache::Property_Reflectivity, m_FloatParameters[UniformCache::Property_Reflectivity]);

                Renderer::BindTextureUnit(0, m_Textures[UniformCache::Texture_AmbientTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_AmbientTexture, 0);

                Renderer::BindTextureUnit(1, m_Textures[UniformCache::Texture_DiffuseTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_DiffuseTexture, 1);

                Renderer::BindTextureUnit(2, m_Textures[UniformCache::Texture_SpecularTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_SpecularTexture, 2);

                Renderer::BindTextureUnit(3, m_Textures[UniformCache::Texture_ShininessTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_ShininessTexture, 3);

                Renderer::BindTextureUnit(4, m_Textures[UniformCache::Texture_EmissiveTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_EmissiveTexture, 4);

                Renderer::BindTextureUnit(5, m_Textures[UniformCache::Texture_NormalMapTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_NormalMapTexture, 5);

                Renderer::BindTextureUnit(6, m_Textures[UniformCache::Texture_OpacityTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_OpacityTexture, 6);
            }

            if (m_ShadingMethod & MaterialShadingMethod::Unlit)
            {
                shader->SetUniform(UniformCache::Color_EmissiveColor, m_Vec3Parameters[UniformCache::Color_EmissiveColor]);

                Renderer::BindTextureUnit(0, m_Textures[UniformCache::Texture_EmissiveTexture]->GetID());
                shader->SetUniform(UniformCache::Texture_EmissiveTexture, 0);
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
        if (m_ShadingMethod & MaterialShadingMethod::PBR)
        {
            Renderer::UnbindTextureUnit(0);
            Renderer::UnbindTextureUnit(1);
            Renderer::UnbindTextureUnit(2);
            Renderer::UnbindTextureUnit(3);
            Renderer::UnbindTextureUnit(4);
            Renderer::UnbindTextureUnit(5);
            Renderer::UnbindTextureUnit(6);
            Renderer::UnbindTextureUnit(7);
        }

        if (m_ShadingMethod & MaterialShadingMethod::Phong)
        {
            Renderer::UnbindTextureUnit(0);
            Renderer::UnbindTextureUnit(1);
            Renderer::UnbindTextureUnit(2);
            Renderer::UnbindTextureUnit(3);
            Renderer::UnbindTextureUnit(4);
            Renderer::UnbindTextureUnit(5);
            Renderer::UnbindTextureUnit(6);
        }

        if (m_ShadingMethod & MaterialShadingMethod::Unlit)
        {
            Renderer::UnbindTextureUnit(0);
        }
    }

    /**
     * @brief Checks if the given texture is one of the default fallback textures.
     *
     * This function compares the provided texture with predefined fallback textures
     * (White, Black, Grey, Normal) and returns true if it matches any of them.
     *
     * @param texture The MaterialTexture to check against the default textures.
     * @return true if the texture is a default fallback texture, false otherwise.
     */
    static bool IsDefaultTexture(const MaterialTexture& texture)
    {
        return texture == MaterialFallbackTextures::White ||
            texture == MaterialFallbackTextures::Black ||
            texture == MaterialFallbackTextures::Grey ||
            texture == MaterialFallbackTextures::Normal;
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
                m_Textures.contains(UniformCache::Texture_AmbientOcclusionTexture);

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

            MOTION_CORE_WARN("Material shading method could not be determined, defaulting to Unlit.");
            m_ShadingMethod = MaterialShadingMethod::Unlit;
        }
    }

    /**
     * @brief Sets up texture parameters for the material based on the textures associated with it.
     *
     * This function iterates through all textures associated with the material and sets default
     * values for various uniform parameters if the texture is a default texture. It ensures that
     * the material has sensible defaults for rendering, even when specific textures are not provided.
     *
     * @note This method is noexcept and does not throw exceptions.
     */
    void Material::SetupTextureParameters() noexcept
    {
        for (auto& [uniformName, texture] : m_Textures)
        {
            if (IsDefaultTexture(texture))
            {
                if (uniformName == UniformCache::Texture_BaseColorTexture)
                    SetUniform(UniformCache::Factor_BaseColorFactor, glm::vec3(1.0f));

                if (uniformName == UniformCache::Texture_MetallicTexture)
                    SetUniform(UniformCache::Factor_MetallicFactor, 0.0f);

                if (uniformName == UniformCache::Texture_RoughnessTexture)
                    SetUniform(UniformCache::Factor_RoughnessFactor, 0.8f);

                if (uniformName == UniformCache::Texture_EmissiveTexture)
                    SetUniform(UniformCache::Color_EmissiveColor, glm::vec3(0.0f));

                if (uniformName == UniformCache::Texture_AmbientOcclusionTexture)
                    SetUniform(UniformCache::Factor_AmbientOcclusionFactor, 1.0f);

                if (uniformName == UniformCache::Texture_ClearCoatTexture)
                {
                    SetUniform(UniformCache::Factor_ClearCoatFactor, 0.0f);
                    SetUniform(UniformCache::Factor_ClearCoatRoughnessFactor, 0.1f);
                }

                if (uniformName == UniformCache::Texture_SheenTexture)
                {
                    SetUniform(UniformCache::Factor_SheenFactor, 0.0f);
                    SetUniform(UniformCache::Factor_SheenRoughnessFactor, 0.3f);
                }

                if (uniformName == UniformCache::Texture_TransmissionTexture)
                {
                    SetUniform(UniformCache::Factor_TransmissionFactor, 0.0f);
                    SetUniform(UniformCache::Property_IndexOfRefraction, 1.5f);
                }

                if (uniformName == UniformCache::Texture_DiffuseTexture)
                    SetUniform(UniformCache::Color_DiffuseColor, glm::vec3(0.8f));

                if (uniformName == UniformCache::Texture_SpecularTexture)
                    SetUniform(UniformCache::Color_SpecularColor, glm::vec3(0.5f));

                if (uniformName == UniformCache::Texture_ShininessTexture)
                    SetUniform(UniformCache::Property_Shininess, 32.0f);

                if (uniformName == UniformCache::Texture_EmissiveTexture)
                    SetUniform(UniformCache::Color_EmissiveColor, glm::vec3(0.0f));

                if (uniformName == UniformCache::Texture_OpacityTexture)
                    SetUniform(UniformCache::Property_Opacity, 1.0f);

                if (uniformName == UniformCache::Texture_AmbientTexture)
                    SetUniform(UniformCache::Color_AmbientColor, glm::vec3(0.0f));
            }
        }
    }



}


