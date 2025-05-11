#include "CorePCH.hpp"
#include "Shaders.hpp"

namespace Motion::Core
{
    std::shared_ptr<IShader> ShaderBuilder::CreateShader(const std::string& name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateShader(name, vertexPath, fragmentPath);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    void ShaderBuilder::DestroyShader(const std::string& name)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_DestroyShader(name); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    std::shared_ptr<IShader> ShaderBuilder::GetShader(const std::string& name)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_GetShader(name);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    void ShaderBuilder::DeleteShaderProgram(ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_DeleteShaderProgram(programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    ShaderProgramID ShaderBuilder::CreateShaderProgram()
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateShaderProgram();
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return 0; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return 0; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return 0;
        };
    }

    void ShaderBuilder::AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_AttachShaderProgram(shaderID, programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    ShaderID ShaderBuilder::CompileShader(ShaderType shaderType, const std::string& sourceCode)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CompileShader(shaderType, sourceCode);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return 0; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return 0; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return 0;
        };
    }

    void ShaderBuilder::LinkShaderProgram(ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_LinkShaderProgram(programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

   
    void ShaderBuilder::ValidateShaderProgram(ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ValidateShaderProgram(programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    std::string ShaderBuilder::ReadShaderFiles(const std::filesystem::path& filePath)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ReadShaderFiles(filePath);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return ""; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return ""; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return "";
        };
    }
}