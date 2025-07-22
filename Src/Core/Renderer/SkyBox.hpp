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

        void Render(const glm::mat4& transformMatrix, const glm::mat4 viewProjectionMatrix) noexcept;

    private:
        std::shared_ptr<ICubeMapTexture> m_CubeMapTexture;
        std::shared_ptr<IShader> m_ShaderProgram;
        std::shared_ptr<Mesh> m_SkyBoxMesh;
    };
}