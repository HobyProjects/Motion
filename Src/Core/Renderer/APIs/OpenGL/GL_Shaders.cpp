#include "CorePCH.hpp"

namespace Motion::Core
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
    ShaderProgramID GL_ShaderFactory::CreateShaderProgram()
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
    void GL_ShaderFactory::AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID)
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
    ShaderID GL_ShaderFactory::CompileShader(ShaderType shaderType, const std::string& sourceCode)
    {
        GLenum glShaderType = GetShaderType(shaderType);
        ShaderID shaderID = glCreateShader(glShaderType);
        MOTION_ASSERT(shaderID, "Failed to create shader of type {0}", static_cast<int>(shaderType));
        const char* source = sourceCode.c_str();
        glShaderSource(shaderID, 1, &source, nullptr);
        glCompileShader(shaderID);

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
    void GL_ShaderFactory::LinkShaderProgram(ShaderProgramID programID)
    {
        glLinkProgram(programID);
    }

    /**
     * @brief Validates the specified OpenGL shader program.
     *
     * This function calls glValidateProgram on the given shader program ID to check if the program can execute given the current OpenGL state.
     * It is typically used after linking a shader program to ensure it is valid and ready for use.
     *
     * @param programID The OpenGL identifier of the shader program to validate.
     */
    void GL_ShaderFactory::ValidateShaderProgram(ShaderProgramID programID)
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
    void GL_ShaderFactory::DeleteShaderProgram(ShaderProgramID programID)
    {
        glDeleteProgram(programID);
    }

    /**
     * @brief Constructs a GL_Shader object by compiling and linking shader sources.
     *
     * This constructor initializes a shader asset with the given name and source file path.
     * It creates a new OpenGL shader program, compiles each shader source provided in the
     * shaderSources map (keyed by ShaderType), attaches them to the program, and then links
     * and validates the program. Upon successful creation, the shader is marked as loaded.
     *
     * @param name The name of the shader asset.
     * @param shaderSources A map associating ShaderType with its corresponding GLSL source code.
     * @param sourceFile The filesystem path to the original shader source file.
     */
    GL_Shader::GL_Shader(const std::string& name, const std::unordered_map<ShaderType, std::string>& shaderSources, const std::filesystem::path& sourceFile) :
        AssetBase<IShader>(UniqueIdentity::GetUniqueID(), name, AssetType::Shader, sourceFile.string())
    {
        m_Name = name;
        m_ProgramID = GL_ShaderFactory::CreateShaderProgram();

        for (const auto& [type, source] : shaderSources)
        {
            ShaderID compiledShaderID = GL_ShaderFactory::CompileShader(type, source);
            GL_ShaderFactory::AttachShaderProgram(compiledShaderID, m_ProgramID);
        }

        GL_ShaderFactory::LinkShaderProgram(m_ProgramID);
        GL_ShaderFactory::ValidateShaderProgram(m_ProgramID);
        m_MetaData.IsAssetInitialized = true;
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
    GL_Shader::GL_Shader(UUID uuid, const std::string& name, const std::unordered_map<ShaderType, std::string>& shaderSources, const std::filesystem::path& sourceFile) :
        AssetBase<IShader>(uuid, name, AssetType::Shader, sourceFile.string())
    {
        m_Name = name;
        m_ProgramID = GL_ShaderFactory::CreateShaderProgram();

        for (const auto& [type, source] : shaderSources)
        {
            ShaderID compiledShaderID = GL_ShaderFactory::CompileShader(type, source);
            GL_ShaderFactory::AttachShaderProgram(compiledShaderID, m_ProgramID);
        }

        GL_ShaderFactory::LinkShaderProgram(m_ProgramID);
        GL_ShaderFactory::ValidateShaderProgram(m_ProgramID);
        m_MetaData.IsAssetInitialized = true;
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
        GL_ShaderFactory::DeleteShaderProgram(m_ProgramID);
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

    /**
     * @brief Sets the value of a float uniform variable in the currently active OpenGL shader program.
     *
     * This function locates the specified uniform variable by name and assigns it the provided float value.
     *
     * @param uniformName The name of the uniform variable in the shader program.
     * @param value The float value to set for the uniform variable.
     */
    void GL_Shader::SetUniform(const std::string_view& uniformName, float value)
    {
        glUniform1f(GetUniformLocation(uniformName.data()), value);
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
    void GL_Shader::SetUniform(const std::string_view& uniformName, int32_t value)
    {
        glUniform1i(GetUniformLocation(uniformName.data()), value);
    }

    /**
     * @brief Sets an unsigned integer uniform variable in the shader program.
     *
     * This function uploads a 32-bit unsigned integer value to the specified uniform variable
     * in the currently active OpenGL shader program.
     *
     * @param uniformName The name of the uniform variable to set.
     * @param value The unsigned integer value to assign to the uniform variable.
     */
    void GL_Shader::SetUniform(const std::string_view& uniformName, uint32_t value)
    {
        glUniform1ui(GetUniformLocation(uniformName.data()), value);
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
    void GL_Shader::SetUniform(const std::string_view& uniformName, const glm::vec2& value)
    {
        glUniform2fv(GetUniformLocation(uniformName.data()), 1, glm::value_ptr(value));
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
    void GL_Shader::SetUniform(const std::string_view& uniformName, const glm::vec3& value)
    {
        glUniform3fv(GetUniformLocation(uniformName.data()), 1, glm::value_ptr(value));
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
    void GL_Shader::SetUniform(const std::string_view& uniformName, const glm::vec4& value)
    {
        glUniform4fv(GetUniformLocation(uniformName.data()), 1, glm::value_ptr(value));
    }

    /**
     * @brief Sets a 2x2 matrix uniform variable in the currently active OpenGL shader program.
     *
     * This function uploads a 2x2 matrix (glm::mat2) to the shader uniform specified by its name.
     *
     * @param uniformName The name of the uniform variable in the shader.
     * @param value The 2x2 matrix value to set for the uniform.
     */
    void GL_Shader::SetUniform(const std::string_view& uniformName, const glm::mat2& value)
    {
        glUniformMatrix2fv(GetUniformLocation(uniformName.data()), 1, GL_FALSE, glm::value_ptr(value));
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
    void GL_Shader::SetUniform(const std::string_view& uniformName, const glm::mat3& value)
    {
        glUniformMatrix3fv(GetUniformLocation(uniformName.data()), 1, GL_FALSE, glm::value_ptr(value));
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
    void GL_Shader::SetUniform(const std::string_view& uniformName, const glm::mat4& value)
    {
        glUniformMatrix4fv(GetUniformLocation(uniformName.data()), 1, GL_FALSE, glm::value_ptr(value));
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
        m_InUse = true;
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
        m_InUse = false;
    }
}
