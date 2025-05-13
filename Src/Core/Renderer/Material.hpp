#pragma once

#include "Shaders.hpp"
#include "Texture.hpp"

namespace Motion::Core
{
    class Material
    {
        public:
            explicit Material(const std::shared_ptr<IShader>& shader);
            ~Material() = default;

            void Bind();
            void Unbind();

            void SetUniform(const std::string& name, float value);
            void SetUniform(const std::string& name, const glm::vec3& value);
            void SetTexture(const std::string& name, const std::shared_ptr<ITexture>& texture);

        private:
            std::shared_ptr<IShader> m_Shader;
            std::unordered_map<std::string, float> m_FloatUniformsMaps;
            std::unordered_map<std::string, glm::vec3> m_Vec3UniformsMaps;
            std::unordered_map<std::string, std::shared_ptr<ITexture>> m_TexturesMaps;
    };
}