#include "CorePCH.hpp"
#include "Renderer.hpp"

namespace Motion
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

    /**
     * @brief Initializes the Renderer subsystem.
     *
     * This function sets up the rendering backend based on the selected rendering API.
     * It initializes the appropriate renderer (e.g., OpenGL) and starts the render thread,
     * which waits for frame readiness, consumes queued render commands, and processes them.
     *
     * @note Currently, only OpenGL is implemented. Vulkan and DirectX will trigger assertions.
     * @note The render thread runs in the background and processes commands when a new frame is ready.
     *
     * @throws Assertion failure if an unsupported or unknown rendering API is selected.
     */
    void Renderer::Init()
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Init();
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

    /**
     * @brief Shuts down the renderer and cleans up resources.
     *
     * This function stops the rendering thread, notifies any waiting threads,
     * and joins the rendering thread if it is still running. It then performs
     * cleanup specific to the currently selected rendering API.
     *
     * For OpenGL, it calls the appropriate cleanup routine. For Vulkan and DirectX,
     * this function asserts as those APIs are not yet implemented. If an unknown
     * rendering API is selected, an assertion is triggered.
     */
    void Renderer::Quit()
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Quit();
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

    /**
     * @brief Retrieves the current rendering API in use.
     *
     * @return The currently selected RenderingAPI.
     */
    RenderingAPI Renderer::GetAPI() noexcept
    {
        return s_RenderingAPI;
    }

    /**
     * @brief Clears the current rendering target using the selected rendering API.
     *
     * This function dispatches the clear operation to the appropriate rendering backend
     * based on the value of s_RenderingAPI. Currently, only OpenGL is implemented.
     * For Vulkan and DirectX, the function will trigger an assertion as they are not yet implemented.
     *
     * @note If an unknown rendering API is selected, an assertion will be triggered.
     */
    void Renderer::Clear()
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Clear();
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

    /**
     * @brief Sets the clear color for the current rendering context.
     *
     * This function sets the color used to clear the rendering target (e.g., the screen or framebuffer)
     * based on the currently selected rendering API. If the rendering API is not implemented,
     * an assertion will be triggered.
     *
     * @param color The color to use when clearing, represented as a glm::vec4 (RGBA).
     */
    void Renderer::ClearColor(const glm::vec4& color)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_ClearColor(color);
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

    /**
     * @brief Sets the viewport for rendering.
     *
     * Configures the rendering viewport to the specified position and size.
     * The implementation depends on the currently selected rendering API.
     *
     * @param x The x-coordinate of the lower left corner of the viewport.
     * @param y The y-coordinate of the lower left corner of the viewport.
     * @param width The width of the viewport.
     * @param height The height of the viewport.
     */
    void Renderer::SetViewport(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_SetViewport(x, y, width, height);
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

    /**
     * @brief Retrieves the maximum number of texture slots available for the current rendering API.
     *
     * This function queries the rendering backend to determine how many texture units can be used.
     * The implementation varies based on the selected rendering API.
     *
     * @return The maximum number of texture slots available.
     */
    std::int32_t Renderer::GetMaxTextureSlots() noexcept
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            return GL_GetMaxTextureSlots();
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            return 0;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            return 0;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            return 0;
        }
    }

    /**
         * @brief Binds a texture to a specified texture unit slot for the active rendering API.
         *
         * This function binds the given texture (identified by textureID) to the specified slot,
         * depending on the currently selected rendering API. If the rendering API is not implemented,
         * an assertion will be triggered.
         *
         * @param slot The texture unit slot to bind the texture to.
         * @param textureID The identifier of the texture to bind.
         */
    void Renderer::BindTextureUnit(std::int32_t slot, std::uint32_t textureID)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_BindTextureUnit(slot, textureID);
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


    /**
     * @brief Unbinds a texture unit slot for the active rendering API.
     *
     * This function unbinds the specified texture unit slot, effectively clearing any texture bound to it.
     * The implementation depends on the currently selected rendering API. If the rendering API is not implemented,
     * an assertion will be triggered.
     *
     * @param slot The texture unit slot to unbind.
     */
    void Renderer::UnbindTextureUnit(std::int32_t slot)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_UnbindTextureUnit(slot);
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


    /**
     * @brief Draws indexed geometry using the currently selected rendering API.
     *
     * This function dispatches the indexed draw call to the appropriate rendering backend
     * (e.g., OpenGL, Vulkan, DirectX) based on the value of s_RenderingAPI. If the selected
     * API is not implemented, an assertion will be triggered.
     *
     * @param indicesCount The number of indices to draw.
     */
    void Renderer::DrawIndexed(std::int32_t indicesCount)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_DrawIndexed(indicesCount);
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

    /**
     * @brief Applies the specified draw flags to the current rendering context.
     *
     * This function sets various rendering options based on the provided draw flags.
     * It modifies depth writing, polygon mode, and other rendering states as needed.
     *
     * @param flags The draw flags to apply.
     */
    void Renderer::ApplyDrawFlags(DrawFlags flags)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_ApplyDrawFlags(flags);
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
        };
    }

    /**
     * @brief Resets the draw flags to their default state for the current rendering API.
     *
     * This function clears any previously set draw flags and restores the default rendering state.
     * It is typically called at the end of a frame or before starting a new frame.
     *
     * @param flags The draw flags to reset (e.g., SkipDepthMask, Wireframe).
     */
    void Renderer::ResetDrawFlags(DrawFlags flags)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_ResetDrawFlags(flags);
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
}