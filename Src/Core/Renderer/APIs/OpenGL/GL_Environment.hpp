#pragma once

#include <filesystem>

#include "Buffers.hpp"
#include "Environment.hpp"

namespace Motion
{
    class GL_Environment : public IEnvironment
    {
    public:
        GL_Environment(const std::filesystem::path& hdrFile);
        virtual ~GL_Environment();

        virtual void BindCubeTexture(std::uint32_t slot) const noexcept override;
        virtual void BindBRDFLUTTexture(std::uint32_t slot) const noexcept override;
        virtual void BindPrefilteredTexture(std::uint32_t slot) const noexcept override;
        virtual void BindIrradianceTexture(std::uint32_t slot) const noexcept override;
        virtual void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix) noexcept override;

        [[nodiscard]] virtual TextureID GetEnvironmentCubeTexture() const noexcept override { return m_EnvironmentCubeTextureID; }
        [[nodiscard]] virtual TextureID GetPrefilteredTexture() const noexcept override { return m_PrefilteredTextureID; }
        [[nodiscard]] virtual TextureID GetIrradianceTexture() const noexcept override { return m_IrradianceTextureID; }
        [[nodiscard]] virtual TextureID GetBRDFLUTTexture() const noexcept override { return m_BRDFLUTTextureID; }

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
    };
}