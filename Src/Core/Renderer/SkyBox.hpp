#pragma once

#include "Shaders.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"

namespace Motion::Core
{
    class SkyBox
    {
    public:
        SkyBox();
        ~SkyBox() = default;

        void Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) noexcept;

    private:
        std::shared_ptr<ICubeMapTexture> m_CubeMapTexture;
        std::shared_ptr<IShader> m_ShaderProgram;
        std::shared_ptr<Mesh> m_SkyBoxMesh;
    };
}