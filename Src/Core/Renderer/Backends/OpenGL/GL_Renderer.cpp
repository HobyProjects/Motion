#include "CorePCH.hpp"
#include "GL_Renderer.hpp"

namespace Motion 
{
    /**
     * @brief Convert a PrimitiveTopology enum to a GLenum.
     *
     * This function takes in an IndexType enum and returns the corresponding
     * GLenum that can be used in OpenGL functions.
     *
     * @param[in] t The IndexType enum to convert.
     *
     * @return The corresponding GLenum.
     */
    GLenum ToGL(IndexType t) 
    {
        return (t == IndexType::UInt16) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    }

    /**
     * @brief Convert a PrimitiveTopology enum to a GLenum.
     *
     * @param[in] topo The PrimitiveTopology enum to convert.
     *
     * @return The corresponding GLenum value.
     */
    GLenum ToGL(PrimitiveTopology topo) 
    {
        switch (topo) 
        {
            case PrimitiveTopology::Triangles:      return GL_TRIANGLES;
            case PrimitiveTopology::Lines:          return GL_LINES;
            case PrimitiveTopology::Points:         return GL_POINTS;
            case PrimitiveTopology::TriangleStrip:  return GL_TRIANGLE_STRIP;
            case PrimitiveTopology::LineStrip:      return GL_LINE_STRIP;
            case PrimitiveTopology::Patches:        return GL_PATCHES;
        }

        return GL_TRIANGLES;
    }

    /**
     * @brief Initializes the OpenGL state.
     *
     * This function enables depth testing, disables face culling, sets the polygon mode to fill, sets the blend function to source alpha and one minus source alpha, and enables texture cube mapping.
     * If the GL_KHR_debug extension is available, it also enables debug output and sets up filters to ignore low severity messages and notifications.
     */
    void GL_Init()
    {
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);               
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);           
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

#ifdef MOTION_BUILD_DEBUG
        if (GLAD_GL_KHR_debug) 
        {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(GL_MessageCallBack, nullptr);

            glDebugMessageControl(
                GL_DONT_CARE,                // source
                GL_DONT_CARE,                // type
                GL_DONT_CARE,                // severity (we'll filter below)
                0, nullptr,                  // IDs
                GL_TRUE                      // enable all first
            );
            
            glDebugMessageControl(
                GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
                0, nullptr, GL_FALSE         // turn off notifications
            );

            glDebugMessageControl(
                GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW,
                0, nullptr, GL_FALSE         // turn off low severity
            );

        }
