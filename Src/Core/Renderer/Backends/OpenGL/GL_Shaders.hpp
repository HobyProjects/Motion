#pragma once

#include "Shaders.hpp"

namespace Motion
{
    class GL_Shader final : public IShader
    {
        public:
            GL_Shader(std::unordered_map<ShaderType, std::string>& shaderSources);
            virtual ~GL_Shader() override;

            virtual void Bind() const override;
            virtual void Unbind() const override;

            virtual void SetUniform(const std::string_view uniformName, float value) override;
            virtual void SetUniform(const std::string_view uniformName, std::int32_t value) override;
            virtual void SetUniform(const std::string_view uniformName, const glm::vec2& value) override;
            virtual void SetUniform(const std::string_view uniformName, const glm::vec3& value) override;
            virtual void SetUniform(const std::string_view uniformName, const glm::vec4& value) override;
            virtual void SetUniform(const std::string_view uniformName, const glm::mat2& value) override;
            virtual void SetUniform(const std::string_view uniformName, const glm::mat3& value) override;
            virtual void SetUniform(const std::string_view uniformName, const glm::mat4& value) override;
            virtual void SetUniform(const std::string_view uniformName, std::int32_t size, std::uint32_t* values) override;

            [[nodiscard]] virtual ShaderProgramID ProgramID() const override { return m_ProgramID; }
            [[nodiscard]] virtual UniformLocation GetUniformLocation(const std::string_view uniformName) override;

            static void LinkShaderProgram(ShaderProgramID programID);
            static void ValidateShaderProgram(ShaderProgramID programID);
            static void AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID);
            static void DeleteShaderProgram(ShaderProgramID programID);

            [[nodiscard]] static ShaderProgramID CreateShaderProgram();
            [[nodiscard]] static ShaderID CompileShader(ShaderType shaderType, const std::string& sourceCode);

        private:
            ShaderProgramID m_ProgramID{ 0 };
            ShaderType m_ShaderType{ ShaderType::None };
            std::unordered_map<std::string_view, UniformLocation> m_UniformLocations{};
    };
}