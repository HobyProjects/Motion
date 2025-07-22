#pragma once

#include "Shaders.hpp"
#include "Asset.hpp"

namespace Motion::Core
{
    class GL_Shader final : public AssetBase<IShader>
    {
    public:
        GL_Shader(UUID uuid, const std::string& name, const std::unordered_map<ShaderType, std::string>& shaderSources, const std::filesystem::path& sourceFile);
        virtual ~GL_Shader();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void SetUniform(const std::string_view uniformName, float value) override;
        virtual void SetUniform(const std::string_view uniformName, int32_t value) override;
        virtual void SetUniform(const std::string_view uniformName, uint32_t value) override;
        virtual void SetUniform(const std::string_view uniformName, const glm::vec2& value) override;
        virtual void SetUniform(const std::string_view uniformName, const glm::vec3& value) override;
        virtual void SetUniform(const std::string_view uniformName, const glm::vec4& value) override;
        virtual void SetUniform(const std::string_view uniformName, const glm::mat2& value) override;
        virtual void SetUniform(const std::string_view uniformName, const glm::mat3& value) override;
        virtual void SetUniform(const std::string_view uniformName, const glm::mat4& value) override;
        virtual void SetUniform(const std::string_view uniformName, std::uint32_t size, std::uint32_t* values) override;
        virtual void ReflectUniforms() override;

        [[nodiscard]] virtual ShaderProgramID ProgramID() const override { return m_ProgramID; }
        [[nodiscard]] virtual std::string GetName() const override { return AssetInfo.AssetName; }
        [[nodiscard]] virtual UniformLocation GetUniformLocation(const std::string_view uniformName) override;

    private:
        ShaderProgramID m_ProgramID{ 0 };
        ShaderType m_ShaderType{ ShaderType::None };
        std::unordered_map<std::string_view, UniformInfomation> m_UniformInformationCache{};
        std::unordered_map<std::string_view, UniformLocation> m_UniformLocationsCache{};
    };

    void GL_LinkShaderProgram(ShaderProgramID programID);
    void GL_ValidateShaderProgram(ShaderProgramID programID);
    void GL_AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID);
    void GL_DeleteShaderProgram(ShaderProgramID programID);

    [[nodiscard]] ShaderProgramID GL_CreateShaderProgram();
    [[nodiscard]] ShaderID GL_CompileShader(ShaderType shaderType, const std::string& sourceCode);
}