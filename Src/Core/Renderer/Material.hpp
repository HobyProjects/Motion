#pragma once

#include "Shaders.hpp"
#include "Texture.hpp"
#include "Asset.hpp"

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
        glm::vec3 BaseColor{0.0f};
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


    class Material final : public AssetBase<IAsset>
    {
        public:
            enum class ShadingMethod { Phong, PBR, Unlit, Unknown };

        public:
            Material(const std::string& name, const std::string& materialFile):
                AssetBase<IAsset>(UniqueIdentity::GetUniqueID(), name, AssetType::Material, materialFile){}
            Material(UUID uuid, const std::string& name, ShadingMethod shadingMethod, const std::string& materialFile):
                AssetBase<IAsset>(uuid, name, AssetType::Material, materialFile), m_Shading(shadingMethod){}
            virtual ~Material() = default;

            void Bind();
            void Bind(const std::shared_ptr<IShader>& shader);
            void Unbind();

            void SetUniform(const std::string& name, float value);
            void SetUniform(const std::string& name, const glm::vec3& value);
            void SetUniform(const std::string& name, const glm::vec4& value);
            void SetTexture(const std::string& name, const std::shared_ptr<ITexture>& texture);

            float GetFloatUniform(const std::string& name) const { return m_FloatUniformsMaps.at(name); }
            glm::vec3 GetVec3Uniform(const std::string& name) const { return m_Vec3UniformsMaps.at(name); }
            glm::vec4 GetVec4Uniform(const std::string& name) const { return m_Vec4UniformsMaps.at(name); }
            std::shared_ptr<ITexture> GetTexture(const std::string& name) const { return m_TexturesMaps.at(name); }
            ShadingMethod GetShadingMethod();

            std::unordered_map<std::string, float> GetFloatUniforms() const { return m_FloatUniformsMaps; }
            std::unordered_map<std::string, glm::vec3> GetVec3Uniforms() const { return m_Vec3UniformsMaps; }
            std::unordered_map<std::string, glm::vec4> GetVec4Uniforms() const { return m_Vec4UniformsMaps; }
            std::unordered_map<std::string, std::shared_ptr<ITexture>> GetTextures() const { return m_TexturesMaps; }

        private:
            ShadingMethod DetectShadingMethod(); 

        private:
            std::unordered_map<std::string, float> m_FloatUniformsMaps;
            std::unordered_map<std::string, glm::vec3> m_Vec3UniformsMaps;
            std::unordered_map<std::string, glm::vec4> m_Vec4UniformsMaps;
            std::unordered_map<std::string, std::shared_ptr<ITexture>> m_TexturesMaps;
            ShadingMethod m_Shading{ShadingMethod::Unknown};
    };
}