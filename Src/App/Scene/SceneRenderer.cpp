#include "CorePCH.hpp"
#include "SceneRenderer.hpp"
#include "Scene.hpp"

namespace Motion
{
    static Scene* s_CurrentScene = nullptr;

    void SceneRenderer::BeginScene() noexcept
    {
        Renderer::Begin();
    }

    void SceneRenderer::Submit(Scene* scene) noexcept
    {
        if (!scene) { MOTION_CORE_ERROR("Scene is null >> SKIPPING SUBMISSION"); return; }
        s_CurrentScene = scene;
        AssetManager& assets = AssetManager::GetInstance();

        for (const auto& entity : scene->m_Entities)
        {
            if (!entity || !entity->HasComponent<StaticMeshComponent>()) continue;

            const auto& sm = entity->GetComponent<StaticMeshComponent>();
            if (!sm.Model || sm.Model->GetMeshesCount() <= 0) continue;

            auto    modelMatrix     = entity->HasComponent<TransformComponent>() ? entity->GetComponent<TransformComponent>().GetTransform() : glm::mat4(1.0f);
            auto&   camera          = scene->GetCamera();
            auto&   env             = scene->GetEnvironment();

            for (auto& mesh : *sm.Model)
            {
                RenderCommand cmd{};
                cmd.SortKey             = sm.ID;
                cmd.MaterialPointer     = mesh->Materials.get();
                cmd.MeshPointer         = mesh.get();
                cmd.EnvPointer          = env.EnvironmentInstance.get();

                cmd.CameraData.CameraPosition   = camera.Camera.Position;
                cmd.CameraData.View             = camera.Camera.View;
                cmd.CameraData.Projection       = camera.Camera.Projection;

                cmd.ModelData.Model        = modelMatrix;
                cmd.ModelData.Normal       = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));

                cmd.LightData.Color           = env.Sun.Color;
                cmd.LightData.Direction       = env.Sun.Direction;
                cmd.LightData.Intensity       = env.Sun.Intensity;

                Renderer::Submit(cmd);
            }
        }
    }

    void SceneRenderer::EndScene() noexcept
    {
        RenderSkyboxPass(s_CurrentScene);
        Renderer::End();
    }

    void SceneRenderer::RenderSkyboxPass(Scene* scene) noexcept
    {
        if (!scene) return;

        auto&           env = scene->GetEnvironment();
        IEnvironment*   ibl = env.EnvironmentInstance ? env.EnvironmentInstance.get() : nullptr;
        if (!ibl)       return;

        const auto& cam = scene->GetCamera().Camera;
        ibl->RenderSkyBox(cam.Projection, cam.View);
    }
}
