#include "CorePCH.hpp"

namespace Motion
{
    // ========================================
    // ScreenQuad Implementation
    // ========================================
    
    ScreenQuad::ScreenQuad()
    {
        Initialize();
    }

    ScreenQuad::~ScreenQuad() = default;

    void ScreenQuad::Initialize()
    {
        if (m_Initialized)
            return;

        // Full-screen quad vertices
        float quadVertices[] = {
            // Positions   // TexCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        m_VertexBuffer = IVertexBuffer::Create(quadVertices, sizeof(quadVertices));
        
        BufferLayout layout = {
            { "aPosition", BufferComponents::XY, BufferStride::F2, false, 0 },
            { "aTexCoord", BufferComponents::XY, BufferStride::F2, false, 2 * sizeof(float) }
        };
        
        m_VertexBuffer->SetLayout(layout);

        m_VertexArray = IVertexArray::Create();
        m_VertexArray->EmplaceVertexBuffer(m_VertexBuffer);

        m_Initialized = true;
    }

    void ScreenQuad::Render()
    {
        if (!m_Initialized)
            Initialize();

        m_VertexArray->Bind();
        Renderer::DrawArrays(PrimitiveTopology::Triangles, 6);
    }

    ScreenQuad& ScreenQuad::GetInstance()
    {
        static ScreenQuad instance;
        return instance;
    }

    // ========================================
    // PostProcessStack Implementation
    // ========================================

    PostProcessStack::PostProcessStack()
    {
        InitializeIntermediateBuffers();
    }

    PostProcessStack::~PostProcessStack() = default;

    void PostProcessStack::InitializeIntermediateBuffers()
    {
        // Create two intermediate framebuffers for ping-pong rendering
        FrameBufferSpecification spec;
        spec.Name = "PostProcess_Intermediate1";
        spec.Width = 1280;
        spec.Height = 720;
        spec.Samples = 1;
        spec.SwapChainTarget = false;
        spec.Colors = {
            { 0, 0, FrameBufferColorAttachmentStandards::HighDynamicRange }
        };
        spec.Depth = { 0, 0, FrameBufferDepthAttachmentStandards::None };

        m_IntermediateBuffer1 = IFrameBuffer::Create(spec);

        spec.Name = "PostProcess_Intermediate2";
        m_IntermediateBuffer2 = IFrameBuffer::Create(spec);

        m_Width = spec.Width;
        m_Height = spec.Height;
    }

    void PostProcessStack::ResizeIntermediateBuffers(std::int32_t width, std::int32_t height)
    {
        if (width == m_Width && height == m_Height)
            return;

        m_Width = width;
        m_Height = height;

        if (m_IntermediateBuffer1)
            m_IntermediateBuffer1->ResizeFrame(width, height);
        
        if (m_IntermediateBuffer2)
            m_IntermediateBuffer2->ResizeFrame(width, height);
    }

    void PostProcessStack::AddEffect(PostProcessEffectType type)
    {
        auto effect = IPostProcessEffect::Create(type);
        if (effect)
        {
            effect->Init();
            m_Effects.push_back(effect);
        }
    }

    void PostProcessStack::AddEffect(std::shared_ptr<IPostProcessEffect> effect)
    {
        if (effect)
        {
            effect->Init();
            m_Effects.push_back(effect);
        }
    }

    void PostProcessStack::RemoveEffect(PostProcessEffectType type)
    {
        m_Effects.erase(
            std::remove_if(m_Effects.begin(), m_Effects.end(),
                [type](const std::shared_ptr<IPostProcessEffect>& effect) {
                    return effect->GetType() == type;
                }),
            m_Effects.end()
        );
    }

    void PostProcessStack::ClearEffects()
    {
        m_Effects.clear();
    }

    std::shared_ptr<IPostProcessEffect> PostProcessStack::GetEffect(PostProcessEffectType type)
    {
        for (auto& effect : m_Effects)
        {
            if (effect->GetType() == type)
                return effect;
        }
        return nullptr;
    }

    void PostProcessStack::SetEffectEnabled(PostProcessEffectType type, bool enabled)
    {
        auto effect = GetEffect(type);
        if (effect)
            effect->SetEnabled(enabled);
    }

    bool PostProcessStack::IsEffectEnabled(PostProcessEffectType type) const
    {
        for (const auto& effect : m_Effects)
        {
            if (effect->GetType() == type)
                return effect->IsEnabled();
        }
        return false;
    }

    void PostProcessStack::Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer)
    {
        if (m_Effects.empty())
        {
            // No effects, just copy input to output
            // This would require a simple blit or copy shader
            return;
        }

        // Filter enabled effects
        std::vector<IPostProcessEffect*> enabledEffects;
        for (auto& effect : m_Effects)
        {
            if (effect->IsEnabled())
                enabledEffects.push_back(effect.get());
        }

        if (enabledEffects.empty())
            return;

        // Ping-pong between intermediate buffers
        FrameTextureID currentInput = inputTexture;
        IFrameBuffer* currentOutput = nullptr;
        
        for (std::size_t i = 0; i < enabledEffects.size(); ++i)
        {
            bool isLastEffect = (i == enabledEffects.size() - 1);
            
            // Choose output buffer
            if (isLastEffect)
            {
                currentOutput = outputFrameBuffer;
            }
            else
            {
                currentOutput = (i % 2 == 0) ? m_IntermediateBuffer1.get() : m_IntermediateBuffer2.get();
            }

            // Process effect
            enabledEffects[i]->Process(currentInput, currentOutput);

            // Update input for next iteration
            if (!isLastEffect)
            {
                currentInput = currentOutput->GetAttachment(
                    FrameBufferColorAttachmentStandards::HighDynamicRange
                ).ID;
            }
        }
    }

    void PostProcessStack::Resize(std::int32_t width, std::int32_t height)
    {
        ResizeIntermediateBuffers(width, height);
        
        for (auto& effect : m_Effects)
            effect->Resize(width, height);
    }
}