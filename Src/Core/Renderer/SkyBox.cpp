#include "CorePCH.hpp"

namespace Motion::Core
{
    SkyBox::SkyBox()
    {
        auto& assetManager = AssetManager::GetInstance();
        m_CubeMapTexture = CreateUnregisteredCubeMapTexture("Assets/SkyBox/SkyBox_Texture_1.jpg");
        if (!m_CubeMapTexture)
        {
            MOTION_CORE_ERROR("Failed to create CubeMapTexture for SkyBox");
            return;
        }

        m_ShaderProgram = assetManager.Get<IShader>("SkyBoxShader");
        if (!m_ShaderProgram)
        {
            MOTION_CORE_ERROR("Failed to retrieve SkyBoxShader");
            return;
        }

        m_SkyBoxMesh = QuickMesh::CreateCube(false, "SkyBoxMesh", 200.0f, 2000.0f, 200.0f);
        if (!m_SkyBoxMesh)
        {
            MOTION_CORE_ERROR("Failed to create SkyBoxMesh");
            return;
        }
    }

    void SkyBox::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) noexcept
    {
        if (!m_CubeMapTexture || !m_ShaderProgram || !m_SkyBoxMesh)
        {
            MOTION_CORE_ERROR("SkyBox resources are not properly initialized");
            return;
        }

        Renderer::ApplyDrawFlags(DrawFlags::SkipDepthMask);
        m_ShaderProgram->Bind();

        glm::mat4 skyboxView = glm::mat4(glm::mat3(viewMatrix));
        m_ShaderProgram->SetUniform(UniformCache::GlobalAttri_ViewMatrix, skyboxView);
        m_ShaderProgram->SetUniform(UniformCache::GlobalAttri_ProjectionMatrix, projectionMatrix);

        m_CubeMapTexture->Bind();
        m_SkyBoxMesh->Render();
        m_CubeMapTexture->Unbind();

        m_ShaderProgram->Unbind();
        Renderer::ResetDrawFlags(DrawFlags::SkipDepthMask);
    }


}