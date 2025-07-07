#pragma once

#include "Shaders.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

namespace Motion::Core
{
    enum class RenderPass : uint32_t
    {
        Opaque = 0,
        Transparent,
        PostProcessing,
        Shadow
    };

    inline uint32_t operator|(RenderPass a, RenderPass b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(RenderPass a, RenderPass b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }
    inline uint32_t operator^(RenderPass a, RenderPass b) { return static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b); }
    inline uint32_t operator~(RenderPass a) { return ~static_cast<uint32_t>(a); }

    struct DrawCommand
    {
        std::shared_ptr<IShader> Shader{nullptr};
        std::shared_ptr<Mesh> SubMesh{nullptr};
        std::shared_ptr<Material> MeshMaterial{nullptr};
        std::shared_ptr<IFrameTexture> FrameTexture{nullptr};
        RenderPass RendererPasses{RenderPass::Opaque};
        glm::mat4 ModelTransform{1.0f};
        glm::mat4 CameraMatrix{1.0f};

        bool operator<(const DrawCommand& other) const
        {
            return std::tie(Shader, SubMesh, MeshMaterial, RendererPasses) < std::tie(other.Shader, other.SubMesh, other.MeshMaterial, other.RendererPasses);
        }
    };
}