#include "CorePCH.hpp"
#include "SceneRenderer.hpp"
#include "Scene.hpp"

namespace Motion
{
    Scene* SceneRenderer::s_CurrentScene = nullptr;
    static std::vector<SceneDrawCommand> s_CommandQueue{};

    void SceneRenderer::BeginScene() noexcept
    {
        s_CommandQueue.clear();
        s_CurrentScene = nullptr;
    }

    namespace TEX_SLOTS
    {
        // IBL (keep contiguous & consistent with shaders)
        static constexpr std::int32_t Irradiance = 1;
        static constexpr std::int32_t Prefilter = 2;
        static constexpr std::int32_t BRDFLUT = 3;

        // Leave 3..7 free for skybox/utility if needed

        // Material samplers start here
        static constexpr std::int32_t Base = 8;
        static constexpr std::int32_t BaseColor = Base + 0;
        static constexpr std::int32_t Metallic = Base + 1;
        static constexpr std::int32_t Roughness = Base + 2;
        static constexpr std::int32_t Normal = Base + 3;
        static constexpr std::int32_t AO = Base + 4;
        static constexpr std::int32_t Emissive = Base + 5;
        static constexpr std::int32_t Opacity = Base + 6;
        static constexpr std::int32_t ORM = Base + 7;
        static constexpr std::int32_t Clearcoat = Base + 8;
        static constexpr std::int32_t ClearcoatR = Base + 9;
        static constexpr std::int32_t SpecularColor = Base + 10;
        static constexpr std::int32_t Specular = Base + 11;
        static constexpr std::int32_t SheenColor = Base + 12;
        static constexpr std::int32_t SheenR = Base + 13;
        static constexpr std::int32_t Transmission = Base + 14;
        static constexpr std::int32_t Thickness = Base + 15;
    }

    static constexpr std::int32_t TB_BaseColor = 1 << 0;
    static constexpr std::int32_t TB_Metallic = 1 << 1;
    static constexpr std::int32_t TB_Roughness = 1 << 2;
    static constexpr std::int32_t TB_Normal = 1 << 3;
    static constexpr std::int32_t TB_AO = 1 << 4;
    static constexpr std::int32_t TB_Emissive = 1 << 5;
    static constexpr std::int32_t TB_Opacity = 1 << 6;
    static constexpr std::int32_t TB_ORM = 1 << 7;
    static constexpr std::int32_t TB_Clearcoat = 1 << 8;
    static constexpr std::int32_t TB_ClearcoatR = 1 << 9;
    static constexpr std::int32_t TB_SpecColor = 1 << 10;
    static constexpr std::int32_t TB_Spec = 1 << 11;
    static constexpr std::int32_t TB_SheenColor = 1 << 12;
    static constexpr std::int32_t TB_SheenR = 1 << 13;
    static constexpr std::int32_t TB_Trans = 1 << 14;
    static constexpr std::int32_t TB_Thick = 1 << 15;


    static inline void BindIBL(IShader* shader, IEnvironment* env)
    {
        // Ensure environment binds: irradiance (cube) @0, prefiltered (cube) @1, BRDF LUT (2D) @2
        env->BindIBLAll(TEX_SLOTS::Irradiance, TEX_SLOTS::Prefilter, TEX_SLOTS::BRDFLUT);

        shader->SetUniform("u_IrradianceTexture", TEX_SLOTS::Irradiance);
        shader->SetUniform("u_PrefilteredTexture", TEX_SLOTS::Prefilter);
        shader->SetUniform("u_BRDFLUTTexture", TEX_SLOTS::BRDFLUT);

        auto I = env->GetIntensity();
        auto MIP = env->GetMipLevel();
        shader->SetUniform("u_IBLIntensity_Diffuse", I.Diffuse);
        shader->SetUniform("u_IBLIntensity_Specular", I.Specular);
        shader->SetUniform("u_IBLMipLevels", (float)MIP);
    }

