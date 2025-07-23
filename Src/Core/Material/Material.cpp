#include "CorePCH.hpp"
#include "Material.hpp"

namespace Motion
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
        AssetInfo.IsInitialized = true;
        m_ShadingMethod = MaterialShadingMethod::Auto;
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
    void Material::SetTexture(const std::string_view uniformName, const MaterialTexture& texture)
    {
        m_Textures.push_back({ uniformName, texture });
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
        if (m_ShadingMethod & MaterialShadingMethod::Auto)
        {
            DetermineShadingMethod();
            Bind(shader);
        }

        if (m_ShadingMethod & MaterialShadingMethod::Phong)
        {

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


