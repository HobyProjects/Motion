#pragma once

#include "Shaders.hpp"
#include "Asset.hpp"

namespace Motion::Core
{
    class GL_Shader final : public AssetBase<IShader>
    {
    public:
        GL_Shader(const std::string& name, const std::unordered_map<ShaderType, std::string>& shaderSources, const std::filesystem::path& sourceFile);
        GL_Shader(UUID uuid, const std::string& name, const std::unordered_map<ShaderType, std::string>& shaderSources, const std::filesystem::path& sourceFile);
        virtual ~GL_Shader();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual ShaderProgramID ProgramID() const override { return m_ProgramID; }
        virtual std::string GetName() const override { return m_MetaData.AssetName; }
        virtual UniformLocation GetUniformLocation(const std::string_view uniformName) override;

        virtual void SetUniform(const std::string_view uniformName, float value) override;
        virtual void SetUniform(const std::string_view uniformName, int32_t value) override;
        virtual void SetUniform(const std::string_view uniformName, uint32_t value) override;
        virtual void SetUniform(const std::string_view uniformName, const glm::vec2& value) override;
        virtual void SetUniform(const std::string_view uniformName, const glm::vec3& value) override;
        virtual void SetUniform(const std::string_view uniformName, const glm::vec4& value) override;
        virtual void SetUniform(const std::string_view uniformName, const glm::mat2& value) override;
        virtual void SetUniform(const std::string_view uniformName, const glm::mat3& value) override;
        virtual void SetUniform(const std::string_view uniformName, const glm::mat4& value) override;
        virtual void ReflectUniforms() override;

    public:
        virtual std::int32_t GetMaxTextureUnits() const override;

    private:
        ShaderProgramID m_ProgramID{ 0 };
        ShaderType m_ShaderType{ ShaderType::None };
        std::unordered_map<std::string_view, UniformInfomation> m_UniformInformationCache{};
        std::unordered_map<std::string_view, UniformLocation> m_UniformLocationsCache{};
    };

    class GL_ShaderFactory
    {
    private:
        GL_ShaderFactory() = default;
        ~GL_ShaderFactory() = default;

        GL_ShaderFactory(const GL_ShaderFactory&) = delete;
        GL_ShaderFactory& operator=(const GL_ShaderFactory&) = delete;
        GL_ShaderFactory(GL_ShaderFactory&&) = delete;
        GL_ShaderFactory& operator=(GL_ShaderFactory&&) = delete;

    public:
        static ShaderProgramID CreateShaderProgram();
        static void AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID);
        static ShaderID CompileShader(ShaderType shaderType, const std::string& sourceCode);
        static void LinkShaderProgram(ShaderProgramID programID);
        static void ValidateShaderProgram(ShaderProgramID programID);
        static void DeleteShaderProgram(ShaderProgramID programID);
    };
}