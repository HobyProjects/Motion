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
        static constexpr const char* PositionLayout = "a_Position";
        static constexpr const char* TextureCoordsLayout = "a_TexCoords";
        static constexpr const char* NormalsLayout = "a_Normals";

        static constexpr const char* CameraProjection = "u_CameraProjectionMatrix";
        static constexpr const char* ModelMatrix = "u_ModelMatrix";

        // Material Color values (Legacy)
        static constexpr const char* DiffuseColor = "u_DiffuseColor";
        static constexpr const char* AmbientColor = "u_AmbientColor";
        static constexpr const char* SpecularColor = "u_SpecularColor";
        static constexpr const char* EmissiveColor = "u_EmissiveColor";
        static constexpr const char* TransparentColor = "u_TransparentColor";
        static constexpr const char* ReflectiveColor = "u_ReflectiveColor";
        
        // Material Color values (Modern (PBR))
        static constexpr const char* BaseColor = "u_BaseColor";
        static constexpr const char* EmissionColor = EmissiveColor;

        static constexpr const char* Shininess = "u_Shininess";
        static constexpr const char* ShininessStrenght = "u_ShininessStrenght";
        static constexpr const char* Opacity = "u_Opacity";
        static constexpr const char* ReflectiveIndex = "u_ReflectiveIndex";
        static constexpr const char* MetallicFactor = "u_MetallicFactor";
        static constexpr const char* RoughnessFactor = "u_RoughnessFactor";
        static constexpr const char* ClearcoatFactor = "u_ClearcoatFactor";
        static constexpr const char* SheenFactor = "u_SheenFactor";
        static constexpr const char* TransmissionFactor = "u_TransmissionFactor";
        static constexpr const char* AmbientOcclusion = "u_AmbientOcclusion";

        static constexpr const char* AmbientTexture = "u_AmbientTexture";
        static constexpr const char* DiffuseTexture = "u_DiffuseTexture";
        static constexpr const char* SpecularTexture = "u_SpecularTexture";
        static constexpr const char* NormalsTexture = "u_NormalsTexture";
        static constexpr const char* EmissiveTexture = "u_EmissiveTexture";
        static constexpr const char* ShininessTexture = "u_ShininessTexture";
        static constexpr const char* OpacityTexture = "u_OpacityTexture";

        static constexpr const char* BaseColorTexture = "u_BaseColorTexture";
        static constexpr const char* MetalnessTexture = "u_MetalnessTexture";
        static constexpr const char* DiffuseRoughnessTexture = "u_DiffuseRoughnessTexture";
        static constexpr const char* AmbientOcclusionTexture = "u_AmbientOcclusionTexture";
        static constexpr const char* EmissiveColorTexture = "u_EmissiveColorTexture";
        static constexpr const char* ClearCoatTexture = "u_ClearCoatTexture";
        static constexpr const char* SheenTexture = "u_SheenTexture";
        static constexpr const char* TransmissionTexture = "u_TransmissionTexture";

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

    template<typename TShader>
    requires std::derived_from<TShader, IShader>
    class ShaderContainer
    {
        public:
            ShaderContainer() = default;
            ~ShaderContainer() = default;

            void InsertShader(const std::string& name, const std::shared_ptr<TShader>& shader)
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
                }
            }

            std::shared_ptr<TShader> GetShader(const std::string& name) const
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
            std::unordered_map<std::string, std::shared_ptr<TShader>> m_Shaders;
    };
}

