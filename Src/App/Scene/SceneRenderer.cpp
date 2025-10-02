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
            while(current)
            {
                std::shared_ptr<Entity> next{ nullptr };
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

                cmd.CameraData.View             = camera.View;
                cmd.CameraData.CameraPosition   = camera.Position;
                cmd.CameraData.Projection       = camera.Projection;

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
        Renderer::End();
    }
}
