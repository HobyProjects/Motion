#pragma once

#include "Texture.hpp"

namespace Motion
{
    struct EnvironmentIntensity
    {
        float Diffuse = 1.0f;
        float Specular = 1.0f;
    };

    class IEnvironment
    {
    public:
        virtual ~IEnvironment() = default;

        virtual void BindCubeTexture(std::uint32_t slot) const noexcept = 0;
        virtual void BindBRDFLUTTexture(std::uint32_t slot) const noexcept = 0;
        virtual void BindPrefilteredTexture(std::uint32_t slot) const noexcept = 0;
        virtual void BindIrradianceTexture(std::uint32_t slot) const noexcept = 0;

        virtual void BindAll(std::uint32_t irrSlot, std::uint32_t preSlot, std::uint32_t brdfSlot) const noexcept = 0;
        virtual void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix) noexcept = 0;

        virtual void SetIntensity(const EnvironmentIntensity& i) noexcept = 0;
        virtual EnvironmentIntensity GetIntensity() const noexcept = 0;
        virtual std::int32_t GetMipLevel() const noexcept = 0;

        virtual void  SetSkyboxRotationY(float radians) noexcept = 0;
        virtual float GetSkyboxRotationY() const noexcept = 0;

        [[nodiscard]] virtual TextureID GetEnvironmentCubeTexture() const noexcept = 0;
        [[nodiscard]] virtual TextureID GetPrefilteredTexture() const noexcept = 0;
        [[nodiscard]] virtual TextureID GetIrradianceTexture()  const noexcept = 0;
        [[nodiscard]] virtual TextureID GetBRDFLUTTexture()     const noexcept = 0;

        static std::shared_ptr<IEnvironment> Create(const std::filesystem::path& hdrFile);
    };
}