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
        std::array<std::vector<RendererCallbackFunction>, MAX_CALLBACKS> CallbackSlots;
        std::unordered_map<std::string, UniformVariant> CustomUniforms{};
        std::shared_ptr<Material> MaterialRef{nullptr};
        std::shared_ptr<IShader> ShaderRef{nullptr};
        std::shared_ptr<Mesh> MeshRef{nullptr};
        glm::mat4 ModelMatrix{1.0f};
        glm::mat4 ViewProjMatrix{1.0f};
        RenderPass RenderPassMask{RenderPass::Opaque};

        void SetUniform(const std::string& name, float value) { CustomUniforms[name] = value; }
        void SetUniform(const std::string& name, int32_t value) { CustomUniforms[name] = value; }
        void SetUniform(const std::string& name, uint32_t value) { CustomUniforms[name] = value; }
        void SetUniform(const std::string& name, glm::vec2 value) { CustomUniforms[name] = value; }
        void SetUniform(const std::string& name, glm::vec3 value) { CustomUniforms[name] = value; }
        void SetUniform(const std::string& name, glm::vec4 value) { CustomUniforms[name] = value; }
        void SetUniform(const std::string& name, glm::mat2 value) { CustomUniforms[name] = value; }
        void SetUniform(const std::string& name, glm::mat3 value) { CustomUniforms[name] = value; }
        void SetUniform(const std::string& name, glm::mat4 value) { CustomUniforms[name] = value; }

        void InvokeCallback(RendererCallbackOrder order) const
        {
            if (CallbackSlots[static_cast<size_t>(order)].empty())
                return;

            const auto& callbacks = CallbackSlots[static_cast<size_t>(order)];
            for (const auto& fn : callbacks)
                fn();
        }

        void AddCallback(RendererCallbackOrder order, RendererCallbackFunction fn)
        {
            if (CallbackSlots[static_cast<size_t>(order)].size() >= MAX_CALLBACKS)
            {
                MOTION_ASSERT(false, "Max number of callbacks reached!");
                return;
            }
            
            CallbackSlots[static_cast<size_t>(order)].emplace_back(std::move(fn));
        }

        bool operator<(const DrawCommand& other) const
        {
            return std::tie(ShaderRef, MeshRef, MaterialRef, RenderPassMask) <
                   std::tie(other.ShaderRef, other.MeshRef, other.MaterialRef, other.RenderPassMask);
        }

        bool HasCallback(RendererCallbackOrder order) const
        {
            return !CallbackSlots[static_cast<size_t>(order)].empty();
        }

        DrawCommand() = default;
        ~DrawCommand() = default;
    };
}