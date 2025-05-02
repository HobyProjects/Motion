#include "CorePCH.hpp"
#include "GL_Shaders.hpp"
#include "Shaders.hpp"

namespace Motion::Core
{
    static std::shared_ptr<ShaderContainer<GL_Shader>> s_ShaderContainer = std::make_shared<ShaderContainer<GL_Shader>>();

    std::shared_ptr<GL_Shader> GL_CreateShader(const std::string& name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
    {
        auto shader = std::make_shared<GL_Shader>(name, vertexPath, fragmentPath);
        s_ShaderContainer->InsertShader(name, shader);
        return shader;
    }

    void GL_DestroyShader(const std::string & name)
    {
        s_ShaderContainer->RemoveShader(name);
    }

    std::shared_ptr<GL_Shader> GL_GetShader(const std::string& name)
    {
        return s_ShaderContainer->GetShader(name);
    }

    ShaderProgramID GL_CreateShaderProgram()
    {
        ShaderProgramID programID = glCreateProgram();
        MOTION_ASSERT(programID, "Failed to create shader program");
        return programID;
    }

    void GL_AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID)
    {
        glAttachShader(programID, shaderID);
    }

    GLenum GetShaderType(ShaderType shaderType)
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

    ShaderID GL_CompileShader(ShaderType shaderType, const std::string& sourceCode) 
    {
        GLenum glShaderType = GetShaderType(shaderType);
        ShaderID shaderID = glCreateShader(glShaderType);
        MOTION_ASSERT(shaderID, "Failed to create shader of type {0}", static_cast<int>(shaderType));
        const char* source = sourceCode.c_str();
        glShaderSource(shaderID, 1, &source, nullptr);
        glCompileShader(shaderID);

        return shaderID;
    }

    void GL_LinkShaderProgram(ShaderProgramID programID)
    {
        glLinkProgram(programID);
    }

    void GL_ValidateShaderProgram(ShaderProgramID programID)
    {
        glValidateProgram(programID);
    }

    void GL_DeleteShaderProgram(ShaderProgramID programID)
    {
        glDeleteProgram(programID);
    }

    std::string GL_ReadShaderFiles(const std::filesystem::path& filePath)
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

    GL_Shader::GL_Shader(const std::string& name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
    {
        m_Name = name;
        std::string vertexSource = GL_ReadShaderFiles(vertexPath);
        std::string fragmentSource = GL_ReadShaderFiles(fragmentPath);

        m_ProgramID = GL_CreateShaderProgram();
        ShaderID vertexShader = GL_CompileShader(ShaderType::Vertex, vertexSource);
        ShaderID fragmentShader = GL_CompileShader(ShaderType::Fragment, fragmentSource);
        GL_AttachShaderProgram(vertexShader, m_ProgramID);
        GL_AttachShaderProgram(fragmentShader, m_ProgramID);
        GL_LinkShaderProgram(m_ProgramID);
        GL_ValidateShaderProgram(m_ProgramID);
    }
    
    GL_Shader::~GL_Shader()
    {
        GL_DeleteShaderProgram(m_ProgramID);
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