    static void FlushQueue() noexcept
    {
        if (s_CommandQueue.empty()) return;

        std::sort(s_CommandQueue.begin(), s_CommandQueue.end());

        AssetManager& assetManager = AssetManager::GetInstance();
        IShader* const PBR = assetManager.Get<IShader>("PBR").get();

        IShader* currentShader = nullptr;
        Material* currentMaterial = nullptr;
        Mesh* currentMesh = nullptr;
        IEnvironment* currentEnv = nullptr;

        UUID currentViewKey{ 0 };
        bool haveView = false;

        for (const auto& cmd : s_CommandQueue)
        {
            IShader* const nextShader = PBR;
            Mesh* const nextMesh = cmd.MeshPtr;
            auto* const nextMat = cmd.MaterialPointer;
            auto* const nextEnv = cmd.EnvironmentPtr;
            if (!nextShader || !nextMesh || !nextMat || !nextEnv) continue;

            if (currentShader != nextShader)
            {
                if (currentShader) currentShader->Unbind();
                currentShader = nextShader;
                currentShader->Bind();
            }

            if (currentEnv != nextEnv)
            {
                currentEnv = nextEnv;
                BindIBL(currentShader, currentEnv);
            }

            if (!haveView || currentViewKey != cmd.SortKey)
            {
                haveView = true;
                currentViewKey = cmd.SortKey;

                currentShader->SetUniform("u_ViewMatrix", cmd.ViewMatrix);
                currentShader->SetUniform("u_ProjectionMatrix", cmd.ProjectionMatrix);
                currentShader->SetUniform("u_CameraPosition", cmd.CameraPosition);
                currentShader->SetUniform("u_SunLight.Direction", cmd.SunDirection);
                currentShader->SetUniform("u_SunLight.Color", cmd.SunColor);
                currentShader->SetUniform("u_SunLight.Intensity", cmd.SunIntensity);
            }

            if (currentMesh != nextMesh)
            {
                if (currentMesh) currentMesh->Unbind();
                currentMesh = nextMesh;
                currentMesh->Bind();
            }

            if (currentMaterial != nextMat)
            {
                float normalYFlip = 0.0f;
                std::int32_t texMask = 0;
                currentMaterial = nextMat;

                if (currentMaterial->HasTexture<CoreTextures>())
                {
                    const auto& C = currentMaterial->GetTexture<CoreTextures>();
                    const std::shared_ptr<BaseMaterial> baseMat = currentMaterial->GetBaseMaterial();

                    if (C.AlbedoTexture)
                    {
                        C.AlbedoTexture->Bind(TEX_SLOTS::BaseColor);
                        currentShader->SetUniform("u_Textures.BaseColorTexture", TEX_SLOTS::BaseColor);
                        currentShader->SetUniform("u_Attributes.BaseColor", C.BaseColor);
                        texMask |= TB_BaseColor;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::BaseColorTexture])
                        {
                            baseMat->Textures[TextureType::BaseColorTexture]->Bind(TEX_SLOTS::BaseColor);
                            currentShader->SetUniform("u_Textures.BaseColorTexture", TEX_SLOTS::BaseColor);
                            currentShader->SetUniform("u_Attributes.BaseColor", C.BaseColor);
                            texMask |= TB_BaseColor;
                        }
                        else
                        {
                            MOTION_ASSERT(false, "Material has CoreTextures but no BaseMaterial!");
                            continue;
                        }
                    }

                    if (C.MetallicTexture)
                    {
                        C.MetallicTexture->Bind(TEX_SLOTS::Metallic);
                        currentShader->SetUniform("u_Textures.MetallicTexture", TEX_SLOTS::Metallic);
                        currentShader->SetUniform("u_Attributes.MetallicFactor", C.MetallicFactor);
                        texMask |= TB_Metallic;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::MetallicTexture])
                        {
                            baseMat->Textures[TextureType::MetallicTexture]->Bind(TEX_SLOTS::Metallic);
                            currentShader->SetUniform("u_Textures.MetallicTexture", TEX_SLOTS::Metallic);
                            currentShader->SetUniform("u_Attributes.MetallicFactor", C.MetallicFactor);
                            texMask |= TB_Metallic;
                        }
                        else
                        {
                            MOTION_ASSERT(false, "Material has CoreTextures but no BaseMaterial!");
                            continue;
                        }
                    }

