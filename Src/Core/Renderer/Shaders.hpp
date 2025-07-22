#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include <glm/glm.hpp>

#include "Asset.hpp"
#include "UUID.hpp"

#define INVALID_UNIFORM_LOCATION  -1

namespace Motion::Core
{
    using ShaderID = uint32_t;
    using ShaderProgramID = uint32_t;
    using UniformLocation = uint32_t;

    enum class ShaderType : uint32_t
    {
        None = 0,
        Vertex = 1,
        Fragment = 2,
        Geometry = 3,
        Compute = 4,
        TessellationControl = 5,
        TessellationEvaluation = 6
    };

    inline uint32_t operator|(ShaderType a, ShaderType b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(ShaderType a, ShaderType b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }

    enum class UniformType : std::uint32_t
    {
        None = 0,
        Float = 1,
        Int = 2,
        UInt = 3,
        Vec2 = 4,
        Vec3 = 5,
        Vec4 = 6,
        Mat2 = 7,
        Mat3 = 8,
        Mat4 = 9,
        Sampler2D = 10,
        Sampler2DArray = 11,
        SamplerCube = 12,
    };

    struct UniformInfomation
    {
        std::string_view UniformName;
        UniformType Type{ UniformType::None };
        UniformLocation Location{ 0 };
        bool IsValid{ false };

        UniformInfomation() = default;
        UniformInfomation(std::string_view name, UniformType type, UniformLocation location) :
            UniformName(name), Type(type), Location(location) {
            IsValid = (location != -1) ? true : false;
        }
        ~UniformInfomation() = default;
    };

    struct UniformCache
    {
        inline static constexpr std::string_view VertexAttri_Position = "a_Position";
        inline static constexpr std::string_view VertexAttri_TexCoords = "a_TexCoords";
        inline static constexpr std::string_view VertexAttri_Normals = "a_Normals";
        inline static constexpr std::string_view VertexAttri_Tangents = "a_Tangents";
        inline static constexpr std::string_view VertexAttri_Bitangents = "a_Bitangents";

        inline static constexpr std::string_view GlobalAttri_ModelMatrix = "u_ModelMatrix";
        inline static constexpr std::string_view GlobalAttri_ViewProjMatrix = "u_ViewProjMatrix";
        inline static constexpr std::string_view GlobalAttri_Sampler2DArray = "u_Sampler2DArray";
        inline static constexpr std::string_view GlobalAttri_CameraPosition = "u_CameraPosition";
        inline static constexpr std::string_view GlobalAttri_ViewMatrix = "u_ViewMatrix";
        inline static constexpr std::string_view GlobalAttri_ProjectionMatrix = "u_ProjectionMatrix";
        inline static constexpr std::string_view GlobalAttri_EnvironmentTexture = "u_EnvironmentTexture";

        inline static constexpr std::string_view LightAttri_Position = "u_LightPosition";
        inline static constexpr std::string_view LightAttri_Color = "u_LightColor";
        inline static constexpr std::string_view LightAttri_Intensity = "u_LightIntensity";

        inline static constexpr std::string_view Color_AmbientColor = "u_AmbientColor";
        inline static constexpr std::string_view Color_DiffuseColor = "u_DiffuseColor";
        inline static constexpr std::string_view Color_SpecularColor = "u_SpecularColor";
        inline static constexpr std::string_view Color_EmissiveColor = "u_EmissiveColor";
        inline static constexpr std::string_view Color_TransparentColor = "u_TransparentColor";
        inline static constexpr std::string_view Color_ReflectiveColor = "u_ReflectiveColor";

        inline static constexpr std::string_view Property_Shininess = "u_Shininess";
        inline static constexpr std::string_view Property_ShininessStrength = "u_ShininessStrength";
        inline static constexpr std::string_view Property_Opacity = "u_Opacity";
        inline static constexpr std::string_view Property_IndexOfRefraction = "u_IndexOfRefraction";
        inline static constexpr std::string_view Property_BumpScaling = "u_BumpScaling";
        inline static constexpr std::string_view Property_Reflectivity = "u_Reflectivity";

        inline static constexpr std::string_view Factor_BaseColorFactor = "u_BaseColorFactor";
        inline static constexpr std::string_view Factor_MetallicFactor = "u_MetallicFactor";
        inline static constexpr std::string_view Factor_RoughnessFactor = "u_RoughnessFactor";
        inline static constexpr std::string_view Factor_TransmissionFactor = "u_TransmissionFactor";
        inline static constexpr std::string_view Factor_ClearCoatFactor = "u_ClearCoatFactor";
        inline static constexpr std::string_view Factor_ClearCoatRoughnessFactor = "u_ClearCoatRoughnessFactor";
        inline static constexpr std::string_view Factor_SheenFactor = "u_SheenFactor";
        inline static constexpr std::string_view Factor_SheenRoughnessFactor = "u_SheenRoughnessFactor";
        inline static constexpr std::string_view Factor_AmbientOcclusionFactor = "u_AmbientOcclusion";
        inline static constexpr std::string_view Factor_IndexOfRefraction = "u_IndexOfRefraction";

        inline static constexpr std::string_view Texture_DiffuseTexture = "u_DiffuseTexture";
        inline static constexpr std::string_view Texture_AmbientTexture = "u_AmbientTexture";
        inline static constexpr std::string_view Texture_SpecularTexture = "u_SpecularTexture";
        inline static constexpr std::string_view Texture_EmissiveTexture = "u_EmissiveTexture";
        inline static constexpr std::string_view Texture_NormalMapTexture = "u_NormalMapsTexture";
        inline static constexpr std::string_view Texture_HightMapTexture = "u_HightMapsTexture";
        inline static constexpr std::string_view Texture_ShininessTexture = "u_ShininessTexture";
        inline static constexpr std::string_view Texture_OpacityTexture = "u_OpacityTexture";
        inline static constexpr std::string_view Texture_LightMapsTexture = "u_LightMapsTexture";
        inline static constexpr std::string_view Texture_BaseColorTexture = "u_BaseColorTexture";
        inline static constexpr std::string_view Texture_MetallicTexture = "u_MetallicTexture";
        inline static constexpr std::string_view Texture_RoughnessTexture = "u_RoughnessTexture";
        inline static constexpr std::string_view Texture_AmbientOcclusionTexture = "u_AmbientOcclusionTexture";
        inline static constexpr std::string_view Texture_ClearCoatTexture = "u_ClearCoatTexture";
        inline static constexpr std::string_view Texture_SheenTexture = "u_SheenTexture";
        inline static constexpr std::string_view Texture_TransmissionTexture = "u_TransmissionTexture";

        inline static constexpr std::string_view Texture_PostProcessTexture = "u_PostProcessTexture";
        inline static constexpr std::string_view Texture_SkyboxTexture = "u_SkyboxTexture";


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

        virtual void SetUniform(const std::string_view uniformName, float value) = 0;
        virtual void SetUniform(const std::string_view uniformName, std::int32_t value) = 0;
        virtual void SetUniform(const std::string_view uniformName, std::uint32_t value) = 0;
        virtual void SetUniform(const std::string_view uniformName, const glm::vec2& value) = 0;
        virtual void SetUniform(const std::string_view uniformName, const glm::vec3& value) = 0;
        virtual void SetUniform(const std::string_view uniformName, const glm::vec4& value) = 0;
        virtual void SetUniform(const std::string_view uniformName, const glm::mat2& value) = 0;
        virtual void SetUniform(const std::string_view uniformName, const glm::mat3& value) = 0;
        virtual void SetUniform(const std::string_view uniformName, const glm::mat4& value) = 0;
        virtual void SetUniform(const std::string_view uniformName, std::uint32_t size, std::uint32_t* values) = 0;
        virtual void ReflectUniforms() = 0;

        [[nodiscard]] virtual ShaderProgramID ProgramID() const = 0;
        [[nodiscard]] virtual std::string GetName() const = 0;
        [[nodiscard]] virtual UniformLocation GetUniformLocation(const std::string_view uniformName) = 0;
    };

    class ShaderCompiler
    {
    private:
        ShaderCompiler() = default;
        ~ShaderCompiler() = default;

        ShaderCompiler(const ShaderCompiler&) = delete;
        ShaderCompiler& operator=(const ShaderCompiler&) = delete;
        ShaderCompiler(ShaderCompiler&&) = delete;
        ShaderCompiler& operator=(ShaderCompiler&&) = delete;

    public:
        static void AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID);
        static void LinkShaderProgram(ShaderProgramID programID);
        static void ValidateShaderProgram(ShaderProgramID programID);
        static void DeleteShaderProgram(ShaderProgramID programID);

        [[nodiscard]] static ShaderID CompileShader(ShaderType shaderType, const std::string& sourceCode);
        [[nodiscard]] static std::string ReadShaderFile(const std::filesystem::path& filePath);
        [[nodiscard]] static std::unordered_map<ShaderType, std::string> ReadFullShaderFile(const std::filesystem::path& filePath);
        [[nodiscard]] static ShaderProgramID CreateShaderProgram();
        [[nodiscard]] static std::unordered_map<ShaderType, std::string> ReadShaderFiles(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
    };
}

