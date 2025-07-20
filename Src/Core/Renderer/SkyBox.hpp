#pragma once

#include "Shaders.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"

namespace Motion::Core
{
    class ISkyBox
    {
    public:
        ISkyBox() = default;
        ~ISkyBox() = default;

        virtual void Load(const std::filesystem::path& textureFile) noexcept = 0;
        virtual void Render(const glm::mat4& viewMatrix, const glm::mat4 projectionMatrix) noexcept = 0;
    };
}