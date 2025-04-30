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

