#include "CorePCH.hpp"

namespace Motion::Core
{
    static GLenum GetShaderType(ShaderType shaderType)
    {
        switch (shaderType)
        {
            case ShaderType::Vertex:    return GL_VERTEX_SHADER;
            case ShaderType::Fragment:  return GL_FRAGMENT_SHADER;
            case ShaderType::Geometry:  return GL_GEOMETRY_SHADER;
            case ShaderType::Compute:   return GL_COMPUTE_SHADER;
            case ShaderType::TessellationControl: return GL_TESS_CONTROL_SHADER;
            case ShaderType::TessellationEvaluation: return GL_TESS_EVALUATION_SHADER;
            default: return GL_NONE;
        }
    }

    ShaderProgramID GL_ShaderCompiler::CreateShaderProgram()
    {
        ShaderProgramID programID = glCreateProgram();
        MOTION_ASSERT(programID, "Failed to create shader program");
        return programID;
    }

    void GL_ShaderCompiler::AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID)
    {
        glAttachShader(programID, shaderID);
    }


    ShaderID GL_ShaderCompiler::CompileShader(ShaderType shaderType, const std::string& sourceCode) 
    {
        GLenum glShaderType = GetShaderType(shaderType);
        ShaderID shaderID = glCreateShader(glShaderType);
        MOTION_ASSERT(shaderID, "Failed to create shader of type {0}", static_cast<int>(shaderType));
        const char* source = sourceCode.c_str();
        glShaderSource(shaderID, 1, &source, nullptr);
        glCompileShader(shaderID);

        return shaderID;
    }

    void GL_ShaderCompiler::LinkShaderProgram(ShaderProgramID programID)
    {
        glLinkProgram(programID);
    }

    void GL_ShaderCompiler::ValidateShaderProgram(ShaderProgramID programID)
    {
        glValidateProgram(programID);
    }

    void GL_ShaderCompiler::DeleteShaderProgram(ShaderProgramID programID)
    {
        glDeleteProgram(programID);
    }

    std::string GL_ShaderCompiler::ReadShaderFiles(const std::filesystem::path& filePath)
    {
        if(!std::filesystem::exists(filePath))
        {
            MOTION_ASSERT(false, "Shader file does not exist: {0}", filePath.string());
            return std::string("");
        }

        std::string result{};
        std::ifstream in_file(filePath, std::ios::in | std::ios::binary);
        if (in_file)
        {
            in_file.seekg(0, std::ios::end);
            result.resize(in_file.tellg());
            in_file.seekg(0, std::ios::beg);
            in_file.read(&result[0], result.size());
            return result;
        }

        MOTION_ASSERT(false, "Failed to read shader file: {0}", filePath.string());
        return std::string("");
    }

    GL_Shader::GL_Shader(const std::string& name, const std::unordered_map<ShaderType, std::string>& shaderSources, const std::filesystem::path& sourceFile):
        AssetBase<IShader>(UniqueIdentity::GetUniqueID(), name, AssetType::Shader, sourceFile.string())
    {
        m_Name = name;
        m_ProgramID = GL_ShaderCompiler::CreateShaderProgram();

        for(const auto& [type, source] : shaderSources)
        {
            ShaderID compiledShaderID = GL_ShaderCompiler::CompileShader(type, source);
            GL_ShaderCompiler::AttachShaderProgram(compiledShaderID, m_ProgramID);
        }

        GL_ShaderCompiler::LinkShaderProgram(m_ProgramID);
        GL_ShaderCompiler::ValidateShaderProgram(m_ProgramID);
        m_MetaData.IsLoaded = true;
    }

    GL_Shader::GL_Shader(UUID uuid, const std::string& name, const std::unordered_map<ShaderType, std::string>& shaderSources, const std::filesystem::path& sourceFile):
        AssetBase<IShader>(uuid, name, AssetType::Shader, sourceFile.string())
    {
        m_Name = name;
        m_ProgramID = GL_ShaderCompiler::CreateShaderProgram();

        for(const auto& [type, source] : shaderSources)
        {
            ShaderID compiledShaderID = GL_ShaderCompiler::CompileShader(type, source);
            GL_ShaderCompiler::AttachShaderProgram(compiledShaderID, m_ProgramID);
        }

        GL_ShaderCompiler::LinkShaderProgram(m_ProgramID);
        GL_ShaderCompiler::ValidateShaderProgram(m_ProgramID);
        m_MetaData.IsLoaded = true;
    }
    
    GL_Shader::~GL_Shader()
    {
        GL_ShaderCompiler::DeleteShaderProgram(m_ProgramID);
    }

    UniformLocation GL_Shader::GetUniformLocation(const std::string& uniformName) const 
    {
        if (m_UniformLocations.find(uniformName) == m_UniformLocations.end())
        {
            UniformLocation location = glGetUniformLocation(m_ProgramID, uniformName.c_str());
            if (location == -1)
            {
                MOTION_ASSERT(location, "Failed to get uniform location: {0}", uniformName);
                return -1;
            }
            m_UniformLocations[uniformName] = location;
        }

        return m_UniformLocations[uniformName];
    }

    void GL_Shader::SetUniform(const std::string& uniformName, float value) 
    {
        glUniform1f(GetUniformLocation(uniformName), value);
    }

    void GL_Shader::SetUniform(const std::string& uniformName, int32_t value) 
    {
        glUniform1i(GetUniformLocation(uniformName), value);
    }

    void GL_Shader::SetUniform(const std::string& uniformName, uint32_t value) 
    {
        glUniform1ui(GetUniformLocation(uniformName), value);
    }

    void GL_Shader::SetUniform(const std::string& uniformName, const glm::vec2& value) 
    {
        glUniform2fv(GetUniformLocation(uniformName), 1, glm::value_ptr(value));
    }

    void GL_Shader::SetUniform(const std::string& uniformName, const glm::vec3& value) 
    {
        glUniform3fv(GetUniformLocation(uniformName), 1, glm::value_ptr(value));
    }

    void GL_Shader::SetUniform(const std::string& uniformName, const glm::vec4& value) 
    {
        glUniform4fv(GetUniformLocation(uniformName), 1, glm::value_ptr(value));
    }

    void GL_Shader::SetUniform(const std::string& uniformName, const glm::mat2& value) 
    {
        glUniformMatrix2fv(GetUniformLocation(uniformName), 1, GL_FALSE, glm::value_ptr(value));
    }

    void GL_Shader::SetUniform(const std::string& uniformName, const glm::mat3& value) 
    {
        glUniformMatrix3fv(GetUniformLocation(uniformName), 1, GL_FALSE, glm::value_ptr(value));
    }

    void GL_Shader::SetUniform(const std::string& uniformName, const glm::mat4& value) 
    {
        glUniformMatrix4fv(GetUniformLocation(uniformName), 1, GL_FALSE, glm::value_ptr(value));
    }

    void GL_Shader::Bind() const 
    {
        glUseProgram(m_ProgramID);
    }

    void GL_Shader::Unbind() const 
    {
        glUseProgram(0);
    }
}
