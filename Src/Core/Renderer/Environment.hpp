#pragma once

#include "Texture.hpp"

namespace Motion
{
    class IEnvironment
    {
    public:
        IEnvironment() = default;
        virtual ~IEnvironment() = default;

        virtual void BindCubeTexture(std::uint32_t slot) const noexcept = 0;
        virtual void BindBRDFLUTTexture(std::uint32_t slot) const noexcept = 0;
        virtual void BindPrefilteredTexture(std::uint32_t slot) const noexcept = 0;
        virtual void BindIrradianceTexture(std::uint32_t slot) const noexcept = 0;
        virtual void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix) noexcept = 0;

        [[nodiscard]] virtual TextureID GetEnvironmentCubeTexture() const noexcept = 0;
        [[nodiscard]] virtual TextureID GetPrefilteredTexture() const noexcept = 0;
        [[nodiscard]] virtual TextureID GetIrradianceTexture() const noexcept = 0;
        [[nodiscard]] virtual TextureID GetBRDFLUTTexture() const noexcept = 0;

        static std::shared_ptr<IEnvironment> Create(const std::filesystem::path& hdrFile);
    };
}