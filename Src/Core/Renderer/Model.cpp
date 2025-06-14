#include "CorePCH.hpp"

namespace Motion::Core
{
    void Model::Render(const glm::mat4& transform)
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

            drawCommand.shader = shader;
            drawCommand.mesh = subMesh->MeshPtr;
            drawCommand.material = material;
            drawCommand.renderPass = RenderPass::Opaque;
            drawCommand.modelMatrix = transform;

            Renderer::Submit(drawCommand);
        }
    }
}