#pragma once

#include <array>
#include <functional>
#include <initializer_list>

#include "Shaders.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

namespace Motion::Core
{
    enum class RenderPass : uint32_t
    {
        Opaque              = 0,
        Transparent         = 1,
        PostProcessing      = 2,
        Shadow              = 3
    };

    enum class RendererCallbackOrder : uint32_t
    {
        FromBeginning           = 0, // Trigger the callback before bind in to shader, materials or mesh
        AfterShaderBinding      = 1, // Trigger the callback after bind in to shader
        AfterMaterialBinding    = 2, // Trigger the callback after bind in to material
        AfterMeshBinding        = 3, // Trigger the callback after bind in to mesh
        AfterDrawCall           = 4  // Trigger the callback after draw call
    };

    constexpr size_t MAX_CALLBACKS = 5;
    using RendererCallbackFunction = std::function<void()>;

    inline uint32_t operator|(RenderPass a, RenderPass b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(RenderPass a, RenderPass b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }
    inline uint32_t operator^(RenderPass a, RenderPass b) { return static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b); }
    inline uint32_t operator~(RenderPass a) { return ~static_cast<uint32_t>(a); }

    struct DrawCommand
    {
        std::shared_ptr<IShader> Shader{nullptr};
        std::shared_ptr<Mesh> SubMesh{nullptr};
        std::shared_ptr<Material> MeshMaterial{nullptr};
        glm::mat4 ModelTransform{1.0f};
        glm::mat4 CameraMatrix{1.0f};
        RenderPass RendererPasses{RenderPass::Opaque};

        // Efficient, grouped callback storage
        std::array<std::vector<RendererCallbackFunction>, MAX_CALLBACKS> CallbackTable;

        void InvokeCallback(RendererCallbackOrder order) const
        {
            if (CallbackTable[static_cast<size_t>(order)].empty())
                return;

            const auto& callbacks = CallbackTable[static_cast<size_t>(order)];
            for (const auto& fn : callbacks)
                fn();
        }

        void AddCallback(RendererCallbackOrder order, RendererCallbackFunction fn)
        {
            if (CallbackTable[static_cast<size_t>(order)].size() >= MAX_CALLBACKS)
            {
                MOTION_ASSERT(false, "Max number of callbacks reached!");
                return;
            }
            
            CallbackTable[static_cast<size_t>(order)].emplace_back(std::move(fn));
        }

        bool operator<(const DrawCommand& other) const
        {
            return std::tie(Shader, SubMesh, MeshMaterial, RendererPasses) <
                   std::tie(other.Shader, other.SubMesh, other.MeshMaterial, other.RendererPasses);
        }

        bool HasCallback(RendererCallbackOrder order) const
        {
            return !CallbackTable[static_cast<size_t>(order)].empty();
        }
    };
}