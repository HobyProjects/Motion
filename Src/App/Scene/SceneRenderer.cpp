#include "CorePCH.hpp"
#include "SceneRenderer.hpp"
#include "Scene.hpp"

namespace Motion
{
    static std::vector<SceneDrawCommand> s_CommandQueue{};

    void SceneRenderer::BeginScene() noexcept
    {
        s_CommandQueue.clear();
    }

    namespace TEX_SLOTS {
        static constexpr std::int32_t Irradiance = 0;
        static constexpr std::int32_t Prefilter = 1;
        static constexpr std::int32_t BRDFLUT = 2;

        static constexpr std::int32_t BaseColor = 3;
        static constexpr std::int32_t Metallic = 4;
        static constexpr std::int32_t Roughness = 5;
        static constexpr std::int32_t AO = 6;
        static constexpr std::int32_t Normal = 7;
        static constexpr std::int32_t Displace = 8;
    }

    static inline void BindIBL(IShader* shader, IEnvironment* env)
    {
        env->BindIrradianceTexture(TEX_SLOTS::Irradiance);
        shader->SetUniform(UniformCache::IrradianceTextures, TEX_SLOTS::Irradiance);

        env->BindPrefilteredTexture(TEX_SLOTS::Prefilter);
        shader->SetUniform(UniformCache::PrefilteredTextures, TEX_SLOTS::Prefilter);

        env->BindBRDFLUTTexture(TEX_SLOTS::BRDFLUT);
        shader->SetUniform(UniformCache::BRDFLUT, TEX_SLOTS::BRDFLUT);
    }

    static void FlushQueue() noexcept
    {
        if (s_CommandQueue.empty()) return;

        std::sort(s_CommandQueue.begin(), s_CommandQueue.end(),
            [](const SceneDrawCommand& a, const SceneDrawCommand& b) { return a < b; });

        AssetManager& assetManager = AssetManager::GetInstance();
        IShader* const PBR_SHADER = assetManager.Get<IShader>("PBR").get();

        IShader* currentShader = nullptr;
        PhysicalBasedMaterialInstance* currentMaterial = nullptr;
        Mesh* currentMesh = nullptr;
        IEnvironment* currentEnv = nullptr;

        UUID currentViewKey{ 0 };
        bool haveView = false;

        auto bindPBRTexture = [&](const std::string_view& uniformName, std::int32_t slot, TextureType type)
            {
                if (currentMaterial->Texture[type])
                {
                    currentMaterial->Texture[type]->Bind(slot);
                }
                else if (currentMaterial->BaseMaterial->Texture[type])
                {
                    currentMaterial->BaseMaterial->Texture[type]->Bind(slot);
                }
                else
                {
                    // Optionally: bind a 1x1 dummy texture to keep happy samplers.
                }
                currentShader->SetUniform(uniformName, slot);
            };

        for (const auto& cmd : s_CommandQueue)
        {
            IShader* const nextShader = PBR_SHADER;
            Mesh* const nextMesh = cmd.MeshPtr;
            auto* const nextMat = cmd.PBR_MatPtr;
            auto* const nextEnv = cmd.EnvironmentPtr;

            if (!nextShader || !nextMesh || !nextMat || !nextEnv)
                continue;

            if (currentShader != nextShader)
            {
                if (currentShader) currentShader->Unbind();
                currentShader = nextShader;
                currentShader->Bind();
            }

            const bool envChanged = (currentEnv != nextEnv);
            const bool viewChanged = (!haveView) || (currentViewKey != cmd.SortKey);

            if (envChanged)
            {
                currentEnv = nextEnv;
                BindIBL(currentShader, currentEnv);
            }

            if (viewChanged)
            {
                haveView = true;
                currentViewKey = cmd.SortKey;

                currentShader->SetUniform(UniformCache::ViewMatrix, cmd.ViewMatrix);
                currentShader->SetUniform(UniformCache::ProjectionMatrix, cmd.ProjectionMatrix);
                currentShader->SetUniform(UniformCache::CameraPosition, cmd.CameraPosition);

                for (std::int32_t i = 0; i < DirectionalLight::LIGHT_COUNT; ++i)
                {
                    currentShader->SetUniform(std::format("u_LightPosition[{}]", i), cmd.LightPosition[i]);
                    currentShader->SetUniform(std::format("u_LightColor[{}]", i), cmd.LightColor[i]);
                    currentShader->SetUniform(std::format("u_LightIntensity[{}]", i), cmd.LightIntensity[i]);
                }
            }

            if (currentMesh != nextMesh)
            {
                if (currentMesh) currentMesh->Unbind();
                currentMesh = nextMesh;
                currentMesh->Bind();
            }

            const bool materialChanged = (currentMaterial != nextMat);
            if (materialChanged)
            {
                currentMaterial = nextMat;
                currentMaterial->UploadAttributes();

                bindPBRTexture(UniformCache::PBR_BaseColorTextures, TEX_SLOTS::BaseColor, TextureType::BaseColorTexture);
                bindPBRTexture(UniformCache::PBR_MetallicTextures, TEX_SLOTS::Metallic, TextureType::MetallicTexture);
                bindPBRTexture(UniformCache::PBR_RoughnessTextures, TEX_SLOTS::Roughness, TextureType::RoughnessTexture);
                bindPBRTexture(UniformCache::PBR_AmbientOcclusionTextures, TEX_SLOTS::AO, TextureType::AmbientOcclusionTexture);
                bindPBRTexture(UniformCache::PBR_NormalTextures, TEX_SLOTS::Normal, TextureType::NormalTexture);
                bindPBRTexture(UniformCache::PBR_DisplacementTextures, TEX_SLOTS::Displace, TextureType::DisplacementTexture);
            }

            currentShader->SetUniform(UniformCache::ModelMatrix, cmd.ModelMatrix);
            currentShader->SetUniform(UniformCache::NormalMatrix, cmd.NormalMatrix);

            currentMesh->Render();
        }

        if (currentMesh)   currentMesh->Unbind();
        if (currentShader) currentShader->Unbind();

        s_CommandQueue.clear();
    }


