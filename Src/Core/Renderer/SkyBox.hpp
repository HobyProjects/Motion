#pragma once

#include "Shaders.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"

namespace Motion
{
    class SkyBox
    {
    private:
        SkyBox() = default;
        ~SkyBox() = default;

        SkyBox(const SkyBox&) = delete;
        SkyBox& operator=(const SkyBox&) = delete;
        SkyBox(SkyBox&&) = delete;
        SkyBox& operator=(SkyBox&&) = delete;

    public:
        static void Init() noexcept;
        static void Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) noexcept;

        [[nodiscard]] static std::shared_ptr<ICubeMapTexture> GetTexture() noexcept;
    };
}