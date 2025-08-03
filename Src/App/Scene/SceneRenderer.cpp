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
    }

    /**
     * @brief Flushes the accumulated draw commands by sorting and executing them.
     *
     * This method sorts the draw commands based on their sort key, material ID, and mesh ID,
     * then iterates through the sorted commands to render each mesh with its associated material.
     * It uses the AssetManager to retrieve the necessary shader, material, and mesh resources for rendering.
     */
    static void FlushQueue() noexcept
    {
        if (s_CommandQueue.empty())
            return;

        std::sort(s_CommandQueue.begin(), s_CommandQueue.end(), [](const SceneDrawCommand& a, const SceneDrawCommand& b) { return a < b; });

        AssetManager& assetManager = AssetManager::GetInstance();
        IShader* currentShader = assetManager.Get<IShader>("PBR").get();
        MaterialInstance* currentMaterial = nullptr;
        Mesh* currentMesh = nullptr;

        std::int32_t TextureBindingPoint = 0;
        const std::int32_t MAX_TEXTURE_SLOTS = Renderer::GetMaxTextureSlots();

        for (const auto& command : s_CommandQueue)
        {
            auto material = command.MaterialInstancePtr;
            auto mesh = command.MeshPtr;

            if (!currentShader || !material || !mesh) continue;

            std::int32_t requiredTextureSlots = material->GetTexturesCount() + 3; //< For Environment Textures
            if (TextureBindingPoint + requiredTextureSlots > MAX_TEXTURE_SLOTS)
            {
                if (currentShader) currentShader->Unbind();
                if (currentMaterial) currentMaterial->Unbind();
                if (currentMesh) currentMesh->Unbind();

                currentMaterial = nullptr;
                currentMesh = nullptr;
                TextureBindingPoint = 0; // Reset texture binding point
            }

            if (mesh != currentMesh)
            {
                if (currentMesh) currentMesh->Unbind();
                currentMesh = mesh;
                currentMesh->Bind();
            }

            if (currentShader)
            {
                currentShader->Bind();

                Renderer::BindTextureUnit(TextureBindingPoint, command.IrradianceTexture);
                currentShader->SetUniform(UniformCache::IrradianceTextures, TextureBindingPoint++);
                Renderer::BindTextureUnit(TextureBindingPoint, command.PrefilteredTexture);
                currentShader->SetUniform(UniformCache::PrefilteredTextures, TextureBindingPoint++);
                Renderer::BindTextureUnit(TextureBindingPoint, command.BRDFLUTTexture);
                currentShader->SetUniform(UniformCache::BRDFLUT, TextureBindingPoint++);

                currentShader->SetUniform(UniformCache::ViewMatrix, command.ViewMatrix);
                currentShader->SetUniform(UniformCache::ProjectionMatrix, command.ProjectionMatrix);
                currentShader->SetUniform(UniformCache::ModelMatrix, command.ModelMatrix);
                currentShader->SetUniform(UniformCache::NormalMatrix, command.NormalMatrix);

                currentShader->SetUniform(UniformCache::CameraPosition, command.CameraPosition);
                currentShader->SetUniform(UniformCache::LightPosition, command.LightPosition);
                currentShader->SetUniform(UniformCache::LightColor, command.LightColor);
                currentShader->SetUniform(UniformCache::LightIntensity, command.LightIntensity);
            }

            if (material != currentMaterial)
            {
                if (currentMaterial) currentMaterial->Unbind();
                currentMaterial = material;

                auto bindTexture =
                    [&](const std::string_view name, const MaterialInstance* material)
                    {
                        if (material->Texture.contains(name) && material->Texture.at(name) != nullptr)
                        {
                            material->Texture.at(name)->Bind(TextureBindingPoint);
                            currentShader->SetUniform(name, TextureBindingPoint++);
                        }
                        else
                        {
                            material->BaseMaterial->Texture.at(name)->Bind(TextureBindingPoint);
                            currentShader->SetUniform(name, TextureBindingPoint++);
                        }
                    };

                bindTexture(UniformCache::BaseColorTextures, currentMaterial);
                bindTexture(UniformCache::MetallicTextures, currentMaterial);
                bindTexture(UniformCache::RoughnessTextures, currentMaterial);
                bindTexture(UniformCache::AmbientOcclusionTextures, currentMaterial);
                bindTexture(UniformCache::NormalTextures, currentMaterial);

                currentMaterial->Bind();
            }

            currentMesh->Render();
            s_DrawCallsCount++;
        }

        if (currentMesh) currentMesh->Unbind();
        if (currentMaterial) currentMaterial->Unbind();
        if (currentShader) currentShader->Unbind();

        s_CommandQueue.clear();
        s_DrawCallsCount = 0;
    }


    /**
     * @brief Submits draw commands for all valid mesh entities in the given scene.
     *
     * Iterates through all entities in the provided scene, checking for the presence of a StaticMeshComponent
     * and a valid mesh. For each valid mesh segment, constructs a SceneDrawCommand with the appropriate
     * transformation and material information, and appends it to the renderer's draw command list.
     *
     * @param scene The scene containing entities to be rendered.
     * @param environment The environment settings to be used during rendering.
     *
     * @note If the scene or its main camera is null, the function logs an error and returns early.
     *       Entities without a StaticMeshComponent or with invalid mesh data are skipped with a warning.
     *       If an entity lacks a TransformComponent, an identity matrix is used as its transform.
     */
    void SceneRenderer::Submit(const std::shared_ptr<Scene>& scene, const std::shared_ptr<IEnvironment>& environment) noexcept
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
            if (!entity || !entity->HasComponent<StaticMeshComponent>())
            {
                MOTION_WARN("Entity is null or does not have a MeshComponent >> SKIPPING SUBMISSION");
                continue;
            }

            const auto& staticMeshComponent = entity->GetComponent<StaticMeshComponent>();
            if (!staticMeshComponent.Model || staticMeshComponent.Model->GetMeshesCount() <= 0)
            {
                MOTION_WARN("StaticMeshComponent has no mesh assigned. {} >> SKIPPING SUBMISSION", staticMeshComponent.Name);
                continue;
            }
            else
            {
                auto& model = staticMeshComponent.Model;
                for (auto it = model->begin(); it != model->end(); ++it)
                {
                    auto& meshSegment = *it;
                    command.SortKey = scene->GetSceneID();
                    command.MaterialInstancePtr = meshSegment.Materials.get();
                    command.MeshPtr = meshSegment.MeshSelf.get();

                    command.ModelMatrix = entity->HasComponent<TransformComponent>() ? entity->GetComponent<TransformComponent>().GetTransform() : glm::mat4(1.0f);
                    command.ViewMatrix = scene->m_SceneCamera->SceneViewCamera.View;
                    command.ProjectionMatrix = scene->m_SceneCamera->SceneViewCamera.Projection;
                    command.NormalMatrix = glm::mat3(glm::transpose(glm::inverse(glm::mat3(command.ModelMatrix))));

                    command.IrradianceTexture = environment->GetIrradianceTexture();
                    command.PrefilteredTexture = environment->GetPrefilteredTexture();
                    command.BRDFLUTTexture = environment->GetBRDFLUTTexture();

                    command.CameraPosition = scene->m_SceneCamera->SceneViewCamera.Position;
                    command.LightPosition = scene->m_Environment.DirectionalLight.Direction;
                    command.LightColor = scene->m_Environment.DirectionalLight.Color;
                    command.LightIntensity = scene->m_Environment.DirectionalLight.AmbientIntensity;

                    s_CommandQueue.push_back(command);
                }
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

        FlushQueue();
    }
}