#pragma once

#include <filesystem>

#include "Buffers.hpp"
#include "Environment.hpp"

namespace Motion
{
    class GL_Environment : public IEnvironment
    {
    public:
        explicit GL_Environment(const std::filesystem::path& hdrFile);
        ~GL_Environment() override;

        // IEnvironment
        void BindCubeTexture(std::uint32_t slot) const noexcept override;
        void BindBRDFLUTTexture(std::uint32_t slot) const noexcept override;
        void BindPrefilteredTexture(std::uint32_t slot) const noexcept override;
        void BindIrradianceTexture(std::uint32_t slot) const noexcept override;
        void BindIBLAll(std::uint32_t irrSlot, std::uint32_t preSlot, std::uint32_t brdfSlot) const noexcept override;
        void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix) noexcept override;

        void SetIntensity(const EnvIntensity& i) noexcept override { m_Intensity = i; }
        EnvIntensity GetIntensity() const noexcept override { return m_Intensity; }
        std::int32_t GetMipLevel() const noexcept override { return m_MipLevel; }

        void SetSkyboxRotationY(float radians) noexcept override { m_SkyboxRotationY = radians; }
        float GetSkyboxRotationY() const noexcept override { return m_SkyboxRotationY; }

        [[nodiscard]] TextureID GetEnvironmentCubeTexture() const noexcept override { return m_EnvironmentCubeTextureID; }
        [[nodiscard]] TextureID GetPrefilteredTexture() const noexcept override { return m_PrefilteredTextureID; }
        [[nodiscard]] TextureID GetIrradianceTexture()  const noexcept override { return m_IrradianceTextureID; }
        [[nodiscard]] TextureID GetBRDFLUTTexture()     const noexcept override { return m_BRDFLUTTextureID; }

    private:
        BufferID m_FrameBufferID{ 0 };
        BufferID m_RenderBufferID{ 0 };

        TextureID m_EnvironmentCubeTextureID{ 0 };
        TextureID m_PrefilteredTextureID{ 0 };
        TextureID m_IrradianceTextureID{ 0 };
        TextureID m_BRDFLUTTextureID{ 0 };

        std::shared_ptr<GL_Shader> m_EnvironmentShader{ nullptr };
        std::shared_ptr<GL_Shader> m_CubeConvertShader{ nullptr };
        std::shared_ptr<GL_Shader> m_PrefilteredShader{ nullptr };
        std::shared_ptr<GL_Shader> m_IrradianceShader{ nullptr };
        std::shared_ptr<GL_Shader> m_BRDFShader{ nullptr };

        EnvIntensity m_Intensity{};
        float m_SkyboxRotationY{ 0.0f };
        std::int32_t m_MipLevel{ 0 };
    };
}