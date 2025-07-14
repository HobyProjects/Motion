#include "CorePCH.hpp"
#include "Shaders.hpp"

namespace Motion::Core
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
    void ShaderFactory::DeleteShaderProgram(ShaderProgramID programID)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         GL_ShaderFactory::DeleteShaderProgram(programID); break;
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
    ShaderProgramID ShaderFactory::CreateShaderProgram()
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_ShaderFactory::CreateShaderProgram();
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
    void ShaderFactory::AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         GL_ShaderFactory::AttachShaderProgram(shaderID, programID); break;
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
    ShaderID ShaderFactory::CompileShader(ShaderType shaderType, const std::string& sourceCode)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_ShaderFactory::CompileShader(shaderType, sourceCode);
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
    void ShaderFactory::LinkShaderProgram(ShaderProgramID programID)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         GL_ShaderFactory::LinkShaderProgram(programID); break;
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
    void ShaderFactory::ValidateShaderProgram(ShaderProgramID programID)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         GL_ShaderFactory::ValidateShaderProgram(programID); break;
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    /**
     * @brief Reads the shader file from the specified file path based on the current rendering API.
     *
     * This function delegates the reading of shader files to the appropriate shader factory
     * implementation depending on the active rendering API (e.g., OpenGL, Vulkan, DirectX).
     * If the rendering API is not implemented or unknown, an assertion is triggered and an empty string is returned.
     *
     * @param filePath The path to the shader file to be read.
     * @return The contents of the shader file as a std::string. Returns an empty string if the rendering API is not implemented or unknown.
     */
    std::string ShaderFactory::ReadShaderFiles(const std::filesystem::path& filePath)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_ShaderFactory::ReadShaderFiles(filePath);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return "";
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return "";
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return "";
        };
    }
}

