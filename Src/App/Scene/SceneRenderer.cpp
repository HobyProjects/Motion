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
        IShader* PBR_SHADER = assetManager.Get<IShader>("PBR").get();
        IShader* STD_SHADER = assetManager.Get<IShader>("PHONG").get();

        IShader* currentShader = nullptr;
        PhysicalBasedMaterialInstance* currentPBRMaterial = nullptr;
        StandardMaterialInstance* currentSTDMaterial = nullptr;
        Mesh* currentMesh = nullptr;


        std::int32_t TextureBindingPoint = 0;
        const std::int32_t MAX_TEXTURE_SLOTS = Renderer::GetMaxTextureSlots();

        for (const auto& command : s_CommandQueue)
        {
            IShader* SHADER_PTR = (command.ShadingMethod == ShadingMethod::PhysicalBased) ? PBR_SHADER : STD_SHADER;
            PhysicalBasedMaterialInstance* PBR_MATERIAL = command.PBR_MatPtr;
            StandardMaterialInstance* STD_MATERIAL = command.STD_MatPtr;
            Mesh* MESH_PTR = command.MeshPtr;

            if (!SHADER_PTR || !PBR_MATERIAL || !STD_MATERIAL || !MESH_PTR) continue;

            std::int32_t requiredTextureSlots;
            if (command.ShadingMethod == ShadingMethod::PhysicalBased)
            {
                currentPBRMaterial = PBR_MATERIAL;
                requiredTextureSlots = PBR_MATERIAL->GetTexturesCount() + 3; // +3 for Irradiance, Prefiltered, and BRDF LUT textures
            }
            else
            {
                currentSTDMaterial = STD_MATERIAL;
                requiredTextureSlots = STD_MATERIAL->GetTexturesCount() + 3; // +3 for Irradiance, Prefiltered, and BRDF LUT textures
            }


            if (TextureBindingPoint + requiredTextureSlots > MAX_TEXTURE_SLOTS)
            {
                if (currentShader) currentShader->Unbind();
                if (currentMesh) currentMesh->Unbind();

                currentPBRMaterial = nullptr;
                currentSTDMaterial = nullptr;
                currentMesh = nullptr;
                currentShader = nullptr;
                TextureBindingPoint = 0;
            }

            if (currentMesh != MESH_PTR)
            {
                if (currentMesh) currentMesh->Unbind();
                currentMesh = MESH_PTR;
                currentMesh->Bind();
            }

            if (currentShader != SHADER_PTR)
            {
                if (currentShader) currentShader->Unbind();
                currentShader = SHADER_PTR;
                currentShader->Bind();

                if (command.ShadingMethod == ShadingMethod::PhysicalBased)
                {
                    Renderer::BindTextureUnit(TextureBindingPoint, command.IrradianceTexture);
                    currentShader->SetUniform(UniformCache::IrradianceTextures, TextureBindingPoint++);
                    Renderer::BindTextureUnit(TextureBindingPoint, command.PrefilteredTexture);
                    currentShader->SetUniform(UniformCache::PrefilteredTextures, TextureBindingPoint++);
                    Renderer::BindTextureUnit(TextureBindingPoint, command.BRDFLUTTexture);
                    currentShader->SetUniform(UniformCache::BRDFLUT, TextureBindingPoint++);
                }
                else
                {
                    Renderer::BindTextureUnit(TextureBindingPoint, command.IrradianceTexture);
                    currentShader->SetUniform(UniformCache::IrradianceTextures, TextureBindingPoint++);
                    Renderer::BindTextureUnit(TextureBindingPoint, command.PrefilteredTexture);
                    currentShader->SetUniform(UniformCache::PrefilteredTextures, TextureBindingPoint++);
                }

                currentShader->SetUniform(UniformCache::ViewMatrix, command.ViewMatrix);
                currentShader->SetUniform(UniformCache::ProjectionMatrix, command.ProjectionMatrix);
                currentShader->SetUniform(UniformCache::ModelMatrix, command.ModelMatrix);
                currentShader->SetUniform(UniformCache::NormalMatrix, command.NormalMatrix);

                currentShader->SetUniform(UniformCache::CameraPosition, command.CameraPosition);
                currentShader->SetUniform(UniformCache::LightPosition, command.LightPosition);
                currentShader->SetUniform(UniformCache::LightColor, command.LightColor);
                currentShader->SetUniform(UniformCache::LightIntensity, command.LightIntensity);

            }

            if (currentPBRMaterial != PBR_MATERIAL || currentSTDMaterial != STD_MATERIAL)
            {
                if (currentPBRMaterial) currentPBRMaterial = nullptr;
                if (currentSTDMaterial) currentSTDMaterial = nullptr;

                if (command.ShadingMethod == ShadingMethod::PhysicalBased)
                    currentPBRMaterial = PBR_MATERIAL;
                else
                    currentSTDMaterial = STD_MATERIAL;

                auto bindTexture =
                    [&](const std::string_view& uniformName, TextureType textureType)
                    {
                        if (command.ShadingMethod == ShadingMethod::PhysicalBased)
                        {
                            if (currentPBRMaterial->Texture[textureType])
                            {
                                currentPBRMaterial->Texture[textureType]->Bind(TextureBindingPoint);
                                currentShader->SetUniform(uniformName, TextureBindingPoint++);
                            }
                            else
                            {
                                currentPBRMaterial->BaseMaterial->Texture[textureType]->Bind(TextureBindingPoint);
                                currentShader->SetUniform(uniformName, TextureBindingPoint++);
                            }
                        }
                        else
                        {
                            if (currentSTDMaterial->Texture[textureType])
                            {
                                currentSTDMaterial->Texture[textureType]->Bind(TextureBindingPoint);
                                currentShader->SetUniform(uniformName, TextureBindingPoint++);
                            }
                            else
                            {
                                currentSTDMaterial->BaseMaterial->Texture[textureType]->Bind(TextureBindingPoint);
                                currentShader->SetUniform(uniformName, TextureBindingPoint++);
                            }
                        }
                    };



                if (command.ShadingMethod == ShadingMethod::PhysicalBased)
                {
                    if (currentPBRMaterial)
                    {
                        currentPBRMaterial->UploadAttributes();
                        bindTexture(UniformCache::PBR_BaseColorTextures, TextureType::BaseColorTexture);
                        bindTexture(UniformCache::PBR_MetallicTextures, TextureType::MetallicTexture);
                        bindTexture(UniformCache::PBR_RoughnessTextures, TextureType::RoughnessTexture);
                        bindTexture(UniformCache::PBR_AmbientOcclusionTextures, TextureType::AmbientOcclusionTexture);
                        bindTexture(UniformCache::PBR_NormalTextures, TextureType::NormalTexture);
                        bindTexture(UniformCache::PBR_DisplacementTextures, TextureType::DisplacementTexture);
                    }
                }
                else
                {
                    if (currentSTDMaterial)
                    {
                        currentSTDMaterial->UploadAttributes();
                        bindTexture(UniformCache::STD_DiffuseTexture, TextureType::DiffuseTexture);
                        bindTexture(UniformCache::STD_SpecularTexture, TextureType::SpecularTexture);
                        bindTexture(UniformCache::STD_EmissiveTexture, TextureType::EmissiveTexture);
                        bindTexture(UniformCache::STD_OpacityTexture, TextureType::OpacityTexture);
                    }
                }
            }

            currentMesh->Render();
            s_DrawCallsCount++;
        }

        if (currentMesh) currentMesh->Unbind();
        if (currentShader) currentShader->Unbind();
        if (currentPBRMaterial) currentPBRMaterial = nullptr;
        if (currentSTDMaterial) currentSTDMaterial = nullptr;

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
                for (auto& mesh : *model)
                {
                    command.SortKey = scene->GetSceneID();
                    command.PBR_MatPtr = mesh->PhysicalBasedMaterials.get();
                    command.STD_MatPtr = mesh->StandardMaterials.get();
                    command.MeshPtr = mesh.get();
                    command.ShadingMethod = model->ModelShadingMethod;
                    command.ModelMatrix = entity->HasComponent<TransformComponent>() ? entity->GetComponent<TransformComponent>().GetTransform() : glm::mat4(1.0f);

                    auto& camera = scene->GetSceneCamera();
                    auto& env = scene->GetEnvironment();

                    command.ViewMatrix = camera.View;
                    command.ProjectionMatrix = camera.Projection;
                    command.NormalMatrix = glm::mat3(glm::transpose(glm::inverse(glm::mat3(command.ModelMatrix))));
                    command.CameraPosition = camera.Position;
                    command.LightPosition = env.DirectionalLight.Direction;
                    command.LightColor = env.DirectionalLight.Color;
                    command.LightIntensity = env.DirectionalLight.AmbientIntensity;

                    command.IrradianceTexture = environment->GetIrradianceTexture();
                    command.PrefilteredTexture = environment->GetPrefilteredTexture();
                    command.BRDFLUTTexture = environment->GetBRDFLUTTexture();

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