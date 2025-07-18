#include "CorePCH.hpp"
#include "PostProcessor.hpp"

namespace Motion::Core
{
    PostProcessor::PostProcessor(const FrameBufferSpecification& spec, const UUID& shaderAssetID)
    {
        m_FrameBuffer = BufferFactory::CreateFrameBuffer(spec);

        auto& assetManager = AssetManager::GetInstance();
        m_Shader = assetManager.Get<IShader>(shaderAssetID);

        auto& quickMesh = QuickMesh::GetInstance();
        m_ScreenQuad = quickMesh.CreateQuad(spec.Name, spec.Width, spec.Height);
    }

    void PostProcessor::Process(FrameTextureID inputTextureID)
    {
        if (!m_FrameBuffer || !m_Shader || !m_ScreenQuad)
        {
            MOTION_CORE_ERROR("PostProcessor is not properly initialized!");
            return;
        }

        m_FrameBuffer->Bind();

        m_Shader->Bind();
        m_Shader->SetUniform("u_PostProcessTexture", 0);

        m_FrameBuffer->BindTextureUnit(0, inputTextureID);
        m_ScreenQuad->Render();
        m_FrameBuffer->UnbindTextureUnit();

        m_Shader->Unbind();
        m_FrameBuffer->Unbind();
    }

    FrameTextureID PostProcessor::GetOutputTextureID() const
    {
        if (m_FrameBuffer)
        {
            return m_FrameBuffer->GetAttachment(FrameBufferColorAttachments::Standard).TextureID;
        }

        MOTION_CORE_ERROR("PostProcessor FrameBuffer is not initialized!");
        return 0; // Return an invalid texture ID
    }

    void PostProcessor::OnResize(std::uint32_t width, std::uint32_t height)
    {
        if (m_FrameBuffer)
        {
            m_FrameBuffer->ResizeFrame(width, height);
        }
    }


}