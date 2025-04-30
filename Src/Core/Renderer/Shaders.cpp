#include "CorePCH.hpp"

namespace Motion::Core
{
    std::shared_ptr<IShader> ShaderBuilder::CreateShader(const std::string& name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ShaderBuilder::CreateShader(name, vertexPath, fragmentPath);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; // TODO: Implement Vulkan shader creation
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; // TODO: Implement DirectX shader creation
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    void ShaderBuilder::DestroyShader(const std::string& name)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ShaderBuilder::DestroyShader(name); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; // TODO: Implement Vulkan shader destruction
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; // TODO: Implement DirectX shader destruction
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    std::shared_ptr<IShader> ShaderBuilder::GetShader(const std::string& name)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ShaderBuilder::GetShader(name);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; // TODO: Implement Vulkan shader retrieval
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; // TODO: Implement DirectX shader retrieval
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    void ShaderBuilder::DeleteShaderProgram(ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ShaderBuilder::DeleteShaderProgram(programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; // TODO: Implement Vulkan shader program deletion
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; // TODO: Implement DirectX shader program deletion
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    ShaderProgramID ShaderBuilder::CreateShaderProgram()
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ShaderBuilder::CreateShaderProgram();
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return 0; // TODO: Implement Vulkan shader program creation
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return 0; // TODO: Implement DirectX shader program creation
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return 0;
        };
    }

    void ShaderBuilder::AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ShaderBuilder::AttachShaderProgram(shaderID, programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; // TODO: Implement Vulkan shader program attachment
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; // TODO: Implement DirectX shader program attachment
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    ShaderID ShaderBuilder::CompileShader(ShaderType shaderType, const std::string& sourceCode)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ShaderBuilder::CompileShader(shaderType, sourceCode);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return 0; // TODO: Implement Vulkan shader compilation
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return 0; // TODO: Implement DirectX shader compilation
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return 0;
        };
    }

    void ShaderBuilder::LinkShaderProgram(ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ShaderBuilder::LinkShaderProgram(programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; // TODO: Implement Vulkan shader program linking
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; // TODO: Implement DirectX shader program linking
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    std::string ShaderBuilder::ReadShaderFiles(const std::filesystem::path& filePath)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ShaderBuilder::ReadShaderFiles(filePath);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return ""; // TODO: Implement Vulkan shader file reading
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return ""; // TODO: Implement DirectX shader file reading
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return "";
        };
    }
}