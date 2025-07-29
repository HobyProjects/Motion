#include "CorePCH.hpp"
namespace Motion
{
    /**
     * @brief Converts a ShaderType enum value to the corresponding OpenGL shader type constant.
     *
     * This function maps the custom ShaderType enumeration to the appropriate OpenGL
     * shader type constant (e.g., GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.).
     *
     * @param shaderType The ShaderType enum value representing the type of shader.
     * @return GLenum The corresponding OpenGL shader type constant, or GL_NONE if the type is unrecognized.
     */
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

    /**
     * @brief Creates a new OpenGL shader program.
     *
     * This function generates a new shader program object using OpenGL's glCreateProgram,
     * asserts that the creation was successful, and returns the program ID.
     *
     * @return ShaderProgramID The unique identifier for the newly created shader program.
     *
     * @throws Assertion failure if the shader program could not be created.
     */
    ShaderProgramID GL_CreateShaderProgram()
    {
        ShaderProgramID programID = glCreateProgram();
        MOTION_ASSERT(programID, "Failed to create shader program");
        return programID;
    }

    /**
     * @brief Attaches a compiled shader object to a shader program.
     *
     * This function attaches the shader identified by @p shaderID to the shader program
     * identified by @p programID using the OpenGL function glAttachShader.
     *
     * @param shaderID The identifier of the compiled shader object to attach.
     * @param programID The identifier of the shader program to which the shader will be attached.
     */
    void GL_AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID)
    {
        glAttachShader(programID, shaderID);
    }

    /**
     * @brief Compiles an OpenGL shader from the provided source code.
     *
     * This function creates and compiles a shader object of the specified type using the given GLSL source code.
     * If shader creation fails, an assertion is triggered.
     *
     * @param shaderType The type of shader to compile (e.g., vertex, fragment).
     * @param sourceCode The GLSL source code for the shader.
     * @return ShaderID The OpenGL identifier for the compiled shader.
     */
    ShaderID GL_CompileShader(ShaderType shaderType, const std::string& sourceCode)
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

        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(shaderID, maxLength, &maxLength, infoLog.data());
            MOTION_CORE_ERROR("Shader Compile Error: Shader Type {} | Compile Errors Message: {}", typeStr, infoLog.data());
            MOTION_ASSERT(false, "Shader compilation failed: Shader ID {}", shaderID);
        }

        return shaderID;
    }

    /**
     * @brief Links the specified OpenGL shader program.
     *
     * This function finalizes the shader program by linking all attached shader objects.
     * After linking, the program can be used for rendering operations.
     *
     * @param programID The identifier of the shader program to link.
     */
    void GL_LinkShaderProgram(ShaderProgramID programID)
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

    /**
     * @brief Validates the specified OpenGL shader program.
     *
     * This function calls glValidateProgram on the given shader program ID to check if the program can execute given the current OpenGL state.
     * It is typically used after linking a shader program to ensure it is valid and ready for use.
     *
     * @param programID The OpenGL identifier of the shader program to validate.
     */
    void GL_ValidateShaderProgram(ShaderProgramID programID)
    {
        glValidateProgram(programID);
    }

    /**
     * @brief Deletes an OpenGL shader program.
     *
     * This function deletes the shader program associated with the given program ID,
     * freeing any resources allocated by OpenGL for that program.
     *
     * @param programID The identifier of the shader program to delete.
     */
    void GL_DeleteShaderProgram(ShaderProgramID programID)
    {
        glDeleteProgram(programID);
    }


    /**
     * @brief Constructs a GL_Shader object, compiles and links shader sources into an OpenGL shader program.
     *
     * @param uuid         Unique identifier for the shader asset.
     * @param name         Name of the shader.
     * @param shaderSources A map associating ShaderType with corresponding GLSL source code strings.
     * @param sourceFile   Filesystem path to the original shader source file.
     *
     * This constructor initializes the shader asset, creates a new OpenGL shader program,
     * compiles each provided shader source, attaches them to the program, and then links
     * and validates the program. Upon successful completion, the shader is marked as loaded.
     */
    GL_Shader::GL_Shader(UUID uuid, const std::string& name, const std::unordered_map<ShaderType, std::string>& shaderSources, const std::filesystem::path& sourceFile) : AssetBase<IShader>(uuid, name, AssetType::Shader, sourceFile.string())
    {
        m_ProgramID = GL_CreateShaderProgram();
        MOTION_CORE_INFO("Shader program created with ID: {0} for {1}", m_ProgramID, name);

        for (const auto& [type, source] : shaderSources)
        {
            ShaderID compiledShaderID = GL_CompileShader(type, source);
            GL_AttachShaderProgram(compiledShaderID, m_ProgramID);
        }

        GL_LinkShaderProgram(m_ProgramID);
        GL_ValidateShaderProgram(m_ProgramID);

        AssetInfo.IsInitialized = true;
    }

    /**
     * @brief Destructor for the GL_Shader class.
     *
     * This destructor is responsible for cleaning up resources associated with the shader program.
     * Specifically, it deletes the OpenGL shader program identified by m_ProgramID using the
     * GL_ShaderFactory::DeleteShaderProgram method.
     */
    GL_Shader::~GL_Shader()
    {
        GL_DeleteShaderProgram(m_ProgramID);
    }

    /**
     * Retrieves the location of a uniform variable within the shader program.
     *
     * If the location for the specified uniform name has not been previously queried,
     * this function queries OpenGL for the location and caches it for future use.
     * If the uniform does not exist in the shader program, an assertion is triggered
     * and -1 is returned.
     *
     * @param uniformName The name of the uniform variable whose location is to be retrieved.
     * @return The location of the uniform variable, or -1 if the uniform is not found.
     */
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

        // Cache the uniform location for future use
        m_UniformLocations[uniformName] = location;
        return location;
    }

    /**
     * @brief Sets the value of a float uniform variable in the currently active OpenGL shader program.
     *
     * This function locates the specified uniform variable by name and assigns it the provided float value.
     *
     * @param uniformName The name of the uniform variable in the shader program.
     * @param value The float value to set for the uniform variable.
     */
    void GL_Shader::SetUniform(const std::string_view uniformName, float value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniform1f(location, value);
        }
    }

    /**
     * @brief Sets the value of an integer uniform variable in the currently active OpenGL shader program.
     *
     * This function locates the uniform variable specified by @p uniformName within the shader program
     * and sets its value to the provided integer @p value using glUniform1i.
     *
     * @param uniformName The name of the uniform variable to set.
     * @param value The integer value to assign to the uniform variable.
     */
    void GL_Shader::SetUniform(const std::string_view uniformName, int32_t value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniform1i(location, value);
        }
    }

    /**
     * @brief Sets a 2-component floating point vector uniform variable in the shader.
     *
     * This function uploads the given glm::vec2 value to the shader program's uniform variable
     * specified by uniformName. It retrieves the uniform location and sets its value using OpenGL's glUniform2fv.
     *
     * @param uniformName The name of the uniform variable in the shader.
     * @param value The glm::vec2 value to set for the uniform variable.
     */
    void GL_Shader::SetUniform(const std::string_view uniformName, const glm::vec2& value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniform2fv(location, 1, glm::value_ptr(value));
        }
    }

    /**
     * @brief Sets a vec3 uniform variable in the currently active OpenGL shader program.
     *
     * This function uploads a 3-component floating point vector (glm::vec3) to the shader uniform
     * specified by its name. The uniform must exist in the shader program currently in use.
     *
     * @param uniformName The name of the uniform variable in the shader.
     * @param value The glm::vec3 value to set for the uniform.
     */
    void GL_Shader::SetUniform(const std::string_view uniformName, const glm::vec3& value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniform3fv(location, 1, glm::value_ptr(value));
        }
    }

    /**
     * @brief Sets the value of a vec4 uniform variable in the shader program.
     *
     * This function uploads a 4-component floating point vector (glm::vec4) to the specified uniform variable
     * in the currently active OpenGL shader program.
     *
     * @param uniformName The name of the uniform variable in the shader.
     * @param value The glm::vec4 value to set for the uniform.
     */
    void GL_Shader::SetUniform(const std::string_view uniformName, const glm::vec4& value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniform4fv(location, 1, glm::value_ptr(value));
        }
    }

    /**
     * @brief Sets a 2x2 matrix uniform variable in the currently active OpenGL shader program.
     *
     * This function uploads a 2x2 matrix (glm::mat2) to the shader uniform specified by its name.
     *
     * @param uniformName The name of the uniform variable in the shader.
     * @param value The 2x2 matrix value to set for the uniform.
     */
    void GL_Shader::SetUniform(const std::string_view uniformName, const glm::mat2& value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    /**
     * @brief Sets the value of a mat3 uniform variable in the shader program.
     *
     * This function uploads a 3x3 matrix (glm::mat3) to the specified uniform variable
     * in the currently active OpenGL shader program.
     *
     * @param uniformName The name of the uniform variable in the shader.
     * @param value The glm::mat3 value to set for the uniform.
     */
    void GL_Shader::SetUniform(const std::string_view uniformName, const glm::mat3& value)
    {
        UniformLocation location = GetUniformLocation(uniformName);
        if (location != INVALID_UNIFORM_LOCATION)
        {
            glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    /**
     * @brief Sets a 4x4 matrix uniform variable in the currently bound OpenGL shader program.
     *
     * This function uploads the given glm::mat4 value to the shader uniform specified by uniformName.
     * It retrieves the uniform location using GetUniformLocation and uses glUniformMatrix4fv to set the value.
     *
     * @param uniformName The name of the uniform variable in the shader.
     * @param value The 4x4 matrix (glm::mat4) to set for the uniform.
     */
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

    /**
     * @brief Binds the shader program for use in the current OpenGL context.
     *
     * This method activates the shader program associated with this GL_Shader instance,
     * making it the current program used for subsequent rendering operations.
     */
    void GL_Shader::Bind() const
    {
        glUseProgram(m_ProgramID);
    }

    /**
     * @brief Unbinds the currently active OpenGL shader program.
     *
     * This method deactivates any shader program currently bound to the OpenGL context
     * by setting the active program to 0. After calling this function, no shader program
     * will be used for subsequent rendering operations until another program is bound.
     */
    void GL_Shader::Unbind() const
    {
        glUseProgram(0);
    }
}
