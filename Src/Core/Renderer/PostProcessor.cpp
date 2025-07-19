#include "CorePCH.hpp"

namespace Motion::Core
{
    /**
     * @brief Constructs a PostProcessor object with the specified framebuffer configuration and shader asset.
     *
     * This constructor initializes the framebuffer using the provided specification,
     * retrieves the shader asset using the given shader asset ID, and creates a screen quad
     * mesh for post-processing operations.
     *
     * @param spec The specification for the framebuffer to be created.
     * @param shaderAssetID The unique identifier for the shader asset to be used in post-processing.
     */
    PostProcessor::PostProcessor(const FrameBufferSpecification& spec, const UUID& shaderAssetID)
    {
        m_FrameBuffer = BufferFactory::CreateFrameBuffer(spec);

        auto& assetManager = AssetManager::GetInstance();
        m_Shader = assetManager.Get<IShader>(shaderAssetID);

        auto& quickMesh = QuickMesh::GetInstance();
        m_ScreenQuad = quickMesh.CreateQuad(spec.Name, spec.Width, spec.Height);
    }

    /**
     * @brief Applies post-processing effects to the given input frame texture.
     *
     * This function binds the framebuffer, shader, and screen quad, then renders the input texture
     * using the post-processing shader. It ensures all required resources are initialized before processing.
     * The processed result is rendered to the framebuffer.
     *
     * @param inputTextureID The ID of the input frame texture to be post-processed.
     */
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

    /**
     * Retrieves the texture ID of the output color attachment from the post-processing frame buffer.
     *
     * @return FrameTextureID The texture ID of the standard color attachment if the frame buffer is initialized;
     * otherwise, returns 0 and logs an error.
     *
     * @note If the frame buffer is not initialized, an error is logged and an invalid texture ID (0) is returned.
     */
    FrameTextureID PostProcessor::GetOutputTextureID() const
    {
        if (m_FrameBuffer)
        {
            return m_FrameBuffer->GetAttachment(FrameBufferColorAttachments::Standard).TextureID;
        }

        MOTION_CORE_ERROR("PostProcessor FrameBuffer is not initialized!");
        return 0; // Return an invalid texture ID
    }

    /**
     * @brief Handles resizing of the post-processing framebuffer.
     *
     * This method is called when the rendering viewport or window is resized.
     * It updates the size of the internal framebuffer to match the new width and height.
     *
     * @param width The new width of the framebuffer.
     * @param height The new height of the framebuffer.
     */
    void PostProcessor::OnResize(std::uint32_t width, std::uint32_t height)
    {
        if (m_FrameBuffer)
        {
            m_FrameBuffer->ResizeFrame(width, height);
        }
    }


}