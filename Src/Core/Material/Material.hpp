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

namespace Motion
{
    using MaterialTexture = std::shared_ptr<ITexture>;

    enum class MaterialShadingMethod : std::uint8_t
    {
        Auto,
        Phong,
        PBR,
        Unlit
    };

    inline std::uint8_t operator|(MaterialShadingMethod lhs, MaterialShadingMethod rhs) { return static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs); }
    inline std::uint8_t operator&(MaterialShadingMethod lhs, MaterialShadingMethod rhs) { return static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs); }

    struct StandardMaterialConfig
    {
        // Surface Colors (legacy/compatibility with older formats)
        static constexpr glm::vec3 AmbientColor = { 0.0f, 0.0f, 0.0f };         // Typically ignored in modern PBR
        static constexpr glm::vec3 DiffuseColor = { 0.8f, 0.8f, 0.8f };         // Neutral gray
        static constexpr glm::vec3 SpecularColor = { 0.5f, 0.5f, 0.5f };        // F0 reflectance for dielectrics
        static constexpr glm::vec3 EmissiveColor = { 0.0f, 0.0f, 0.0f };        // No emission by default
        static constexpr glm::vec3 ReflectiveColor = { 0.0f, 0.0f, 0.0f };
        static constexpr glm::vec3 TransparentColor = { 0.0f, 0.0f, 0.0f };

        // Material properties (legacy/Blinn-Phong)
        static constexpr float Shininess = 32.0f;                               // Moderate gloss
        static constexpr float ShininessStrength = 1.0f;
        static constexpr float Opacity = 1.0f;                                  // Fully opaque
        static constexpr float IndexOfRefraction = 1.5f;                        // Glass-like IOR
        static constexpr float Reflectivity = 0.0f;                             // Non-metallic
        static constexpr float BumpScaling = 1.0f;

        // PBR Factors
        static constexpr glm::vec3 BaseColorFactor = { 1.0f, 1.0f, 1.0f };      // White albedo
        static constexpr float MetallicFactor = 0.0f;                           // Non-metallic by default
        static constexpr float RoughnessFactor = 0.8f;                          // Rough (not glossy)
        static constexpr float TransmissionFactor = 0.0f;                       // Opaque
        static constexpr float ClearCoatFactor = 0.0f;                          // No clearcoat by default
        static constexpr float ClearCoatRoughnessFactor = 0.1f;                 // Slightly smooth clearcoat (if used)
        static constexpr float SheenFactor = 0.0f;                              // Disabled by default
        static constexpr float SheenRoughnessFactor = 0.3f;                     // Slightly blurred sheen (if used)
        static constexpr float AmbientOcclusionFactor = 1.0f;                   // Fully lit (no occlusion loss)
        static constexpr float IndexOfRefractionFactor = 1.5f;                  // Used in transmission/refraction models
    };

    enum class MaterialParameterType : std::uint8_t
    {
        Float,
        Vec3,
        Texture
    };

    struct MaterialFallbackTextures
    {
        static std::shared_ptr<ITexture> White;
        static std::shared_ptr<ITexture> Black;
        static std::shared_ptr<ITexture> Grey;
        static std::shared_ptr<ITexture> Normal;

        static void Initialize();
    };

    class Material final : public AssetBase<IAsset>
    {
    public:
        Material() = default;
        Material(const UUID& uuid, const std::string& name);
        virtual ~Material() = default;

        void Bind(const std::shared_ptr<IShader>& shader) noexcept;
        void Unbind() const noexcept;

        void SetUniform(const std::string_view uniformName, float value);
        void SetUniform(const std::string_view uniformName, const glm::vec3& value);
        void SetUniform(const std::string_view uniformName, const glm::vec4& value);
        void SetTexture(const std::string_view uniformName, const std::shared_ptr<ITexture>& texture);
        void DetermineShadingMethod() noexcept;
        void SetupTextureParameters() noexcept;

        [[nodiscard]] const std::unordered_map<std::string_view, float>& GetFloatParameters() const noexcept { return m_FloatParameters; }
        [[nodiscard]] const std::unordered_map<std::string_view, glm::vec3>& GetVec3Parameters() const noexcept { return m_Vec3Parameters; }
        [[nodiscard]] const std::unordered_map<std::string_view, glm::vec4>& GetVec4Parameters() const noexcept { return m_Vec4Parameters; }
        [[nodiscard]] const std::unordered_map<std::string_view, MaterialTexture>& GetTextures() const noexcept { return m_Textures; }
        [[nodiscard]] MaterialShadingMethod GetShadingMethod() const noexcept { return m_ShadingMethod; }

    private:
        std::unordered_map<std::string_view, float> m_FloatParameters;
        std::unordered_map<std::string_view, glm::vec3> m_Vec3Parameters;
        std::unordered_map<std::string_view, glm::vec4> m_Vec4Parameters;
        std::unordered_map<std::string_view, MaterialTexture> m_Textures;
        MaterialShadingMethod m_ShadingMethod{ MaterialShadingMethod::Auto };
    };
}