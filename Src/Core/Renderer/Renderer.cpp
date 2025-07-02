#include "CorePCH.hpp"

namespace Motion::Core
{
    #ifdef MOTION_PLATFORM_WINDOWS
        // This should be DirectX but for now we are using OpenGL
        static RenderingAPI s_RenderingAPI = RenderingAPI::OpenGL;
    #elif defined(MOTION_PLATFORM_LINUX)
        // This should be Vulkan but for now we are using OpenGL
        static RenderingAPI s_RenderingAPI = RenderingAPI::OpenGL;
    #else
        #error "Unknown platform!"
    #endif

    static std::vector<DrawCommand> s_RenderQueue;
    static uint32_t s_DrawCalls = 0;

    void Renderer::Init()
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:
                GL_Renderer::Init();
                break;
            case RenderingAPI::Vulkan:
                MOTION_ASSERT(false, "Vulkan is not implemented yet!");
                break;
            case RenderingAPI::DirectX:
                MOTION_ASSERT(false, "DirectX is not implemented yet!");
                break;
            default:
                MOTION_ASSERT(false, "Unknown rendering API!");
                break;
        }
    }

    void Renderer::Quit()
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:
                GL_Renderer::Quit();
                break;
            case RenderingAPI::Vulkan:
                MOTION_ASSERT(false, "Vulkan is not implemented yet!");
                break;
            case RenderingAPI::DirectX:
                MOTION_ASSERT(false, "DirectX is not implemented yet!");
                break;
            default:
                MOTION_ASSERT(false, "Unknown rendering API!");
                break;
        }
    }

    RenderingAPI Renderer::GetAPI()
    {
        return s_RenderingAPI;
    }

    void Renderer::Clear()
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:
                GL_Renderer::Clear();
                break;
            case RenderingAPI::Vulkan:
                MOTION_ASSERT(false, "Vulkan is not implemented yet!");
                break;
            case RenderingAPI::DirectX:
                MOTION_ASSERT(false, "DirectX is not implemented yet!");
                break;
            default:
                MOTION_ASSERT(false, "Unknown rendering API!");
                break;
        }
    }

    void Renderer::ClearColor(const glm::vec4& color)
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:
                GL_Renderer::ClearColor(color);
                break;
            case RenderingAPI::Vulkan:
                MOTION_ASSERT(false, "Vulkan is not implemented yet!");
                break;
            case RenderingAPI::DirectX:
                MOTION_ASSERT(false, "DirectX is not implemented yet!");
                break;
            default:
                MOTION_ASSERT(false, "Unknown rendering API!");
                break;
        }
    }

    void Renderer::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:
                GL_Renderer::SetViewport(x, y, width, height);
                break;
            case RenderingAPI::Vulkan:
                MOTION_ASSERT(false, "Vulkan is not implemented yet!");
                break;
            case RenderingAPI::DirectX:
                MOTION_ASSERT(false, "DirectX is not implemented yet!");
                break;
            default:
                MOTION_ASSERT(false, "Unknown rendering API!");
                break;
        }
    }

    void Renderer::DrawIndexed(uint32_t indicesCount)
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:
                GL_Renderer::DrawIndexed(indicesCount);
                break;
            case RenderingAPI::Vulkan:
                MOTION_ASSERT(false, "Vulkan is not implemented yet!");
                break;
            case RenderingAPI::DirectX:
                MOTION_ASSERT(false, "DirectX is not implemented yet!");
                break;
            default:
                MOTION_ASSERT(false, "Unknown rendering API!");
                break;
        }
    }

    void Renderer::Submit(const DrawCommand& drawCommand) 
    {
        s_RenderQueue.push_back(drawCommand);
    }

    void Renderer::BeginFrame()
    {
        s_RenderQueue.clear();
        s_DrawCalls = 0;
    }

    void Renderer::EndFrame()
    {
        Flush();
    }

    void Renderer::Flush()
    {
        std::sort(s_RenderQueue.begin(), s_RenderQueue.end());

        std::shared_ptr<IShader> currentShader = nullptr;
        std::shared_ptr<Mesh> currentMesh = nullptr;
        std::shared_ptr<Material> currentMaterial = nullptr;

        for(const auto& draw : s_RenderQueue)
        {
            if(currentShader != draw.Shader)
            {
                currentShader = draw.Shader;
                currentShader->Bind();
            }

            if(currentMaterial != draw.MeshMaterial)
            {
                currentMaterial = draw.MeshMaterial;
                currentMaterial->Bind(currentShader); 
            }

            if(currentMesh != draw.SubMesh)
            {
                currentMesh = draw.SubMesh;
                currentMesh->Bind();
            }

            currentShader->SetUniform(UniformCache::CameraMatrix, draw.CameraMatrix);
            currentShader->SetUniform(UniformCache::ModelMatrix, draw.ModelTransform);
            DrawIndexed(draw.SubMesh->GetIndicesCount());
            s_DrawCalls++;
        }

        currentMaterial = nullptr;
        currentMesh = nullptr;
        currentShader = nullptr;
    }

    uint32_t Renderer::GetDrawCalls()
    {
        return s_DrawCalls;
    }
}