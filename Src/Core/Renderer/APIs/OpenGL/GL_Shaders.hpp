#pragma once

#include "Shaders.hpp"

namespace Motion::Core
{
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
    };

    std::shared_ptr<GL_Shader> GL_CreateShader(const std::string& name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
    ShaderID GL_CompileShader(ShaderType shaderType, const std::string& sourceCode);
    std::string GL_ReadShaderFiles(const std::filesystem::path& filePath);
    std::shared_ptr<GL_Shader> GL_GetShader(const std::string& name);
    ShaderProgramID GL_CreateShaderProgram();

    void GL_AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID);
    void GL_ValidateShaderProgram(ShaderProgramID programID);
    void GL_DeleteShaderProgram(ShaderProgramID programID);
    void GL_LinkShaderProgram(ShaderProgramID programID);
    void GL_DestroyShader(const std::string& name);
}