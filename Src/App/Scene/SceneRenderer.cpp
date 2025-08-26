#include "CorePCH.hpp"
#include "SceneRenderer.hpp"
#include "Scene.hpp"

namespace Motion
{
    Scene* SceneRenderer::s_CurrentScene = nullptr;
    static std::vector<SceneDrawCommand> s_DrawCommands;

    namespace TEX_SLOTS
    {
        static constexpr std::int32_t Irradiance                 = 1;
        static constexpr std::int32_t Prefilter                  = 2;
        static constexpr std::int32_t BRDFLUT                    = 3;

        // Leave 4..7 free for skybox/utility if needed

        static constexpr std::int32_t Base                       = 8;
        static constexpr std::int32_t BaseColor                  = Base + 0;
        static constexpr std::int32_t Metallic                   = Base + 1;
        static constexpr std::int32_t Roughness                  = Base + 2;
        static constexpr std::int32_t Normal                     = Base + 3;
        static constexpr std::int32_t AO                         = Base + 4;
        static constexpr std::int32_t Emissive                   = Base + 5;
        static constexpr std::int32_t Opacity                    = Base + 6;
        static constexpr std::int32_t ORM                        = Base + 7;
        static constexpr std::int32_t Clearcoat                  = Base + 8;
        static constexpr std::int32_t ClearcoatR                 = Base + 9;
        static constexpr std::int32_t SpecularColor              = Base + 10;
        static constexpr std::int32_t Specular                   = Base + 11;
        static constexpr std::int32_t SheenColor                 = Base + 12;
        static constexpr std::int32_t SheenR                     = Base + 13;
        static constexpr std::int32_t Transmission               = Base + 14;
        static constexpr std::int32_t Thickness                  = Base + 15;
        static constexpr std::int32_t ClearcoatN                 = Base + 16;
        static constexpr std::int32_t Displacement               = Base + 17;
        static constexpr std::int32_t Anisotropy                 = Base + 18;
        static constexpr std::int32_t Iridescence                = Base + 19;
        static constexpr std::int32_t IridescenceThickness       = Base + 20;
    }

    static constexpr std::int32_t TB_BASE_COLOR                 = 1 << 0;
    static constexpr std::int32_t TB_METALLIC                   = 1 << 1;
    static constexpr std::int32_t TB_ROUGHNESS                  = 1 << 2;
    static constexpr std::int32_t TB_NORMAL                     = 1 << 3;
    static constexpr std::int32_t TB_AO                         = 1 << 4;
    static constexpr std::int32_t TB_EMISSIVE                   = 1 << 5;
    static constexpr std::int32_t TB_OPACITY                    = 1 << 6;
    static constexpr std::int32_t TB_ORM                        = 1 << 7;
    static constexpr std::int32_t TB_DISPLACEMENT               = 1 << 8;

    static constexpr std::int32_t TB_CLEARCOAT                   = 1 << 9;
    static constexpr std::int32_t TB_CLEARCOAT_R                 = 1 << 10;
    static constexpr std::int32_t TB_SPEC_COLOR                  = 1 << 11;
    static constexpr std::int32_t TB_SPEC                        = 1 << 12;
    static constexpr std::int32_t TB_SHEEN_COLOR                 = 1 << 13;
    static constexpr std::int32_t TB_SHEEN_R                     = 1 << 14;
    static constexpr std::int32_t TB_TRANSMISSION                = 1 << 15;
    static constexpr std::int32_t TB_THICKNESS                   = 1 << 16;
    static constexpr std::int32_t TB_CLEARCOAT_N                 = 1 << 17;
    static constexpr std::int32_t TB_ANISOTROPY                  = 1 << 18;
    static constexpr std::int32_t TB_IRIDESCENCE                 = 1 << 19;
    static constexpr std::int32_t TB_IRIDESCENCE_THICKNESS       = 1 << 20;

    static inline void BindIBL(IShader* shader, IEnvironment* env)
    {
        IBLTextureBinding binding;
        binding.SlotBRDFLUT = TEX_SLOTS::BRDFLUT;
        binding.SlotIrradiance = TEX_SLOTS::Irradiance;
        binding.SlotPrefiltered =  TEX_SLOTS::Prefilter;

        env->BindIBL(binding);

        shader->SetUniform("u_IrradianceTexture",   TEX_SLOTS::Irradiance);
        shader->SetUniform("u_PrefilteredTexture",  TEX_SLOTS::Prefilter);
        shader->SetUniform("u_BRDFLUTTexture",      TEX_SLOTS::BRDFLUT);
        shader->SetUniform("u_IBLIntensity_Diffuse",    1.0f);
        shader->SetUniform("u_IBLIntensity_Specular",   1.0f);
        shader->SetUniform("u_IBLMipLevels",            5.0f);
    }

