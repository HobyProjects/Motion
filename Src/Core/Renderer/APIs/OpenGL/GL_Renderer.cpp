#include "CorePCH.hpp"
#include "GL_Renderer.hpp"

namespace Motion::Core
{
    void GL_Renderer::Init()
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);

#ifdef MOTION_BUILD_DEBUG

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(GL_MessageCallBack, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

#endif
    }

    void GL_Renderer::Quit()
    {

    }

    void GL_Renderer::Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GL_Renderer::ClearColor(const glm::vec4& color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void GL_Renderer::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
    {
        glViewport(x, y, width, height);
    }

    void GL_Renderer::DrawIndexed(uint32_t indicesCount)
    {
        glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, NULL);
    }

    void GL_Renderer::ApplyDrawFlags(DrawFlags flags)
    {
        switch (flags)
        {
        case DrawFlags::None:
        {
            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        }
        case DrawFlags::SkipDepthWrite:
        {
            glDepthMask(GL_FALSE);
            break;
        }
        case DrawFlags::Wireframe:
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        }
        case DrawFlags::Instanced:
        {
            // Instancing is not yet implemented
            MOTION_ASSERT(false, "Instancing is not yet implemented!");
            break;
        }
        };
    }

    void GL_Renderer::ResetDrawFlags()
    {
        glDepthMask(GL_TRUE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
    }
}

