#include "CorePCH.hpp"
#include "GL_Shaders.hpp"

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
        m_ProgramID = GL_CreateShaderProgram();

        for (const auto& [type, source] : shaderSources)
        {
            ShaderID compiledShaderID = GL_CompileShader(type, source);
            GL_AttachShaderProgram(compiledShaderID, m_ProgramID);
        }

        GL_LinkShaderProgram(m_ProgramID);
        GL_ValidateShaderProgram(m_ProgramID);
        ReflectUniforms();

        AssetInfo.IsAssetInitialized = true;
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
        m_ProgramID = GL_CreateShaderProgram();

        for (const auto& [type, source] : shaderSources)
        {
            ShaderID compiledShaderID = GL_CompileShader(type, source);
            GL_AttachShaderProgram(compiledShaderID, m_ProgramID);
        }

        GL_LinkShaderProgram(m_ProgramID);
        GL_ValidateShaderProgram(m_ProgramID);
        ReflectUniforms();

        AssetInfo.IsAssetInitialized = true;
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
        auto iterator = m_UniformLocationsCache.find(uniformName);
        if (iterator != m_UniformLocationsCache.end())
        {
            return iterator->second;
        }

        UniformLocation location = glGetUniformLocation(m_ProgramID, uniformName.data());
        if (location == INVALID_UNIFORM_LOCATION)
        {
            MOTION_ASSERT(false, "Uniform '{0}' not found in shader program '{1}'", uniformName, AssetInfo.AssetName);
            return INVALID_UNIFORM_LOCATION; // Return an invalid location if the uniform is not found
        }

        // Cache the uniform location for future use
        m_UniformLocationsCache[uniformName] = location;
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
        auto uniformInfoIterator = m_UniformInformationCache.find(uniformName);
        if (uniformInfoIterator != m_UniformInformationCache.end())
        {
            const UniformInfomation& uniformInfo = uniformInfoIterator->second;
            if (uniformInfo.IsValid && uniformInfo.Type == UniformType::Float)
            {
                glUniform1f(uniformInfo.Location, value);
            }
        }
        else
        {
            MOTION_CORE_WARN("Uniform '{0}' is not of type Float or is invalid", uniformName);
            UniformLocation location = GetUniformLocation(uniformName);
            if (location != INVALID_UNIFORM_LOCATION)
            {
                m_UniformInformationCache[uniformName] = UniformInfomation(uniformName, UniformType::Float, location);
                glUniform1f(location, value);
            }
            else
            {
                MOTION_CORE_ERROR("Uniform '{0}' not found in shader program '{1}'", uniformName, AssetInfo.AssetName);
            }
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
        auto uniformInfoIterator = m_UniformInformationCache.find(uniformName);
        if (uniformInfoIterator != m_UniformInformationCache.end())
        {
            const UniformInfomation& uniformInfo = uniformInfoIterator->second;
            if (uniformInfo.IsValid && uniformInfo.Type == UniformType::Int)
            {
                glUniform1i(uniformInfo.Location, value);
            }
        }
        else
        {
            MOTION_CORE_WARN("Uniform '{0}' is not of type Int or is invalid", uniformName);
            UniformLocation location = GetUniformLocation(uniformName);
            if (location != INVALID_UNIFORM_LOCATION)
            {
                m_UniformInformationCache[uniformName] = UniformInfomation(uniformName, UniformType::Int, location);
                glUniform1i(location, value);
            }
            else
            {
                MOTION_CORE_ERROR("Uniform '{0}' not found in shader program '{1}'", uniformName, AssetInfo.AssetName);
            }
        }
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
    void GL_Shader::SetUniform(const std::string_view uniformName, uint32_t value)
    {
        auto uniformInfoIterator = m_UniformInformationCache.find(uniformName);
        if (uniformInfoIterator != m_UniformInformationCache.end())
        {
            const UniformInfomation& uniformInfo = uniformInfoIterator->second;
            if (uniformInfo.IsValid && uniformInfo.Type == UniformType::UInt)
            {
                glUniform1ui(uniformInfo.Location, value);
            }
        }
        else
        {
            MOTION_CORE_WARN("Uniform '{0}' is not of type UInt or is invalid", uniformName);
            UniformLocation location = GetUniformLocation(uniformName);
            if (location != INVALID_UNIFORM_LOCATION)
            {
                m_UniformInformationCache[uniformName] = UniformInfomation(uniformName, UniformType::UInt, location);
                glUniform1ui(location, value);
            }
            else
            {
                MOTION_CORE_ERROR("Uniform '{0}' not found in shader program '{1}'", uniformName, AssetInfo.AssetName);
            }
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
        auto uniformInfoIterator = m_UniformInformationCache.find(uniformName);
        if (uniformInfoIterator != m_UniformInformationCache.end())
        {
            const UniformInfomation& uniformInfo = uniformInfoIterator->second;
            if (uniformInfo.IsValid && uniformInfo.Type == UniformType::Vec2)
            {
                glUniform2fv(uniformInfo.Location, 1, glm::value_ptr(value));
            }
        }
        else
        {
            MOTION_CORE_WARN("Uniform '{0}' is not of type Vec2 or is invalid", uniformName);
            UniformLocation location = GetUniformLocation(uniformName);
            if (location != INVALID_UNIFORM_LOCATION)
            {
                m_UniformInformationCache[uniformName] = UniformInfomation(uniformName, UniformType::Vec2, location);
                glUniform2fv(location, 1, glm::value_ptr(value));
            }
            else
            {
                MOTION_CORE_ERROR("Uniform '{0}' not found in shader program '{1}'", uniformName, AssetInfo.AssetName);
            }
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
        auto uniformInfoIterator = m_UniformInformationCache.find(uniformName);
        if (uniformInfoIterator != m_UniformInformationCache.end())
        {
            const UniformInfomation& uniformInfo = uniformInfoIterator->second;
            if (uniformInfo.IsValid && uniformInfo.Type == UniformType::Vec3)
            {
                glUniform3fv(uniformInfo.Location, 1, glm::value_ptr(value));
            }
        }
        else
        {
            MOTION_CORE_WARN("Uniform '{0}' is not of type Vec3 or is invalid", uniformName);
            UniformLocation location = GetUniformLocation(uniformName);
            if (location != INVALID_UNIFORM_LOCATION)
            {
                m_UniformInformationCache[uniformName] = UniformInfomation(uniformName, UniformType::Vec3, location);
                glUniform3fv(location, 1, glm::value_ptr(value));
            }
            else
            {
                MOTION_CORE_ERROR("Uniform '{0}' not found in shader program '{1}'", uniformName, AssetInfo.AssetName);
            }
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
        auto uniformInfoIterator = m_UniformInformationCache.find(uniformName);
        if (uniformInfoIterator != m_UniformInformationCache.end())
        {
            const UniformInfomation& uniformInfo = uniformInfoIterator->second;
            if (uniformInfo.IsValid && uniformInfo.Type == UniformType::Vec4)
            {
                glUniform4fv(uniformInfo.Location, 1, glm::value_ptr(value));
            }
        }
        else
        {
            MOTION_CORE_WARN("Uniform '{0}' is not of type Vec4 or is invalid", uniformName);
            UniformLocation location = GetUniformLocation(uniformName);
            if (location != INVALID_UNIFORM_LOCATION)
            {
                m_UniformInformationCache[uniformName] = UniformInfomation(uniformName, UniformType::Vec4, location);
                glUniform4fv(location, 1, glm::value_ptr(value));
            }
            else
            {
                MOTION_CORE_ERROR("Uniform '{0}' not found in shader program '{1}'", uniformName, AssetInfo.AssetName);
            }
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
        auto uniformInfoIterator = m_UniformInformationCache.find(uniformName);
        if (uniformInfoIterator != m_UniformInformationCache.end())
        {
            const UniformInfomation& uniformInfo = uniformInfoIterator->second;
            if (uniformInfo.IsValid && uniformInfo.Type == UniformType::Mat2)
            {
                glUniformMatrix2fv(uniformInfo.Location, 1, GL_FALSE, glm::value_ptr(value));
            }
        }
        else
        {
            MOTION_CORE_WARN("Uniform '{0}' is not of type Mat2 or is invalid", uniformName);
            UniformLocation location = GetUniformLocation(uniformName);
            if (location != INVALID_UNIFORM_LOCATION)
            {
                m_UniformInformationCache[uniformName] = UniformInfomation(uniformName, UniformType::Mat2, location);
                glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(value));
            }
            else
            {
                MOTION_CORE_ERROR("Uniform '{0}' not found in shader program '{1}'", uniformName, AssetInfo.AssetName);
            }
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
        auto uniformInfoIterator = m_UniformInformationCache.find(uniformName);
        if (uniformInfoIterator != m_UniformInformationCache.end())
        {
            const UniformInfomation& uniformInfo = uniformInfoIterator->second;
            if (uniformInfo.IsValid && uniformInfo.Type == UniformType::Mat3)
            {
                glUniformMatrix3fv(uniformInfo.Location, 1, GL_FALSE, glm::value_ptr(value));
            }
        }
        else
        {
            MOTION_CORE_WARN("Uniform '{0}' is not of type Mat3 or is invalid", uniformName);
            UniformLocation location = GetUniformLocation(uniformName);
            if (location != INVALID_UNIFORM_LOCATION)
            {
                m_UniformInformationCache[uniformName] = UniformInfomation(uniformName, UniformType::Mat3, location);
                glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
            }
            else
            {
                MOTION_CORE_ERROR("Uniform '{0}' not found in shader program '{1}'", uniformName, AssetInfo.AssetName);
            }
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
        auto uniformInfoIterator = m_UniformInformationCache.find(uniformName);
        if (uniformInfoIterator != m_UniformInformationCache.end())
        {
            const UniformInfomation& uniformInfo = uniformInfoIterator->second;
            if (uniformInfo.IsValid && uniformInfo.Type == UniformType::Mat4)
            {
                glUniformMatrix4fv(uniformInfo.Location, 1, GL_FALSE, glm::value_ptr(value));
            }
        }
        else
        {
            MOTION_CORE_WARN("Uniform '{0}' is not of type Mat4 or is invalid", uniformName);
            UniformLocation location = GetUniformLocation(uniformName);
            if (location != INVALID_UNIFORM_LOCATION)
            {
                m_UniformInformationCache[uniformName] = UniformInfomation(uniformName, UniformType::Mat4, location);
                glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
            }
            else
            {
                MOTION_CORE_ERROR("Uniform '{0}' not found in shader program '{1}'", uniformName, AssetInfo.AssetName);
            }
        }
    }

    void GL_Shader::SetUniform(const std::string_view uniformName, std::uint32_t size, std::uint32_t* values)
    {
        auto uniformInfoIterator = m_UniformInformationCache.find(uniformName);
        if (uniformInfoIterator != m_UniformInformationCache.end())
        {
            const UniformInfomation& uniformInfo = uniformInfoIterator->second;
            if (uniformInfo.IsValid && uniformInfo.Type == UniformType::UInt)
            {
                glUniform1uiv(uniformInfo.Location, size, values);
            }
        }
        else
        {
            MOTION_CORE_WARN("Uniform '{0}' is not of type UInt or is invalid", uniformName);
            UniformLocation location = GetUniformLocation(uniformName);
            if (location != INVALID_UNIFORM_LOCATION)
            {
                //[FIXME]: Hmm... Sampler2DArray, This isn't make any sense, but it works for now
                m_UniformInformationCache[uniformName] = UniformInfomation(uniformName, UniformType::Sampler2DArray, location);
                glUniform1uiv(location, size, values);
            }
            else
            {
                MOTION_CORE_ERROR("Uniform '{0}' not found in shader program '{1}'", uniformName, AssetInfo.AssetName);
            }
        }
    }


    /**
     * @brief Reflects and caches the locations and types of all relevant shader uniforms.
     *
     * This method populates the m_UniformInformationCache map with information about
     * various shader uniform variables, including their names, types, and locations
     * within the currently active OpenGL shader program. The uniforms covered include
     * material colors, factors, properties, global attributes, light attributes, and
     * texture samplers. This caching mechanism allows for efficient uniform updates
     * during rendering by avoiding repeated location queries.
     *
     * @note This function assumes that the shader program is already compiled and linked,
     *       and that all uniform names used are present in the shader source.
     */
    void GL_Shader::ReflectUniforms()
    {
        m_UniformInformationCache[UniformCache::Color_AmbientColor] = { UniformCache::Color_AmbientColor, UniformType::Vec4, GetUniformLocation(UniformCache::Color_AmbientColor) };
        m_UniformInformationCache[UniformCache::Color_DiffuseColor] = { UniformCache::Color_DiffuseColor, UniformType::Vec4, GetUniformLocation(UniformCache::Color_DiffuseColor) };
        m_UniformInformationCache[UniformCache::Color_SpecularColor] = { UniformCache::Color_SpecularColor, UniformType::Vec4, GetUniformLocation(UniformCache::Color_SpecularColor) };
        m_UniformInformationCache[UniformCache::Color_EmissiveColor] = { UniformCache::Color_EmissiveColor, UniformType::Vec4, GetUniformLocation(UniformCache::Color_EmissiveColor) };
        m_UniformInformationCache[UniformCache::Color_ReflectiveColor] = { UniformCache::Color_ReflectiveColor, UniformType::Vec4, GetUniformLocation(UniformCache::Color_ReflectiveColor) };
        m_UniformInformationCache[UniformCache::Color_TransparentColor] = { UniformCache::Color_TransparentColor, UniformType::Vec4, GetUniformLocation(UniformCache::Color_TransparentColor) };

        m_UniformInformationCache[UniformCache::Factor_AmbientOcclusionFactor] = { UniformCache::Factor_AmbientOcclusionFactor, UniformType::Float, GetUniformLocation(UniformCache::Factor_AmbientOcclusionFactor) };
        m_UniformInformationCache[UniformCache::Factor_BaseColorFactor] = { UniformCache::Factor_BaseColorFactor, UniformType::Vec4, GetUniformLocation(UniformCache::Factor_BaseColorFactor) };
        m_UniformInformationCache[UniformCache::Factor_MetallicFactor] = { UniformCache::Factor_MetallicFactor, UniformType::Float, GetUniformLocation(UniformCache::Factor_MetallicFactor) };
        m_UniformInformationCache[UniformCache::Factor_RoughnessFactor] = { UniformCache::Factor_RoughnessFactor, UniformType::Float, GetUniformLocation(UniformCache::Factor_RoughnessFactor) };
        m_UniformInformationCache[UniformCache::Factor_ClearCoatFactor] = { UniformCache::Factor_ClearCoatFactor, UniformType::Float, GetUniformLocation(UniformCache::Factor_ClearCoatFactor) };
        m_UniformInformationCache[UniformCache::Factor_ClearCoatRoughnessFactor] = { UniformCache::Factor_ClearCoatRoughnessFactor, UniformType::Float, GetUniformLocation(UniformCache::Factor_ClearCoatRoughnessFactor) };
        m_UniformInformationCache[UniformCache::Factor_SheenFactor] = { UniformCache::Factor_SheenFactor, UniformType::Float, GetUniformLocation(UniformCache::Factor_SheenFactor) };
        m_UniformInformationCache[UniformCache::Factor_SheenRoughnessFactor] = { UniformCache::Factor_SheenRoughnessFactor, UniformType::Float, GetUniformLocation(UniformCache::Factor_SheenRoughnessFactor) };
        m_UniformInformationCache[UniformCache::Factor_TransmissionFactor] = { UniformCache::Factor_TransmissionFactor, UniformType::Float, GetUniformLocation(UniformCache::Factor_TransmissionFactor) };
        m_UniformInformationCache[UniformCache::Factor_IndexOfRefraction] = { UniformCache::Factor_IndexOfRefraction, UniformType::Float, GetUniformLocation(UniformCache::Factor_IndexOfRefraction) };

        m_UniformInformationCache[UniformCache::Property_BumpScaling] = { UniformCache::Property_BumpScaling, UniformType::Float, GetUniformLocation(UniformCache::Property_BumpScaling) };
        m_UniformInformationCache[UniformCache::Property_Shininess] = { UniformCache::Property_Shininess, UniformType::Float, GetUniformLocation(UniformCache::Property_Shininess) };
        m_UniformInformationCache[UniformCache::Property_IndexOfRefraction] = { UniformCache::Property_IndexOfRefraction, UniformType::Float, GetUniformLocation(UniformCache::Property_IndexOfRefraction) };
        m_UniformInformationCache[UniformCache::Property_Opacity] = { UniformCache::Property_Opacity, UniformType::Float, GetUniformLocation(UniformCache::Property_Opacity) };
        m_UniformInformationCache[UniformCache::Property_Reflectivity] = { UniformCache::Property_Reflectivity, UniformType::Float, GetUniformLocation(UniformCache::Property_Reflectivity) };
        m_UniformInformationCache[UniformCache::Property_Shininess] = { UniformCache::Property_Shininess, UniformType::Float, GetUniformLocation(UniformCache::Property_Shininess) };
        m_UniformInformationCache[UniformCache::Property_ShininessStrength] = { UniformCache::Property_ShininessStrength, UniformType::Float, GetUniformLocation(UniformCache::Property_ShininessStrength) };

        m_UniformInformationCache[UniformCache::GlobalAttri_ModelMatrix] = { UniformCache::GlobalAttri_ModelMatrix, UniformType::Mat4, GetUniformLocation(UniformCache::GlobalAttri_ModelMatrix) };
        m_UniformInformationCache[UniformCache::GlobalAttri_ViewProjMatrix] = { UniformCache::GlobalAttri_ViewProjMatrix, UniformType::Mat4, GetUniformLocation(UniformCache::GlobalAttri_ViewProjMatrix) };
        m_UniformInformationCache[UniformCache::GlobalAttri_Sampler2DArray] = { UniformCache::GlobalAttri_Sampler2DArray, UniformType::Sampler2DArray, GetUniformLocation(UniformCache::GlobalAttri_Sampler2DArray) };

        m_UniformInformationCache[UniformCache::LightAttri_Position] = { UniformCache::LightAttri_Position, UniformType::Vec3, GetUniformLocation(UniformCache::LightAttri_Position) };
        m_UniformInformationCache[UniformCache::LightAttri_Color] = { UniformCache::LightAttri_Color, UniformType::Vec3, GetUniformLocation(UniformCache::LightAttri_Color) };
        m_UniformInformationCache[UniformCache::LightAttri_Intensity] = { UniformCache::LightAttri_Intensity, UniformType::Float, GetUniformLocation(UniformCache::LightAttri_Intensity) };

        m_UniformInformationCache[UniformCache::Texture_AmbientTexture] = { UniformCache::Texture_AmbientTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_AmbientTexture) };
        m_UniformInformationCache[UniformCache::Texture_DiffuseTexture] = { UniformCache::Texture_DiffuseTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_DiffuseTexture) };
        m_UniformInformationCache[UniformCache::Texture_SpecularTexture] = { UniformCache::Texture_SpecularTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_SpecularTexture) };
        m_UniformInformationCache[UniformCache::Texture_EmissiveTexture] = { UniformCache::Texture_EmissiveTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_EmissiveTexture) };
        m_UniformInformationCache[UniformCache::Texture_NormalMapsTexture] = { UniformCache::Texture_NormalMapsTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_NormalMapsTexture) };
        m_UniformInformationCache[UniformCache::Texture_HightMapsTexture] = { UniformCache::Texture_HightMapsTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_HightMapsTexture) };
        m_UniformInformationCache[UniformCache::Texture_ShininessTexture] = { UniformCache::Texture_ShininessTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_ShininessTexture) };
        m_UniformInformationCache[UniformCache::Texture_OpacityMapsTexture] = { UniformCache::Texture_OpacityMapsTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_OpacityMapsTexture) };
        m_UniformInformationCache[UniformCache::Texture_LightMapsTexture] = { UniformCache::Texture_LightMapsTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_LightMapsTexture) };
        m_UniformInformationCache[UniformCache::Texture_BaseColorTexture] = { UniformCache::Texture_BaseColorTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_BaseColorTexture) };
        m_UniformInformationCache[UniformCache::Texture_MetallicTexture] = { UniformCache::Texture_MetallicTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_MetallicTexture) };
        m_UniformInformationCache[UniformCache::Texture_RoughnessTexture] = { UniformCache::Texture_RoughnessTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_RoughnessTexture) };
        m_UniformInformationCache[UniformCache::Texture_AOMapTexture] = { UniformCache::Texture_AOMapTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_AOMapTexture) };
        m_UniformInformationCache[UniformCache::Texture_EmissiveTexture] = { UniformCache::Texture_EmissiveTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_EmissiveTexture) };
        m_UniformInformationCache[UniformCache::Texture_ClearCoatTexture] = { UniformCache::Texture_ClearCoatTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_ClearCoatTexture) };
        m_UniformInformationCache[UniformCache::Texture_SheenTexture] = { UniformCache::Texture_SheenTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_SheenTexture) };
        m_UniformInformationCache[UniformCache::Texture_TransmissionTexture] = { UniformCache::Texture_TransmissionTexture, UniformType::Sampler2D, GetUniformLocation(UniformCache::Texture_TransmissionTexture) };
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
