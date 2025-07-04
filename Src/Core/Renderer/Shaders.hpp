#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

#include "Asset.hpp"
#include "UUID.hpp"

namespace Motion::Core
{
    using ShaderID          = uint32_t;
    using ShaderProgramID   = uint32_t;
    using UniformLocation   = uint32_t;

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
    inline uint32_t operator^(ShaderType a, ShaderType b) { return static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b); }
    inline uint32_t operator~(ShaderType a) { return ~static_cast<uint32_t>(a); }

    struct UniformCache
    {
        //Model
        struct ModelUniforms
        {
            static constexpr const char* ModelMatrix = "u_ModelMatrix";
            static constexpr const char* CameraMatrix = "u_CameraMatrix";
        };

        // Surface Colors
        struct SurfaceColorsUniforms
        {
            static constexpr const char* AmbientColor = "u_AmbientColor";
            static constexpr const char* DiffuseColor = "u_DiffuseColor";
            static constexpr const char* SpecularColor = "u_SpecularColor";
            static constexpr const char* EmissiveColor = "u_EmissiveColor";
            static constexpr const char* TransparentColor = "u_TransparentColor";
            static constexpr const char* ReflectiveColor = "u_ReflectiveColor";
        };

        struct MaterialPropertiesUniforms
        {
            static constexpr const char* Shininess = "u_Shininess";
            static constexpr const char* ShininessStrength = "u_ShininessStrength";
            static constexpr const char* Opacity = "u_Opacity";
            static constexpr const char* IndexOfRefraction = "u_IndexOfRefraction";
            static constexpr const char* BumpScaling = "u_BumpScaling";
            static constexpr const char* Reflectivity = "u_Reflectivity";
        };

        struct MaterialFactorsUniforms
        {
            static constexpr const char* BaseColor = "u_BaseColor";
            static constexpr const char* MetallicFactor = "u_MetallicFactor";
            static constexpr const char* RoughnessFactor = "u_RoughnessFactor";
            static constexpr const char* TransmissionFactor = "u_TransmissionFactor";
            static constexpr const char* ClearCoatFactor = "u_ClearCoatFactor";
            static constexpr const char* ClearCoatRoughnessFactor = "u_ClearCoatRoughnessFactor";
            static constexpr const char* SheenFactor = "u_SheenFactor";
            static constexpr const char* SheenRoughnessFactor = "u_SheenRoughnessFactor";
            static constexpr const char* AmbientOcclusionFactor = "u_AmbientOcclusion";
            static constexpr const char* IndexOfRefraction = "u_IndexOfRefraction";
        };

        struct LegacyTextureUniforms
        {
            static constexpr const char* DiffuseTexture = "u_DiffuseTexture";
            static constexpr const char* AmbientTexture = "u_AmbientTexture";
            static constexpr const char* SpecularTexture = "u_SpecularTexture";
            static constexpr const char* EmissiveTexture = "u_EmissiveTexture";
            static constexpr const char* NormalMapsTexture = "u_NormalMapsTexture";
            static constexpr const char* HightMapsTexture = "u_HightMapsTexture";
            static constexpr const char* ShininessTexture = "u_ShininessTexture";
            static constexpr const char* OpacityMapsTexture = "u_OpacityTexture";
            static constexpr const char* LightMapsTexture = "u_LightTexture";
        };

        struct PBRTextureUniforms
        {
            static constexpr const char* BaseColorTexture = "u_BaseColorTexture";
            static constexpr const char* MetallicTexture = "u_MetallicTexture";
            static constexpr const char* RoughnessTexture = "u_RoughnessTexture";
            static constexpr const char* AOMapTexture = "u_AmbientOcclusionTexture";
            static constexpr const char* EmissiveTexture = "u_EmissiveTexture";
            static constexpr const char* ClearCoatTexture = "u_ClearCoatTexture";
            static constexpr const char* SheenTexture = "u_SheenTexture";
            static constexpr const char* TransmissionTexture = "u_TransmissionTexture";
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
            
            virtual void SetUniform(const std::string& uniformName, float value) = 0;
            virtual void SetUniform(const std::string& uniformName, int32_t value) = 0;
            virtual void SetUniform(const std::string& uniformName, uint32_t value) = 0;
            virtual void SetUniform(const std::string& uniformName, const glm::vec2& value) = 0;
            virtual void SetUniform(const std::string& uniformName, const glm::vec3& value) = 0;
            virtual void SetUniform(const std::string& uniformName, const glm::vec4& value) = 0;
            virtual void SetUniform(const std::string& uniformName, const glm::mat2& value) = 0;
            virtual void SetUniform(const std::string& uniformName, const glm::mat3& value) = 0;
            virtual void SetUniform(const std::string& uniformName, const glm::mat4& value) = 0;
    };

    class ShaderFactory
    {
        private:
            ShaderFactory() = default;
            ~ShaderFactory() = default;

            ShaderFactory(const ShaderFactory&) = delete;
            ShaderFactory& operator=(const ShaderFactory&) = delete;
            ShaderFactory(ShaderFactory&&) = delete;
            ShaderFactory& operator=(ShaderFactory&&) = delete;

        public:
            static ShaderProgramID CreateShaderProgram();
            static void AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID);
            static ShaderID CompileShader(ShaderType shaderType, const std::string& sourceCode);
            static void LinkShaderProgram(ShaderProgramID programID);
            static void ValidateShaderProgram(ShaderProgramID programID);
            static void DeleteShaderProgram(ShaderProgramID programID);
            static std::string ReadShaderFiles(const std::filesystem::path& filePath);
    };

    class ShaderManager
    {
        private:
            ShaderManager() = default;
            ~ShaderManager() = default;

            ShaderManager(const ShaderManager&) = delete;
            ShaderManager& operator=(const ShaderManager&) = delete;
            ShaderManager(ShaderManager&&) = delete;
            ShaderManager& operator=(ShaderManager&&) = delete;

        public:
            static void InsertShader(const UUID& uuid, const std::shared_ptr<IShader>& shader);
            static std::shared_ptr<IShader> GetShader(const UUID& uuid);
            static std::shared_ptr<IShader> GetShader(const std::string& name);
            static std::unordered_map<UUID, std::shared_ptr<IShader>>::const_iterator GetShaders();
    };
}

