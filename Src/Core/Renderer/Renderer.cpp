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

    static std::vector<RenderState> s_RenderQueue;

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

    void Renderer::Submit(const RenderState& renderSate) 
    {
        s_RenderQueue.push_back(renderSate);
    }

    void Renderer::Flush()
    {
        std::sort(s_RenderQueue.begin(), s_RenderQueue.end());

        std::shared_ptr<IShader> currentShader = nullptr;
        std::shared_ptr<Mesh> currentMesh = nullptr;
        std::shared_ptr<Material> currentMaterial = nullptr;

        for(const auto& draw : s_RenderQueue)
        {
            if(currentShader != draw.shader)
            {
                currentShader = draw.shader;
                currentShader->Bind();
            }

            if(currentMaterial != draw.material)
            {
                currentMaterial = draw.material;
                //currentMaterial->Bind(); // <-- [TODO]: Uncomment this line when the material system is implemented
            }

            if(currentMesh != draw.mesh)
            {
                currentMesh = draw.mesh;
                currentMesh->Bind();
            }

            currentShader->SetUniform("u_ModelMatrix", draw.modelMatrix);
            DrawIndexed(draw.mesh->GetIndicesCount());

            currentMesh->Unbind();
            //currentMaterial->Unbind(); // <-- [TODO]: Uncomment this line when the material system is implemented
            currentShader->Unbind();

            currentMaterial = nullptr;
            currentMesh = nullptr;
            currentShader = nullptr;
        }

        s_RenderQueue.clear();
    }
}