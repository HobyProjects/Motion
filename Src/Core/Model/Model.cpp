#include "CorePCH.hpp"
#include "Model.hpp"

namespace Motion::Core
{
    void Model::Render(const glm::mat4& transform, const glm::mat4& cameraMatrix)
    {
        for(const auto& subMesh : m_SubMeshes)
        {
            DrawCommand drawCommand{};

            std::shared_ptr<SubMeshMaterial> subMeshMaterial = m_SubMeshMaterialMapping[subMesh->MaterialIndex];
            std::shared_ptr<Material> material = subMeshMaterial->Materials;
            Material::ShadingMethod shadingMethod = material->GetShadingMethod();
            std::shared_ptr<IShader> shader{nullptr};
            
            switch(shadingMethod)
            {
                case Material::ShadingMethod::PBR: shader = AssetManager::GetShader("PBRShader"); break;
                case Material::ShadingMethod::Phong: shader = AssetManager::GetShader("PhongShader"); break;
                case Material::ShadingMethod::Unlit: shader = AssetManager::GetShader("UnlitShader"); break;
            };

            drawCommand.Shader = shader;
            drawCommand.SubMesh = subMesh->MeshPtr;
            drawCommand.MeshMaterial = material;
            drawCommand.RendererPasses = RenderPass::Opaque;
            drawCommand.ModelTransform = transform;
            drawCommand.CameraMatrix = cameraMatrix;

            Renderer::Submit(drawCommand);
        }
    }

    void Model::InsertMaterial(const std::shared_ptr<Material>& material)
    {
        if(!m_SubMeshMaterialMapping.empty())
        {
            std::weak_ptr<SubMeshMaterial> subMeshMaterial = m_SubMeshMaterialMapping.end()->second;
            if(!subMeshMaterial.expired())
            {
                auto subMeshMat = subMeshMaterial.lock();
                std::shared_ptr<SubMeshMaterial> newSubMeshMaterial = std::make_shared<SubMeshMaterial>();
                newSubMeshMaterial->MaterialIndex = subMeshMat->MaterialIndex + 1;
                newSubMeshMaterial->Materials = material;
                newSubMeshMaterial->ParentModel = this;
                m_SubMeshMaterialMapping[newSubMeshMaterial->MaterialIndex + 1] = newSubMeshMaterial;
            }        
        }
        else
        {
            std::shared_ptr<SubMeshMaterial> newSubMeshMaterial = std::make_shared<SubMeshMaterial>();
            newSubMeshMaterial->MaterialIndex = 0;
            newSubMeshMaterial->Materials = material;
            newSubMeshMaterial->ParentModel = this;
            m_SubMeshMaterialMapping[0] = newSubMeshMaterial;
        }
    }

    std::shared_ptr<Model::SubMeshMaterial> Motion::Core::Model::GetSubMeshMaterial(uint32_t subMeshIndex) const
    {
        auto it = m_SubMeshMaterialMapping.find(subMeshIndex);
        if(it != m_SubMeshMaterialMapping.end())
        {
            return it->second;
        }
        
        return nullptr;
    }
}


