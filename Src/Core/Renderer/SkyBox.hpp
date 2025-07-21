#pragma once

#include "Shaders.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"

namespace Motion::Core
{
    class SkyBox
    {
    public:
        SkyBox() = default;
        ~SkyBox() = default;

        void Load(const std::filesystem::path& textureFile) noexcept;
        void Render(const glm::mat4& viewMatrix, const glm::mat4 projectionMatrix) noexcept;

    private:
    };
}