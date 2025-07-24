#include "CorePCH.hpp"
#include "SkyBox.hpp"

namespace Motion
{
    static std::shared_ptr<ICubeMapTexture> s_SkyBoxCubeTexture = nullptr;
    static std::shared_ptr<IShader> s_SkyBoxShader = nullptr;
    static std::shared_ptr<Mesh> s_SkyBoxMesh = nullptr;

    /**
     * @brief Initializes the SkyBox by loading the cube map texture and shader.
     *
     * This function retrieves the cube map texture and shader from the asset manager,
     * and creates a mesh for the skybox. It should be called once before rendering the skybox.
     */
    void SkyBox::Init() noexcept
    {
        auto& assetManager = AssetManager::GetInstance();
        s_SkyBoxCubeTexture = CreateUnregisteredCubeMapTexture("Assets/SkyBox/SkyBox_Texture_1.jpg");
        if (!s_SkyBoxCubeTexture)
        {
            MOTION_CORE_ERROR("Failed to create CubeMapTexture for SkyBox");
            return;
        }

        s_SkyBoxShader = assetManager.Get<IShader>("SkyBoxShader");
        if (!s_SkyBoxShader)
        {
            MOTION_CORE_ERROR("Failed to retrieve SkyBoxShader");
            return;
        }

        s_SkyBoxMesh = QuickMesh::CreateCube(false, "SkyBoxMesh", 200.0f, 200.0f, 200.0f);
        if (!s_SkyBoxMesh)
        {
            MOTION_CORE_ERROR("Failed to create SkyBoxMesh");
            return;
        }
    }

    /**
     * @brief Binds the SkyBox shader and texture for rendering.
     *
     * This function applies the necessary draw flags and binds the skybox shader and texture.
     * It should be called before rendering the skybox.
     */
    void SkyBox::Bind() noexcept
    {
        Renderer::ApplyDrawFlags(DrawFlags::SkipDepthMask);

        s_SkyBoxShader->Bind();
        std::uint32_t bindingPoint = TextureBinding::Point();
        s_SkyBoxCubeTexture->Bind(bindingPoint);
        s_SkyBoxShader->SetUniform(UniformCache::SkyboxTexture, bindingPoint);
    }

    /**
     * @brief Unbinds the SkyBox shader and texture after rendering.
     *
     * This function unbinds the skybox shader and texture, restoring the previous state.
     * It should be called after rendering the skybox.
     */
    void SkyBox::Unbind() noexcept
    {
        s_SkyBoxCubeTexture->Unbind();
        s_SkyBoxShader->Unbind();

        Renderer::ResetDrawFlags(DrawFlags::SkipDepthMask);
    }

    /**
     * @brief Renders the SkyBox using the provided view and projection matrices.
     *
     * This function sets up the view and projection matrices for the skybox shader,
     * and renders the skybox mesh. It should be called after binding the skybox.
     *
     * @param viewMatrix The view matrix to use for rendering the skybox.
     * @param projectionMatrix The projection matrix to use for rendering the skybox.
     */
    void SkyBox::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) noexcept
    {
        if (!s_SkyBoxShader || !s_SkyBoxCubeTexture || !s_SkyBoxMesh)
        {
            MOTION_CORE_ERROR("SkyBox is not initialized properly. Cannot render.");
            return;
        }

        glm::mat4 skyBoxView = glm::mat4(glm::mat3(viewMatrix));
        s_SkyBoxShader->SetUniform(UniformCache::ViewMatrix, skyBoxView);
        s_SkyBoxShader->SetUniform(UniformCache::ProjectionMatrix, projectionMatrix);

        s_SkyBoxMesh->Render();
    }

    /**
     * @brief Retrieves the currently bound SkyBox cube map texture.
     *
     * This function returns the shared pointer to the SkyBox cube map texture,
     * which can be used for further processing or rendering operations.
     *
     * @return std::shared_ptr<ICubeMapTexture> The currently bound SkyBox cube map texture.
     */
    std::shared_ptr<ICubeMapTexture> SkyBox::GetTexture() noexcept
    {
        return s_SkyBoxCubeTexture;
    }

}