                    if (C.RoughnessTexture)
                    {
                        C.RoughnessTexture->Bind(TEX_SLOTS::Roughness);
                        currentShader->SetUniform("u_Textures.RoughnessTexture", TEX_SLOTS::Roughness);
                        currentShader->SetUniform("u_Attributes.RoughnessFactor", C.RoughnessFactor);
                        texMask |= TB_Roughness;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::RoughnessTexture])
                        {
                            baseMat->Textures[TextureType::RoughnessTexture]->Bind(TEX_SLOTS::Roughness);
                            currentShader->SetUniform("u_Textures.RoughnessTexture", TEX_SLOTS::Roughness);
                            currentShader->SetUniform("u_Attributes.RoughnessFactor", C.RoughnessFactor);
                            texMask |= TB_Roughness;
                        }
                        else
                        {
                            MOTION_ASSERT(false, "Material has CoreTextures but no BaseMaterial!");
                            continue;
                        }
                    }

                    if (C.NormalMapTexture)
                    {
                        C.NormalMapTexture->Bind(TEX_SLOTS::Normal);
                        currentShader->SetUniform("u_Textures.NormalMapTexture", TEX_SLOTS::Normal);
                        normalYFlip = C.NormalMapTexture->GetSpecification().InvertGreen ? 1.0f : 0.0f;
                        texMask |= TB_Normal;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::NormalTexture])
                        {
                            baseMat->Textures[TextureType::NormalTexture]->Bind(TEX_SLOTS::Normal);
                            currentShader->SetUniform("u_Textures.NormalMapTexture", TEX_SLOTS::Normal);
                            normalYFlip = baseMat->Textures[TextureType::NormalTexture]->GetSpecification().InvertGreen ? 1.0f : 0.0f;
                            texMask |= TB_Normal;
                        }
                        else
                        {
                            MOTION_ASSERT(false, "Material has CoreTextures but no BaseMaterial!");
                            continue;
                        }
                    }

                    if (C.AmbientOcclusionTexture)
                    {
                        C.AmbientOcclusionTexture->Bind(TEX_SLOTS::AO);
                        currentShader->SetUniform("u_Textures.AOTexture", TEX_SLOTS::AO);
                        texMask |= TB_AO;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::AmbientOcclusionTexture])
                        {
                            baseMat->Textures[TextureType::AmbientOcclusionTexture]->Bind(TEX_SLOTS::AO);
                            currentShader->SetUniform("u_Textures.AOTexture", TEX_SLOTS::AO);
                            texMask |= TB_AO;
                        }
                        else
                        {
                            MOTION_ASSERT(false, "Material has CoreTextures but no BaseMaterial!");
                            continue;
                        }
                    }

                    if (C.EmissiveTexture)
                    {
                        C.EmissiveTexture->Bind(TEX_SLOTS::Emissive);
                        currentShader->SetUniform("u_Textures.EmissiveTexture", TEX_SLOTS::Emissive);
                        texMask |= TB_Emissive;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::EmissiveTexture])
                        {
                            baseMat->Textures[TextureType::EmissiveTexture]->Bind(TEX_SLOTS::Emissive);
                            currentShader->SetUniform("u_Textures.EmissiveTexture", TEX_SLOTS::Emissive);
                            texMask |= TB_Emissive;
                        }
                        else
                        {
                            MOTION_ASSERT(false, "Material has CoreTextures but no BaseMaterial!");
                            continue;
                        }
                    }

                    if (C.OpacityTexture)
                    {
                        C.OpacityTexture->Bind(TEX_SLOTS::Opacity);
                        currentShader->SetUniform("u_Textures.OpacityTexture", TEX_SLOTS::Opacity);
                        currentShader->SetUniform("u_Attributes.Opacity", C.Opacity);
                        texMask |= TB_Opacity;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::OpacityTexture])
                        {
                            baseMat->Textures[TextureType::OpacityTexture]->Bind(TEX_SLOTS::Opacity);
                            currentShader->SetUniform("u_Textures.OpacityTexture", TEX_SLOTS::Opacity);
                            currentShader->SetUniform("u_Attributes.Opacity", C.Opacity);
                            texMask |= TB_Opacity;
                        }
                        else
                        {
                            MOTION_ASSERT(false, "Material has CoreTextures but no BaseMaterial!");
                            continue;
                        }
                    }
                }
                else
                {
                    MOTION_ASSERT(false, "Material does not have CoreTextures or BaseMaterial!");
                    continue;
                }

                if (currentMaterial->HasTexture<ExtendedTextures>())
                {
                    const auto& A = currentMaterial->GetTexture<ExtendedTextures>();

                    if (A.ClearcoatTexture)
                    {
                        A.ClearcoatTexture->Bind(TEX_SLOTS::Clearcoat);
                        currentShader->SetUniform("u_Textures.ClearcoatTexture", TEX_SLOTS::Clearcoat);
                        currentShader->SetUniform("u_Attributes.ClearcoatFactor", A.ClearcoatFactor);
                        texMask |= TB_Clearcoat;
                    }

                    if (A.ClearcoatRoughnessTexture)
                    {
                        A.ClearcoatRoughnessTexture->Bind(TEX_SLOTS::ClearcoatR);
                        currentShader->SetUniform("u_Textures.ClearcoatRoughnessTexture", TEX_SLOTS::ClearcoatR);
                        currentShader->SetUniform("u_Attributes.ClearcoatRoughnessFactor", A.ClearcoatRoughnessFactor);
                        texMask |= TB_ClearcoatR;
                    }

                    if (A.SpecularColorTexture)
                    {
                        A.SpecularColorTexture->Bind(TEX_SLOTS::SpecularColor);
                        currentShader->SetUniform("u_Textures.SpecularColorTexture", TEX_SLOTS::SpecularColor);
                        currentShader->SetUniform("u_Attributes.SpecularColor", A.SpecularColor);
                        texMask |= TB_SpecColor;
                    }

                    if (A.SpecularTexture)
                    {
                        A.SpecularTexture->Bind(TEX_SLOTS::Specular);
                        currentShader->SetUniform("u_Textures.SpecularTexture", TEX_SLOTS::Specular);
                        currentShader->SetUniform("u_Attributes.SpecularLevel", A.SpecularLevel);
                        texMask |= TB_Spec;
                    }
                }

                if (currentMaterial->HasTexture<PackedTextures>())
                {
                    const auto& A = currentMaterial->GetTexture<PackedTextures>();
                    if (A.ORMTexture)
                    {
                        A.ORMTexture->Bind(TEX_SLOTS::ORM);
                        currentShader->SetUniform("u_Textures.ORMTexture", TEX_SLOTS::ORM);
                        texMask |= TB_ORM;
                    }
                }

                if (currentMaterial->HasTexture<SheenFabricTextures>())
                {
                    const auto& A = currentMaterial->GetTexture<SheenFabricTextures>();
                    if (A.SheenTexture)
                    {
                        A.SheenTexture->Bind(TEX_SLOTS::SheenColor);
                        currentShader->SetUniform("u_Textures.SheenColorTexture", TEX_SLOTS::SheenColor);
                        currentShader->SetUniform("u_Attributes.SheenColor", A.SheenColor);
                        texMask |= TB_SheenColor;
                    }

                    if (A.SheenRoughnessTexture)
                    {
                        A.SheenRoughnessTexture->Bind(TEX_SLOTS::SheenR);
                        currentShader->SetUniform("u_Textures.SheenRoughnessTexture", TEX_SLOTS::SheenR);
                        currentShader->SetUniform("u_Attributes.SheenRoughnessFactor", A.SheenRoughness);
                        texMask |= TB_SheenR;
                    }
                }

                if (currentMaterial->HasTexture<TransmissionSubsurfaceTextures>())
                {
                    const auto& A = currentMaterial->GetTexture<TransmissionSubsurfaceTextures>();

                    if (A.TransmissionTexture)
                    {
                        A.TransmissionTexture->Bind(TEX_SLOTS::Transmission);
                        currentShader->SetUniform("u_Textures.TransmissionTexture", TEX_SLOTS::Transmission);
                        currentShader->SetUniform("u_Attributes.TransmissionFactor", A.Transmission);
                        texMask |= TB_Trans;
                    }

                    if (A.ThicknessTexture)
                    {
                        A.ThicknessTexture->Bind(TEX_SLOTS::Thickness);
                        currentShader->SetUniform("u_Textures.ThicknessTexture", TEX_SLOTS::Thickness);
                        currentShader->SetUniform("u_Attributes.ThicknessFactor", A.Thickness);

                        texMask |= TB_Thick;
                    }

                    if (A.ThicknessTexture && A.TransmissionTexture)
                    {
                        currentShader->SetUniform("u_Attributes.AttenuationColor", A.AttenuationColor);
                        currentShader->SetUniform("u_Attributes.AttenuationDistance", A.AttenuationDistance);
                        currentShader->SetUniform("u_Attributes.IOR", A.IOR);
                    }
                }

                currentShader->SetUniform("u_TextureBitmask", texMask);
                currentShader->SetUniform("u_NormalYFlip", normalYFlip);
            }

            // Per-draw transforms
            currentShader->SetUniform("u_ModelMatrix", cmd.ModelMatrix);
            currentShader->SetUniform("u_NormalMatrix", cmd.NormalMatrix);

            currentMesh->Render();
        }

        if (currentMesh)   currentMesh->Unbind();
        if (currentShader) currentShader->Unbind();
        s_CommandQueue.clear();
    }

    void SceneRenderer::Submit(Scene* scene) noexcept
    {
        if (!scene) { MOTION_CORE_ERROR("Scene is null >> SKIPPING SUBMISSION"); return; }
        s_CurrentScene = scene;

        SceneDrawCommand cmd{};
        for (const auto& entity : scene->m_Entities)
        {
            if (!entity || !entity->HasComponent<StaticMeshComponent>()) continue;

            const auto& sm = entity->GetComponent<StaticMeshComponent>();
            if (!sm.Model || sm.Model->GetMeshesCount() <= 0) continue;

            auto& model = sm.Model;
            for (auto& mesh : *model)
            {
                cmd.SortKey = scene->GetID();
                cmd.MaterialPointer = mesh->Materials.get();
                cmd.MeshPtr = mesh.get();
                cmd.EnvironmentPtr = scene->GetEnvironment().Env.get();

                cmd.ModelMatrix = entity->HasComponent<TransformComponent>()
                    ? entity->GetComponent<TransformComponent>().GetTransform()
                    : glm::mat4(1.0f);

                auto& camera = scene->GetCamera();
                auto& env = scene->GetEnvironment();

                cmd.ViewMatrix = camera.Camera.View;
                cmd.ProjectionMatrix = camera.Camera.Projection;
                cmd.NormalMatrix = glm::mat3(glm::transpose(glm::inverse(glm::mat3(cmd.ModelMatrix))));
                cmd.CameraPosition = camera.Camera.Position;

                cmd.SunDirection = env.Sun.Direction;
                cmd.SunColor = env.Sun.Color;
                cmd.SunIntensity = env.Sun.Intensity;

                s_CommandQueue.push_back(cmd);
            }
        }
    }

    void SceneRenderer::EndScene() noexcept
    {
        RenderSkyboxPass(s_CurrentScene);
        if (!s_CommandQueue.empty())
            FlushQueue();
    }

    void SceneRenderer::RenderSkyboxPass(Scene* scene) noexcept
    {
        if (!scene) return;

        auto& env = scene->GetEnvironment();
        IEnvironment* ibl = env.Env ? env.Env.get() : nullptr;
        if (!ibl) return;

        const auto& cam = scene->GetCamera().Camera;
        ibl->Render(cam.View, cam.Projection);
    }
}
