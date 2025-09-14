#pragma once

#include <filesystem>

#include "Buffers.hpp"
#include "Texture.hpp"
#include "Environment.hpp"

namespace Motion
{
    struct GLTextureRAII 
    {
        TextureID id{0};
        GLTextureRAII() = default;
        explicit GLTextureRAII(TextureID t) : id(t) {}
        GLTextureRAII(const GLTextureRAII&) = delete;
        GLTextureRAII& operator=(const GLTextureRAII&) = delete;
        GLTextureRAII(GLTextureRAII&& o) noexcept : id(std::exchange(o.id, 0)) {}
        GLTextureRAII& operator=(GLTextureRAII&& o) noexcept { if (this != &o) { reset(); id = std::exchange(o.id, 0);} return *this; }
        ~GLTextureRAII(){ reset(); }
        void reset(){ if(id) glDeleteTextures(1, &id), id = 0; }
        operator TextureID() const { return id; }
    };

    struct GLBufferRAII 
    {
        BufferID id{0};
        GLBufferRAII() = default;
        explicit GLBufferRAII(BufferID b) : id(b) {}
        GLBufferRAII(const GLBufferRAII&) = delete;
        GLBufferRAII& operator=(const GLBufferRAII&) = delete;
        GLBufferRAII(GLBufferRAII&& o) noexcept : id(std::exchange(o.id, 0)) {}
        GLBufferRAII& operator=(GLBufferRAII&& o) noexcept { if (this != &o) { reset(); id = std::exchange(o.id, 0);} return *this; }
        ~GLBufferRAII(){ reset(); }
        void reset(){ if(id) glDeleteBuffers(1, &id), id = 0; }
        operator BufferID() const { return id; }
    };

    struct GLFramebufferRAII 
    {
        GLuint id{0};
        GLFramebufferRAII() = default;
        explicit GLFramebufferRAII(GLuint v) : id(v) {}
        GLFramebufferRAII(const GLFramebufferRAII&) = delete;
        GLFramebufferRAII& operator=(const GLFramebufferRAII&) = delete;
        GLFramebufferRAII(GLFramebufferRAII&& o) noexcept : id(std::exchange(o.id, 0)) {}
        GLFramebufferRAII& operator=(GLFramebufferRAII&& o) noexcept { if (this != &o) { reset(); id = std::exchange(o.id, 0);} return *this; }
        ~GLFramebufferRAII(){ reset(); }
        void reset(){ if(id) glDeleteFramebuffers(1, &id), id = 0; }
        operator GLuint() const { return id; }
    };

    struct GLRenderbufferRAII 
    {
        GLuint id{0};
        GLRenderbufferRAII() = default;
        explicit GLRenderbufferRAII(GLuint v) : id(v) {}
        GLRenderbufferRAII(const GLRenderbufferRAII&) = delete;
        GLRenderbufferRAII& operator=(const GLRenderbufferRAII&) = delete;
        GLRenderbufferRAII(GLRenderbufferRAII&& o) noexcept : id(std::exchange(o.id, 0)) {}
        GLRenderbufferRAII& operator=(GLRenderbufferRAII&& o) noexcept { if (this != &o) { reset(); id = std::exchange(o.id, 0);} return *this; }
        ~GLRenderbufferRAII(){ reset(); }
        void reset(){ if(id) glDeleteRenderbuffers(1, &id), id = 0; }
        operator GLuint() const { return id; }
    };

    struct GLVertexArrayRAII 
    {
        GLuint id{0};
        GLVertexArrayRAII() = default;
        explicit GLVertexArrayRAII(GLuint v) : id(v) {}
        GLVertexArrayRAII(const GLVertexArrayRAII&) = delete;
        GLVertexArrayRAII& operator=(const GLVertexArrayRAII&) = delete;
        GLVertexArrayRAII(GLVertexArrayRAII&& o) noexcept : id(std::exchange(o.id, 0)) {}
        GLVertexArrayRAII& operator=(GLVertexArrayRAII&& o) noexcept { if (this != &o) { reset(); id = std::exchange(o.id, 0);} return *this; }
        ~GLVertexArrayRAII(){ reset(); }
        void reset(){ if(id) glDeleteVertexArrays(1, &id), id = 0; }
        operator GLuint() const { return id; }
    };

    class GeometryCache 
    {
    public:
        static GeometryCache& Get();
        GLuint QuadVAO()  const { return m_quadVAO.id; }
        GLuint CubeVAO()  const { return m_cubeVAO.id; }
    private:
        GeometryCache();
        GLVertexArrayRAII m_quadVAO{}; GLuint m_quadVBO{};
        GLVertexArrayRAII m_cubeVAO{}; GLuint m_cubeVBO{};
    };

    class GL_Environment final : public IEnvironment
    {
    public:
        GL_Environment(const EnvironmentSpecification& spec);
        ~GL_Environment() override;

        void RenderSkyBox(const glm::mat4& proj, const glm::mat4& view, float yawnRadians = 0.0f) const override;
        void BindIBL(const IBLTextureBinding& params) override;

        void UpdateSpecification(const EnvironmentSpecification& spec) override; // NEW
        [[nodiscard]] EnvironmentSpecification& GetSpecification() override { return m_Spec; }
        [[nodiscard]] SH9&  GetDiffuseSH() override { return m_SH9Diffuse; }
        [[nodiscard]] bool IsUsingSH() override { return m_Spec.UseSHDiffuse; }

    protected:
        void BakeHDR() override;

    private:
        // Steps broken down clearly
        bool  LoadHDR();
        void  BuildEnvironmentCube();
        void  BuildBRDFLUT();
        void  BuildIrradiance();
        void  BuildPrefiltered();
        void  BuildSH();
        void  DestroyGPU();

    private:
        // GPU objects
        GLTextureRAII m_texEnvironmentCube{};
        GLTextureRAII m_texPrefiltered{};
        GLTextureRAII m_texIrradiance{};
        GLTextureRAII m_texBRDFLUT{};
        GLTextureRAII m_texHDR{};

        GLFramebufferRAII  m_fbo{};
        GLRenderbufferRAII m_rbo{};
        GLBufferRAII  m_shUBO{};

        // Shaders
        std::shared_ptr<GL_Shader> m_shEquiToCube{nullptr};
        std::shared_ptr<GL_Shader> m_shIrradiance{nullptr};
        std::shared_ptr<GL_Shader> m_shPrefilter{nullptr};
        std::shared_ptr<GL_Shader> m_shBRDF{nullptr};
        std::shared_ptr<GL_Shader> m_shSky{nullptr};

        // Data
        EnvironmentSpecification m_Spec{};
        SH9 m_SH9Diffuse{};
        std::array<glm::vec4, 9> m_shPacked{};
        bool m_shReady{false};

        // Cached
        int m_hdrW{0}, m_hdrH{0};
    };
}
