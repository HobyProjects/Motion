#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

namespace Motion::Core
{
    using ShaderID          = uint32_t;
    using ShaderProgramID   = uint32_t;
    using UniformLocation   = uint32_t;

    enum class ShaderType
    {
        None = 0,
        Vertex,
        Fragment,
        Geometry,
        Compute,
        TessellationControl,
        TessellationEvaluation
    };

    struct UniformCache
    {
        // Surface Colors
        static constexpr const char* AmbientColor = "u_AmbientColor";
        static constexpr const char* DiffuseColor = "u_DiffuseColor";
        static constexpr const char* SpecularColor = "u_SpecularColor";
        static constexpr const char* EmissiveColor = "u_EmissiveColor";
        static constexpr const char* TransparentColor = "u_TransparentColor";
        static constexpr const char* ReflectiveColor = "u_ReflectiveColor";

        // Material Properties
        static constexpr const char* Shininess = "u_Shininess";
        static constexpr const char* ShininessStrenght = "u_ShininessStrenght";
        static constexpr const char* Opacity = "u_Opacity";
        static constexpr const char* IndexOfRefraction = "u_RefractionIndex";
        static constexpr const char* BumpScaling = "u_BumpScaling";
        static constexpr const char* Reflectivity = "u_Reflectivity";

        // Material Factors
        static constexpr const char* BaseColor = "u_BaseColor";
        static constexpr const char* MetallicFactor = "u_MetallicFactor";
        static constexpr const char* RoughnessFactor = "u_RoughnessFactor";
        static constexpr const char* TransmissionFactor = "u_TransmissionFactor";
        static constexpr const char* ClearCoatFactor = "u_ClearCoatFactor";
        static constexpr const char* ClearCoatRoughnessFactor = "u_ClearCoatRoughnessFactor";
        static constexpr const char* SheenFactor = "u_SheenFactor";
        static constexpr const char* SheenRoughnessFactor = "u_SheenRoughnessFactor";
        static constexpr const char* AmbientOcclusionFactor = "u_AmbientOcclusion";


        // Texture Type (Legacy)
        static constexpr const char* DiffuseTexture = "u_DiffuseTexture";
        static constexpr const char* AmbientTexture = "u_AmbientTexture";
        static constexpr const char* SpecularTexture = "u_SpecularTexture";
        static constexpr const char* EmissiveTexture = "u_EmissiveTexture";
        static constexpr const char* NormalMapsTexture = "u_NormalMapsTexture";
        static constexpr const char* HightMapsTexture = "u_HightMapsTexture";
        static constexpr const char* ShininessTexture = "u_ShininessTexture";
        static constexpr const char* OpacityMapsTexture = "u_OpacityTexture";
        static constexpr const char* LightMapsTexture = "u_LightTexture";

        // Texture Type (PBR)
        static constexpr const char* BaseColorMapsTexture = "u_BaseColorMapsTexture";
        static constexpr const char* MetallicMapsTexture = "u_MetalnessMapsTexture";
        static constexpr const char* RoughnessMapsTexture = "u_RoughnessMapsTexture";
        static constexpr const char* AOMapTexture = "u_AmbientOcclusionMapsTexture";
        static constexpr const char* EmissiveMapsTexture = "u_EmissiveMapsTexture";
        static constexpr const char* ClearCoatMapsTexture = "u_ClearCoatMapsTexture";
        static constexpr const char* SheenMapsTexture = "u_SheenMapsTexture";
        static constexpr const char* TransmissionMapsTexture = "u_TransmissionMapsTexture";

        UniformCache() = default;
        ~UniformCache() = default;

    };

    class IShader
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

    class ShaderBuilder
    {
        private:
            ShaderBuilder() = default;
            ~ShaderBuilder() = default;

            ShaderBuilder(const ShaderBuilder&) = delete;
            ShaderBuilder& operator=(const ShaderBuilder&) = delete;
            ShaderBuilder(ShaderBuilder&&) = delete;
            ShaderBuilder& operator=(ShaderBuilder&&) = delete;

        public:
            static std::shared_ptr<IShader> CreateShader(const std::string& name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
            static void DestroyShader(const std::string& name);
            static std::shared_ptr<IShader> GetShader(const std::string& name);

            static ShaderProgramID CreateShaderProgram();
            static void AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID);
            static ShaderID CompileShader(ShaderType shaderType, const std::string& sourceCode);
            static void LinkShaderProgram(ShaderProgramID programID);
            static void ValidateShaderProgram(ShaderProgramID programID);
            static void DeleteShaderProgram(ShaderProgramID programID);
            static std::string ReadShaderFiles(const std::filesystem::path& filePath);
    };

    template<typename T>
    requires std::derived_from<T, IShader>
    class ShaderContainer
    {
        public:
            ShaderContainer() = default;
            ~ShaderContainer() = default;

            void InsertShader(const std::string& name, const std::shared_ptr<T>& shader)
            {
                if (m_Shaders.find(name) != m_Shaders.end())
                {
                    MOTION_ASSERT(false, "Shader with name {0} already exists!", name);
                    return;
                }
                
                m_Shaders[name] = shader;
            }

            void RemoveShader(const std::string& name)
            {
                auto it = m_Shaders.find(name);
                if (it != m_Shaders.end())
                {
                    m_Shaders.erase(it);
                }
                else
                {
                    MOTION_ASSERT(false, "Shader with name {0} does not exist!", name);
                    return;
                }
            }

            std::shared_ptr<T> GetShader(const std::string& name) const
            {
                auto it = m_Shaders.find(name);
                if (it != m_Shaders.end())
                {
                    return it->second;
                }
                else
                {
                    MOTION_ASSERT(false, "Shader with name {0} does not exist!", name);
                    return nullptr;
                }
            }

            void Clear()
            {
                m_Shaders.clear();
            }

        private:
            std::unordered_map<std::string, std::shared_ptr<T>> m_Shaders;
    };
}

