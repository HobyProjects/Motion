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
        static constexpr std::int32_t ClearcoatN = Base + 16;   // Clearcoat Normal
        static constexpr std::int32_t Displacement = Base + 17; // Displacement
        static constexpr std::int32_t Anisotropy = Base + 18;   // Anisotropy
        static constexpr std::int32_t Iridescence = Base + 19;  // Iridescence
        static constexpr std::int32_t IridescenceThickness = Base + 20;  // Iridescence Thickness   
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
    static constexpr std::int32_t TB_ClearcoatN = 1 << 16;
    static constexpr std::int32_t TB_Displacement = 1 << 17;
    static constexpr std::int32_t TB_Aniso = 1 << 18;
    static constexpr std::int32_t TB_Iridescence = 1 << 19;
    static constexpr std::int32_t TB_IridescenceThickness = 1 << 20;



    static inline void BindIBL(IShader* shader, IEnvironment* env)
    {
        // Ensure environment binds: irradiance (cube) @0, prefiltered (cube) @1, BRDF LUT (2D) @2
        env->BindAll(TEX_SLOTS::Irradiance, TEX_SLOTS::Prefilter, TEX_SLOTS::BRDFLUT);

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
                currentMaterial = nextMat;

                float normalYFlip = 0.0f;
                std::int32_t texMask = 0;
                auto baseMat = currentMaterial->GetBaseMaterial();
                if (!baseMat) continue;

                if (currentMaterial->HasTexture<AlphaProperties>())
                {
                    auto& ALPHA = currentMaterial->GetTexture<AlphaProperties>();
                    currentShader->SetUniform("u_Attributes.AlphaMode", static_cast<std::int32_t>(ALPHA.Mode));
                    currentShader->SetUniform("u_Attributes.AlphaCutoff", ALPHA.AlphaCutoff);
                    currentShader->SetUniform("u_Attributes.Opacity", ALPHA.OpacityFactor);

                    if (ALPHA.OpacityTexture)
                    {
                        ALPHA.OpacityTexture->Bind(TEX_SLOTS::Opacity);
                        currentShader->SetUniform("u_OpacityTexture", TEX_SLOTS::Opacity);
                        texMask |= TB_Opacity;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::OpacityTexture])
                        {
                            baseMat->Textures[TextureType::OpacityTexture]->Bind(TEX_SLOTS::Opacity);
                            currentShader->SetUniform("u_OpacityTexture", TEX_SLOTS::Opacity);
                            texMask |= TB_Opacity;
                        }
                    }
                }

                if (currentMaterial->HasTexture<CorePBR>())
                {
                    auto& C = currentMaterial->GetTexture<CorePBR>();

                    currentShader->SetUniform("u_Attributes.BaseColor", C.BaseColorFactor);
                    currentShader->SetUniform("u_Attributes.MetallicFactor", C.MetallicFactor);
                    currentShader->SetUniform("u_Attributes.RoughnessFactor", C.RoughnessFactor);
                    currentShader->SetUniform("u_Attributes.NormalScale", C.NormalScale);
                    currentShader->SetUniform("u_Attributes.AmbientOcclusion", C.OcclusionStrength);
                    currentShader->SetUniform("u_Attributes.EmissiveStrength", C.EmissiveStrength);
                    currentShader->SetUniform("u_Attributes.EmissiveColor", C.EmissiveFactor);
                    currentShader->SetUniform("u_DisplacementScale", C.DisplacementScale);
                    currentShader->SetUniform("u_DisplacementBias", C.DisplacementBias);

                    if (C.BaseColorTexture)
                    {
                        C.BaseColorTexture->Bind(TEX_SLOTS::BaseColor);
                        currentShader->SetUniform("u_BaseColorTexture", TEX_SLOTS::BaseColor);
                        texMask |= TB_BaseColor;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::BaseColorTexture])
                        {
                            baseMat->Textures[TextureType::BaseColorTexture]->Bind(TEX_SLOTS::BaseColor);
                            currentShader->SetUniform("u_BaseColorTexture", TEX_SLOTS::BaseColor);
                            texMask |= TB_BaseColor;
                        }
                    }

                    if (C.NormalTexture)
                    {
                        C.NormalTexture->Bind(TEX_SLOTS::Normal);
                        currentShader->SetUniform("u_NormalTexture", TEX_SLOTS::Normal);
                        normalYFlip = C.NormalTexture->GetSpecification().InvertGreen ? 1.0f : 0.0f;
                        texMask |= TB_Normal;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::NormalTexture])
                        {
                            baseMat->Textures[TextureType::NormalTexture]->Bind(TEX_SLOTS::Normal);
                            currentShader->SetUniform("u_NormalTexture", TEX_SLOTS::Normal);
                            texMask |= TB_Normal;
                        }
                    }

                    if (C.MetallicTexture)
                    {
                        C.MetallicTexture->Bind(TEX_SLOTS::Metallic);
                        currentShader->SetUniform("u_MetallicTexture", TEX_SLOTS::Metallic);
                        texMask |= TB_Metallic;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::MetallicTexture])
                        {
                            baseMat->Textures[TextureType::MetallicTexture]->Bind(TEX_SLOTS::Metallic);
                            currentShader->SetUniform("u_MetallicTexture", TEX_SLOTS::Metallic);
                            texMask |= TB_Metallic;
                        }
                    }

                    if (C.RoughnessTexture)
                    {
                        C.RoughnessTexture->Bind(TEX_SLOTS::Roughness);
                        currentShader->SetUniform("u_RoughnessTexture", TEX_SLOTS::Roughness);
                        texMask |= TB_Roughness;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::RoughnessTexture])
                        {
                            baseMat->Textures[TextureType::RoughnessTexture]->Bind(TEX_SLOTS::Roughness);
                            currentShader->SetUniform("u_RoughnessTexture", TEX_SLOTS::Roughness);
                            texMask |= TB_Roughness;
                        }
                    }

                    if (C.EmissiveTexture)
                    {
                        C.EmissiveTexture->Bind(TEX_SLOTS::Emissive);
                        currentShader->SetUniform("u_EmissiveTexture", TEX_SLOTS::Emissive);
                        texMask |= TB_Emissive;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::EmissiveTexture])
                        {
                            baseMat->Textures[TextureType::EmissiveTexture]->Bind(TEX_SLOTS::Emissive);
                            currentShader->SetUniform("u_EmissiveTexture", TEX_SLOTS::Emissive);
                            texMask |= TB_Emissive;
                        }
                    }

                    if (C.OcclusionTexture)
                    {
                        C.OcclusionTexture->Bind(TEX_SLOTS::AO);
                        currentShader->SetUniform("u_OcclusionTexture", TEX_SLOTS::AO);
                        texMask |= TB_AO;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::AmbientOcclusionTexture])
                        {
                            baseMat->Textures[TextureType::AmbientOcclusionTexture]->Bind(TEX_SLOTS::AO);
                            currentShader->SetUniform("u_OcclusionTexture", TEX_SLOTS::AO);
                            texMask |= TB_AO;
                        }
                    }

                    if (C.DisplacementTexture)
                    {
                        C.DisplacementTexture->Bind(TEX_SLOTS::Displacement);
                        currentShader->SetUniform("u_DisplacementTexture", TEX_SLOTS::Displacement);
                        currentShader->SetUniform("u_DisplacementBitMask", TB_Displacement);
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::DisplacementTexture])
                        {
                            baseMat->Textures[TextureType::DisplacementTexture]->Bind(TEX_SLOTS::Displacement);
                            currentShader->SetUniform("u_DisplacementTexture", TEX_SLOTS::Displacement);
                            currentShader->SetUniform("u_DisplacementBitMask", TB_Displacement);
                        }
                    }

                }

                if (currentMaterial->HasTexture<PackedMaps>())
                {
                    auto& P = currentMaterial->GetTexture<PackedMaps>();
                    P.ORMTexture->Bind(TEX_SLOTS::ORM);
                    currentShader->SetUniform("u_ORMTexture", TEX_SLOTS::ORM);
                    texMask |= TB_ORM;
                }

                if (currentMaterial->HasTexture<ClearcoatExtension>())
                {
                    auto& CC = currentMaterial->GetTexture<ClearcoatExtension>();
                    currentShader->SetUniform("u_Attributes.ClearcoatFactor", CC.ClearcoatFactor);
                    currentShader->SetUniform("u_Attributes.ClearcoatRoughnessFactor", CC.ClearcoatRoughnessFactor);
                    currentShader->SetUniform("u_Attributes.ClearcoatNormalScale", CC.ClearcoatNormalScale);

                    if (CC.ClearcoatTexture)
                    {
                        CC.ClearcoatTexture->Bind(TEX_SLOTS::Clearcoat);
                        currentShader->SetUniform("u_ClearcoatTexture", TEX_SLOTS::Clearcoat);
                        texMask |= TB_Clearcoat;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::ClearcoatTexture])
                        {
                            baseMat->Textures[TextureType::ClearcoatTexture]->Bind(TEX_SLOTS::Clearcoat);
                            currentShader->SetUniform("u_ClearcoatTexture", TEX_SLOTS::Clearcoat);
                            texMask |= TB_Clearcoat;
                        }
                    }

                    if (CC.ClearcoatRoughnessTexture)
                    {
                        CC.ClearcoatRoughnessTexture->Bind(TEX_SLOTS::ClearcoatR);
                        currentShader->SetUniform("u_ClearcoatRoughnessTexture", TEX_SLOTS::ClearcoatR);
                        texMask |= TB_ClearcoatR;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::ClearcoatRoughnessTexture])
                        {
                            baseMat->Textures[TextureType::ClearcoatRoughnessTexture]->Bind(TEX_SLOTS::ClearcoatR);
                            currentShader->SetUniform("u_ClearcoatRoughnessTexture", TEX_SLOTS::ClearcoatR);
                            texMask |= TB_ClearcoatR;
                        }
                    }

                    if (CC.ClearcoatNormalTexture)
                    {
                        CC.ClearcoatNormalTexture->Bind(TEX_SLOTS::ClearcoatN);
                        currentShader->SetUniform("u_ClearcoatNormalTexture", TEX_SLOTS::ClearcoatN);
                        texMask |= TB_ClearcoatN;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::ClearcoatNormalTexture])
                        {
                            baseMat->Textures[TextureType::ClearcoatNormalTexture]->Bind(TEX_SLOTS::ClearcoatN);
                            currentShader->SetUniform("u_ClearcoatNormalTexture", TEX_SLOTS::ClearcoatN);
                            texMask |= TB_ClearcoatN;
                        }
                    }
                }

                if (currentMaterial->HasTexture<SheenExtension>())
                {
                    auto& S = currentMaterial->GetTexture<SheenExtension>();
                    currentShader->SetUniform("u_Attributes.SheenColor", S.SheenColorFactor);
                    currentShader->SetUniform("u_Attributes.SheenRoughnessFactor", S.SheenRoughnessFactor);

                    if (S.SheenColorTexture)
                    {
                        S.SheenColorTexture->Bind(TEX_SLOTS::SheenColor);
                        currentShader->SetUniform("u_SheenColorTexture", TEX_SLOTS::SheenColor);
                        texMask |= TB_SheenColor;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::SheenColorTexture])
                        {
                            baseMat->Textures[TextureType::SheenColorTexture]->Bind(TEX_SLOTS::SheenColor);
                            currentShader->SetUniform("u_SheenColorTexture", TEX_SLOTS::SheenColor);
                            texMask |= TB_SheenColor;
                        }
                    }

                    if (S.SheenRoughnessTexture)
                    {
                        S.SheenRoughnessTexture->Bind(TEX_SLOTS::SheenR);
                        currentShader->SetUniform("u_SheenRoughnessTexture", TEX_SLOTS::SheenR);
                        texMask |= TB_SheenR;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::SheenRoughnessTexture])
                        {
                            baseMat->Textures[TextureType::SheenRoughnessTexture]->Bind(TEX_SLOTS::SheenR);
                            currentShader->SetUniform("u_SheenRoughnessTexture", TEX_SLOTS::SheenR);
                            texMask |= TB_SheenR;
                        }
                    }
                }

                if (currentMaterial->HasTexture<TransmissionExtension>())
                {
                    auto& T = currentMaterial->GetTexture<TransmissionExtension>();
                    currentShader->SetUniform("u_Attributes.TransmissionFactor", T.TransmissionFactor);

                    if (T.TransmissionTexture)
                    {
                        T.TransmissionTexture->Bind(TEX_SLOTS::Transmission);
                        currentShader->SetUniform("u_TransmissionTexture", TEX_SLOTS::Transmission);
                        texMask |= TB_Trans;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::TransmissionTexture])
                        {
                            baseMat->Textures[TextureType::TransmissionTexture]->Bind(TEX_SLOTS::Transmission);
                            currentShader->SetUniform("u_TransmissionTexture", TEX_SLOTS::Transmission);
                            texMask |= TB_Trans;
                        }
                    }
                }

                if (currentMaterial->HasTexture<VolumeExtension>())
                {
                    auto& V = currentMaterial->GetTexture<VolumeExtension>();
                    currentShader->SetUniform("u_Attributes.ThicknessFactor", V.ThicknessFactor);
                    currentShader->SetUniform("u_Attributes.AttenuationDistance", V.AttenuationDistance);
                    currentShader->SetUniform("u_Attributes.AttenuationColor", V.AttenuationColor);

                    if (V.ThicknessTexture)
                    {
                        V.ThicknessTexture->Bind(TEX_SLOTS::Thickness);
                        currentShader->SetUniform("u_ThicknessTexture", TEX_SLOTS::Thickness);
                        texMask |= TB_Thick;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::ThicknessTexture])
                        {
                            baseMat->Textures[TextureType::ThicknessTexture]->Bind(TEX_SLOTS::Thickness);
                            currentShader->SetUniform("u_ThicknessTexture", TEX_SLOTS::Thickness);
                            texMask |= TB_Thick;
                        }
                    }
                }

                if (currentMaterial->HasTexture<IORExtension>())
                {
                    auto& IOR = currentMaterial->GetTexture<IORExtension>();
                    currentShader->SetUniform("u_Attributes.IOR", IOR.IOR);
                }

                if (currentMaterial->HasTexture<AnisotropyExtension>())
                {
                    auto& ANISO = currentMaterial->GetTexture<AnisotropyExtension>();
                    currentShader->SetUniform("u_Attributes.AnisotropyStrength", ANISO.AnisotropyStrength);
                    currentShader->SetUniform("u_Attributes.AnisotropyRotation", ANISO.AnisotropyRotation);

                    if (ANISO.AnisotropyTexture)
                    {
                        ANISO.AnisotropyTexture->Bind(TEX_SLOTS::Anisotropy);
                        currentShader->SetUniform("u_AnisotropyTexture", TEX_SLOTS::Anisotropy);
                        texMask |= TB_Aniso;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::AnisotropyTexture])
                        {
                            baseMat->Textures[TextureType::AnisotropyTexture]->Bind(TEX_SLOTS::Anisotropy);
                            currentShader->SetUniform("u_AnisotropyTexture", TEX_SLOTS::Anisotropy);
                            texMask |= TB_Aniso;
                        }
                    }
                }

                if (currentMaterial->HasTexture<IridescenceExtension>())
                {
                    auto& IRID = currentMaterial->GetTexture<IridescenceExtension>();
                    currentShader->SetUniform("u_Attributes.IridescenceFactor", IRID.IridescenceFactor);
                    currentShader->SetUniform("u_Attributes.IridescenceIor", IRID.IridescenceIor);
                    currentShader->SetUniform("u_Attributes.IridescenceThicknessMin", IRID.IridescenceThicknessMin);
                    currentShader->SetUniform("u_Attributes.IridescenceThicknessMax", IRID.IridescenceThicknessMax);

                    if (IRID.IridescenceTexture)
                    {
                        IRID.IridescenceTexture->Bind(TEX_SLOTS::Iridescence);
                        currentShader->SetUniform("u_IridescenceTexture", TEX_SLOTS::Iridescence);
                        texMask |= TB_Iridescence;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::IridescenceTexture])
                        {
                            baseMat->Textures[TextureType::IridescenceTexture]->Bind(TEX_SLOTS::Iridescence);
                            currentShader->SetUniform("u_IridescenceTexture", TEX_SLOTS::Iridescence);
                            texMask |= TB_Iridescence;
                        }
                    }

                    if (IRID.IridescenceThicknessTexture)
                    {
                        IRID.IridescenceThicknessTexture->Bind(TEX_SLOTS::IridescenceThickness);
                        currentShader->SetUniform("u_IridescenceThicknessTexture", TEX_SLOTS::IridescenceThickness);
                        texMask |= TB_IridescenceThickness;
                    }
                    else
                    {
                        if (baseMat->Textures[TextureType::IridescenceThicknessTexture])
                        {
                            baseMat->Textures[TextureType::IridescenceThicknessTexture]->Bind(TEX_SLOTS::IridescenceThickness);
                            currentShader->SetUniform("u_IridescenceThicknessTexture", TEX_SLOTS::IridescenceThickness);
                            texMask |= TB_IridescenceThickness;
                        }
                    }
                }


                currentShader->SetUniform("u_TextureBitmask", texMask);
                currentShader->SetUniform("u_NormalYFlip", normalYFlip);
            }

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
                cmd.EnvironmentPtr = scene->GetEnvironment().EnvironmentInstance.get();

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
        IEnvironment* ibl = env.EnvironmentInstance ? env.EnvironmentInstance.get() : nullptr;
        if (!ibl) return;

        const auto& cam = scene->GetCamera().Camera;
        ibl->Render(cam.View, cam.Projection);
    }
}
