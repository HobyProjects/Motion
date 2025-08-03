#include "CorePCH.hpp"

namespace Motion
{
    /**
     * @brief Deletes a shader program identified by the given program ID.
     *
     * This function delegates the deletion of the shader program to the appropriate
     * backend implementation based on the current rendering API in use. If the rendering
     * API is not implemented or unknown, an assertion will be triggered.
     *
     * @param programID The identifier of the shader program to be deleted.
     */
    void IShader::DeleteShaderProgram(ShaderProgramID programID)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         GL_Shader::DeleteShaderProgram(programID); break;
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }


    /**
     * @brief Creates a new shader program based on the current rendering API.
     *
     * This function selects the appropriate shader program creation routine depending on the
     * rendering API currently in use (e.g., OpenGL, Vulkan, DirectX). If the selected API is not
     * implemented, an assertion will be triggered and the function will return 0.
     *
     * @return ShaderProgramID The identifier of the created shader program, or 0 if creation failed or the API is not implemented.
     *
     * @note Currently, only OpenGL is implemented. Vulkan and DirectX will trigger assertions.
     */
    ShaderProgramID IShader::CreateShaderProgram()
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_Shader::CreateShaderProgram();
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return 0;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return 0;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return 0;
        };
    }

    /**
     * @brief Attaches a shader to a shader program for the current rendering API.
     *
     * This function binds the specified shader (identified by shaderID) to the given shader program (identified by programID).
     * The implementation is API-dependent and currently only supports OpenGL.
     * For unsupported APIs (Vulkan, DirectX, or unknown), an assertion will be triggered.
     *
     * @param shaderID    The identifier of the shader to attach.
     * @param programID   The identifier of the shader program to which the shader will be attached.
     */
    void IShader::AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         GL_Shader::AttachShaderProgram(shaderID, programID); break;
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    /**
     * @brief Compiles a shader of the specified type from the provided source code.
     *
     * This function dispatches the shader compilation to the appropriate backend implementation
     * based on the currently selected rendering API. If the rendering API is not implemented,
     * an assertion will be triggered and the function will return 0.
     *
     * @param shaderType The type of shader to compile (e.g., vertex, fragment).
     * @param sourceCode The source code of the shader as a string.
     * @return ShaderID The identifier of the compiled shader, or 0 if compilation failed or the API is not implemented.
     */
    ShaderID IShader::CompileShader(ShaderType shaderType, const std::string& sourceCode)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_Shader::CompileShader(shaderType, sourceCode);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return 0;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return 0;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return 0;
        };
    }

    /**
     * @brief Links a shader program identified by the given program ID.
     *
     * This function dispatches the linking operation to the appropriate backend
     * implementation based on the currently selected rendering API. If the rendering
     * API is not implemented or unknown, an assertion will be triggered.
     *
     * @param programID The identifier of the shader program to link.
     *
     * @note Currently, only the OpenGL backend is implemented. Vulkan and DirectX
     *       backends are not yet supported.
     */
    void IShader::LinkShaderProgram(ShaderProgramID programID)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         GL_Shader::LinkShaderProgram(programID); break;
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }


    /**
     * @brief Validates a shader program for the currently selected rendering API.
     *
     * This function checks the validity of the shader program identified by the given programID.
     * The validation process is delegated to the appropriate backend implementation based on the
     * active rendering API (e.g., OpenGL). If the rendering API is not implemented or unknown,
     * an assertion is triggered.
     *
     * @param programID The identifier of the shader program to validate.
     */
    void IShader::ValidateShaderProgram(ShaderProgramID programID)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         GL_Shader::ValidateShaderProgram(programID); break;
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    /**
     * @brief Converts a string representation of a shader type to its corresponding ShaderType enum value.
     *
     * This function takes a string (e.g., "vertex", "fragment", "geometry", etc.) and returns the matching
     * ShaderType enum. If the string does not match any known shader type, ShaderType::None is returned.
     *
     * @param typeStr The string representation of the shader type.
     * @return ShaderType The corresponding ShaderType enum value, or ShaderType::None if no match is found.
     */
    static ShaderType GetShaderTypeFromString(const std::string& typeStr)
    {
        if (typeStr == "vertex")                      return ShaderType::Vertex;
        if (typeStr == "fragment")                    return ShaderType::Fragment;
        if (typeStr == "geometry")                    return ShaderType::Geometry;
        if (typeStr == "compute")                     return ShaderType::Compute;
        if (typeStr == "tessellation_control")        return ShaderType::TessellationControl;
        if (typeStr == "tessellation_evaluation")     return ShaderType::TessellationEvaluation;

        return ShaderType::None;
    }

    /**
     * @brief Reads the contents of a shader file from the specified file path.
     *
     * This function attempts to open and read the entire contents of the shader file
     * located at the given file path. If the file does not exist or cannot be opened,
     * an assertion is triggered and an empty string is returned.
     *
     * @param filePath The path to the shader file to be read.
     * @return A std::string containing the contents of the shader file, or an empty string if the file does not exist or cannot be opened.
     */
    std::string IShader::ReadShaderFile(const std::filesystem::path& filePath)
    {
        if (!std::filesystem::exists(filePath))
        {
            MOTION_ASSERT(false, "Shader file does not exist: {0}", filePath.string());
            return {};
        }

        std::ifstream file(filePath);
        if (!file.is_open())
        {
            MOTION_ASSERT(false, "Failed to open shader file: {0}", filePath.string());
            return {};
        }

        std::string sourceCode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return sourceCode;
    }

    /**
     * @brief Reads a shader file containing multiple shader stages and extracts their source code.
     *
     * This function reads the entire contents of the specified shader file and parses it to extract
     * the source code for each shader stage (e.g., vertex, fragment, etc.) defined within the file.
     * Shader stages are expected to be marked with a line of the form `#type <shader_type>`.
     *
     * @param filePath The path to the shader file to read.
     * @return An unordered map associating each ShaderType with its corresponding source code string.
     *         If the file is empty or no valid shader stages are found, the returned map will be empty.
     *
     * @note If an unknown shader type is encountered, a warning is logged and that section is skipped.
     */
    std::unordered_map<ShaderType, std::string> IShader::ReadFullShaderFile(const std::filesystem::path& filePath)
    {
        std::string source = ReadShaderFile(filePath);
        if (source.empty())
            return {};

        std::unordered_map<ShaderType, std::string> shaderSources;
        std::regex typeRegex(R"(#type\s+(\w+))");
        std::sregex_iterator it(source.begin(), source.end(), typeRegex);
        std::sregex_iterator end;

        std::vector<std::pair<size_t, ShaderType>> shaderPositions;
        for (; it != end; ++it)
        {
            std::string typeStr = (*it)[1];
            ShaderType shaderType = GetShaderTypeFromString(typeStr);
            if (shaderType == ShaderType::None)
            {
                MOTION_CORE_WARN("Unknown shader type: {0}", typeStr);
                continue;
            }
            shaderPositions.emplace_back(it->position(), shaderType);
        }

        for (size_t i = 0; i < shaderPositions.size(); ++i)
        {
            size_t begin = source.find('\n', shaderPositions[i].first) + 1;
            size_t end = (i + 1 < shaderPositions.size()) ? shaderPositions[i + 1].first : source.size();
            shaderSources[shaderPositions[i].second] = source.substr(begin, end - begin);
        }

        return shaderSources;
    }

    /**
     * @brief Reads the contents of vertex and fragment shader files and returns them as a map.
     *
     * This function checks if the provided vertex and fragment shader file paths exist.
     * If either file does not exist, it asserts and returns an empty map.
     * It then reads the contents of both files using ReadShaderFile and stores them in a map
     * with their corresponding ShaderType as the key. If reading either file fails (resulting in
     * an empty string), it asserts and returns an empty map.
     *
     * @param vertexPath The filesystem path to the vertex shader file.
     * @param fragmentPath The filesystem path to the fragment shader file.
     * @return std::unordered_map<ShaderType, std::string> A map containing the shader source code
     *         for each shader type. Returns an empty map if files do not exist or cannot be read.
     */
    std::unordered_map<ShaderType, std::string> IShader::ReadShaderFiles(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
    {
        if (!std::filesystem::exists(vertexPath) || !std::filesystem::exists(fragmentPath))
        {
            MOTION_ASSERT(false, "One or both shader files do not exist: {0}, {1}", vertexPath.string(), fragmentPath.string());
            return {};
        }

        std::unordered_map<ShaderType, std::string> shaderSources;
        shaderSources[ShaderType::Vertex] = ReadShaderFile(vertexPath);
        shaderSources[ShaderType::Fragment] = ReadShaderFile(fragmentPath);

        if (shaderSources[ShaderType::Vertex].empty() || shaderSources[ShaderType::Fragment].empty())
        {
            MOTION_ASSERT(false, "Failed to read shader files: {0}, {1}", vertexPath.string(), fragmentPath.string());
            return {};
        }

        return shaderSources;
    }
}

