#pragma once

#include <array>
#include <mutex>
#include <atomic>
#include <vector>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <initializer_list>

#include "Shaders.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

namespace Motion::Core
{
    class Renderer; // forward declaration

    enum class RenderPass : std::uint8_t
    {
        Opaque = 0,
        Transparent = Bits<1>::value,
        PostProcessing = Bits<2>::value,
        Shadow = Bits<3>::value
    };

    inline std::uint8_t operator|(RenderPass a, RenderPass b) { return static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b); }
    inline std::uint8_t operator&(RenderPass a, RenderPass b) { return static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b); }

    enum DrawFlags : std::uint8_t
    {
        None = 0,
        SkipDepthWrite = Bits<1>::value,
        Wireframe = Bits<2>::value,
        Instanced = Bits<3>::value, // Note: Instancing is not yet implemented
    };

    inline std::uint8_t operator|(DrawFlags a, DrawFlags b) { return static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b); }
    inline std::uint8_t operator&(DrawFlags a, DrawFlags b) { return static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b); }

    struct DrawCommand
    {
        UUID SortKey{ 0 };
        UUID MaterialID{ 0 };
        UUID MeshID{ 0 };
        glm::mat4 ModelMatrix{ 1.0f };
        glm::mat4 ViewProjectionMatrix{ 1.0f };
        RenderPass RenderPass{ RenderPass::Opaque };
        DrawFlags Flags{ DrawFlags::None };

        bool operator<(const DrawCommand& other) const
        {
            return std::tie(SortKey, MaterialID, MeshID) <
                std::tie(other.SortKey, other.MaterialID, other.MeshID);
        }
    };

    class DrawCommandQueue
    {
    public:
        void Submit(const DrawCommand& drawCommand);
        std::vector<DrawCommand>& Consume();
        void SwapBuffers();

    private:
        std::vector<DrawCommand> m_Buffers[2];
        std::atomic<std::uint32_t> m_WriteIndex = 0;
        std::mutex m_SubmitMutex;

        friend class Renderer; // Allow Renderer to access private members
    };



}