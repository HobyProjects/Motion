#pragma once

#include <filesystem>

#include "Buffers.hpp"
#include "Texture.hpp"
#include "Environment.hpp"

namespace Motion
{
    class GL_Environment final : public IEnvironment
    {
        public:
            GL_Environment(const EnvironmentSpecification& spec);
            virtual ~GL_Environment();

            virtual void RenderSkyBox(const glm::mat4& proj, const glm::mat4& view, float yawnRadians = 0.0f) const override;
            virtual void BindIBL(const IBLTextureBinding& params) override;
            virtual void SetIntensity(float diffuse, float specular) override;

            [[nodiscard]] virtual SH9& GetDiffuseSH() override { return m_SH9Diffuse; }
            [[nodiscard]] virtual bool IsUsingSH() override { return m_Specification.UseSHDiffuse; }

        protected:
            virtual void BakeHDR() override;

        private:
            TextureID m_EnvironmentCube{0};
            TextureID m_PrefilterdCube{0};
            TextureID m_IrradianceCube{0};
            TextureID m_BRDFLUT{0};
            TextureID m_HDR{0};

            BufferID m_FrameBuffer{0};
            BufferID m_RenderBuffer{0};
            BufferID m_SHUBO{0};

            std::shared_ptr<GL_Shader> m_SH_EquirectToCube{nullptr};
            std::shared_ptr<GL_Shader> m_SH_Irradiance{nullptr};
            std::shared_ptr<GL_Shader> m_SH_Prefilter{nullptr};
            std::shared_ptr<GL_Shader> m_SH_BRDFLUT{nullptr};
            std::shared_ptr<GL_Shader> m_SH_SkyBox{nullptr};

            EnvironmentSpecification m_Specification{};

            SH9 m_SH9Diffuse{};
            bool m_SHReady{false};
            std::array<glm::vec4, 9> m_SHPacked{};
    };
}