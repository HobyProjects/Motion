#pragma once

#include <cstdint>
#include <glm/glm.hpp>

#include "Base.hpp"
#include "Window.hpp"
#include "Texture.hpp"
#include "RenderCommand.hpp"

namespace Motion 
{

    enum class RenderingAPI : std::uint32_t 
    {
        OpenGL  = BIT(1),
        Vulkan  = BIT(2),
        DirectX = BIT(3)
    };

    template<>
    struct enable_bitmask_operations<RenderingAPI> : std::true_type {};

    enum class PrimitiveTopology : std::uint8_t 
    {
        Triangles,
        Lines,
        Points,
        TriangleStrip,
        LineStrip,
        Patches       
    };

    enum class IndexType : std::uint8_t 
    {
        UInt16,
        UInt32
    };

    struct DrawIndexedArgs 
    {
        int                 indexCount         = 0;
        int                 instanceCount      = 1;
        int                 firstIndex         = 0;   
        int                 baseVertex         = 0;   
        int                 baseInstance       = 0;   
        IndexType           indexType          = IndexType::UInt32;
        PrimitiveTopology   topology           = PrimitiveTopology::Triangles;
        int                 patchControlPoints = 3;   
    };

    struct ClearParams 
    {
        bool        color           = true;
        bool        depth           = true;
        bool        stencil         = false;
        glm::vec4   colorValue      = glm::vec4(0,0,0,1);
        float       depthValue      = 1.0f;
        std::int32_t stencilValue   = 0;
    };
    struct GPUCaptures 
    {
        int  maxCombinedTextureUnits = 0;
        int  maxPatchVertices        = 0;
        int  glMajor                 = 0;
        int  glMinor                 = 0;
        bool khrDebug                = false;
    };

    class Renderer 
    {
        public:
            static void Init();
            static void Quit();

            static void Clear();                                  
            static void ClearColor(const glm::vec4& color);       
            static void Clear(const ClearParams& p);               

            static void SetViewport(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);

            static void DrawArrays(PrimitiveTopology topology, std::uint32_t count);
            static void DrawIndexed(std::int32_t indicesCount);   
            static void DrawIndexed(const DrawIndexedArgs& args);

            static void Begin();
            static void End();
            static void Submit(const RenderCommand& command); 
            static void Flush();

            static std::int32_t GetMaxTextureSlots() noexcept;
            static void BindTextureUnit(std::int32_t slot, std::uint32_t textureID);
            static void UnbindTextureUnit(std::int32_t slot);

            static void PushDebugGroup(const char* label);
            static void PopDebugGroup();

            static const GPUCaptures& Caps();
            [[nodiscard]] static RenderingAPI GetAPI() noexcept;

        private:
            static void QueryCaps_();
    };
} 
