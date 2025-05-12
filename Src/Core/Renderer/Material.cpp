#include "CorePCH.hpp"

namespace Motion::Core
{
    Material::Material(const std::shared_ptr<IShader>& shader): m_Shader(shader){}

    
    void Material::SetUniform(const std::string& name, float value) 
    {
        m_FloatUniforms[name] = value;
    }

    void Material::SetUniform(const std::string& name, const glm::vec3& value) 
    {
        m_Vec3Uniforms[name] = value;
    }

    void Material::SetTexture(const std::string& name, const std::shared_ptr<ITexture>& texture) 
    {
        m_Textures[name] = texture;
    }

    void Material::Bind() 
    {
        m_Shader->Bind();

        for (const auto& [name, val] : m_FloatUniforms)
            m_Shader->SetUniform(name, val);
        for (const auto& [name, val] : m_Vec3Uniforms)
            m_Shader->SetUniform(name, val);

        uint32_t slot = 0;
        for (const auto& [name, tex] : m_Textures) 
        {
            tex->Bind(slot);
            m_Shader->SetUniform(name, slot);
            ++slot;
        }
    }

    void Material::Unbind() 
    {
        for (const auto& [name, tex] : m_Textures) 
        {
            tex->Unbind();
        }
    }
}