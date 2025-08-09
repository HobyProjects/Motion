#pragma once

#include "Texture.hpp"

namespace Motion
{
    struct EnvIntensity { float Diffuse = 1.0f; float Specular = 1.0f; };

    class IEnvironment
    {
    public:
        virtual ~IEnvironment() = default;

        // Bind IBL resources
        virtual void BindCubeTexture(std::uint32_t slot) const noexcept = 0;      // environment skybox cube (background)
        virtual void BindBRDFLUTTexture(std::uint32_t slot) const noexcept = 0;   // 2D LUT
        virtual void BindPrefilteredTexture(std::uint32_t slot) const noexcept = 0; // prefiltered specular cube
        virtual void BindIrradianceTexture(std::uint32_t slot) const noexcept = 0;  // diffuse irradiance cube

        // Convenience: bind all IBL inputs in one call
        virtual void BindIBLAll(std::uint32_t irrSlot,
            std::uint32_t preSlot,
            std::uint32_t brdfSlot) const noexcept = 0;

        // Skybox (background) render hook
        virtual void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix) noexcept = 0;

        // Intensity knobs for IBL balance (read by renderer to pass to shader)
        virtual void         SetIntensity(const EnvIntensity& i) noexcept = 0;
        virtual EnvIntensity GetIntensity() const noexcept = 0;

        // Optional skybox rotation (Y axis)
        virtual void  SetSkyboxRotationY(float radians) noexcept = 0;
        virtual float GetSkyboxRotationY() const noexcept = 0;

        // Raw handles (useful for debug views)
        [[nodiscard]] virtual TextureID GetEnvironmentCubeTexture() const noexcept = 0;
        [[nodiscard]] virtual TextureID GetPrefilteredTexture() const noexcept = 0;
        [[nodiscard]] virtual TextureID GetIrradianceTexture()  const noexcept = 0;
        [[nodiscard]] virtual TextureID GetBRDFLUTTexture()     const noexcept = 0;

        // Factory
        static std::shared_ptr<IEnvironment> Create(const std::filesystem::path& hdrFile);
    };
}