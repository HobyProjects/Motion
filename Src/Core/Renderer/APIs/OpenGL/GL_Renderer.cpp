#include "CorePCH.hpp"

namespace Motion
{
    void GL_Init()
    {
        glDepthMask(GL_TRUE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);


#ifdef MOTION_BUILD_DEBUG

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        glDebugMessageCallback(GL_MessageCallBack, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

#endif
    }

    void GL_Quit()
    {

    }

    void GL_Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GL_ClearColor(const glm::vec4& color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void GL_SetViewport(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height)
    {
        glViewport(x, y, width, height);
    }

    void GL_DrawIndexed(std::int32_t indicesCount)
    {
        glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, NULL);
    }

    void GL_ApplyDrawFlags(DrawFlags flags)
    {
        switch (flags)
        {
        case DrawFlags::DepthTest:
        {
            glEnable(GL_DEPTH_TEST);
            break;
        }
        case DrawFlags::SkipDepthMask:
        {
            glDepthMask(GL_FALSE);
            break;
        }
        case DrawFlags::Wireframe:
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        }
        case DrawFlags::CullFace:
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        }
        case DrawFlags::Blending:
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        }
        default:
            MOTION_CORE_WARN("Unknown draw flag: {0}", static_cast<std::uint8_t>(flags));
            break;
        };
    }

    void GL_ResetDrawFlags(DrawFlags flags)
    {
        switch (flags)
        {
        case DrawFlags::DepthTest:
        {
            glDisable(GL_DEPTH_TEST);
            break;
        }
        case DrawFlags::SkipDepthMask:
        {
            glDepthMask(GL_TRUE);
            break;
        }
        case DrawFlags::Wireframe:
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        }
        case DrawFlags::CullFace:
        {
            glDisable(GL_CULL_FACE);
            break;
        }
        case DrawFlags::Blending:
        {
            glDisable(GL_BLEND);
            break;
        }
        default:
            MOTION_CORE_WARN("Unknown draw flag: {0}", static_cast<std::uint8_t>(flags));
            break;
        };
    }

    void GL_ApplyDepthFunction(DepthFunction depthFunction)
    {
        switch (depthFunction)
        {
        case DepthFunction::Never:
            glDepthFunc(GL_NEVER);
            break;
        case DepthFunction::Less:
            glDepthFunc(GL_LESS);
            break;
        case DepthFunction::Equal:
            glDepthFunc(GL_EQUAL);
            break;
        case DepthFunction::LessEqual:
            glDepthFunc(GL_LEQUAL);
            break;
        case DepthFunction::Greater:
            glDepthFunc(GL_GREATER);
            break;
        case DepthFunction::NotEqual:
            glDepthFunc(GL_NOTEQUAL);
            break;
        case DepthFunction::GreaterEqual:
            glDepthFunc(GL_GEQUAL);
            break;
        case DepthFunction::Always:
            glDepthFunc(GL_ALWAYS);
            break;
        default:
            MOTION_CORE_WARN("Unknown depth function: {0}", static_cast<GLenum>(depthFunction));
            break;
        }
    }

    void GL_ResetDepthFunction()
    {
        glDepthFunc(GL_LESS);
    }

    void GL_BindTextureUnit(std::int32_t slot, std::uint32_t textureID)
    {
        glBindTextureUnit(slot, textureID);
    }

    void GL_UnbindTextureUnit(std::int32_t slot)
    {
        glBindTextureUnit(slot, 0);
    }

    std::int32_t GL_GetMaxTextureSlots() noexcept
    {
        static GLint maxTextureUnits = 0;
        if (maxTextureUnits == 0)
        {
            glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
        }

        return maxTextureUnits;
    }
}

