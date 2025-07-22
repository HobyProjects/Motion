#include "CorePCH.hpp"
#include "GL_Renderer.hpp"

namespace Motion::Core
{
    /**
     * @brief Initializes OpenGL state for rendering.
     *
     * This function sets up essential OpenGL features such as blending and depth testing.
     * In debug builds, it also enables OpenGL debug output and sets up a debug message callback
     * for improved error and warning reporting.
     *
     * - Enables alpha blending with source alpha and one minus source alpha blend function.
     * - Enables depth testing to ensure correct rendering of 3D objects.
     * - In debug mode:
     *   - Enables OpenGL debug output and synchronous debug output.
     *   - Registers a debug message callback function.
     *   - Suppresses notification-level debug messages.
     */
    void GL_Init()
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);

#ifdef MOTION_BUILD_DEBUG

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(GL_MessageCallBack, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

#endif
    }

    /**
     * @brief Cleans up and releases all resources used by the OpenGL renderer.
     *
     * This function should be called before application exit or when the OpenGL
     * renderer is no longer needed. It ensures that any allocated resources,
     * such as buffers, shaders, or contexts, are properly released to prevent
     * memory leaks and other issues.
     */
    void GL_Quit()
    {

    }

    /**
     * @brief Clears the OpenGL color and depth buffers.
     *
     * This function calls glClear with GL_COLOR_BUFFER_BIT and GL_DEPTH_BUFFER_BIT,
     * effectively resetting the color and depth information in the current framebuffer.
     * It should be called at the beginning of each frame to prepare for new rendering.
     */
    void GL_Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    /**
     * @brief Sets the clear color for the OpenGL rendering context.
     *
     * This function specifies the red, green, blue, and alpha values used by OpenGL
     * when clearing the color buffer. The color is provided as a glm::vec4, where
     * each component should be in the range [0.0, 1.0].
     *
     * @param color The color to use when clearing the color buffer (RGBA).
     */
    void GL_ClearColor(const glm::vec4& color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    /**
     * @brief Sets the OpenGL viewport dimensions and position.
     *
     * This function defines the affine transformation of x and y from normalized device coordinates to window coordinates.
     * It specifies the lower left corner of the viewport rectangle, as well as the width and height of the viewport.
     *
     * @param x The x coordinate of the lower left corner of the viewport, in pixels.
     * @param y The y coordinate of the lower left corner of the viewport, in pixels.
     * @param width The width of the viewport, in pixels.
     * @param height The height of the viewport, in pixels.
     */
    void GL_SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
    {
        glViewport(x, y, width, height);
    }

    /**
     * @brief Draws indexed geometry using OpenGL.
     *
     * This function issues a draw call to render geometry using the currently bound index buffer.
     * It draws primitives as triangles, using the specified number of indices.
     *
     * @param indicesCount The number of indices to be rendered.
     */
    void GL_DrawIndexed(uint32_t indicesCount)
    {
        glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, NULL);
    }

    /**
     * @brief Applies the specified drawing flags to the OpenGL rendering pipeline.
     *
     * This function configures OpenGL state based on the provided DrawFlags value.
     * It can enable or disable depth testing, set polygon rendering mode to fill or wireframe,
     * and control depth buffer writing. If an unknown flag is provided, a warning is logged.
     *
     * @param flags The drawing flags to apply (e.g., None, SkipDepthMask, Wireframe).
     */
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
        default:
            MOTION_CORE_WARN("Unknown draw flag: {0}", static_cast<std::uint8_t>(flags));
            break;
        };
    }

    /**
     * @brief Resets the OpenGL drawing flags to their default state.
     *
     * This function clears any previously set draw flags and restores the default rendering state.
     * It is typically called at the end of a frame or before starting a new frame.
     *
     * @param flags The draw flags to reset (e.g., SkipDepthMask, Wireframe).
     */
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
        default:
            MOTION_CORE_WARN("Unknown draw flag: {0}", static_cast<std::uint8_t>(flags));
            break;
        };
    }

    /**
     * @brief Binds a texture to a specified texture unit slot in OpenGL.
     *
     * This function binds the texture identified by `textureID` to the texture unit specified by `slot`.
     * It uses the OpenGL function `glBindTextureUnit` to perform the binding.
     *
     * @param slot The texture unit slot to which the texture will be bound.
     * @param textureID The OpenGL texture ID to bind to the specified slot.
     */
    void GL_BindTextureUnit(uint32_t slot, TextureID textureID)
    {
        glBindTextureUnit(slot, textureID);
    }

    /**
     * @brief Unbinds any texture from texture unit 0 in OpenGL.
     *
     * This function calls glBindTextureUnit with unit 0 and texture 0,
     * effectively unbinding any texture that was previously bound to texture unit 0.
     * Useful for resetting texture state and avoiding unintended texture usage.
     */
    void GL_UnbindTextureUnit()
    {
        glBindTextureUnit(0, 0);
    }

    /**
     * @brief Retrieves the maximum number of texture slots available in OpenGL.
     *
     * This function queries OpenGL for the maximum number of combined texture image units
     * that can be used in a single rendering pass. It is useful for determining how many textures
     * can be bound simultaneously.
     *
     * @return The maximum number of texture slots available.
     */
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

