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
    enum class MaterialShadingMethod
    {
        Auto,
        Phong,
        PBR,
        Unlit
    };

    class Material final : public AssetBase<IAsset>
    {
    public:
        Material() = default;
        Material(const UUID& uuid, const std::string& name);
        Material(const std::string& name);
        virtual ~Material() = default;

        void Bind(const std::shared_ptr<IShader>& shader) noexcept;
        void Unbind() const noexcept;

        void SetUniform(const std::string_view uniformName, float value);
        void SetUniform(const std::string_view uniformName, const glm::vec3& value);
        void SetUniform(const std::string_view uniformName, const glm::vec4& value);
        void SetTexture(const std::string_view uniformName, const std::shared_ptr<ITexture>& texture);
        void DetermineShadingMethod() noexcept;

        [[nodiscard]] MaterialShadingMethod GetShadingMethod() const noexcept { return m_ShadingMethod; }

    private:
        std::unordered_map<std::string_view, float> m_FloatParameters;
        std::unordered_map<std::string_view, glm::vec3> m_Vec3Parameters;
        std::unordered_map<std::string_view, glm::vec4> m_Vec4Parameters;
        std::unordered_map<std::string_view, std::shared_ptr<ITexture>> m_Textures;
        MaterialShadingMethod m_ShadingMethod{ MaterialShadingMethod::Auto };
    };
}