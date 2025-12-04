#pragma once

#include "PostProcessing.hpp"
#include "Shaders.hpp"
#include <memory>

namespace Motion
{
    // Base class for OpenGL post-processing effects
    class GL_PostProcessEffect : public IPostProcessEffect
    {
    public:
        GL_PostProcessEffect(PostProcessEffectType type);
        virtual ~GL_PostProcessEffect() = default;

        PostProcessEffectType GetType() const override { return m_Type; }
        bool IsEnabled() const override { return m_Enabled; }
        void SetEnabled(bool enabled) override { m_Enabled = enabled; }

    protected:
        PostProcessEffectType m_Type;
        bool m_Enabled{true};
        std::shared_ptr<IShader> m_Shader;
    };

    // ToneMapping Effect
    class GL_ToneMappingEffect : public GL_PostProcessEffect
    {
    public:
        GL_ToneMappingEffect();
        ~GL_ToneMappingEffect() = default;

        void Init() override;
        void Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer) override;
        void Resize(std::int32_t width, std::int32_t height) override;
        void* GetConfig() override { return &m_Config; }

    private:
        ToneMappingConfig m_Config;
    };

    // Bloom Effect
    class GL_BloomEffect : public GL_PostProcessEffect
    {
    public:
        GL_BloomEffect();
        ~GL_BloomEffect() = default;

        void Init() override;
        void Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer) override;
        void Resize(std::int32_t width, std::int32_t height) override;
        void* GetConfig() override { return &m_Config; }

    private:
        void ExtractBrightness(FrameTextureID inputTexture);
        void BlurBrightness();
        void Composite(FrameTextureID sceneTexture, IFrameBuffer* outputFrameBuffer);

    private:
        BloomConfig m_Config;
        std::shared_ptr<IShader> m_BrightnessShader;
        std::shared_ptr<IShader> m_BlurShader;
        std::shared_ptr<IShader> m_CompositeShader;
        std::vector<std::shared_ptr<IFrameBuffer>> m_BlurBuffers;
        std::shared_ptr<IFrameBuffer> m_BrightnessBuffer;
        std::int32_t m_Width{0};
        std::int32_t m_Height{0};
    };

    // FXAA Effect
    class GL_FXAAEffect : public GL_PostProcessEffect
    {
    public:
        GL_FXAAEffect();
        ~GL_FXAAEffect() = default;

        void Init() override;
        void Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer) override;
        void Resize(std::int32_t width, std::int32_t height) override;
        void* GetConfig() override { return &m_Config; }

    private:
        FXAAConfig m_Config;
        glm::vec2 m_InvScreenSize{0.0f};
    };

    // ColorGrading Effect
    class GL_ColorGradingEffect : public GL_PostProcessEffect
    {
    public:
        GL_ColorGradingEffect();
        ~GL_ColorGradingEffect() = default;

        void Init() override;
        void Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer) override;
        void Resize(std::int32_t width, std::int32_t height) override;
        void* GetConfig() override { return &m_Config; }

    private:
        ColorGradingConfig m_Config;
    };

    // Vignette Effect
    class GL_VignetteEffect : public GL_PostProcessEffect
    {
    public:
        GL_VignetteEffect();
        ~GL_VignetteEffect() = default;

        void Init() override;
        void Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer) override;
        void Resize(std::int32_t width, std::int32_t height) override;
        void* GetConfig() override { return &m_Config; }

    private:
        VignetteConfig m_Config;
    };

    // ChromaticAberration Effect
    class GL_ChromaticAberrationEffect : public GL_PostProcessEffect
    {
    public:
        GL_ChromaticAberrationEffect();
        ~GL_ChromaticAberrationEffect() = default;

        void Init() override;
        void Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer) override;
        void Resize(std::int32_t width, std::int32_t height) override;
        void* GetConfig() override { return &m_Config; }

    private:
        ChromaticAberrationConfig m_Config;
    };
}