#include "CorePCH.hpp"

namespace Motion::Core
{
    void ShaderFactory::DeleteShaderProgram(ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ShaderFactory::DeleteShaderProgram(programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    ShaderProgramID ShaderFactory::CreateShaderProgram()
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ShaderFactory::CreateShaderProgram();
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return 0; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return 0; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return 0;
        };
    }

    void ShaderFactory::AttachShaderProgram(ShaderID shaderID, ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ShaderFactory::AttachShaderProgram(shaderID, programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    ShaderID ShaderFactory::CompileShader(ShaderType shaderType, const std::string& sourceCode)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ShaderFactory::CompileShader(shaderType, sourceCode);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return 0; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return 0; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return 0;
        };
    }

    void ShaderFactory::LinkShaderProgram(ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ShaderFactory::LinkShaderProgram(programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

   
    void ShaderFactory::ValidateShaderProgram(ShaderProgramID programID)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         GL_ShaderFactory::ValidateShaderProgram(programID); break;
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); break; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };
    }

    std::string ShaderFactory::ReadShaderFiles(const std::filesystem::path& filePath)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ShaderFactory::ReadShaderFiles(filePath);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return ""; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return ""; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return "";
        };
    }


    static std::unordered_map<UUID, std::shared_ptr<IShader>> s_ShaderRegistry{};

    void ShaderManager::InsertShader(const UUID& uuid, const std::shared_ptr<IShader>& shader)
    {
        if (s_ShaderRegistry.find(uuid) != s_ShaderRegistry.end())
        {
            MOTION_CORE_WARN("Shader with UUID {} already exists in the cache. Overwriting.", uuid);
            return;
        }
        
        s_ShaderRegistry[uuid] = shader;
    }

    std::shared_ptr<IShader> ShaderManager::GetShader(const UUID& uuid)
    {
        auto it = s_ShaderRegistry.find(uuid);
        if (it != s_ShaderRegistry.end())
        {
            return it->second;
        }
        return nullptr; 
    }

    std::shared_ptr<IShader> ShaderManager::GetShader(const std::string& name)
    {
        for (const auto& pair : s_ShaderRegistry)
        {
            if (pair.second->GetName() == name)
            {
                return pair.second;
            }
        }
        return nullptr;
    }
}