    static ShaderFeatureMask GetShaderMask(Material* currentMaterial)
    {
        if (!currentMaterial) return 0;
        ShaderFeatureMask m{GLSL_SHADER_EXT_NONE};

        if (currentMaterial->HasTexture<ClearcoatExtension>())      m |= GLSL_SHADER_EXT_CLEARCOAT;
        if (currentMaterial->HasTexture<SheenExtension>())          m |= GLSL_SHADER_EXT_SHEEN;
        if (currentMaterial->HasTexture<AnisotropyExtension>())     m |= GLSL_SHADER_EXT_ANISOTROPY;
        if (currentMaterial->HasTexture<IridescenceExtension>())    m |= GLSL_SHADER_EXT_IRIDESCENCE;
        if (currentMaterial->HasTexture<TransmissionExtension>())   m |= GLSL_SHADER_EXT_TRANSMISSION;
        
        return m;
    }

    static void BindMaterialAndTextures(IShader* currentShader, Material* currentMaterial, int& texMask)
    {
        auto baseMat = currentMaterial->GetBaseMaterial();
        if (!baseMat) return;

        if (currentMaterial->HasTexture<AlphaProperties>())
        {
            auto& ALPHA = currentMaterial->GetTexture<AlphaProperties>();
            currentShader->SetUniform("u_Attributes.AlphaMode", static_cast<std::int32_t>(ALPHA.Mode));
            currentShader->SetUniform("u_Attributes.AlphaCutoff", ALPHA.AlphaCutoff);
            currentShader->SetUniform("u_Attributes.OpacityFactor", ALPHA.OpacityFactor);

            if (ALPHA.OpacityTexture)
            {
                ALPHA.OpacityTexture->Bind(TEX_SLOTS::Opacity);
                currentShader->SetUniform("u_OpacityTexture", TEX_SLOTS::Opacity);
                texMask |= TB_OPACITY;
            }
        }

        if (currentMaterial->HasTexture<CorePBR>())
        {
            auto& C = currentMaterial->GetTexture<CorePBR>();

            currentShader->SetUniform("u_Attributes.BaseColorFactor", C.BaseColorFactor);
            currentShader->SetUniform("u_Attributes.MetallicFactor", C.MetallicFactor);
            currentShader->SetUniform("u_Attributes.RoughnessFactor", C.RoughnessFactor);
            currentShader->SetUniform("u_Attributes.NormalScale", C.NormalScale);
            currentShader->SetUniform("u_Attributes.OcclusionStrength", C.OcclusionStrength);
            currentShader->SetUniform("u_Attributes.EmissiveStrength", C.EmissiveStrength);
            currentShader->SetUniform("u_Attributes.EmissiveFactor", C.EmissiveFactor);
            currentShader->SetUniform("u_Attributes.DisplacementScale", C.DisplacementScale);
            currentShader->SetUniform("u_Attributes.DisplacementBias", C.DisplacementBias);

            if (C.NormalTexture)
            {
                float normalYFlip = C.NormalTexture->GetSpecification().InvertGreen ? 1.0f : 0.0f;
                currentShader->SetUniform("u_Attributes.NormalYFlip", normalYFlip);
            }

            if (C.BaseColorTexture)
            {
                C.BaseColorTexture->Bind(TEX_SLOTS::BaseColor);
                currentShader->SetUniform("u_BaseColorTexture", TEX_SLOTS::BaseColor);
                texMask |= TB_BASE_COLOR;
            }

            if (C.NormalTexture)
            {
                C.NormalTexture->Bind(TEX_SLOTS::Normal);
                currentShader->SetUniform("u_NormalTexture", TEX_SLOTS::Normal);
                texMask |= TB_NORMAL;
            }

            if (C.MetallicTexture)
            {
                C.MetallicTexture->Bind(TEX_SLOTS::Metallic);
                currentShader->SetUniform("u_MetallicTexture", TEX_SLOTS::Metallic);
                texMask |= TB_METALLIC;
            }

            if (C.RoughnessTexture)
            {
                C.RoughnessTexture->Bind(TEX_SLOTS::Roughness);
                currentShader->SetUniform("u_RoughnessTexture", TEX_SLOTS::Roughness);
                texMask |= TB_ROUGHNESS;
            }

            if (C.EmissiveTexture)
            {
                C.EmissiveTexture->Bind(TEX_SLOTS::Emissive);
                currentShader->SetUniform("u_EmissiveTexture", TEX_SLOTS::Emissive);
                texMask |= TB_EMISSIVE;
            }

            if (C.OcclusionTexture)
            {
                C.OcclusionTexture->Bind(TEX_SLOTS::AO);
                currentShader->SetUniform("u_OcclusionTexture", TEX_SLOTS::AO);
                texMask |= TB_AO;
            }

            if (C.DisplacementTexture)
            {
                C.DisplacementTexture->Bind(TEX_SLOTS::Displacement);
                currentShader->SetUniform("u_DisplacementTexture", TEX_SLOTS::Displacement);
                texMask |= TB_DISPLACEMENT;
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
                texMask |= TB_CLEARCOAT;
            }
            else if (baseMat->Textures[TextureType::ClearcoatTexture])
            {
                baseMat->Textures[TextureType::ClearcoatTexture]->Bind(TEX_SLOTS::Clearcoat);
                currentShader->SetUniform("u_ClearcoatTexture", TEX_SLOTS::Clearcoat);
                texMask |= TB_CLEARCOAT;
            }

            if (CC.ClearcoatRoughnessTexture)
            {
                CC.ClearcoatRoughnessTexture->Bind(TEX_SLOTS::ClearcoatR);
                currentShader->SetUniform("u_ClearcoatRoughnessTexture", TEX_SLOTS::ClearcoatR);
                texMask |= TB_CLEARCOAT_R;
            }
            else if (baseMat->Textures[TextureType::ClearcoatRoughnessTexture])
            {
                baseMat->Textures[TextureType::ClearcoatRoughnessTexture]->Bind(TEX_SLOTS::ClearcoatR);
                currentShader->SetUniform("u_ClearcoatRoughnessTexture", TEX_SLOTS::ClearcoatR);
                texMask |= TB_CLEARCOAT_R;
            }

            if (CC.ClearcoatNormalTexture)
            {
                CC.ClearcoatNormalTexture->Bind(TEX_SLOTS::ClearcoatN);
                currentShader->SetUniform("u_ClearcoatNormalTexture", TEX_SLOTS::ClearcoatN);
                texMask |= TB_CLEARCOAT_N;
            }
            else if (baseMat->Textures[TextureType::ClearcoatNormalTexture])
            {
                baseMat->Textures[TextureType::ClearcoatNormalTexture]->Bind(TEX_SLOTS::ClearcoatN);
                currentShader->SetUniform("u_ClearcoatNormalTexture", TEX_SLOTS::ClearcoatN);
                texMask |= TB_CLEARCOAT_N;
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
                texMask |= TB_SHEEN_COLOR;
            }
            else if (baseMat->Textures[TextureType::SheenColorTexture])
            {
                baseMat->Textures[TextureType::SheenColorTexture]->Bind(TEX_SLOTS::SheenColor);
                currentShader->SetUniform("u_SheenColorTexture", TEX_SLOTS::SheenColor);
                texMask |= TB_SHEEN_COLOR;
            }

            if (S.SheenRoughnessTexture)
            {
                S.SheenRoughnessTexture->Bind(TEX_SLOTS::SheenR);
                currentShader->SetUniform("u_SheenRoughnessTexture", TEX_SLOTS::SheenR);
                texMask |= TB_SHEEN_R;
            }
            else if (baseMat->Textures[TextureType::SheenRoughnessTexture])
            {
                baseMat->Textures[TextureType::SheenRoughnessTexture]->Bind(TEX_SLOTS::SheenR);
                currentShader->SetUniform("u_SheenRoughnessTexture", TEX_SLOTS::SheenR);
                texMask |= TB_SHEEN_R;
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
                texMask |= TB_TRANSMISSION;
            }
            else if (baseMat->Textures[TextureType::TransmissionTexture])
            {
                baseMat->Textures[TextureType::TransmissionTexture]->Bind(TEX_SLOTS::Transmission);
                currentShader->SetUniform("u_TransmissionTexture", TEX_SLOTS::Transmission);
                texMask |= TB_TRANSMISSION;
            }
        }

        if (currentMaterial->HasTexture<VolumeExtension>())
        {
            auto& V = currentMaterial->GetTexture<VolumeExtension>();
            currentShader->SetUniform("u_Attributes.ThicknessFactor", V.ThicknessFactor);
            currentShader->SetUniform("u_Attributes.AttenuationDistance", V.AttenuationDistance);
            currentShader->SetUniform("u_Attributes.AttenuationColor", V.AttenuationColor);
            currentShader->SetUniform("u_Attributes.IOR", V.IOR);

            if (V.ThicknessTexture)
            {
                V.ThicknessTexture->Bind(TEX_SLOTS::Thickness);
                currentShader->SetUniform("u_ThicknessTexture", TEX_SLOTS::Thickness);
                texMask |= TB_THICKNESS;
            }
            else if (baseMat->Textures[TextureType::ThicknessTexture])
            {
                baseMat->Textures[TextureType::ThicknessTexture]->Bind(TEX_SLOTS::Thickness);
                currentShader->SetUniform("u_ThicknessTexture", TEX_SLOTS::Thickness);
                texMask |= TB_THICKNESS;
            }
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
                texMask |= TB_ANISOTROPY;
            }
            else if (baseMat->Textures[TextureType::AnisotropyTexture])
            {
                baseMat->Textures[TextureType::AnisotropyTexture]->Bind(TEX_SLOTS::Anisotropy);
                currentShader->SetUniform("u_AnisotropyTexture", TEX_SLOTS::Anisotropy);
                texMask |= TB_ANISOTROPY;
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
                texMask |= TB_IRIDESCENCE;
            }
            else if (baseMat->Textures[TextureType::IridescenceTexture])
            {
                baseMat->Textures[TextureType::IridescenceTexture]->Bind(TEX_SLOTS::Iridescence);
                currentShader->SetUniform("u_IridescenceTexture", TEX_SLOTS::Iridescence);
                texMask |= TB_IRIDESCENCE;
            }

            if (IRID.IridescenceThicknessTexture)
            {
                IRID.IridescenceThicknessTexture->Bind(TEX_SLOTS::IridescenceThickness);
                currentShader->SetUniform("u_IridescenceThicknessTexture", TEX_SLOTS::IridescenceThickness);
                texMask |= TB_IRIDESCENCE_THICKNESS;
            }
            else if (baseMat->Textures[TextureType::IridescenceThicknessTexture])
            {
                baseMat->Textures[TextureType::IridescenceThicknessTexture]->Bind(TEX_SLOTS::IridescenceThickness);
                currentShader->SetUniform("u_IridescenceThicknessTexture", TEX_SLOTS::IridescenceThickness);
                texMask |= TB_IRIDESCENCE_THICKNESS;
            }
        }
    }

    static void ApplyPipeline(const DrawFlags& d)
    {
        if(d & DrawFlags::DepthTest)        Renderer::ApplyDrawFlags(DrawFlags::DepthTest);
        if(d & DrawFlags::SkipDepthMask)    Renderer::ApplyDrawFlags(DrawFlags::SkipDepthMask);
        if(d & DrawFlags::Wireframe)        Renderer::ApplyDrawFlags(DrawFlags::Wireframe);
        if(d & DrawFlags::Blending)         Renderer::ApplyDrawFlags(DrawFlags::Blending);
        if(d & DrawFlags::CullFace)         Renderer::ApplyDrawFlags(DrawFlags::CullFace);
    }

    static void ResetPipeline(const DrawFlags& d)
    {
        if(d & DrawFlags::DepthTest)        Renderer::ResetDrawFlags(DrawFlags::DepthTest);
        if(d & DrawFlags::SkipDepthMask)    Renderer::ResetDrawFlags(DrawFlags::SkipDepthMask);
        if(d & DrawFlags::Wireframe)        Renderer::ResetDrawFlags(DrawFlags::Wireframe);
        if(d & DrawFlags::Blending)         Renderer::ResetDrawFlags(DrawFlags::Blending);
        if(d & DrawFlags::CullFace)         Renderer::ResetDrawFlags(DrawFlags::CullFace);
    }

    static void FlushCommands() noexcept
    {
        if(s_DrawCommands.empty()) return;
        std::sort(s_DrawCommands.begin(), s_DrawCommands.end(), [](const SceneDrawCommand& a, const SceneDrawCommand& b) { return a < b; });

        IShader* currentShader      = nullptr;
        Material* currentMaterial   = nullptr;
        Mesh* currentMesh           = nullptr;
        IEnvironment* currentEnv    = nullptr;

        AssetManager& AM          = AssetManager::GetInstance();
        ShaderVariant& SV         = ShaderVariant::GetInstance();
        IShader* const basePBR    = AM.Get<IShader>("PBR").get();
        static const std::filesystem::path modularPath  = std::filesystem::path("Assets/Shaders/ModularPBR.glsl");

        for(auto& cmd : s_DrawCommands)
        {
            Mesh* nextMesh              = cmd.MeshPointer;
            Material* nextMaterial      = cmd.MaterialPointer;
            IEnvironment* nextEnv       = cmd.EnvironmentPointer;

            if(!nextMesh || !nextMaterial || !nextEnv)
            {
                MOTION_CORE_ERROR("Invalid draw command encountered, skipping.");
                continue;
            }

            IShader* nextShader;
            if(cmd.ShaderMask == GLSL_SHADER_EXT_NONE)
                nextShader = basePBR;
            else
                nextShader = SV.GetVariant(basePBR->GetUUID(), "PBR_MOD", modularPath, cmd.ShaderMask).get();

            if(currentShader != nextShader)
            {
                if(currentShader) currentShader->Unbind();
                currentShader = nextShader;
                nextShader->Bind();
            }

            if(currentEnv != nextEnv)
            {
                currentEnv = nextEnv;
                BindIBL(currentShader, currentEnv);
            }

            currentShader->SetUniform("u_ModelMatrix",          cmd.ModelMatrix);
            currentShader->SetUniform("u_NormalMatrix",         cmd.NormalMatrix);
            currentShader->SetUniform("u_ViewMatrix",           cmd.ViewMatrix);
            currentShader->SetUniform("u_ProjectionMatrix",     cmd.ProjectionMatrix);
            currentShader->SetUniform("u_CameraPosition",       cmd.CameraPosition);

            currentShader->SetUniform("u_SunLight.Direction",   cmd.SunDirection);
            currentShader->SetUniform("u_SunLight.Color",       cmd.SunColor);
            currentShader->SetUniform("u_SunLight.Intensity",   cmd.SunIntensity);

            if (currentMesh != nextMesh)
            {
                if (currentMesh) currentMesh->Unbind();
                currentMesh = nextMesh;
                currentMesh->Bind();
            }

            if (currentMaterial != nextMaterial)
            {
                currentMaterial = nextMaterial;
                int texMask = 0;
                BindMaterialAndTextures(currentShader, currentMaterial, texMask);
                currentShader->SetUniform("u_TextureBitmask", texMask);
            }

            ApplyPipeline(cmd.Flags);

            currentMesh->Render();
            
            ResetPipeline(cmd.Flags);
        }

        if(currentShader) currentShader->Unbind();
        if(currentMesh) currentMesh->Unbind();
        s_DrawCommands.clear();
    }

    void SceneRenderer::BeginScene() noexcept
    {
        s_CurrentScene = nullptr;
        s_DrawCommands.clear();
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
                SceneDrawCommand cmd{};
                cmd.SortKey             = sm.ID;
                cmd.MaterialPointer     = mesh->Materials.get();
                cmd.MeshPointer         = mesh.get();
                cmd.EnvironmentPointer  = env.EnvironmentInstance.get();
                cmd.ShaderMask          = GetShaderMask(mesh->Materials.get());

                cmd.ModelMatrix         = modelMatrix;
                cmd.ViewMatrix          = camera.Camera.GetView();
                cmd.ProjectionMatrix    = camera.Camera.GetProjection();
                cmd.NormalMatrix        = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
                cmd.CameraPosition      = camera.Camera.Position;

                cmd.SunDirection       = env.Sun.Direction;
                cmd.SunColor           = env.Sun.Color;
                cmd.SunIntensity       = env.Sun.Intensity;

                s_DrawCommands.push_back(cmd);
            }
        }
    }

    void SceneRenderer::EndScene() noexcept
    {
        RenderSkyboxPass(s_CurrentScene);
        if(!s_DrawCommands.empty())
            FlushCommands();
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
