#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include <glm/glm.hpp>

#include "Asset.hpp"
#include "UUID.hpp"

namespace Motion::Core
{
    using ShaderID = uint32_t;
    using ShaderProgramID = uint32_t;
    using UniformLocation = uint32_t;
    using UniformVariant = std::variant<float, int32_t, uint32_t, glm::vec2, glm::vec3, glm::vec4, glm::mat2, glm::mat3, glm::mat4>;

    enum class ShaderType : uint32_t
    {
        None = 0,
        Vertex,
        Fragment,
        Geometry,
        Compute,
        TessellationControl,
        TessellationEvaluation
    };

    inline uint32_t operator|(ShaderType a, ShaderType b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(ShaderType a, ShaderType b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }

    struct UniformCache
    {
        struct ModelUniforms
        {
            inline static constexpr std::string_view ModelMatrix = "u_ModelMatrix";
            inline static constexpr std::string_view ViewProjMatrix = "u_ViewProjMatrix";
        };

        //Light
        struct LightUniforms
        {
            inline static constexpr std::string_view LightPosition = "u_LightPosition";
            inline static constexpr std::string_view LightColor = "u_LightColor";
            inline static constexpr std::string_view LightIntensity = "u_LightIntensity";
        };

        // Surface Colors
        struct SurfaceColorsUniforms
        {
            inline static constexpr std::string_view AmbientColor = "u_AmbientColor";
            inline static constexpr std::string_view DiffuseColor = "u_DiffuseColor";
            inline static constexpr std::string_view SpecularColor = "u_SpecularColor";
            inline static constexpr std::string_view EmissiveColor = "u_EmissiveColor";
            inline static constexpr std::string_view TransparentColor = "u_TransparentColor";
            inline static constexpr std::string_view ReflectiveColor = "u_ReflectiveColor";
        };

        struct MaterialPropertiesUniforms
        {
            inline static constexpr std::string_view Shininess = "u_Shininess";
            inline static constexpr std::string_view ShininessStrength = "u_ShininessStrength";
            inline static constexpr std::string_view Opacity = "u_Opacity";
            inline static constexpr std::string_view IndexOfRefraction = "u_IndexOfRefraction";
            inline static constexpr std::string_view BumpScaling = "u_BumpScaling";
            inline static constexpr std::string_view Reflectivity = "u_Reflectivity";
        };

        struct MaterialFactorsUniforms
        {
            inline static constexpr std::string_view BaseColor = "u_BaseColor";
            inline static constexpr std::string_view MetallicFactor = "u_MetallicFactor";
            inline static constexpr std::string_view RoughnessFactor = "u_RoughnessFactor";
            inline static constexpr std::string_view TransmissionFactor = "u_TransmissionFactor";
            inline static constexpr std::string_view ClearCoatFactor = "u_ClearCoatFactor";
            inline static constexpr std::string_view ClearCoatRoughnessFactor = "u_ClearCoatRoughnessFactor";
            inline static constexpr std::string_view SheenFactor = "u_SheenFactor";
            inline static constexpr std::string_view SheenRoughnessFactor = "u_SheenRoughnessFactor";
            inline static constexpr std::string_view AmbientOcclusionFactor = "u_AmbientOcclusion";
            inline static constexpr std::string_view IndexOfRefraction = "u_IndexOfRefraction";
        };

        struct LegacyTextureUniforms
        {
            inline static constexpr std::string_view DiffuseTexture = "u_DiffuseTexture";
            inline static constexpr std::string_view AmbientTexture = "u_AmbientTexture";
            inline static constexpr std::string_view SpecularTexture = "u_SpecularTexture";
            inline static constexpr std::string_view EmissiveTexture = "u_EmissiveTexture";
            inline static constexpr std::string_view NormalMapsTexture = "u_NormalMapsTexture";
            inline static constexpr std::string_view HightMapsTexture = "u_HightMapsTexture";
            inline static constexpr std::string_view ShininessTexture = "u_ShininessTexture";
            inline static constexpr std::string_view OpacityMapsTexture = "u_OpacityTexture";
            inline static constexpr std::string_view LightMapsTexture = "u_LightTexture";
        };

        struct PBRTextureUniforms
        {
            inline static constexpr std::string_view BaseColorTexture = "u_BaseColorTexture";
            inline static constexpr std::string_view MetallicTexture = "u_MetallicTexture";
            inline static constexpr std::string_view RoughnessTexture = "u_RoughnessTexture";
            inline static constexpr std::string_view AOMapTexture = "u_AmbientOcclusionTexture";
            inline static constexpr std::string_view EmissiveTexture = "u_EmissiveTexture";
            inline static constexpr std::string_view ClearCoatTexture = "u_ClearCoatTexture";
            inline static constexpr std::string_view SheenTexture = "u_SheenTexture";
            inline static constexpr std::string_view TransmissionTexture = "u_TransmissionTexture";
        };

        UniformCache() = default;
        ~UniformCache() = default;

    };

    class IShader : public IAsset
    {
    public:
        IShader() = default;
        virtual ~IShader() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual ShaderProgramID ProgramID() const = 0;
        virtual std::string GetName() const = 0;
        virtual UniformLocation GetUniformLocation(const std::string& uniformName) const = 0;
        virtual bool InUse() const noexcept = 0;

        virtual void SetUniform(const std::string& uniformName, float value) = 0;
        virtual void SetUniform(const std::string& uniformName, std::int32_t value) = 0;
        virtual void SetUniform(const std::string& uniformName, std::uint32_t value) = 0;
        virtual void SetUniform(const std::string& uniformName, const glm::vec2& value) = 0;
        virtual void SetUniform(const std::string& uniformName, const glm::vec3& value) = 0;
        virtual void SetUniform(const std::string& uniformName, const glm::vec4& value) = 0;
        virtual void SetUniform(const std::string& uniformName, const glm::mat2& value) = 0;
        virtual void SetUniform(const std::string& uniformName, const glm::mat3& value) = 0;
        virtual void SetUniform(const std::string& uniformName, const glm::mat4& value) = 0;
    };

    class ShaderBuilder
    {
    public:
        ShaderBuilder() = default;
        ~ShaderBuilder() = default;

        static ShaderProgramID CreateShaderProgram();
        static void AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID);
        static ShaderID CompileShader(ShaderType shaderType, const std::string& sourceCode);
        static void LinkShaderProgram(ShaderProgramID programID);
        static void ValidateShaderProgram(ShaderProgramID programID);
        static void DeleteShaderProgram(ShaderProgramID programID);
        static std::string ReadShaderFile(const std::filesystem::path& filePath);
        static std::unordered_map<ShaderType, std::string> ReadFullShaderFile(const std::filesystem::path& filePath);
        static std::unordered_map<ShaderType, std::string> ReadShaderFiles(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
    };
}