    void SceneRenderer::Submit(Scene* scene, IEnvironment* environment) noexcept
    {
        if (!scene)
        {
            MOTION_CORE_ERROR("Scene is null >> SKIPPING SUBMISSION");
            return;
        }

        SceneDrawCommand cmd{};

        for (const auto& entity : scene->m_Entities)
        {
            if (!entity || !entity->HasComponent<StaticMeshComponent>())
                continue;

            const auto& sm = entity->GetComponent<StaticMeshComponent>();
            if (!sm.Model || sm.Model->GetMeshesCount() <= 0)
                continue;

            auto& model = sm.Model;
            for (auto& mesh : *model)
            {
                cmd.SortKey = scene->GetID();
                cmd.PBR_MatPtr = mesh->PhysicalBasedMaterials.get();
                cmd.MeshPtr = mesh.get();
                cmd.EnvironmentPtr = environment;
                cmd.ShadingMethod = model->ModelShadingMethod;

                cmd.ModelMatrix = entity->HasComponent<TransformComponent>()
                    ? entity->GetComponent<TransformComponent>().GetTransform()
                    : glm::mat4(1.0f);

                auto& camera = scene->GetCamera();
                auto& env = scene->GetEnvironment();

                cmd.ViewMatrix = camera.Camera.View;
                cmd.ProjectionMatrix = camera.Camera.Projection;
                cmd.NormalMatrix = glm::mat3(glm::transpose(glm::inverse(glm::mat3(cmd.ModelMatrix))));
                cmd.CameraPosition = camera.Camera.Position;
                cmd.LightPosition = env.DirectionalLight.LightPosition;
                cmd.LightColor = env.DirectionalLight.LightColor;
                cmd.LightIntensity = env.DirectionalLight.LightIntensity;

                s_CommandQueue.push_back(cmd);
            }
        }
    }

    void SceneRenderer::EndScene() noexcept
    {
        if (s_CommandQueue.empty()) return;
        FlushQueue();
    }
}