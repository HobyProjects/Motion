#include "CorePCH.hpp"

#include "SceneRenderer.hpp"
#include "Scene.hpp"

namespace Motion::App
{
    /**
     * @brief Begins a new scene by clearing the draw commands and resetting the draw count.
     *
     * This method is called at the start of rendering a new scene to ensure that previous draw commands
     * do not interfere with the current rendering process. It prepares the renderer for a fresh set of draw commands.
     */
    void SceneRenderer::BeginScene() noexcept
    {
        m_DrawCommands.clear();
        m_DrawCount = 0;
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

        if (scene->m_SceneCamera != nullptr)
        {
            MOTION_CORE_ERROR("Scene camera is not set in the scene >> SKIPPING SUBMISSION");
            return;
        }


        SceneDrawCommand command{};

        for (const auto& entity : scene->m_Entities)
        {
            if (!entity || !entity->HasComponent<Motion::Core::MeshComponent>())
            {
                MOTION_WARN("Entity is null or does not have a MeshComponent >> SKIPPING SUBMISSION");
                continue;
            }

            const auto& meshComponent = entity->GetComponent<Motion::Core::MeshComponent>();
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

                if (entity->HasComponent<Motion::Core::TransformComponent>())
                {
                    const auto& transform = entity->GetComponent<Motion::Core::TransformComponent>();
                    command.TransformMatrix = transform.GetTransform();
                }
                else
                {
                    MOTION_WARN("Entity has no TransformComponent, using identity matrix for transform");
                    command.TransformMatrix = glm::mat4(1.0f);
                }

                command.ViewProjectionMatrix = scene->m_SceneCamera->GetCameraMatrix();
                command.ScenePtr = scene;

                m_DrawCommands.push_back(command);
            }
        }
    }

    /**
     * @brief Ends the current scene by flushing the draw commands to the renderer.
     *
     * This method is called at the end of rendering a scene to ensure that all accumulated draw commands
     * are processed and rendered. It invokes the Flush method to execute the draw calls.
     */
    void SceneRenderer::EndScene() noexcept
    {
        Flush();
    }

    /**
     * @brief Executes all queued draw commands, rendering the scene.
     *
     * This method sorts the draw commands by their sort key, retrieves the necessary
     * assets (materials, meshes, shaders) from the AssetManager, and performs the rendering
     * for each command. It selects the appropriate shader based on the material's shading method,
     * binds the shader and material, sets required uniforms, and issues the mesh render call.
     * If any required asset (material or shader) is missing, a warning is logged and the draw call is skipped.
     *
     * @note This method is noexcept and does not throw exceptions.
     */
    void SceneRenderer::Flush() noexcept
    {
        std::sort(m_DrawCommands.begin(), m_DrawCommands.end(), [](const SceneDrawCommand& a, const SceneDrawCommand& b) { return a < b; });

        Motion::Core::AssetManager& assetManager = Motion::Core::AssetManager::GetInstance();
        std::shared_ptr<Motion::Core::IShader> pbrShader = assetManager.Get<Motion::Core::IShader>("PBRShader");
        std::shared_ptr<Motion::Core::IShader> phongShader = assetManager.Get<Motion::Core::IShader>("PhongShader");
        std::shared_ptr<Motion::Core::IShader> unlitShader = assetManager.Get<Motion::Core::IShader>("UnlitShader");

        for (const auto& command : m_DrawCommands)
        {
            std::shared_ptr<Motion::Core::Material> material = assetManager.Get<Motion::Core::Material>(command.MaterialID);
            std::shared_ptr<Motion::Core::Mesh> mesh = assetManager.Get<Motion::Core::Mesh>(command.MeshID);
            std::shared_ptr<Motion::Core::IShader> currentShader{ nullptr };

            if (material)
            {
                Motion::Core::MaterialShadingMethod shadingMethod = material->GetShadingMethod();
                switch (shadingMethod)
                {
                case Motion::Core::MaterialShadingMethod::PBR:      currentShader = pbrShader; break;
                case Motion::Core::MaterialShadingMethod::Phong:    currentShader = phongShader; break;
                case Motion::Core::MaterialShadingMethod::Unlit:    currentShader = unlitShader; break;
                case Motion::Core::MaterialShadingMethod::Auto:     currentShader = unlitShader; break;
                }

                if (currentShader)
                {
                    currentShader->Bind();

                    if (shadingMethod & Motion::Core::MaterialShadingMethod::PBR || shadingMethod & Motion::Core::MaterialShadingMethod::Phong)
                    {
                        currentShader->SetUniform(Motion::Core::UniformCache::GlobalAttri_ViewProjMatrix, command.ViewProjectionMatrix);
                        currentShader->SetUniform(Motion::Core::UniformCache::GlobalAttri_ModelMatrix, command.TransformMatrix);

                        currentShader->SetUniform(Motion::Core::UniformCache::LightAttri_Color, command.ScenePtr->m_Environment.DirectionalLight.Color);
                        currentShader->SetUniform(Motion::Core::UniformCache::LightAttri_Position, command.ScenePtr->m_Environment.DirectionalLight.Direction);
                        currentShader->SetUniform(Motion::Core::UniformCache::LightAttri_Intensity, command.ScenePtr->m_Environment.DirectionalLight.AmbientIntensity);
                    }

                    material->Bind(currentShader);

                    mesh->Render();

                    material->Unbind();
                    currentShader->Unbind();
                    m_DrawCount++;
                }
                else
                {
                    MOTION_WARN("Cannot run the draw call with ID {}. Because Shader is expired", command.SortKey);
                    continue;
                }
            }
            else
            {
                MOTION_WARN("Cannot run the draw call with ID {}. Because Material is expired", command.SortKey);
                continue;
            }
        }
    }
}