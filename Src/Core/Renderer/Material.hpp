#pragma once

#include "Shaders.hpp"
#include "Texture.hpp"

namespace Motion::Core
{
    class Material
    {
        public:
            Material(const std::string& shaderName);
            ~Material() = default;

            void Bind();
            void Unbind();

            void SetUniform(const std::string& name, float value);
            void SetUniform(const std::string& name, const glm::vec3& value);
            void SetUniform(const std::string& name, const glm::vec4& value);
            void SetTexture(const std::string& name, const std::shared_ptr<ITexture>& texture);

        private:
            std::weak_ptr<IShader> m_Shader;
            std::unordered_map<std::string, float> m_FloatUniformsMaps;
            std::unordered_map<std::string, glm::vec3> m_Vec3UniformsMaps;
            std::unordered_map<std::string, glm::vec4> m_Vec4UniformsMaps;
            std::unordered_map<std::string, std::shared_ptr<ITexture>> m_TexturesMaps;
    };
}