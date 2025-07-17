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

    struct DrawCommand
    {
        UUID SortKey{ 0 };
        UUID MaterialID{ 0 };
        UUID MeshID{ 0 };
        glm::mat4 ModelMatrix{ 1.0f };
        glm::mat4 ViewProjectionMatrix{ 1.0f };

        bool operator<(const DrawCommand& other) const
        {
            return std::tie(SortKey, MaterialID, MeshID) <
                std::tie(other.SortKey, other.MaterialID, other.MeshID);
        }
    };

    struct FrameDrawCommand
    {
        UUID SortKey{ 0 };
        UUID MeshID{ 0 };
        TextureID TextureID{ 0 };

        bool operator<(const FrameDrawCommand& other) const
        {
            return std::tie(SortKey, MeshID, TextureID) <
                std::tie(other.SortKey, other.MeshID, other.TextureID);
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