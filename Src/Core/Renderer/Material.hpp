#pragma once

#include "Shaders.hpp"
#include "Texture.hpp"

namespace Motion::Core
{
    struct SurfaceColors
    {
        glm::vec3 AmbientColor{0.0f};
        glm::vec3 DiffuseColor{0.0f};
        glm::vec3 SpecularColor{0.0f};
        glm::vec3 EmissiveColor{0.0f};
        glm::vec3 TransparentColor{0.0f};
        glm::vec3 ReflectiveColor{0.0f};
    };

    struct MaterialProperties
    {
        float Shininess{0.0f};
        float ShininessStrenght{0.0f};
        float Opacity{0.0f};
        float IndexOfRefraction{0.0f};
        float BumpScaling{0.0};
        float Reflectivity{0.0};
    };

    struct MaterialFactors
    {
        glm::vec4 BaseColor{0.0f};
        float MetalicFactor{0.0f};
        float RoughnessFactor{0.0f};
        float TransmissionFactor{0.0f};
        float ClearCoatFactor{0.0f};
        float ClearCoatRoughnessFactor{0.0f};
        float SheenColorFactor{0.0f};
        float SheenRoughnessFactor{0.0f};
        float IndexOfRefraction{0.0f};
        float AmbientOcclusionFactor{0.0f};
    };


    class Material
    {
        public:
            enum class ShadingMethod { Phong, PBR };

        public:
            Material() = default;
            ~Material() = default;

            void Bind();
            void Unbind();

            void SetUniform(const std::string& name, float value);
            void SetUniform(const std::string& name, const glm::vec3& value);
            void SetUniform(const std::string& name, const glm::vec4& value);
            void SetTexture(const std::string& name, const std::shared_ptr<ITexture>& texture);

        private:
            void DetectShadingMethod();

        private:
            std::weak_ptr<IShader> m_Shader;
            std::unordered_map<std::string, float> m_FloatUniformsMaps;
            std::unordered_map<std::string, glm::vec3> m_Vec3UniformsMaps;
            std::unordered_map<std::string, glm::vec4> m_Vec4UniformsMaps;
            std::unordered_map<std::string, std::shared_ptr<ITexture>> m_TexturesMaps;
            ShadingMethod Shading{ShadingMethod::PBR};
    };
}