#include "CorePCH.hpp"
#include "GL_Renderer.hpp"

namespace Motion 
{
    GLenum ToGL(IndexType t) 
    {
        return (t == IndexType::UInt16) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    }

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

    void GL_Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GL_ClearColor(const glm::vec4& color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

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

    void GL_SetViewport(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height)
    {
        glViewport(x, y, width, height);
    }

    void GL_DrawArrays(PrimitiveTopology topology, std::uint32_t count)
    {
        glDrawArrays(ToGL(topology), 0, count);
    }

    void GL_DrawIndexed(std::int32_t indicesCount)
    {
        glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, nullptr);
    }

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
        static GLint maxUnits = 0;
        if (maxUnits == 0) 
        {
            glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
        }

        return maxUnits;
    }


    void GL_PushDebugGroup(const char* label)
    {
#ifdef MOTION_BUILD_DEBUG
        if (GLAD_GL_KHR_debug && label) 
        {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, label);
        }
#endif
    }

    void GL_PopDebugGroup()
    {
#ifdef MOTION_BUILD_DEBUG
        if (GLAD_GL_KHR_debug) glPopDebugGroup();
#endif
    }

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
