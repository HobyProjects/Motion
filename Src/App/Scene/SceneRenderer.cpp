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

        for (const auto& entity : scene->m_Entities)
        {
            std::shared_ptr<Entity> current = entity;
            std::shared_ptr<Entity> next{nullptr};
            while(current)
            {
                if(current->Has<NodeComponent>())
                {
                    next = current->Get<NodeComponent>().EnTTNext;
                    if(current->Get<NodeComponent>().IsRoot)
                    {
                        current = next;
                        continue;
                    }
                }

                const bool active   = current->Get<TagComponent>().IsActive;
                const auto& mesh    = current->Get<MeshComponent>(); // I'm sure there's a better way to do this
                const auto& mat     = current->Get<MaterialComponent>();
                
                if (!active)
                {
                    if(next) current    = next;
                    else current        = nullptr;
                    continue;
                }

                const glm::mat4 meshTransform = current->Has<TransformComponent>() ? current->Get<TransformComponent>().GetTransform() : glm::mat4(1.0f);
                const auto& camera            = scene->GetCamera();
                const auto& env               = scene->GetEnvironment();

                RenderCommand cmd{};
                cmd.SortKey             = mesh.ID;
                cmd.MaterialPointer     = mat.MaterialPointer.get();
                cmd.MeshPointer         = mesh.MeshPointer.get();
                cmd.EnvPointer          = env.EnvironmentInstance.get();

                cmd.CameraData.CameraPosition   = camera.Camera.Position;
                cmd.CameraData.View             = camera.Camera.View;
                cmd.CameraData.Projection       = camera.Camera.Projection;

                cmd.ModelData.Model        = meshTransform;
                cmd.ModelData.Normal       = glm::transpose(glm::inverse(glm::mat3(meshTransform)));

                cmd.LightData.Color           = env.Sun.Color;
                cmd.LightData.Direction       = env.Sun.Direction;
                cmd.LightData.Intensity       = env.Sun.Intensity;

                Renderer::Submit(cmd);

                if(next) current    = next;
                else current        = nullptr;
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
