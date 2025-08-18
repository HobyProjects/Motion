#include "CorePCH.hpp"

namespace Motion
{
    constexpr std::int32_t ENVIRONMENT_CUBE_SIZE = 2048;
    constexpr std::int32_t ENVIRONMENT_IRRADIANCE_SIZE = 32;
    constexpr std::int32_t ENVIRONMENT_PREFILTERED_SIZE = 128;
    constexpr std::int32_t ENVIRONMENT_BRDF_LUT_SIZE = 512;

    // ------------------------------ helpers: fullscreen quad & cube ------------------------------
    static std::uint32_t s_QuadVAO = 0, s_QuadVBO = 0;
    static std::uint32_t s_CubeVAO = 0, s_CubeVBO = 0;

    static void RenderQuad()
    {
        if (!s_QuadVAO)
        {
            const float quad[] = { -1,1,0, 0,1,  -1,-1,0, 0,0,  1,1,0, 1,1,  1,-1,0, 1,0 };
            glGenVertexArrays(1, &s_QuadVAO);
            glGenBuffers(1, &s_QuadVBO);
            glBindVertexArray(s_QuadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, s_QuadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        }
        glBindVertexArray(s_QuadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    static void RenderCube()
    {
        if (!s_CubeVAO)
        {
            float v[] = {
                // pos              // nrm           // uv
                -1,-1,-1, 0,0,-1, 0,0,  1,1,-1, 0,0,-1, 1,1,  1,-1,-1, 0,0,-1, 1,0,
                 1, 1,-1, 0,0,-1, 1,1, -1,-1,-1, 0,0,-1, 0,0, -1, 1,-1, 0,0,-1, 0,1,
                -1,-1, 1, 0,0, 1, 0,0,  1,-1, 1, 0,0, 1, 1,0,  1, 1, 1, 0,0, 1, 1,1,
                 1, 1, 1, 0,0, 1, 1,1, -1, 1, 1, 0,0, 1, 0,1, -1,-1, 1, 0,0, 1, 0,0,
                -1, 1, 1,-1,0,0, 1,0, -1, 1,-1,-1,0,0, 1,1, -1,-1,-1,-1,0,0, 0,1,
                -1,-1,-1,-1,0,0, 0,1, -1,-1, 1,-1,0,0, 0,0, -1, 1, 1,-1,0,0, 1,0,
                 1, 1, 1, 1,0,0, 1,0,  1,-1,-1, 1,0,0, 0,1,  1, 1,-1, 1,0,0, 1,1,
                 1,-1,-1, 1,0,0, 0,1,  1, 1, 1, 1,0,0, 1,0,  1,-1, 1, 1,0,0, 0,0,
                -1,-1,-1, 0,-1,0, 0,1,  1,-1,-1, 0,-1,0, 1,1,  1,-1, 1, 0,-1,0, 1,0,
                 1,-1, 1, 0,-1,0, 1,0, -1,-1, 1, 0,-1,0, 0,0, -1,-1,-1, 0,-1,0, 0,1,
                -1, 1,-1, 0, 1,0, 0,1,  1, 1, 1, 0, 1,0, 1,0,  1, 1,-1, 0, 1,0, 1,1,
                 1, 1, 1, 0, 1,0, 1,0, -1, 1,-1, 0, 1,0, 0,1, -1, 1, 1, 0, 1,0, 0,0
            };
            glGenVertexArrays(1, &s_CubeVAO);
            glGenBuffers(1, &s_CubeVBO);
            glBindVertexArray(s_CubeVAO);
            glBindBuffer(GL_ARRAY_BUFFER, s_CubeVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        }
        glBindVertexArray(s_CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    GL_Environment::GL_Environment(const std::filesystem::path& hdrFile)
    {
        auto& AM = AssetManager::GetInstance();
        m_EnvironmentShader = AM.Get<GL_Shader>("ENV");      // skybox draw
        m_CubeConvertShader = AM.Get<GL_Shader>("ENV_CUB");  // equirect->cube
        m_PrefilteredShader = AM.Get<GL_Shader>("ENV_PRE");  // specular prefilter
        m_IrradianceShader = AM.Get<GL_Shader>("ENV_IRR");  // diffuse irradiance
        m_BRDFShader = AM.Get<GL_Shader>("ENV_BRD");  // BRDF LUT

        if (!m_EnvironmentShader || !m_CubeConvertShader || !m_PrefilteredShader || !m_IrradianceShader || !m_BRDFShader)
        {
            MOTION_CORE_ERROR("Failed to load one or more environment shaders!");
            return;
        }

        // FBO
        glGenFramebuffers(1, &m_FrameBufferID);
        glGenRenderbuffers(1, &m_RenderBufferID);

        // depth storage sized for largest capture target (cube size)
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
        glBindRenderbuffer(GL_RENDERBUFFER, m_RenderBufferID);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ENVIRONMENT_CUBE_SIZE, ENVIRONMENT_CUBE_SIZE);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_RenderBufferID);

        // --- Load HDR equirect
        if (!std::filesystem::exists(hdrFile))
        {
            MOTION_CORE_ERROR("HDR file not found: {}", hdrFile.string());
            MOTION_CORE_INFO("Full path: {}", std::filesystem::absolute(hdrFile).string());
            return;
        }

        stbi_set_flip_vertically_on_load(true);
        int w = 0, h = 0, comp = 0; float* data = stbi_loadf(hdrFile.string().c_str(), &w, &h, &comp, 4);
        if (!data) { MOTION_CORE_ERROR("Failed to load HDR image data"); return; }

        GLuint hdrTex = 0; glGenTextures(1, &hdrTex);
        glBindTexture(GL_TEXTURE_2D, hdrTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // aniso if available
        GLfloat maxAniso = 0; glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        if (maxAniso > 0) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
        stbi_image_free(data);

        // --- Allocate environment cube
        glGenTextures(1, &m_EnvironmentCubeTextureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvironmentCubeTextureID);
        for (int i = 0;i < 6;++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, ENVIRONMENT_CUBE_SIZE, ENVIRONMENT_CUBE_SIZE, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // capture views
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 views[6] = {
            glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
            glm::lookAt(glm::vec3(0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
            glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(0, 0,-1), glm::vec3(0,-1, 0)),
        };

        // equirect -> cube
        m_CubeConvertShader->Bind();
        m_CubeConvertShader->SetUniform("u_EquiRectangular", 0);
        m_CubeConvertShader->SetUniform("u_Proj", proj);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTex);

        glViewport(0, 0, ENVIRONMENT_CUBE_SIZE, ENVIRONMENT_CUBE_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
        for (int i = 0;i < 6;++i)
        {
            m_CubeConvertShader->SetUniform("u_View", views[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_EnvironmentCubeTextureID, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvironmentCubeTextureID);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        // irradiance
        glGenTextures(1, &m_IrradianceTextureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceTextureID);
        for (int i = 0;i < 6;++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, ENVIRONMENT_IRRADIANCE_SIZE, ENVIRONMENT_IRRADIANCE_SIZE, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
        glBindRenderbuffer(GL_RENDERBUFFER, m_RenderBufferID);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ENVIRONMENT_IRRADIANCE_SIZE, ENVIRONMENT_IRRADIANCE_SIZE);

        m_IrradianceShader->Bind();
        m_IrradianceShader->SetUniform("u_EnvironmentTexture", 0);
        m_IrradianceShader->SetUniform("u_Proj", proj);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvironmentCubeTextureID);

        glViewport(0, 0, ENVIRONMENT_IRRADIANCE_SIZE, ENVIRONMENT_IRRADIANCE_SIZE);
        for (int i = 0;i < 6;++i)
        {
            m_IrradianceShader->SetUniform("u_View", views[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_IrradianceTextureID, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // prefiltered
        glGenTextures(1, &m_PrefilteredTextureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_PrefilteredTextureID);
        for (int i = 0;i < 6;++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, ENVIRONMENT_PREFILTERED_SIZE, ENVIRONMENT_PREFILTERED_SIZE, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        m_PrefilteredShader->Bind();
        m_PrefilteredShader->SetUniform("u_EnvironmentTexture", 0);
        m_PrefilteredShader->SetUniform("u_Proj", proj);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvironmentCubeTextureID);

        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
        m_MipLevel = (std::int32_t)(1 + std::floor(std::log2((float)ENVIRONMENT_PREFILTERED_SIZE)));
        for (std::uint32_t mip = 0; mip < m_MipLevel; ++mip)
        {
            const std::uint32_t mipDim = ENVIRONMENT_PREFILTERED_SIZE >> mip;
            glBindRenderbuffer(GL_RENDERBUFFER, m_RenderBufferID);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipDim, mipDim);
            glViewport(0, 0, mipDim, mipDim);

            float roughness = (float)mip / (float)(m_MipLevel - 1);
            m_PrefilteredShader->SetUniform("u_PrefilteredRoughness", roughness);
            for (int i = 0;i < 6;++i)
            {
                m_PrefilteredShader->SetUniform("u_View", views[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_PrefilteredTextureID, mip);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                RenderCube();
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // BRDF LUT
        glGenTextures(1, &m_BRDFLUTTextureID);
        glBindTexture(GL_TEXTURE_2D, m_BRDFLUTTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, ENVIRONMENT_BRDF_LUT_SIZE, ENVIRONMENT_BRDF_LUT_SIZE, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
        glBindRenderbuffer(GL_RENDERBUFFER, m_RenderBufferID);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ENVIRONMENT_BRDF_LUT_SIZE, ENVIRONMENT_BRDF_LUT_SIZE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BRDFLUTTextureID, 0);

        glViewport(0, 0, ENVIRONMENT_BRDF_LUT_SIZE, ENVIRONMENT_BRDF_LUT_SIZE);
        m_BRDFShader->Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // cleanup temp HDR texture
        glDeleteTextures(1, &hdrTex);
    }

    GL_Environment::~GL_Environment()
    {
        if (m_FrameBufferID)        glDeleteFramebuffers(1, &m_FrameBufferID);
        if (m_RenderBufferID)       glDeleteRenderbuffers(1, &m_RenderBufferID);
        if (m_EnvironmentCubeTextureID) glDeleteTextures(1, &m_EnvironmentCubeTextureID);
        if (m_IrradianceTextureID)  glDeleteTextures(1, &m_IrradianceTextureID);
        if (m_PrefilteredTextureID) glDeleteTextures(1, &m_PrefilteredTextureID);
        if (m_BRDFLUTTextureID)     glDeleteTextures(1, &m_BRDFLUTTextureID);
        if (s_QuadVAO) { glDeleteVertexArrays(1, &s_QuadVAO); s_QuadVAO = 0; }
        if (s_QuadVBO) { glDeleteBuffers(1, &s_QuadVBO); s_QuadVBO = 0; }
        if (s_CubeVAO) { glDeleteVertexArrays(1, &s_CubeVAO); s_CubeVAO = 0; }
        if (s_CubeVBO) { glDeleteBuffers(1, &s_CubeVBO); s_CubeVBO = 0; }
    }

    void GL_Environment::BindCubeTexture(std::uint32_t slot) const noexcept { glBindTextureUnit(slot, m_EnvironmentCubeTextureID); }
    void GL_Environment::BindBRDFLUTTexture(std::uint32_t slot) const noexcept { glBindTextureUnit(slot, m_BRDFLUTTextureID); }
    void GL_Environment::BindPrefilteredTexture(std::uint32_t slot) const noexcept { glBindTextureUnit(slot, m_PrefilteredTextureID); }
    void GL_Environment::BindIrradianceTexture(std::uint32_t slot) const noexcept { glBindTextureUnit(slot, m_IrradianceTextureID); }

    void GL_Environment::BindIBLAll(std::uint32_t irr, std::uint32_t pre, std::uint32_t brdf) const noexcept
    {
        BindIrradianceTexture(irr);
        BindPrefilteredTexture(pre);
        BindBRDFLUTTexture(brdf);
    }

    void GL_Environment::Render(const glm::mat4 viewMatrix, const glm::mat4 projectionMatrix) noexcept
    {
        if (!m_EnvironmentShader) { MOTION_CORE_ERROR("Environment shader is not loaded!"); return; }
        if (m_EnvironmentCubeTextureID == 0) { MOTION_CORE_WARN("Environment cube not ready; skipping skybox."); return; }

        // ── Save current GL state we touch
        glEnable(GL_DEPTH_TEST);
        GLint     prevDepthFunc;   glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
        GLboolean prevDepthMask;   glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);

        GLboolean prevCullEnabled = glIsEnabled(GL_CULL_FACE);
        GLint     prevCullFace;    glGetIntegerv(GL_CULL_FACE_MODE, &prevCullFace);

        GLint     prevActiveUnit;  glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveUnit);
        GLint     prevCubeBind;    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &prevCubeBind);

        // ── Configure for skybox
        glDepthFunc(GL_LEQUAL);   // GL_GEQUAL if you run reversed-Z
        glDepthMask(GL_FALSE);    // never write depth

        // Cull front faces so the inside of the unit cube renders
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        // ── Bind shader & set uniforms
        m_EnvironmentShader->Bind();

        // remove translation, apply Y rotation (rotate the sky relative to camera)
        const glm::mat4 viewNoTrans = glm::mat4(glm::mat3(viewMatrix));
        const glm::mat4 R = glm::rotate(glm::mat4(1.0f), m_SkyboxRotationY, glm::vec3(0, 1, 0));

        m_EnvironmentShader->SetUniform("u_View", viewNoTrans * R);
        m_EnvironmentShader->SetUniform("u_Proj", projectionMatrix);

        // Use texture unit 5 for the cube (keep shader uniform in sync)
        constexpr GLint kSkyboxSlot = 5;
        m_EnvironmentShader->SetUniform("u_EnvironmentTexture", kSkyboxSlot);

        glActiveTexture(GL_TEXTURE0 + kSkyboxSlot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvironmentCubeTextureID);

        // ── Draw
        RenderCube();

        // ── Restore state
        glBindTexture(GL_TEXTURE_CUBE_MAP, prevCubeBind);
        glActiveTexture(prevActiveUnit);

        if (!prevCullEnabled) glDisable(GL_CULL_FACE);
        glCullFace(prevCullFace);

        glDepthMask(prevDepthMask);
        glDepthFunc(prevDepthFunc);
    }

}