#endif
    }

    void GL_Quit() {}

    /**
     * @brief Clears the color buffer to the specified color.
     *
     * Clears the color buffer to the specified color.
     */
    void GL_Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    /**
     * @brief Clears the color buffer to the specified color.
     *
     * Clears the color buffer to the specified color.
     *
     * @param color The color to clear the color buffer to.
     */
    void GL_ClearColor(const glm::vec4& color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    /**
     * @brief Clears the OpenGL buffers specified by the ClearParams object.
     *
     * Clears the buffers specified in the ClearParams object.
     * If the color buffer is specified, it is cleared to the specified color.
     * If the depth buffer is specified, it is cleared to the specified depth value.
     * If the stencil buffer is specified, it is cleared to the specified stencil value.
     *
     * @param p The ClearParams object specifying which buffers to clear and their values.
     */
    void GL_ClearEx(const ClearParams& p)
    {
        GLbitfield mask = 0;
        if (p.color)   mask |= GL_COLOR_BUFFER_BIT;
        if (p.depth)   mask |= GL_DEPTH_BUFFER_BIT;
        if (p.stencil) mask |= GL_STENCIL_BUFFER_BIT;

        if (p.color)   glClearColor(p.colorValue.r, p.colorValue.g, p.colorValue.b, p.colorValue.a);
        if (p.depth)   glClearDepth(p.depthValue);
        if (p.stencil) glClearStencil(p.stencilValue);

        glClear(mask);
    }

    /**
     * @brief Sets the viewport for OpenGL rendering.
     *
     * @param x The x-coordinate of the bottom left corner of the viewport.
     * @param y The y-coordinate of the bottom left corner of the viewport.
     * @param width The width of the viewport.
     * @param height The height of the viewport.
     */
    void GL_SetViewport(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height)
    {
        glViewport(x, y, width, height);
    }

    /**
     * @brief Draws non-indexed primitives with the given topology and count.
     *
     * This function draws non-indexed primitives with the given topology and count.
     * The first vertex is assumed to be 0.
     *
     * @param topology The topology of the primitives to draw.
     * @param count The number of primitives to draw.
     */
    void GL_DrawArrays(PrimitiveTopology topology, std::uint32_t count)
    {
        glDrawArrays(ToGL(topology), 0, count);
    }

    /**
     * @brief Draws indexed primitives with the given topology and index count.
     *
     * @param indicesCount The number of indices to draw.
     *
     * This function draws the indexed primitives with the given topology and index count.
     * The index type is assumed to be `GL_UNSIGNED_INT` and the first index is assumed to be 0.
     * The instance count, base vertex and base instance are all assumed to be 0.
     */
    void GL_DrawIndexed(std::int32_t indicesCount)
    {
        glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, nullptr);
    }

    /**
     * @brief Draws indexed primitives with optional instance count, base vertex and base instance.
     *
     * Given the DrawIndexedArgs struct, this function will draw the indexed primitives with the given topology, index count, index type, first index, instance count, base vertex and base instance.
     *
     * The function will automatically switch between different draw functions based on the given parameters.
     *
     * @param a The DrawIndexedArgs struct containing the parameters for the draw call.
     */
    void GL_DrawIndexed(const DrawIndexedArgs& a)
    {
        if (a.topology == PrimitiveTopology::Patches) 
        {
            glPatchParameteri(GL_PATCH_VERTICES, a.patchControlPoints);
        }

        const GLvoid* indexOffset = reinterpret_cast<const void*>(static_cast<uintptr_t>(a.firstIndex) * (a.indexType == IndexType::UInt16 ? 2u : 4u));

#if defined(GL_ARB_base_instance) || defined(GL_VERSION_4_2)
        if (a.instanceCount > 1 || a.baseVertex != 0 || a.baseInstance != 0) 
        {
            glDrawElementsInstancedBaseVertexBaseInstance(
                ToGL(a.topology), a.indexCount, ToGL(a.indexType), indexOffset,
                a.instanceCount, a.baseVertex, a.baseInstance);
            return;
        }
#endif

#if defined(GL_ARB_draw_elements_base_vertex) || defined(GL_VERSION_3_2)
        if (a.baseVertex != 0) {
            glDrawElementsBaseVertex(ToGL(a.topology), a.indexCount, ToGL(a.indexType),
                                     indexOffset, a.baseVertex);
            return;
        }
#endif

#if defined(GL_ARB_instanced_arrays) || defined(GL_VERSION_3_3)
        if (a.instanceCount > 1) {
            glDrawElementsInstanced(ToGL(a.topology), a.indexCount, ToGL(a.indexType),
                                    indexOffset, a.instanceCount);
            return;
        }
#endif
        glDrawElements(ToGL(a.topology), a.indexCount, ToGL(a.indexType), indexOffset);
    }

    /**
     * @brief Bind a texture unit
     *
     * Bind a texture unit by binding a texture ID to the given slot.
     *
     * @param slot The texture unit to bind
     * @param textureID The texture ID to bind to the given slot
     */
    void GL_BindTextureUnit(std::int32_t slot, std::uint32_t textureID)
    {
        glBindTextureUnit(slot, textureID);
    }

    /**
     * @brief Unbind a texture unit
     *
     * Unbinds a texture unit by binding a texture ID of 0 to the given slot.
     *
     * @param slot The texture unit to unbind
     */
    void GL_UnbindTextureUnit(std::int32_t slot)
    {
        glBindTextureUnit(slot, 0);
    }

    /**
     * @brief Get the maximum number of texture slots available
     *
     * @return The maximum number of texture slots available
     *
     * This function returns the maximum number of texture slots available, which is
     * the value of GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS. If the value is not
     * cached, it is retrieved from OpenGL with glGetIntegerv.
     */
    std::int32_t GL_GetMaxTextureSlots() noexcept
    {
        static GLint maxUnits = 0;
        if (maxUnits == 0) 
        {
            glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
        }

        return maxUnits;
    }


    /**
     * Pushes a debug group onto the OpenGL debug stack.
     *
     * If the OpenGL context supports the GL_KHR_debug extension and the
     * given label is not null, this function pushes a debug group onto
     * the OpenGL debug stack with the given label. Otherwise, this
     * function does nothing.
     *
     * @param label The label to associate with the debug group.
     */
    void GL_PushDebugGroup(const char* label)
    {
#ifdef MOTION_BUILD_DEBUG
        if (GLAD_GL_KHR_debug && label) 
        {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, label);
        }
#endif
    }

    /**
     * Pops the current debug group from the OpenGL debug stack.
     *
     * This function is only available if the OpenGL context supports the
     * GL_KHR_debug extension. If this extension is not supported, this
     * function does nothing.
     *
     * @see https://www.khronos.org/registry/extensions/EXT/WGL_debug_group.txt
     * @see https://www.khronos.org/registry/extensions/KHR/KHR_debug.txt
     */
    void GL_PopDebugGroup()
    {
#ifdef MOTION_BUILD_DEBUG
        if (GLAD_GL_KHR_debug) glPopDebugGroup();
#endif
    }

    /**
     * Queries the current OpenGL context for various capabilities and
     * stores the results in the provided GPUCaptures object.
     *
     * The queried capabilities are as follows:
     *   - maxCombinedTextureUnits: The maximum number of texture units that can be combined
     *     in a single shader stage.
     *   - glMajor: The major version number of the OpenGL API.
     *   - glMinor: The minor version number of the OpenGL API.
     *   - maxPatchVertices: The maximum number of vertices that can be used in a single
     *     tessellation control shader patch.
     *   - khrDebug: Whether the current OpenGL context supports the GL_KHR_debug extension.
     *
     * @param outCaps The object to store the queried capabilities in.
     */
    void GL_QueryCaps(GPUCaptures& outCaps)
    {
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &outCaps.maxCombinedTextureUnits);
        glGetIntegerv(GL_MAJOR_VERSION, &outCaps.glMajor);
        glGetIntegerv(GL_MINOR_VERSION, &outCaps.glMinor);
        outCaps.maxPatchVertices = 0;

#ifdef GL_MAX_PATCH_VERTICES
        if (GLAD_GL_VERSION_4_0 || GLAD_GL_ARB_tessellation_shader) 
        {
            glGetIntegerv(GL_MAX_PATCH_VERTICES, &outCaps.maxPatchVertices);
        }
#endif

#ifdef MOTION_BUILD_DEBUG
        outCaps.khrDebug = GLAD_GL_KHR_debug != 0;
#else
        outCaps.khrDebug = false;
#endif
    }

}
