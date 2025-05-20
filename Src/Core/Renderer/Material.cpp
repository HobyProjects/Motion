#include "CorePCH.hpp"

namespace Motion::Core
{
    void Material::SetUniform(const std::string& name, float value) 
    {
        m_FloatUniformsMaps[name] = value;
    }

    void Material::SetUniform(const std::string& name, const glm::vec3& value) 
    {
        m_Vec3UniformsMaps[name] = value;
    }

    void Material::SetUniform(const std::string& name, const glm::vec4& value) 
    {
        m_Vec4UniformsMaps[name] = value;
    }

    void Material::SetTexture(const std::string& name, const std::shared_ptr<ITexture>& texture) 
    {
        m_TexturesMaps[name] = std::move(texture);
    }

    void Material::Bind() 
    {
        // [TODO]: We are going to get shader using it's name from assets manager (AssetsManager is not implemented yet!)
        // [TODO]: Assign the shader depending on Shading Method
        std::weak_ptr<IShader> shader; 

        if(!shader.expired())
        {
            auto materialShader = shader.lock();
            for (const auto& [name, val] : m_FloatUniformsMaps)
            materialShader->SetUniform(name, val);

            for (const auto& [name, val] : m_Vec3UniformsMaps)
                materialShader->SetUniform(name, val);

            for (const auto& [name, val] : m_Vec4UniformsMaps)
                materialShader->SetUniform(name, val);

            uint32_t slot = 0;
            for (const auto& [name, tex] : m_TexturesMaps) 
            {
                tex->Bind(slot);
                materialShader->SetUniform(name, slot);
                ++slot;
            }
        }
    }

    void Material::Unbind() 
    {
        for (const auto& [name, tex] : m_TexturesMaps) 
        {
            tex->Unbind();
        }
    }
}