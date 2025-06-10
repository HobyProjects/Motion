#pragma once

#include "Shaders.hpp"
#include "Asset.hpp"

namespace Motion::Core
{
    class GL_Shader final : public AssetBase<IShader>
    {
        public:
            GL_Shader(const std::string& name, const std::unordered_map<ShaderType, std::string>& shaderSources, const std::filesystem::path& sourceFile);
            virtual ~GL_Shader();

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

    class GL_ShaderCompiler
    {
        private:
            GL_ShaderCompiler() = default;
            ~GL_ShaderCompiler() = default;

            GL_ShaderCompiler(const GL_ShaderCompiler&) = delete;
            GL_ShaderCompiler& operator=(const GL_ShaderCompiler&) = delete;
            GL_ShaderCompiler(GL_ShaderCompiler&&) = delete;
            GL_ShaderCompiler& operator=(GL_ShaderCompiler&&) = delete;

        public:
            static ShaderProgramID CreateShaderProgram();
            static void AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID);
            static ShaderID CompileShader(ShaderType shaderType, const std::string& sourceCode);
            static void LinkShaderProgram(ShaderProgramID programID);
            static void ValidateShaderProgram(ShaderProgramID programID);
            static void DeleteShaderProgram(ShaderProgramID programID);
            static std::string ReadShaderFiles(const std::filesystem::path& filePath);
    };
}