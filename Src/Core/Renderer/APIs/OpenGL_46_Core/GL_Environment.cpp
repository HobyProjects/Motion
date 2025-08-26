#include "CorePCH.hpp"
#include "GL_Environment.hpp"

namespace Motion
{
    static constexpr std::int32_t ENVIRONMENT_CUBE_SIZE            = 2048;
    static constexpr std::int32_t ENVIRONMENT_IRRADIANCE_SIZE      = 32;
    static constexpr std::int32_t ENVIRONMENT_PREFILTERED_SIZE     = 128;
    static constexpr std::int32_t ENVIRONMENT_BRDF_LUT_SIZE        = 512;

    // ------------------------------ helpers: fullscreen quad & cube ------------------------------
    static std::uint32_t s_QuadVAO = 0, s_QuadVBO = 0;
    static std::uint32_t s_CubeVAO = 0, s_CubeVBO = 0;

    static void EnsureQuad()
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
    }

    static void EnsureCube()
    {
        if (!s_CubeVAO)
        {
            float v[] = 
            {
                -1, -1, -1,  0,  0, -1, 0, 0,  1,  1, -1,  0,  0, -1,  1,  1,  1, -1, -1,  0,  0, -1,  1,  0,
                 1,  1, -1,  0,  0, -1, 1, 1, -1, -1, -1,  0,  0, -1,  0,  0, -1,  1, -1,  0,  0, -1,  0,  1,
                -1, -1,  1,  0,  0,  1, 0, 0,  1, -1,  1,  0,  0,  1,  1,  0,  1,  1,  1,  0,  0,  1,  1,  1,
                 1,  1,  1,  0,  0,  1, 1, 1, -1,  1,  1,  0,  0,  1,  0,  1, -1, -1,  1,  0,  0,  1,  0,  0,
                -1,  1,  1, -1,  0,  0, 1, 0, -1,  1, -1, -1,  0,  0,  1,  1, -1, -1, -1, -1,  0,  0,  0,  1,
                -1, -1, -1, -1,  0,  0, 0, 1, -1, -1,  1, -1,  0,  0,  0,  0, -1,  1,  1, -1,  0,  0,  1,  0,
                 1,  1,  1,  1,  0,  0, 1, 0,  1, -1, -1,  1,  0,  0,  0,  1,  1,  1, -1,  1,  0,  0,  1,  1,
                 1, -1, -1,  1,  0,  0, 0, 1,  1,  1,  1,  1,  0,  0,  1,  0,  1, -1,  1,  1,  0,  0,  0,  0,
                -1, -1, -1,  0, -1,  0, 0, 1,  1, -1, -1,  0, -1,  0,  1,  1,  1, -1,  1,  0, -1,  0,  1,  0,
                 1, -1,  1,  0, -1,  0, 1, 0, -1, -1,  1,  0, -1,  0,  0,  0, -1, -1, -1,  0, -1,  0,  0,  1,
                -1,  1, -1,  0,  1,  0, 0, 1,  1,  1,  1,  0,  1,  0,  1,  0,  1,  1, -1,  0,  1,  0,  1,  1,
                 1,  1,  1,  0,  1,  0, 1, 0, -1,  1, -1,  0,  1,  0,  0,  1, -1,  1,  1,  0,  1,  0,  0,  0
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
    }

    GL_Environment::GL_Environment(const EnvironmentSpecification& spec)
    {
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        m_Specification = spec;

        glCreateFramebuffers(1, &m_FrameBuffer);
        glCreateRenderbuffers(1, &m_RenderBuffer);

        EnsureQuad();
        EnsureCube();

        auto& AM = AssetManager::GetInstance();
        m_SH_SkyBox             = AM.Get<GL_Shader>("ENV_SKY");
        m_SH_Irradiance         = AM.Get<GL_Shader>("ENV_IRR");
        m_SH_Prefilter          = AM.Get<GL_Shader>("ENV_PRE");
        m_SH_BRDFLUT            = AM.Get<GL_Shader>("ENV_BRD");
        m_SH_EquirectToCube     = AM.Get<GL_Shader>("ENV_CUB");

        BakeHDR();
    }

    GL_Environment::~GL_Environment()
    {
        glDeleteTextures(1, &m_EnvironmentCube);
        glDeleteTextures(1, &m_PrefilterdCube);
        glDeleteTextures(1, &m_IrradianceCube);
        glDeleteTextures(1, &m_BRDFLUT);
        glDeleteTextures(1, &m_HDR);

        glDeleteFramebuffers(1, &m_FrameBuffer);
        glDeleteRenderbuffers(1, &m_RenderBuffer);
        if (m_SHUBO) glDeleteBuffers(1, &m_SHUBO);

        if (s_QuadVAO) { glDeleteVertexArrays(1, &s_QuadVAO); s_QuadVAO = 0; }
        if (s_QuadVBO) { glDeleteBuffers(1, &s_QuadVBO); s_QuadVBO = 0; }
        if (s_CubeVAO) { glDeleteVertexArrays(1, &s_CubeVAO); s_CubeVAO = 0; }
        if (s_CubeVBO) { glDeleteBuffers(1, &s_CubeVBO); s_CubeVBO = 0; }
    }

    void GL_Environment::RenderSkyBox(const glm::mat4& proj, const glm::mat4& view, float yawnRadians) const
    {
        // ── Save current GL state we touch
        glEnable(GL_DEPTH_TEST);

        GLint prevDepthFunc;    
        glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);

        GLboolean prevDepthMask;    
        glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);

        GLboolean prevCullEnabled = glIsEnabled(GL_CULL_FACE);
        
        GLint prevCullFace;     
        glGetIntegerv(GL_CULL_FACE_MODE, &prevCullFace);

        // ── Configure for skybox
        glDepthFunc(GL_LEQUAL);   // GL_GEQUAL if you run reversed-Z
        glDepthMask(GL_FALSE);    // never write depth

        // Cull front faces so the inside of the unit cube renders
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        m_SH_SkyBox->Bind();

        const glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
        const glm::mat4 R           = glm::rotate(glm::mat4(1), yawnRadians, glm::vec3(0,1,0));

        m_SH_SkyBox->SetUniform("u_Proj", proj);
        m_SH_SkyBox->SetUniform("u_View", R * viewNoTrans); 

        static constexpr std::int32_t ENV_TEX_SLOT = 5;
        m_SH_SkyBox->SetUniform("u_EnvironmentTexture", ENV_TEX_SLOT);
        glBindTextureUnit(ENV_TEX_SLOT, m_EnvironmentCube);

        glBindVertexArray(s_CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // ── Restore state
        if (!prevCullEnabled) glDisable(GL_CULL_FACE);
        glCullFace(prevCullFace);

        glDepthMask(prevDepthMask);
        glDepthFunc(prevDepthFunc);
    }

    void GL_Environment::BindIBL(const IBLTextureBinding & params)
    {
        glBindTextureUnit(params.SlotPrefiltered, m_PrefilterdCube);
        glBindTextureUnit(params.SlotBRDFLUT,     m_BRDFLUT);

        if (m_Specification.UseSHDiffuse && m_SHReady) 
        {
            if (m_SHReady && m_SHUBO)
                glBindBufferBase(GL_UNIFORM_BUFFER, IEnvironment::SH_BUFFER_BINDING_POINT, m_SHUBO);
        } 
        else 
        {
            glBindTextureUnit(params.SlotIrradiance, m_IrradianceCube);
        }
    }

    void GL_Environment::SetIntensity(float diffuse, float specular)
    {
        m_Specification.SpecularIntensity = specular;
        m_Specification.DiffuseIntensity  = diffuse;
    }

    void GL_Environment::BakeHDR()
    {
        if(m_Specification.HDRfile.empty() || !std::filesystem::exists(m_Specification.HDRfile))
        {
            MOTION_ASSERT(false, "Can not find the HDR file for environment creation");
            return;
        }

        std::int32_t w{0}, h{0}, ch{0};
        stbi_set_flip_vertically_on_load(true);
        std::string hdrFile = m_Specification.HDRfile.string();
        float* data = stbi_loadf(hdrFile.c_str(), &w, &h, &ch, 4);
        if(!data || w <= 0 || h <= 0)
        {
            MOTION_ASSERT(false, "Faild to read HDR file");
            return;
        }

        GLenum format = GL_RGBA;
        glCreateTextures(GL_TEXTURE_2D, 1, &m_HDR);
        glTextureStorage2D(m_HDR, 1, GL_RGBA16F, w, h);
        glTextureSubImage2D(m_HDR, 0, 0, 0, w, h, format, GL_FLOAT, data);
        glTextureParameteri(m_HDR, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_HDR, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_HDR, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_HDR, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);

        if(m_Specification.UseSHDiffuse)
        {
            m_SH9Diffuse = SH9::ProjectEquirectHDR(m_Specification.HDRfile); // fills coeff[9]
            for (int i = 0; i < 9; ++i)
                m_SHPacked[i] = glm::vec4(m_SH9Diffuse.coeff[i], 0.0f);
        }


        //01) Creating Environment Cube and Rendering Equirect Cube
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_EnvironmentCube);
        glTextureStorage2D(m_EnvironmentCube, 1, GL_RGBA16F, ENVIRONMENT_CUBE_SIZE, ENVIRONMENT_CUBE_SIZE);
        glTextureParameteri(m_EnvironmentCube, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_EnvironmentCube, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_EnvironmentCube, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_EnvironmentCube, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_EnvironmentCube, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glNamedRenderbufferStorage(m_RenderBuffer, GL_DEPTH_COMPONENT24, ENVIRONMENT_CUBE_SIZE, ENVIRONMENT_CUBE_SIZE);

        const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        const glm::mat4 views[6] = {
            glm::lookAt(glm::vec3(0), glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0))
        };

        m_SH_EquirectToCube->Bind();
        m_SH_EquirectToCube->SetUniform("u_EquiRectangular", 0);
        glBindTextureUnit(0, m_HDR);

        for(std::int32_t face = 0; face < 6; ++face)
        {
            glNamedFramebufferRenderbuffer(m_FrameBuffer, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_RenderBuffer);
            glNamedFramebufferTextureLayer(m_FrameBuffer, GL_COLOR_ATTACHMENT0, m_EnvironmentCube, 0, face);
            
            const GLenum drawBuff = GL_COLOR_ATTACHMENT0;
            glNamedFramebufferDrawBuffers(m_FrameBuffer, 1, &drawBuff); 
            MOTION_ASSERT(glCheckNamedFramebufferStatus(m_FrameBuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
            glViewport(0, 0, ENVIRONMENT_CUBE_SIZE, ENVIRONMENT_CUBE_SIZE);

            m_SH_EquirectToCube->SetUniform("u_Proj", proj);
            m_SH_EquirectToCube->SetUniform("u_View", views[face]);

            glBindVertexArray(s_CubeVAO);
            glDisable(GL_DEPTH_TEST);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glGenerateTextureMipmap(m_EnvironmentCube);


        //02) BRDF LUT
        if(m_Specification.BuildBRDFLUT)
        {
            glCreateTextures(GL_TEXTURE_2D, 1, &m_BRDFLUT);
            glTextureStorage2D(m_BRDFLUT, 1, GL_RG16F, ENVIRONMENT_BRDF_LUT_SIZE, ENVIRONMENT_BRDF_LUT_SIZE);
            glTextureParameteri(m_BRDFLUT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(m_BRDFLUT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(m_BRDFLUT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(m_BRDFLUT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glNamedRenderbufferStorage(m_RenderBuffer, GL_DEPTH_COMPONENT24, ENVIRONMENT_BRDF_LUT_SIZE, ENVIRONMENT_BRDF_LUT_SIZE);
            glNamedFramebufferRenderbuffer(m_FrameBuffer, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_RenderBuffer);
            glNamedFramebufferTexture(m_FrameBuffer, GL_COLOR_ATTACHMENT0, m_BRDFLUT, 0);
            const GLenum db = GL_COLOR_ATTACHMENT0; 
            glNamedFramebufferDrawBuffers(m_FrameBuffer, 1, &db);
            MOTION_ASSERT(glCheckNamedFramebufferStatus(m_FrameBuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
            glViewport(0, 0, ENVIRONMENT_BRDF_LUT_SIZE, ENVIRONMENT_BRDF_LUT_SIZE);

            m_SH_BRDFLUT->Bind();
            glBindVertexArray(s_QuadVAO);
            glDisable(GL_DEPTH_TEST);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //03) Irradiance (if not using SH9)
        if(!m_Specification.UseSHDiffuse)
        {
            glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_IrradianceCube);
            glTextureStorage2D(m_IrradianceCube, 1, GL_RGBA16F, ENVIRONMENT_IRRADIANCE_SIZE, ENVIRONMENT_IRRADIANCE_SIZE);
            glTextureParameteri(m_IrradianceCube, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(m_IrradianceCube, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(m_IrradianceCube, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(m_IrradianceCube, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTextureParameteri(m_IrradianceCube, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            m_SH_Irradiance->Bind();
            m_SH_Irradiance->SetUniform("u_EnvironmentTexture", 0);
            glBindTextureUnit(0, m_EnvironmentCube);
            for (int face=0; face<6; ++face) 
            {
                glNamedFramebufferRenderbuffer(m_FrameBuffer, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_RenderBuffer);
                glNamedFramebufferTextureLayer(m_FrameBuffer, GL_COLOR_ATTACHMENT0, m_IrradianceCube, 0, face);
                const GLenum db = GL_COLOR_ATTACHMENT0; glNamedFramebufferDrawBuffers(m_FrameBuffer, 1, &db);
                MOTION_ASSERT(glCheckNamedFramebufferStatus(m_FrameBuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
                glViewport(0,0, ENVIRONMENT_IRRADIANCE_SIZE, ENVIRONMENT_IRRADIANCE_SIZE);
                
                m_SH_Irradiance->SetUniform("u_Proj", proj);
                m_SH_Irradiance->SetUniform("u_View", views[face]);

                glBindVertexArray(s_CubeVAO);
                glDisable(GL_DEPTH_TEST);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        //04) Prefiltered Specular
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_PrefilterdCube);
        const std::int32_t mipCount = 1 + (std::int32_t)std::floor(std::log2((float)ENVIRONMENT_PREFILTERED_SIZE));
        glTextureStorage2D(m_PrefilterdCube, mipCount, GL_RGBA16F, ENVIRONMENT_PREFILTERED_SIZE, ENVIRONMENT_PREFILTERED_SIZE);
        glTextureParameteri(m_PrefilterdCube, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(m_PrefilterdCube, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_PrefilterdCube, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_PrefilterdCube, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_PrefilterdCube, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        m_SH_Prefilter->Bind();
        m_SH_Prefilter->SetUniform("u_EnvironmentTexture", 0);
        glBindTextureUnit(0, m_EnvironmentCube);

        int size = ENVIRONMENT_PREFILTERED_SIZE;
        for (int mip = 0; mip < mipCount; ++mip) 
        {
            const float rough = (float)mip / (float)(mipCount-1);
            m_SH_Prefilter->SetUniform("u_PrefilteredRoughness", rough);
            glNamedRenderbufferStorage(m_RenderBuffer, GL_DEPTH_COMPONENT24, size, size);
            
            for (int face=0; face<6; ++face) 
            {
                glNamedFramebufferRenderbuffer(m_FrameBuffer, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_RenderBuffer);
                glNamedFramebufferTextureLayer(m_FrameBuffer, GL_COLOR_ATTACHMENT0, m_PrefilterdCube, mip, face);
                const GLenum db = GL_COLOR_ATTACHMENT0; glNamedFramebufferDrawBuffers(m_FrameBuffer, 1, &db);
                MOTION_ASSERT(glCheckNamedFramebufferStatus(m_FrameBuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
                glViewport(0, 0, size, size);

                m_SH_Prefilter->SetUniform("u_Proj", proj);
                m_SH_Prefilter->SetUniform("u_View", views[face]);

                glBindVertexArray(s_CubeVAO);
                glDisable(GL_DEPTH_TEST);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            size = std::max(1, size / 2);
        }

        //06) Creating SH9
        if (m_Specification.UseSHDiffuse) 
        {
            glGenBuffers(1, &m_SHUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, m_SHUBO);
            glBufferData(GL_UNIFORM_BUFFER, m_SHPacked.size() * sizeof(glm::vec4), m_SHPacked.data(), GL_STATIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, IEnvironment::SH_BUFFER_BINDING_POINT, m_SHUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            m_SHReady = true;
        }
    }
}