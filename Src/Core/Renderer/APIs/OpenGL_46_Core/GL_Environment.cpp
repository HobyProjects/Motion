#include "CorePCH.hpp"
#include "GL_Environment.hpp"

namespace Motion
{
    static constexpr std::int32_t ENVIRONMENT_CUBE_SIZE        = 2048;
    static constexpr std::int32_t ENVIRONMENT_IRRADIANCE_SIZE  = 32;
    static constexpr std::int32_t ENVIRONMENT_PREFILTERED_SIZE = 128;
    static constexpr std::int32_t ENVIRONMENT_BRDF_LUT_SIZE    = 512;

    struct ScopedFBOAndViewport 
    {
        GLint prevDraw{}, prevRead{}, vp[4]{};

        ScopedFBOAndViewport(GLuint fbo, int w, int h) 
        {
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDraw);
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevRead);
            glGetIntegerv(GL_VIEWPORT, vp);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
            glViewport(0, 0, w, h);
        }

        ~ScopedFBOAndViewport() 
        {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDraw);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, prevRead);
            glViewport(vp[0], vp[1], vp[2], vp[3]);
        }
    };


    GeometryCache& GeometryCache::Get()
    { 
        static GeometryCache g; 
        return g; 
    }

    GeometryCache::GeometryCache()
    {
        {
            const float quad[] = 
            { 
                // Triangle 1
                -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,  // bottom-left
                 1.0f, -1.0f, 0.0f,   1.0f, 0.0f,  // bottom-right
                 1.0f,  1.0f, 0.0f,   1.0f, 1.0f,  // top-right

                // Triangle 2
                 1.0f,  1.0f, 0.0f,   1.0f, 1.0f,  // top-right
                -1.0f,  1.0f, 0.0f,   0.0f, 1.0f,  // top-left
                -1.0f, -1.0f, 0.0f,   0.0f, 0.0f   // bottom-left
            };

            glGenVertexArrays(1, &m_quadVAO.id); glGenBuffers(1, &m_quadVBO);
            glBindVertexArray(m_quadVAO.id);
            glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0); 
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1); 
        }
        {
            float v[] = 
            {
                // --- Front face ---
                -1.0f, -1.0f,  1.0f,   0.0f, 0.0f, // bottom-left
                 1.0f, -1.0f,  1.0f,   1.0f, 0.0f, // bottom-right
                 1.0f,  1.0f,  1.0f,   1.0f, 1.0f, // top-right
                 1.0f,  1.0f,  1.0f,   1.0f, 1.0f, // top-right
                -1.0f,  1.0f,  1.0f,   0.0f, 1.0f, // top-left
                -1.0f, -1.0f,  1.0f,   0.0f, 0.0f, // bottom-left

                // --- Back face ---
                -1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 
                -1.0f,  1.0f, -1.0f,   1.0f, 1.0f, 
                 1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 
                 1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 
                 1.0f, -1.0f, -1.0f,   0.0f, 0.0f, 
                -1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 

                // --- Left face ---
                -1.0f,  1.0f,  1.0f,   1.0f, 1.0f, 
                -1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 
                -1.0f, -1.0f, -1.0f,   0.0f, 0.0f, 
                -1.0f, -1.0f, -1.0f,   0.0f, 0.0f, 
                -1.0f, -1.0f,  1.0f,   1.0f, 0.0f, 
                -1.0f,  1.0f,  1.0f,   1.0f, 1.0f, 

                // --- Right face ---
                 1.0f,  1.0f,  1.0f,   0.0f, 1.0f, 
                 1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 
                 1.0f,  1.0f, -1.0f,   1.0f, 1.0f, 
                 1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 
                 1.0f,  1.0f,  1.0f,   0.0f, 1.0f, 
                 1.0f, -1.0f,  1.0f,   0.0f, 0.0f, 

                // --- Bottom face ---
                -1.0f, -1.0f, -1.0f,   0.0f, 1.0f, 
                 1.0f, -1.0f, -1.0f,   1.0f, 1.0f, 
                 1.0f, -1.0f,  1.0f,   1.0f, 0.0f, 
                 1.0f, -1.0f,  1.0f,   1.0f, 0.0f, 
                -1.0f, -1.0f,  1.0f,   0.0f, 0.0f, 
                -1.0f, -1.0f, -1.0f,   0.0f, 1.0f, 

                // --- Top face ---
                -1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 
                -1.0f,  1.0f,  1.0f,   0.0f, 0.0f, 
                 1.0f,  1.0f,  1.0f,   1.0f, 0.0f, 
                 1.0f,  1.0f,  1.0f,   1.0f, 0.0f, 
                 1.0f,  1.0f, -1.0f,   1.0f, 1.0f, 
                -1.0f,  1.0f, -1.0f,   0.0f, 1.0f  
            };

            glGenVertexArrays(1, &m_cubeVAO.id); glGenBuffers(1, &m_cubeVBO);
            glBindVertexArray(m_cubeVAO.id);
            glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0); 
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1); 
        }
    }

    GL_Environment::GL_Environment(const EnvironmentSpecification& spec)
    {
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        m_Spec = spec;

        glCreateFramebuffers(1, &m_fbo.id);
        glCreateRenderbuffers(1, &m_rbo.id);

        auto& AM = AssetManager::GetInstance();
        m_shSky        = AM.Get<GL_Shader>("ENV_SKY");
        m_shIrradiance = AM.Get<GL_Shader>("ENV_IRR");
        m_shPrefilter  = AM.Get<GL_Shader>("ENV_PRE");
        m_shBRDF       = AM.Get<GL_Shader>("ENV_BRD");
        m_shEquiToCube = AM.Get<GL_Shader>("ENV_CUB");

        BakeHDR();
    }

    GL_Environment::~GL_Environment()
    {
        DestroyGPU();
    }

    void GL_Environment::DestroyGPU()
    {
        if (m_shUBO.id)
            glBindBufferBase(GL_UNIFORM_BUFFER, IEnvironment::SH_BUFFER_BINDING_POINT, 0);

        m_shUBO.reset();
        m_texEnvironmentCube.reset();
        m_texPrefiltered.reset();
        m_texIrradiance.reset();
        m_texBRDFLUT.reset();
        m_texHDR.reset();
        
        m_shReady = false;
    }

    void GL_Environment::UpdateSpecification(const EnvironmentSpecification& spec)
    {
        m_Spec = spec;
        BakeHDR();
    }

    void GL_Environment::RenderSkyBox(const glm::mat4& proj, const glm::mat4& view, float yawnRadians) const
    {
        if (!m_texEnvironmentCube.id || !m_shSky) return;

        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);           
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);          
        glDepthFunc(GL_LEQUAL);         

        m_shSky->Bind();
        const glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
        const glm::mat4 R = glm::rotate(glm::mat4(1.0f), yawnRadians, glm::vec3(0, 1, 0));
        m_shSky->SetUniform("u_Proj", proj);
        m_shSky->SetUniform("u_View", R * viewNoTrans);

        m_shSky->SetUniform("u_SkyIntensity",   m_Spec.Intensity);
        m_shSky->SetUniform("u_SkyExposure",    m_Spec.Exposure);
        m_shSky->SetUniform("u_SkyGamma",       m_Spec.Gamma);
        m_shSky->SetUniform("u_SkyMipLevel",    m_Spec.MaxMipLevel);
        m_shSky->SetUniform("u_Tonemap",        m_Spec.Tonemap);

        m_shSky->SetUniform("u_EnvironmentTexture", TextureSlot::Skybox);
        glBindTextureUnit(TextureSlot::Skybox, m_texEnvironmentCube.id);

        glBindVertexArray(GeometryCache::Get().CubeVAO());
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glCullFace(GL_BACK);
    }

    void GL_Environment::BindIBL(const IBLTextureBinding & params)
    {
        glBindTextureUnit(params.SlotPrefiltered, m_texPrefiltered.id);
        glBindTextureUnit(params.SlotBRDFLUT,     m_texBRDFLUT.id);

        if (m_Spec.UseSHDiffuse && m_shReady) {
            if (m_shUBO.id)
                glBindBufferBase(GL_UNIFORM_BUFFER, IEnvironment::SH_BUFFER_BINDING_POINT, m_shUBO.id);
        } else {
            glBindTextureUnit(params.SlotIrradiance, m_texIrradiance.id);
        }
    }

    void GL_Environment::BakeHDR()
    {
        DestroyGPU();
        if (!LoadHDR()) return;           
        BuildEnvironmentCube();    
        if (m_Spec.BuildBRDFLUT) BuildBRDFLUT();
        if (!m_Spec.UseSHDiffuse) BuildIrradiance();
        BuildPrefiltered();
        if (m_Spec.UseSHDiffuse) BuildSH();
    }

    bool GL_Environment::LoadHDR()
    {
        if(m_Spec.HDRfile.empty() || !std::filesystem::exists(m_Spec.HDRfile))
        {
            MOTION_ASSERT(false, "Cannot find HDR file for environment");
            return false;
        }

        stbi_set_flip_vertically_on_load(false);
        std::int32_t ch{0};
        float* data = stbi_loadf(m_Spec.HDRfile.string().c_str(), &m_hdrW, &m_hdrH, &ch, 4);
        if(!data || m_hdrW <= 0 || m_hdrH <= 0)
        {
            MOTION_ASSERT(false, "Failed to read HDR file");
            return false;
        }
        glCreateTextures(GL_TEXTURE_2D, 1, &m_texHDR.id);
        glTextureStorage2D(m_texHDR.id, 1, GL_RGBA16F, m_hdrW, m_hdrH);
        glTextureSubImage2D(m_texHDR.id, 0, 0, 0, m_hdrW, m_hdrH, GL_RGBA, GL_FLOAT, data);
        glTextureParameteri(m_texHDR.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_texHDR.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_texHDR.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_texHDR.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
        return true;
    }

    void GL_Environment::BuildEnvironmentCube()
    {
        const int size = ENVIRONMENT_CUBE_SIZE;
        const int levels = 1 + (int)std::floor(std::log2((double)size));

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_texEnvironmentCube.id);
        glTextureStorage2D(m_texEnvironmentCube.id, levels, GL_RGBA16F, size, size);

        glTextureParameteri(m_texEnvironmentCube.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(m_texEnvironmentCube.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_texEnvironmentCube.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_texEnvironmentCube.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_texEnvironmentCube.id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_texEnvironmentCube.id, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(m_texEnvironmentCube.id, GL_TEXTURE_MAX_LEVEL, levels - 1);

        // Depth RBO sized for this pass (ensure m_rbo was created in ctor)
        glNamedRenderbufferStorage(m_rbo.id, GL_DEPTH_COMPONENT24, size, size);

        const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        const glm::mat4 views[6] = {
            glm::lookAt(glm::vec3(0), glm::vec3( 1,0,0), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(-1,0,0), glm::vec3(0,-1,0)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0,1,0), glm::vec3(0, 0,1)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0,-1,0),glm::vec3(0, 0,-1)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0,0,1), glm::vec3(0,-1,0)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0,0,-1),glm::vec3(0,-1,0))
        };

        m_shEquiToCube->Bind();
        m_shEquiToCube->SetUniform("u_EquiRectangular", 0);
        glBindTextureUnit(0, m_texHDR.id);

        ScopedFBOAndViewport bind(m_fbo.id, size, size);

        // Minimal state for inside-cube capture
        GLboolean wasDepth, wasCull;
        glGetBooleanv(GL_DEPTH_TEST, &wasDepth);
        glGetBooleanv(GL_CULL_FACE,  &wasCull);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE); // (or glEnable(GL_CULL_FACE); glCullFace(GL_FRONT);)

        glBindVertexArray(GeometryCache::Get().CubeVAO());

        for (int face = 0; face < 6; ++face)
        {
            glNamedFramebufferRenderbuffer(m_fbo.id, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo.id);
            glNamedFramebufferTextureLayer(m_fbo.id, GL_COLOR_ATTACHMENT0, m_texEnvironmentCube.id, 0, face);
            const GLenum db = GL_COLOR_ATTACHMENT0;
            glNamedFramebufferDrawBuffers(m_fbo.id, 1, &db);

            MOTION_ASSERT(glCheckNamedFramebufferStatus(m_fbo.id, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

            m_shEquiToCube->SetUniform("u_Proj", proj);
            m_shEquiToCube->SetUniform("u_View", views[face]);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Restore state
        if (wasCull)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
        if (wasDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);

        // Build the mip chain used by sky/IBL LOD sampling
        glGenerateTextureMipmap(m_texEnvironmentCube.id);
    }

    void GL_Environment::BuildBRDFLUT()
    {
        const int W = ENVIRONMENT_BRDF_LUT_SIZE;
        const int H = ENVIRONMENT_BRDF_LUT_SIZE;

        glCreateTextures(GL_TEXTURE_2D, 1, &m_texBRDFLUT.id);
        glTextureStorage2D(m_texBRDFLUT.id, 1, GL_RG16F, W, H);
        glTextureParameteri(m_texBRDFLUT.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_texBRDFLUT.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_texBRDFLUT.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_texBRDFLUT.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glNamedRenderbufferStorage(m_rbo.id, GL_DEPTH_COMPONENT24, W, H);
        glNamedFramebufferRenderbuffer(m_fbo.id, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo.id);
        glNamedFramebufferTexture(m_fbo.id, GL_COLOR_ATTACHMENT0, m_texBRDFLUT.id, 0);
        const GLenum db = GL_COLOR_ATTACHMENT0;
        glNamedFramebufferDrawBuffers(m_fbo.id, 1, &db);

        MOTION_ASSERT(glCheckNamedFramebufferStatus(m_fbo.id, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        ScopedFBOAndViewport bind(m_fbo.id, W, H);

        GLboolean wasDepth, wasCull;
        glGetBooleanv(GL_DEPTH_TEST, &wasDepth);
        glGetBooleanv(GL_CULL_FACE,  &wasCull);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        m_shBRDF->Bind();
        glBindVertexArray(GeometryCache::Get().QuadVAO());
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (wasCull)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
        if (wasDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    }

    void GL_Environment::BuildIrradiance()
    {
        const int S = ENVIRONMENT_IRRADIANCE_SIZE;

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_texIrradiance.id);
        glTextureStorage2D(m_texIrradiance.id, 1, GL_RGBA16F, S, S);
        glTextureParameteri(m_texIrradiance.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_texIrradiance.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_texIrradiance.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_texIrradiance.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_texIrradiance.id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glNamedRenderbufferStorage(m_rbo.id, GL_DEPTH_COMPONENT24, S, S);
        ScopedFBOAndViewport bind(m_fbo.id, S, S);

        GLboolean wasDepth, wasCull;
        glGetBooleanv(GL_DEPTH_TEST, &wasDepth);
        glGetBooleanv(GL_CULL_FACE,  &wasCull);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        const glm::mat4 views[6] = {
            glm::lookAt(glm::vec3(0), glm::vec3( 1,0,0), glm::vec3(0,-1,0)),
            glm::lookAt(glm::vec3(0), glm::vec3(-1,0,0), glm::vec3(0,-1,0)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0,1,0), glm::vec3(0, 0,1)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0,-1,0),glm::vec3(0, 0,-1)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0,0,1), glm::vec3(0,-1,0)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0,0,-1),glm::vec3(0,-1,0))
        };

        m_shIrradiance->Bind();
        m_shIrradiance->SetUniform("u_EnvironmentTexture", 0);
        glBindTextureUnit(0, m_texEnvironmentCube.id);

        glBindVertexArray(GeometryCache::Get().CubeVAO());

        for (int face = 0; face < 6; ++face)
        {
            glNamedFramebufferRenderbuffer(m_fbo.id, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo.id);
            glNamedFramebufferTextureLayer(m_fbo.id, GL_COLOR_ATTACHMENT0, m_texIrradiance.id, 0, face);
            const GLenum db = GL_COLOR_ATTACHMENT0;
            glNamedFramebufferDrawBuffers(m_fbo.id, 1, &db);

            MOTION_ASSERT(glCheckNamedFramebufferStatus(m_fbo.id, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

            m_shIrradiance->SetUniform("u_Proj", proj);
            m_shIrradiance->SetUniform("u_View", views[face]);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        if (wasCull)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
        if (wasDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    }

    void GL_Environment::BuildPrefiltered()
    {
        const int BASE = ENVIRONMENT_PREFILTERED_SIZE;
        const int mipCount = 1 + (int)std::floor(std::log2((double)BASE));

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_texPrefiltered.id);
        glTextureStorage2D(m_texPrefiltered.id, mipCount, GL_RGBA16F, BASE, BASE);
        glTextureParameteri(m_texPrefiltered.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(m_texPrefiltered.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_texPrefiltered.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_texPrefiltered.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_texPrefiltered.id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_texPrefiltered.id, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(m_texPrefiltered.id, GL_TEXTURE_MAX_LEVEL,  mipCount - 1);

        m_shPrefilter->Bind();
        m_shPrefilter->SetUniform("u_EnvironmentTexture", 0);
        glBindTextureUnit(0, m_texEnvironmentCube.id);

        GLboolean wasDepth, wasCull;
        glGetBooleanv(GL_DEPTH_TEST, &wasDepth);
        glGetBooleanv(GL_CULL_FACE,  &wasCull);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glBindVertexArray(GeometryCache::Get().CubeVAO());

        int size = BASE;
        for (int mip = 0; mip < mipCount; ++mip)
        {
            const float rough = (mipCount > 1) ? float(mip) / float(mipCount - 1) : 0.0f;
            m_shPrefilter->SetUniform("u_PrefilteredRoughness", rough);

            glNamedRenderbufferStorage(m_rbo.id, GL_DEPTH_COMPONENT24, size, size);

            ScopedFBOAndViewport bind(m_fbo.id, size, size);

            const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
            const glm::mat4 views[6] = {
                glm::lookAt(glm::vec3(0), glm::vec3( 1,0,0), glm::vec3(0,-1,0)),
                glm::lookAt(glm::vec3(0), glm::vec3(-1,0,0), glm::vec3(0,-1,0)),
                glm::lookAt(glm::vec3(0), glm::vec3( 0,1,0), glm::vec3(0, 0,1)),
                glm::lookAt(glm::vec3(0), glm::vec3( 0,-1,0),glm::vec3(0, 0,-1)),
                glm::lookAt(glm::vec3(0), glm::vec3( 0,0,1), glm::vec3(0,-1,0)),
                glm::lookAt(glm::vec3(0), glm::vec3( 0,0,-1),glm::vec3(0,-1,0))
            };

            for (int face = 0; face < 6; ++face)
            {
                glNamedFramebufferRenderbuffer(m_fbo.id, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo.id);
                glNamedFramebufferTextureLayer(m_fbo.id, GL_COLOR_ATTACHMENT0, m_texPrefiltered.id, mip, face);
                const GLenum db = GL_COLOR_ATTACHMENT0;
                glNamedFramebufferDrawBuffers(m_fbo.id, 1, &db);

                MOTION_ASSERT(glCheckNamedFramebufferStatus(m_fbo.id, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                m_shPrefilter->SetUniform("u_Proj", proj);
                m_shPrefilter->SetUniform("u_View", views[face]);

                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            size = std::max(1, size / 2);
        }

        if (wasCull)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
        if (wasDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    }

    void GL_Environment::BuildSH()
    {
        m_SH9Diffuse = SH9::ProjectEquirectHDR(m_Spec.HDRfile);
        for (int i = 0; i < 9; ++i) m_shPacked[i] = glm::vec4(m_SH9Diffuse.coeff[i], 0.0f);

        glGenBuffers(1, &m_shUBO.id);
        glBindBuffer(GL_UNIFORM_BUFFER, m_shUBO.id);
        glBufferData(GL_UNIFORM_BUFFER, m_shPacked.size() * sizeof(glm::vec4), m_shPacked.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, IEnvironment::SH_BUFFER_BINDING_POINT, m_shUBO.id);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        m_shReady = true;
    }
}