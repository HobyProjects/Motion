#include "CorePCH.hpp"

#include "SceneRenderer.hpp"
#include "Scene.hpp"

namespace Motion
{
    static std::vector<SceneDrawCommand> s_CommandQueue{};
    static std::uint32_t s_DrawCallsCount{ 0 };

    /**
     * @brief Begins a new scene by clearing the draw commands and resetting the draw count.
     *
     * This method is called at the start of rendering a new scene to ensure that previous draw commands
     * do not interfere with the current rendering process. It prepares the renderer for a fresh set of draw commands.
     */
    void SceneRenderer::BeginScene() noexcept
    {
        s_CommandQueue.clear();
        s_DrawCallsCount = 0;
        TextureBinding::Reset();
    }

    /**
     * @brief Submits draw commands for all valid mesh entities in the given scene.
     *
     * Iterates through all entities in the provided scene, checking for the presence of a MeshComponent
     * and a valid mesh. For each valid mesh segment, constructs a SceneDrawCommand with the appropriate
     * transformation and material information, and appends it to the renderer's draw command list.
     *
     * @param scene Pointer to the Scene object containing entities to be rendered.
     * @param viewProjectionMatrix The combined view and projection matrix to be used for rendering.
     *
     * @note If the scene or its main camera is null, the function logs an error and returns early.
     *       Entities without a MeshComponent or with invalid mesh data are skipped with a warning.
     *       If an entity lacks a TransformComponent, an identity matrix is used as its transform.
     */
    void SceneRenderer::Submit(Scene* scene) noexcept
    {
        if (!scene)
        {
            MOTION_CORE_ERROR("Scene is null >> SKIPPING SUBMISSION");
            return;
        }

        if (!scene->m_SceneCamera)
        {
            MOTION_CORE_ERROR("Scene camera is not set in the scene >> SKIPPING SUBMISSION");
            return;
        }


        SceneDrawCommand command{};

        for (const auto& entity : scene->m_Entities)
        {
            if (!entity || !entity->HasComponent<MeshComponent>())
            {
                MOTION_WARN("Entity is null or does not have a MeshComponent >> SKIPPING SUBMISSION");
                continue;
            }

            const auto& meshComponent = entity->GetComponent<MeshComponent>();
            if (!meshComponent.Mesh)
            {
                MOTION_WARN("MeshComponent has no mesh assigned. {} >> SKIPPING SUBMISSION", meshComponent.Name);
                continue;
            }

            for (auto it = meshComponent.Mesh->begin(); it != meshComponent.Mesh->end(); ++it)
            {
                const auto& meshSegment = *it;
                if (!meshSegment || !meshSegment->MeshSelf)
                {
                    MOTION_WARN("MeshSegment is null or has no Mesh assigned >> SKIPPING SUBMISSION");
                    continue;
                }

                command.SortKey = scene->GetSceneID();
                command.MaterialID = meshSegment->Materials->GetUUID();
                command.MeshID = meshSegment->MeshSelf->GetUUID();

                if (entity->HasComponent<TransformComponent>())
                {
                    const auto& transform = entity->GetComponent<TransformComponent>();
                    command.ModelMatrix = transform.GetTransform();
                }
                else
                {
                    MOTION_WARN("Entity has no TransformComponent, using identity matrix for transform");
                    command.ModelMatrix = glm::mat4(1.0f);
                }

                command.ViewProjectionMatrix = scene->m_SceneCamera->SceneViewCamera.MVP;
                command.CameraPosition = scene->m_SceneCamera->SceneViewCamera.Position;
                command.LightPosition = scene->m_Environment.DirectionalLight.Direction;
                command.LightColor = scene->m_Environment.DirectionalLight.Color;
                command.LightIntensity = scene->m_Environment.DirectionalLight.AmbientIntensity;

                s_CommandQueue.push_back(command);
            }
        }
    }


    /**
     * @brief Ends the current scene rendering by sorting and executing the draw commands.
     *
     * This method sorts the accumulated draw commands based on their sort key, material ID, and mesh ID,
     * then iterates through the sorted commands to render each mesh with its associated material.
     * It uses the AssetManager to retrieve the necessary shader, material, and mesh resources for rendering.
     */
    void SceneRenderer::EndScene() noexcept
    {
        if (s_CommandQueue.empty())
            return;

        std::sort(s_CommandQueue.begin(), s_CommandQueue.end(), [](const SceneDrawCommand& a, const SceneDrawCommand& b) { return a < b; });

        AssetManager& assetManager = AssetManager::GetInstance();
        std::shared_ptr<IShader> shader = assetManager.Get<IShader>("PBR");

        for (const auto& command : s_CommandQueue)
        {
            std::shared_ptr<MaterialInstance> material = assetManager.Get<MaterialInstance>(command.MaterialID);
            std::shared_ptr<Mesh> mesh = assetManager.Get<Mesh>(command.MeshID);

            shader->Bind();
            shader->SetUniform(UniformCache::ModelMatrix, command.ModelMatrix);
            shader->SetUniform(UniformCache::ViewProjMatrix, command.ViewProjectionMatrix);
            shader->SetUniform(UniformCache::CameraPosition, command.CameraPosition);
            shader->SetUniform(UniformCache::LightPosition, command.LightPosition);
            shader->SetUniform(UniformCache::LightColor, command.LightColor);
            shader->SetUniform(UniformCache::LightIntensity, command.LightIntensity);

            material->Bind();

            std::shared_ptr<ICubeTexture> environmentTexture = SkyBox::GetTexture();
            if (environmentTexture)
            {
                std::int32_t bindingPoint = TextureBinding::Point();
                environmentTexture->Bind(bindingPoint);
                shader->SetUniform(UniformCache::EnvironmentTexture, bindingPoint);
            }

            mesh->Render();

            material->Unbind();
            shader->Unbind();
            s_DrawCallsCount++;
        }
    }
}