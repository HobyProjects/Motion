#pragma once

#include "Shaders.hpp"

namespace Motion::Core
{
    // Forward declarations
    class GL_ShaderBuilder;

    class GL_Shader final : public IShader
    {
        public:
            GL_Shader(const std::string& name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
            virtual ~GL_Shader() override;

            virtual void Bind() const override;
            virtual void Unbind() const override;

            virtual ShaderProgramID ProgramID() const override { return m_ProgramID; }
            virtual std::string GetName() const override { return m_Name; }
            virtual UniformLocation GetUniformLocation(const std::string& uniformName) const override;
            
            virtual void SetUniform(const std::string& uniformName, float value) override;
            virtual void SetUniform(const std::string& uniformName, int32_t value) override;
            virtual void SetUniform(const std::string& uniformName, uint32_t value) override;
            virtual void SetUniform(const std::string& uniformName, const glm::vec2& value) override;
            virtual void SetUniform(const std::string& uniformName, const glm::vec3& value) override;
            virtual void SetUniform(const std::string& uniformName, const glm::vec4& value) override;
            virtual void SetUniform(const std::string& uniformName, const glm::mat2& value) override;
            virtual void SetUniform(const std::string& uniformName, const glm::mat3& value) override;
            virtual void SetUniform(const std::string& uniformName, const glm::mat4& value) override;

        private:
            ShaderProgramID m_ProgramID{ 0 };
            ShaderType m_ShaderType{ ShaderType::None };
            mutable std::unordered_map<std::string, UniformLocation> m_UniformLocations;
            std::string m_Name{ "Default" };

            friend class GL_ShaderBuilder;
    };

    class GL_ShaderBuilder
    {
        private:
            GL_ShaderBuilder() = default;
            ~GL_ShaderBuilder() = default;

            GL_ShaderBuilder(const GL_ShaderBuilder&) = delete;
            GL_ShaderBuilder& operator=(const GL_ShaderBuilder&) = delete;
            GL_ShaderBuilder(GL_ShaderBuilder&&) = delete;
            GL_ShaderBuilder& operator=(GL_ShaderBuilder&&) = delete;

        public:
            static std::shared_ptr<GL_Shader> CreateShader(const std::string& name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
            static void DestroyShader(const std::string& name);
            static std::shared_ptr<GL_Shader> GetShader(const std::string& name);

            static ShaderProgramID CreateShaderProgram();
            static void AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID);
            static ShaderID CompileShader(ShaderType shaderType, const std::string& sourceCode);
            static void LinkShaderProgram(ShaderProgramID programID);
            static void ValidateShaderProgram(ShaderProgramID programID);
            static void DeleteShaderProgram(ShaderProgramID programID);
            static std::string ReadShaderFiles(const std::filesystem::path& filePath);
    };
}