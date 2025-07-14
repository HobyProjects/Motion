#pragma once

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>
#include <variant>

#include <glm/glm.hpp>

#include "Shaders.hpp"
#include "Texture.hpp"
#include "Asset.hpp"

namespace Motion::Core
{
    using MaterialParameters = std::variant<float, glm::vec3, glm::vec4>;

    enum class MaterialShadingMethod : std::uint32_t
    {
        Phong = 0,
        PBR,
        Unlit,
        Auto
    };

    struct MaterialTexturesBinding
    {
        std::string_view UniformName;
        std::shared_ptr<ITexture> Texture{ nullptr };

        MaterialTexturesBinding(const std::string_view& uniformName, std::shared_ptr<ITexture> texture)
            : UniformName(uniformName), Texture(std::move(texture)) {
        }
        ~MaterialTexturesBinding() = default;
    };

    struct MaterialPropertyBinding
    {
        std::string_view UniformName;
        MaterialParameters ParameterValue;

        MaterialPropertyBinding(const std::string_view& uniformName, const MaterialParameters& value)
            : UniformName(uniformName), ParameterValue(value) {
        }
        ~MaterialPropertyBinding() = default;
    };

    class Material final : public AssetBase<IAsset>
    {
    public:
        Material() = default;
        Material(const UUID& uuid, const std::string& name, MaterialShadingMethod shadingMethod = MaterialShadingMethod::Auto);
        Material(const std::string& name, MaterialShadingMethod shadingMethod = MaterialShadingMethod::Auto);
        virtual ~Material() = default;

        void Bind(const std::shared_ptr<IShader>& shader) const noexcept;
        void Unbind() const noexcept;

        void Set(std::string_view name, std::shared_ptr<ITexture> texture);
        void Set(std::string_view name, const MaterialParameters& value);

        [[nodiscard]] bool HasTexture(std::string_view name) const noexcept;
        [[nodiscard]] std::shared_ptr<ITexture> GetTexture(std::string_view name) const noexcept;

        [[nodiscard]] bool HasProperty(std::string_view name) const noexcept;
        [[nodiscard]] MaterialParameters GetProperty(std::string_view name) const noexcept;
        void DetermineShadingMethod() const noexcept;

        /**
         * @brief Retrieves the value of a material property by name and type.
         *
         * This templated function attempts to fetch the value of a property with the specified name,
         * casting it to the requested type T. If the property exists and its type matches T, the value is returned.
         * Otherwise, an error is logged and a default-constructed value of type T is returned.
         *
         * @tparam T The expected type of the property value.
         * @param name The name of the property to retrieve.
         * @return The value of the property if found and type matches; otherwise, a default-constructed T.
         *
         * @note If the property does not exist or the type does not match, an error is logged.
         * @note This function is noexcept and will not throw exceptions.
         */
        template<typename T>
        [[nodiscard]] T GetPropertyValue(std::string_view name) const noexcept
        {
            if (HasProperty(name))
            {
                const auto& binding = m_ParameterBindings.at(name);
                if (const auto* value = std::get_if<T>(&binding.ParameterValue))
                {
                    return *value;
                }
            }

            MOTION_CORE_ERROR("Material does not have a property with name: {}", name);
            return T(0.0f); // Return default value if not found or type mismatch
        }

    private:
        std::unordered_map<std::string_view, MaterialTexturesBinding> m_TextureBindings;
        std::unordered_map<std::string_view, MaterialPropertyBinding> m_ParameterBindings;
        mutable MaterialShadingMethod m_ShadingMethod{ MaterialShadingMethod::Auto };
    };
}