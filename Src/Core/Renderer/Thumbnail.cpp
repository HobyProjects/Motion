#include "CorePCH.hpp"
#include "Thumbnail.hpp"

namespace Motion::Core
{
    static FrameBufferSpecification s_Specification{};
    static std::shared_ptr<IFrameBuffer> s_FrameBuffer{ nullptr };
    static Camera3D s_Camera{};


    static void CreateFrame()
    {
        std::weak_ptr<IWindow> activeWindow = WindowManager::GetActiveWindow();
        if (!activeWindow.expired())
        {
            auto window = activeWindow.lock();
            GraphicSettings graphicSettings = window->GetGraphicSettings();

            switch (graphicSettings.AntiAliasing)
            {
            case GraphicSettings::AntiAliasingLevel::None:
            {
                s_Specification.Samples = 1;
                break;
            }
            case GraphicSettings::AntiAliasingLevel::MSAAx2:
            {
                s_Specification.Samples = 2;
                break;
            }
            case GraphicSettings::AntiAliasingLevel::MSAAx4:
            {
                s_Specification.Samples = 4;
                break;
            }
            case GraphicSettings::AntiAliasingLevel::MSAAx8:
            {
                s_Specification.Samples = 8;
                break;
            }
            }

            if (!s_FrameBuffer)
                s_FrameBuffer = BufferFactory::CreateFrameBuffer(s_Specification);
            else
                s_FrameBuffer->ResizeFrame(s_Specification.Width, s_Specification.Height);

            s_Camera.ViewportWidth = static_cast<float>(s_Specification.Width);
            s_Camera.ViewportHeight = static_cast<float>(s_Specification.Height);
            s_Camera.AspectRatio = static_cast<float>(s_Specification.Width) / static_cast<float>(s_Specification.Height);
            s_Camera.Position = { 0.0f, 0.0f, 200.0f };
        }
        else
        {
            MOTION_ASSERT(false, "Active window is null!");
        }
    }

    std::shared_ptr<IThumbnail> ThumbnailFactory::CreateThumbnail(const std::string& name, const std::shared_ptr<StaticMesh>& model, uint32_t width, uint32_t height)
    {
        /*         if(s_Specification.Width != width || s_Specification.Height != height)
                {
                    s_Specification.Width = width;
                    s_Specification.Height = height;
                    CreateFrame();
                }

                auto modelThumbnail = std::make_shared<ModelThumbnail>(name, width, height, model);
                if(!modelThumbnail->m_Model)
                {
                    MOTION_CORE_ERROR("Can not create thumbnail for model {0}, it is not valid", name);
                    return nullptr;
                }

                if(!s_FrameBuffer)
                    CreateFrame();

                s_FrameBuffer->Bind();

                Renderer::SetViewport(0, 0, (uint32_t)s_Camera.ViewportWidth, (uint32_t)s_Camera.ViewportHeight);
                Renderer::ClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
                Renderer::Clear();
                s_Camera.RefreshCameraMatrix();

                modelThumbnail->m_Model->Render(glm::mat4(1.0f), s_Camera.GetCameraMatrix());

                s_FrameBuffer->Unbind();

                if(s_FrameBuffer->IsMSAA())
                    modelThumbnail->m_Specification.Attachment = s_FrameBuffer->GetResolvedColorAttachment();
                else
                    modelThumbnail->m_Specification.Attachment = s_FrameBuffer->GetColorAttachment();

                return modelThumbnail; */

        return nullptr;
    }

    std::shared_ptr<IThumbnail> ThumbnailFactory::CreateThumbnail(const std::string& name, const std::shared_ptr<ITexture>& texture, uint32_t width, uint32_t height)
    {
        /*         if(s_Specification.Width != width || s_Specification.Height != height)
                {
                    s_Specification.Width = width;
                    s_Specification.Height = height;
                    CreateFrame();
                }

                auto textureThumbnail = std::make_shared<TextureThumbnail>(name, width, height, texture);
                if(!textureThumbnail->m_Texture)
                {
                    MOTION_CORE_ERROR("Can not create thumbnail for texture {0}, it is not valid", name);
                    return nullptr;
                }

                textureThumbnail->m_Mesh = Mesh::CreateQuad(width, height);

                if(!s_FrameBuffer)
                    CreateFrame();

                s_FrameBuffer->Bind();

                Renderer::SetViewport(0, 0, (uint32_t)s_Camera.ViewportWidth, (uint32_t)s_Camera.ViewportHeight);
                Renderer::ClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
                Renderer::Clear();
                s_Camera.RefreshCameraMatrix(); */

                // [FIXME]: There is problem with renderer, need to fix it before implementing this feature

        return nullptr;
    }
}