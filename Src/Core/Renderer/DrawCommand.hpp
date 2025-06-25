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
        Shadow
    };

    inline uint32_t operator|(RenderPass a, RenderPass b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(RenderPass a, RenderPass b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }
    inline uint32_t operator^(RenderPass a, RenderPass b) { return static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b); }
    inline uint32_t operator~(RenderPass a) { return ~static_cast<uint32_t>(a); }

    struct DrawCommand
    {
        std::shared_ptr<IShader> Shader;
        std::shared_ptr<Mesh> SubMesh;
        std::shared_ptr<Material> MeshMaterial;
        RenderPass RendererPasses;
        glm::mat4 ModelTransform;
        glm::mat4 CameraMatrix;

        bool operator<(const DrawCommand& other) const
        {
            return std::tie(Shader, SubMesh, MeshMaterial, RendererPasses) < std::tie(other.Shader, other.SubMesh, other.MeshMaterial, other.RendererPasses);
        }
    };
}