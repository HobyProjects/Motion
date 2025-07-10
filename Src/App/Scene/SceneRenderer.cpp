#include "CorePCH.hpp"

#include "SceneRenderer.hpp"
#include "Scene.hpp"


namespace Motion::App
{
    struct DrawCalls
    {
        std::shared_ptr<Motion::Core::Model> Model{nullptr};
        glm::mat4 Transform;

        bool operator<(const DrawCalls& other) const
        {
            if (Model.get() != other.Model.get())
                return Model.get() < other.Model.get();

            return std::memcmp(&Transform, &other.Transform, sizeof(glm::mat4)) < 0;
        }
    };

    static Scene* s_CurrentScene{nullptr};
    static glm::mat4 s_CurrentCameraMatrix{1.0f};
    static std::vector<DrawCalls> s_DrawCalls;


    void SceneRenderer::BeginScene(Scene* currentScene, const glm::mat4& cameraMatrix)
    {
        s_CurrentScene = currentScene;
        s_CurrentCameraMatrix = cameraMatrix;
        s_DrawCalls.clear();
    }

    void SceneRenderer::SubmitModel(const std::shared_ptr<Motion::Core::Model>& model, const glm::mat4& transform)
    {
        s_DrawCalls.push_back({ model, transform });
    }

    void SceneRenderer::EndScene()
    {
        Flush();
    }

    void SceneRenderer::Flush()
    {
        std::sort(s_DrawCalls.begin(), s_DrawCalls.end(), [](const DrawCalls& a, const DrawCalls& b) { return a < b; });

        for(const auto& drawCall : s_DrawCalls)
        {
            for(std::vector<std::shared_ptr<Motion::Core::Model::SubMesh>>::iterator it = drawCall.Model->begin(); it != drawCall.Model->end(); ++it)
            {
                std::shared_ptr<Motion::Core::Model::SubMesh> subMesh = *it;
                std::shared_ptr<Motion::Core::Model::SubMeshMaterial> subMeshMaterial = drawCall.Model->GetSubMeshMaterial(subMesh->MaterialIndex);
                std::shared_ptr<Motion::Core::Material> material = subMeshMaterial->Materials;
                Motion::Core::Material::ShadingMethod shadingMethod = material->GetShadingMethod();
                std::shared_ptr<Motion::Core::IShader> shader{ nullptr };

                switch (shadingMethod)
                {
                    case Motion::Core::Material::ShadingMethod::Phong:
                        shader = Motion::Core::AssetManager::GetShader("phong");
                        break;
                    case Motion::Core::Material::ShadingMethod::PBR:
                        shader = Motion::Core::AssetManager::GetShader("pbr");
                        break;
                    case Motion::Core::Material::ShadingMethod::Unlit:
                        shader = Motion::Core::AssetManager::GetShader("unlit");
                        break;
                    default:
                        MOTION_WARN("Unknown shading method");
                        break;
                };

                if(shader != nullptr)
                {
                    Motion::Core::DrawCommand drawCommand{};
                    drawCommand.ShaderRef = shader;
                    drawCommand.MeshRef = subMesh->MeshPtr;
                    drawCommand.MaterialRef = material;
                    drawCommand.RenderPassMask = Motion::Core::RenderPass::Opaque;
                    drawCommand.ModelMatrix = drawCall.Transform;
                    drawCommand.ViewProjMatrix = s_CurrentCameraMatrix;

                    if(s_CurrentScene->m_Enviroment.Physics.GetSettings().IsEnabled)
                    {
                        drawCommand.SetUniform(Motion::Core::UniformCache::LightUniforms::LightPosition, s_CurrentScene->m_Enviroment.DirectionalLight.Direction);
                        drawCommand.SetUniform(Motion::Core::UniformCache::LightUniforms::LightColor, s_CurrentScene->m_Enviroment.DirectionalLight.Color);
                        drawCommand.SetUniform(Motion::Core::UniformCache::LightUniforms::LightIntensity, s_CurrentScene->m_Enviroment.DirectionalLight.AmbientIntensity);
                    }

                    Motion::Core::Renderer::Submit(drawCommand);
                }

                MOTION_WARN("Shader not found; for model: {0}", drawCall.Model->GetMetaData().AssetName);
                continue;
            }
        }

        s_DrawCalls.clear();
    }

}