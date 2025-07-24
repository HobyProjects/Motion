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

namespace Motion
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
        inline static constexpr std::string_view Position = "a_Position";
        inline static constexpr std::string_view TexCoords = "a_TexCoords";
        inline static constexpr std::string_view Normals = "a_Normals";
        inline static constexpr std::string_view Tangents = "a_Tangents";
        inline static constexpr std::string_view Bitangents = "a_Bitangents";

        inline static constexpr std::string_view ViewProjMatrix = "u_ViewProjMatrix";
        inline static constexpr std::string_view ModelMatrix = "u_ModelMatrix";
        inline static constexpr std::string_view CameraPosition = "u_CameraPosition";
        inline static constexpr std::string_view LightPosition = "u_LightPosition";
        inline static constexpr std::string_view LightColor = "u_LightColor";
        inline static constexpr std::string_view LightIntensity = "u_LightIntensity";

        inline static constexpr std::string_view BaseColorTextures = "u_BaseColorTextures";
        inline static constexpr std::string_view MetallicTextures = "u_MetallicTextures";
        inline static constexpr std::string_view RoughnessTextures = "u_RoughnessTextures";
        inline static constexpr std::string_view AmbientOcclusionTextures = "u_AmbientOcclusionTextures";
        inline static constexpr std::string_view NormalTextures = "u_NormalTextures";
        inline static constexpr std::string_view OpacityTextures = "u_OpacityTextures";
        inline static constexpr std::string_view BlendMaskTextures = "u_BlendMasks";


        inline static constexpr std::string_view ClearCoatTextures = "u_ClearCoatTextures";
        inline static constexpr std::string_view SheenTextures = "u_SheenTextures";
        inline static constexpr std::string_view TransmissionTextures = "u_TransmissionTextures";

        inline static constexpr std::string_view EmissiveColor = "u_EmissiveColor";
        inline static constexpr std::string_view EmissiveTexture = "u_EmissiveTexture";
        inline static constexpr std::string_view EnvironmentTexture = "u_EnvironmentTexture";

        inline static constexpr std::string_view Texture_PostProcessTexture = "u_PostProcessTexture";
        inline static constexpr std::string_view Texture_SkyboxTexture = "u_SkyboxTexture";

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

