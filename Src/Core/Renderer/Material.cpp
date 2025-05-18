#include "CorePCH.hpp"

namespace Motion::Core
{
    Material::Material(const std::string& shaderName)
    {
        m_Shader = ShaderBuilder::GetShader(shaderName);
        MOTION_ASSERT(m_Shader.expired(), "{0} is not avaliable", shaderName);
    }
    
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
        if(!m_Shader.expired())
        {
            auto materialShader = m_Shader.lock();
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