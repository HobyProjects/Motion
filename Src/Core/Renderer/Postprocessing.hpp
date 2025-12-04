#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>

#include "Base.hpp"
#include "Buffers.hpp"
#include "Arrays.hpp"
#include "Shaders.hpp"

namespace Motion
{
    enum class PostProcessEffectType : std::uint32_t
    {
        None            = 0,
        ToneMapping     = BIT(0),
        Bloom           = BIT(1),
        FXAA            = BIT(2),
        ColorGrading    = BIT(3),
        Vignette        = BIT(4),
        ChromaticAber   = BIT(5),
        Sharpen         = BIT(6),
        Blur            = BIT(7),
        DepthOfField    = BIT(8),
        MotionBlur      = BIT(9),
        Custom          = BIT(31)
    };

    template<>
    struct enable_bitmask_operations<PostProcessEffectType> : std::true_type {};

    struct ToneMappingConfig
    {
        enum class Operator
        {
            Reinhard,
            ReinhardLuminance,
            Uncharted2,
            ACES,
            Exposure
        };

        Operator    toneMappingOp   = Operator::ACES;
        float       exposure        = 1.0f;
        float       gamma           = 2.2f;
        float       whitePoint      = 11.2f;
    };

    struct BloomConfig
    {
        float       threshold       = 1.0f;
        float       intensity       = 0.5f;
        float       radius          = 1.0f;
        int         iterations      = 5;
        bool        enabled         = true;
    };

    struct FXAAConfig
    {
        float       edgeThreshold       = 0.125f;
        float       edgeThresholdMin    = 0.0312f;
        int         searchSteps         = 12;
        float       subpixelQuality     = 0.75f;
    };

    struct ColorGradingConfig
    {
        glm::vec3   shadows         = glm::vec3(1.0f);
        glm::vec3   midtones        = glm::vec3(1.0f);
        glm::vec3   highlights      = glm::vec3(1.0f);
        float       saturation      = 1.0f;
        float       contrast        = 1.0f;
        float       brightness      = 0.0f;
    };

    struct VignetteConfig
    {
        float       intensity       = 0.5f;
        float       smoothness      = 0.5f;
        glm::vec3   color           = glm::vec3(0.0f);
    };

    struct ChromaticAberrationConfig
    {
        float       intensity       = 0.01f;
        glm::vec2   direction       = glm::vec2(1.0f, 0.0f);
    };

    // Base post-processing effect interface
    class IPostProcessEffect
    {
    public:
        IPostProcessEffect() = default;
        virtual ~IPostProcessEffect() = default;

        virtual void Init() = 0;
        virtual void Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer) = 0;
        virtual void Resize(std::int32_t width, std::int32_t height) = 0;
        
        virtual PostProcessEffectType GetType() const = 0;
        virtual bool IsEnabled() const = 0;
        virtual void SetEnabled(bool enabled) = 0;
        virtual void* GetConfig() = 0;

        [[nodiscard]] static std::shared_ptr<IPostProcessEffect> Create(PostProcessEffectType type);
    };

    // Post-processing stack manager
    class PostProcessStack
    {
    public:
        PostProcessStack();
        ~PostProcessStack();

        // Add/Remove effects
        void AddEffect(PostProcessEffectType type);
        void AddEffect(std::shared_ptr<IPostProcessEffect> effect);
        void RemoveEffect(PostProcessEffectType type);
        void ClearEffects();

        // Effect management
        std::shared_ptr<IPostProcessEffect> GetEffect(PostProcessEffectType type);
        void SetEffectEnabled(PostProcessEffectType type, bool enabled);
        bool IsEffectEnabled(PostProcessEffectType type) const;

        // Get configuration for specific effects
        template<typename T>
        T* GetEffectConfig(PostProcessEffectType type)
        {
            auto effect = GetEffect(type);
            if (effect)
                return static_cast<T*>(effect->GetConfig());
            return nullptr;
        }

        // Processing
        void Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer);
        void Resize(std::int32_t width, std::int32_t height);

        // Utility
        std::size_t GetEffectCount() const { return m_Effects.size(); }
        const std::vector<std::shared_ptr<IPostProcessEffect>>& GetEffects() const { return m_Effects; }

    private:
        void InitializeIntermediateBuffers();
        void ResizeIntermediateBuffers(std::int32_t width, std::int32_t height);

    private:
        std::vector<std::shared_ptr<IPostProcessEffect>> m_Effects;
        std::shared_ptr<IFrameBuffer> m_IntermediateBuffer1;
        std::shared_ptr<IFrameBuffer> m_IntermediateBuffer2;
        std::int32_t m_Width{0};
        std::int32_t m_Height{0};
    };

    // Utility class for full-screen quad rendering
    class ScreenQuad
    {
    public:
        ScreenQuad();
        ~ScreenQuad();

        void Render();

        static ScreenQuad& GetInstance();

    private:
        void Initialize();

    private:
        std::shared_ptr<IVertexBuffer> m_VertexBuffer;
        std::shared_ptr<IVertexArray> m_VertexArray;
        bool m_Initialized{false};
    };
}