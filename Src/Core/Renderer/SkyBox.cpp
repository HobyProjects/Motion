#include "CorePCH.hpp"
#include "SkyBox.hpp"

namespace Motion
{
    static std::shared_ptr<ICubeTexture> s_SkyBoxCubeTexture = nullptr;
    static std::shared_ptr<IShader> s_SkyBoxShader = nullptr;

    static std::shared_ptr<IVertexArray> s_SkyBoxVAO = nullptr;
    static std::shared_ptr<IVertexBuffer> s_SkyBoxVBO = nullptr;
    static std::shared_ptr<IElementBuffer> s_SkyBoxEBO = nullptr;

    static std::shared_ptr<EnvironmentIrradianceTexture> s_IrradianceTexture = nullptr;
    static std::shared_ptr<EnvironmentPrefilteredTexture> s_PrefilteredTexture = nullptr;
    static std::shared_ptr<EnvironmentBRDFTexture> s_BRDFTexture = nullptr;
    static std::shared_ptr<ICaptureFrameBuffer> s_CaptureFrameBuffer = nullptr;
    static constexpr std::int32_t CAPTURE_RESOLUTION = 512;

    /**
     * @brief Initializes the SkyBox by loading the cube map texture and shader.
     *
     * This function retrieves the cube map texture and shader from the asset manager,
     * and creates a mesh for the skybox. It should be called once before rendering the skybox.
     */
    void SkyBox::Init() noexcept
    {
        auto& assetManager = AssetManager::GetInstance();
        s_SkyBoxCubeTexture = CreateUnregisteredCubeMapTexture("Assets/SkyBox/RooitouPark/px.png", "Assets/SkyBox/RooitouPark/nx.png", "Assets/SkyBox/RooitouPark/py.png", "Assets/SkyBox/RooitouPark/ny.png", "Assets/SkyBox/RooitouPark/pz.png", "Assets/SkyBox/RooitouPark/nz.png");

        if (!s_SkyBoxCubeTexture)
        {
            MOTION_CORE_ERROR("Failed to create CubeTexture for SkyBox");
            return;
        }

        s_SkyBoxShader = assetManager.Get<IShader>("Environment");
        if (!s_SkyBoxShader)
        {
            MOTION_CORE_ERROR("Failed to retrieve Environment");
            return;
        }

        std::vector<float> vertices = {
            -1.0f,  1.0f, -1.0f, // 0 top-left-back
            -1.0f, -1.0f, -1.0f, // 1 bottom-left-back
             1.0f, -1.0f, -1.0f, // 2 bottom-right-back
             1.0f,  1.0f, -1.0f, // 3 top-right-back
            -1.0f,  1.0f,  1.0f, // 4 top-left-front
            -1.0f, -1.0f,  1.0f, // 5 bottom-left-front
             1.0f, -1.0f,  1.0f, // 6 bottom-right-front
             1.0f,  1.0f,  1.0f  // 7 top-right-front
        };

        std::vector<std::uint32_t> indices = {
            // back face
            0, 1, 2,
            2, 3, 0,

            // front face
            4, 5, 6,
            6, 7, 4,

            // left face
            4, 5, 1,
            1, 0, 4,

            // right face
            3, 2, 6,
            6, 7, 3,

            // bottom face
            1, 5, 6,
            6, 2, 1,

            // top face
            4, 0, 3,
            3, 7, 4
        };

        s_SkyBoxVBO = BufferFactory::CreateVertexBuffer(vertices.data(), static_cast<std::int32_t>(vertices.size()));
        s_SkyBoxEBO = BufferFactory::CreateElementBuffer(indices.data(), static_cast<std::int32_t>(indices.size()));
        s_SkyBoxVAO = std::make_shared<GL_VertexArray>();

        BufferLayout layout({ { UniformCache::Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) } });
        s_SkyBoxVBO->SetLayout(layout);
        s_SkyBoxVAO->EmplaceVertexBuffer(s_SkyBoxVBO);
        s_SkyBoxVAO->EmplaceIndexBuffer(s_SkyBoxEBO);

