#pragma once

#include "Shaders.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

namespace Motion::Core
{
    enum class RenderPass 
    {
        Opaque,
        Transparent,
        Shadow
    };

    struct DrawCommand
    {
        std::shared_ptr<IShader> shader;
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;
        RenderPass renderPass;
        glm::mat4 modelMatrix;

        bool operator<(const DrawCommand& other) const
        {
            return std::tie(shader, mesh, material, renderPass) < std::tie(other.shader, other.mesh, other.material, other.renderPass);
        }
    };
}