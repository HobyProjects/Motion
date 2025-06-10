#include "CorePCH.hpp"

namespace Motion::Core
{
    void ShaderCompiler::DeleteShaderProgram(ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ShaderCompiler::DeleteShaderProgram(programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    ShaderProgramID ShaderCompiler::CreateShaderProgram()
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ShaderCompiler::CreateShaderProgram();
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return 0; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return 0; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return 0;
        };
    }

    void ShaderCompiler::AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ShaderCompiler::AttachShaderProgram(shaderID, programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    ShaderID ShaderCompiler::CompileShader(ShaderType shaderType, const std::string& sourceCode)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ShaderCompiler::CompileShader(shaderType, sourceCode);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return 0; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return 0; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return 0;
        };
    }

    void ShaderCompiler::LinkShaderProgram(ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ShaderCompiler::LinkShaderProgram(programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

   
    void ShaderCompiler::ValidateShaderProgram(ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ShaderCompiler::ValidateShaderProgram(programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    std::string ShaderCompiler::ReadShaderFiles(const std::filesystem::path& filePath)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ShaderCompiler::ReadShaderFiles(filePath);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return ""; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return ""; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return "";
        };
    }
}