        s_IrradianceTexture = EnvironmentIrradianceTexture::Create(CAPTURE_RESOLUTION);
        s_PrefilteredTexture = EnvironmentPrefilteredTexture::Create(CAPTURE_RESOLUTION);
        s_BRDFTexture = EnvironmentBRDFTexture::Create(CAPTURE_RESOLUTION, CAPTURE_RESOLUTION);
        s_CaptureFrameBuffer = BufferFactory::CreateCaptureFrameBuffer(CAPTURE_RESOLUTION, CAPTURE_RESOLUTION);

        if (!s_IrradianceTexture || !s_PrefilteredTexture || !s_BRDFTexture || !s_CaptureFrameBuffer)
        {
            MOTION_CORE_ERROR("Failed to create environment textures or capture frame buffer for SkyBox");
            return;
        }

        s_IrradianceTexture->GenerateIrradiance(s_SkyBoxCubeTexture->GetID(), s_CaptureFrameBuffer);
        s_PrefilteredTexture->GeneratePrefliteredTexture(s_SkyBoxCubeTexture->GetID(), s_CaptureFrameBuffer);
        s_BRDFTexture->Generate(s_CaptureFrameBuffer);
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
        if (!s_SkyBoxShader || !s_SkyBoxCubeTexture)
        {
            MOTION_CORE_ERROR("SkyBox is not initialized properly. Cannot render.");
            return;
        }

        glm::mat4 view = glm::mat4(glm::mat3(viewMatrix));
        glm::mat4 projection = projectionMatrix;

        Renderer::ApplyDrawFlags(DrawFlags::SkipDepthMask);
        s_SkyBoxShader->Bind();

        s_SkyBoxShader->SetUniform(UniformCache::ViewMatrix, view);
        s_SkyBoxShader->SetUniform(UniformCache::ProjectionMatrix, projection);

        std::int32_t skyBoxBindingPoint = TextureBinding::Point();
        s_SkyBoxCubeTexture->Bind(skyBoxBindingPoint);
        s_SkyBoxShader->SetUniform(UniformCache::EnvironmentTexture, skyBoxBindingPoint);

        s_SkyBoxVAO->Bind();
        Renderer::DrawIndexed(s_SkyBoxEBO->GetElementCount());
        s_SkyBoxVAO->Unbind();

        s_SkyBoxCubeTexture->Unbind();
        s_SkyBoxShader->Unbind();
        Renderer::ResetDrawFlags(DrawFlags::SkipDepthMask);
    }

    /**
     * @brief Retrieves the currently bound SkyBox cube map texture.
     *
     * This function returns the shared pointer to the SkyBox cube map texture,
     * which can be used for further processing or rendering operations.
     *
     * @return std::shared_ptr<ICubeMapTexture> The currently bound SkyBox cube map texture.
     */
    std::shared_ptr<ICubeTexture> SkyBox::GetTexture() noexcept
    {
        return s_SkyBoxCubeTexture;
    }

    /**
     * @brief Retrieves the environment irradiance texture used for the SkyBox.
     *
     * This function returns the shared pointer to the environment irradiance texture,
     * which is used for lighting calculations in the SkyBox rendering.
     *
     * @return std::shared_ptr<EnvironmentIrradianceTexture> The environment irradiance texture.
     */
    std::shared_ptr<EnvironmentIrradianceTexture> SkyBox::GetIrradianceTexture() noexcept
    {
        return s_IrradianceTexture;
    }

    /**
     * @brief Retrieves the prefiltered environment texture used for the SkyBox.
     *
     * This function returns the shared pointer to the prefiltered environment texture,
     * which is used for reflections in the SkyBox rendering.
     *
     * @return std::shared_ptr<EnvironmentPrefilteredTexture> The prefiltered environment texture.
     */
    std::shared_ptr<EnvironmentPrefilteredTexture> SkyBox::GetPrefilteredTexture() noexcept
    {
        return s_PrefilteredTexture;
    }

    /**
     * @brief Retrieves the BRDF texture used for the SkyBox.
     *
     * This function returns the shared pointer to the BRDF texture,
     * which is used for calculating the bidirectional reflectance distribution function in the SkyBox rendering.
     *
     * @return std::shared_ptr<EnvironmentBRDFTexture> The BRDF texture.
     */
    std::shared_ptr<EnvironmentBRDFTexture> SkyBox::GetBRDFTexture() noexcept
    {
        return s_BRDFTexture;
    }
}