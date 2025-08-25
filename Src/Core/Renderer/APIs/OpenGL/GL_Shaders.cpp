#include "CorePCH.hpp"
namespace Motion
{
    static GLenum GetShaderType(ShaderType shaderType)
    {
        switch (shaderType)
        {
        case ShaderType::Vertex:                    return GL_VERTEX_SHADER;
        case ShaderType::Fragment:                  return GL_FRAGMENT_SHADER;
        case ShaderType::Geometry:                  return GL_GEOMETRY_SHADER;
        case ShaderType::Compute:                   return GL_COMPUTE_SHADER;
        case ShaderType::TessellationControl:       return GL_TESS_CONTROL_SHADER;
        case ShaderType::TessellationEvaluation:    return GL_TESS_EVALUATION_SHADER;
        default:                                    return GL_NONE;
        }
    }

    ShaderProgramID GL_Shader::CreateShaderProgram()
    {
        ShaderProgramID programID = glCreateProgram();
        MOTION_ASSERT(programID, "Failed to create shader program");
        return programID;
    }

    void GL_Shader::AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID)
    {
        glAttachShader(programID, shaderID);
    }

    ShaderID GL_Shader::CompileShader(ShaderType shaderType, const std::string& sourceCode)
    {
        GLenum glShaderType = GetShaderType(shaderType);
        ShaderID shaderID = glCreateShader(glShaderType);
        MOTION_ASSERT(shaderID, "Failed to create shader of type {0}", static_cast<int>(shaderType));

        const char* source = sourceCode.c_str();
        glShaderSource(shaderID, 1, &source, nullptr);
        glCompileShader(shaderID);

        GLint isCompiled = 0;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);

        std::string typeStr;
        switch (glShaderType)
        {
        case GL_VERTEX_SHADER: typeStr = "Vertex"; break;
        case GL_FRAGMENT_SHADER: typeStr = "Fragment"; break;
        case GL_GEOMETRY_SHADER: typeStr = "Geometry"; break;
        case GL_COMPUTE_SHADER: typeStr = "Compute"; break;
        case GL_TESS_CONTROL_SHADER: typeStr = "Tessellation Control"; break;
        case GL_TESS_EVALUATION_SHADER: typeStr = "Tessellation Evaluation"; break;
        default: typeStr = "Unknown"; break;
        }

        if (!isCompiled)
        {
            GLint maxLength = 0;
            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(shaderID, maxLength, &maxLength, infoLog.data());
            std::string logStr(infoLog.begin(), infoLog.end());

            MOTION_CORE_ERROR("Shader Compile Error: {} Shader", typeStr);

            // Split source code for reference
            std::istringstream sourceStream(sourceCode);
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(sourceStream, line))
                lines.push_back(line);

            // Regex for Nvidia/GLSL style errors: 0(153) : error C7623: message
            std::regex regexPattern(R"(\d+\((\d+)\)\s*:\s*(error|warning)\s+([A-Z]\d+)\s*:\s*(.*))");

            std::istringstream logStream(logStr);
            while (std::getline(logStream, line))
            {
                std::smatch matches;
                if (std::regex_match(line, matches, regexPattern))
                {
                    int lineNum = std::stoi(matches[1].str());
                    std::string msgType = matches[2].str();
                    std::string errorCode = matches[3].str();
                    std::string msg = matches[4].str();

                    // Get corresponding source line
                    std::string codeLine = (lineNum > 0 && lineNum <= lines.size()) ? lines[lineNum - 1] : "";

                    MOTION_CORE_ERROR(" [{}:{}:{}]  : {}   ", msgType, lineNum, errorCode, msg);
                    MOTION_CORE_ERROR(" -----> Code : {} \n", codeLine);

                    // Highlight first token in message (best effort)
                    std::istringstream msgStream(msg);
                    std::string token;
                    msgStream >> token;

                    if (!token.empty())
                    {
                        size_t colPos = codeLine.find(token);
                        if (colPos != std::string::npos)
                        {
                            std::string pointer(colPos, ' ');
                            pointer += "^";
                            MOTION_CORE_ERROR("           {}", pointer);
                        }
                    }
                }
                else
                {
                    // Raw log line if parsing fails
                    MOTION_CORE_ERROR(line);
                }
            }

            MOTION_ASSERT(false, "Shader compilation failed: Shader ID {}", shaderID);
        }

        return shaderID;
    }

    void GL_Shader::LinkShaderProgram(ShaderProgramID programID)
    {
        glLinkProgram(programID);

        GLint isLinked = 0;
        glGetProgramiv(programID, GL_LINK_STATUS, &isLinked);

        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(programID, maxLength, &maxLength, infoLog.data());

            MOTION_CORE_ERROR("Shader Link Error: Program ID {} | Linker Errors Message: {}", programID, infoLog.data());
            MOTION_ASSERT(false, "Shader linking failed: {}", programID);
        }
    }

    void GL_Shader::ValidateShaderProgram(ShaderProgramID programID)
    {
        glValidateProgram(programID);
    }

    void GL_Shader::DeleteShaderProgram(ShaderProgramID programID)
    {
        glDeleteProgram(programID);
    }

    GL_Shader::GL_Shader(UUID uuid, const std::string& name, const std::unordered_map<ShaderType, std::string>& shaderSources, const std::filesystem::path& sourceFile) : AssetBase<IShader>(uuid, name, AssetType::Shader, sourceFile.string())
    {
        m_ProgramID = CreateShaderProgram();
        MOTION_CORE_INFO("Shader program created with ID: {0} for {1}", m_ProgramID, name);

        for (const auto& [type, source] : shaderSources)
        {
            ShaderID compiledShaderID = CompileShader(type, source);
            AttachShaderProgram(compiledShaderID, m_ProgramID);
        }

        LinkShaderProgram(m_ProgramID);
        ValidateShaderProgram(m_ProgramID);

        AssetInfo.IsInitialized = true;
    }

    GL_Shader::~GL_Shader()
    {
        DeleteShaderProgram(m_ProgramID);
    }

    UniformLocation GL_Shader::GetUniformLocation(const std::string_view uniformName)
    {
        auto iterator = m_UniformLocations.find(uniformName);
        if (iterator != m_UniformLocations.end())
        {
            return iterator->second;
        }

        UniformLocation location = glGetUniformLocation(m_ProgramID, uniformName.data());
        if (location == INVALID_UNIFORM_LOCATION)
        {
            MOTION_CORE_WARN("Uniform '{0}' not found in shader program '{1}'", uniformName, GetName());
            return INVALID_UNIFORM_LOCATION;
        }

        m_UniformLocations[uniformName] = location;
        return location;
    }

    void GL_Shader::SetUniform(const std::string_view uniformName, float value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniform1f(location, value);
        }
    }

    void GL_Shader::SetUniform(const std::string_view uniformName, int32_t value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniform1i(location, value);
        }
    }

    void GL_Shader::SetUniform(const std::string_view uniformName, const glm::vec2& value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniform2fv(location, 1, glm::value_ptr(value));
        }
    }

    void GL_Shader::SetUniform(const std::string_view uniformName, const glm::vec3& value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniform3fv(location, 1, glm::value_ptr(value));
        }
    }

    void GL_Shader::SetUniform(const std::string_view uniformName, const glm::vec4& value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniform4fv(location, 1, glm::value_ptr(value));
        }
    }

    void GL_Shader::SetUniform(const std::string_view uniformName, const glm::mat2& value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    void GL_Shader::SetUniform(const std::string_view uniformName, const glm::mat3& value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    void GL_Shader::SetUniform(const std::string_view uniformName, const glm::mat4& value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    void GL_Shader::SetUniform(const std::string_view uniformName, std::int32_t size, std::uint32_t* values)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniform1uiv(location, size, values);
        }
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
