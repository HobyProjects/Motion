#include "CorePCH.hpp"
#include "SceneRenderer.hpp"
#include "Scene.hpp"

namespace Motion
{
    // Current scene & queue
    Scene* SceneRenderer::s_CurrentScene = nullptr;
    static std::vector<SceneDrawCommand> s_CommandQueue{};

    void SceneRenderer::BeginScene() noexcept
    {
        s_CommandQueue.clear();
        s_CurrentScene = nullptr;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Texture slots
    // ─────────────────────────────────────────────────────────────────────────
    namespace TEX_SLOTS
    {
        // IBL (keep contiguous & consistent with shaders)
        static constexpr int Irradiance = 0;
        static constexpr int Prefilter = 1;
        static constexpr int BRDFLUT = 2;

        // Leave 3..7 free for skybox/utility if needed

        // Material samplers start here
        static constexpr int Base = 8;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Material texture bindings (sampler names & bitmask)
    // ─────────────────────────────────────────────────────────────────────────
    struct TexBindDesc
    {
        TextureType type;
        const char* uniformName;  // sampler uniform in shader
        int         slotOffset;   // TEX_SLOTS::Base + offset
        uint32_t    maskBit;      // bit in u_TexMask
    };

    enum TexBits : uint32_t
    {
        TB_BaseColor = 1u << 0,
        TB_Metallic = 1u << 1,
        TB_Roughness = 1u << 2,
        TB_Normal = 1u << 3,
        TB_AO = 1u << 4,
        TB_Emissive = 1u << 5,
        TB_Opacity = 1u << 6,
        TB_ORM = 1u << 7,   // packed R/G/B = AO/Rough/Metal
        TB_Clearcoat = 1u << 8,
        TB_ClearcoatR = 1u << 9,
        TB_SpecColor = 1u << 10,
        TB_Spec = 1u << 11,
        TB_SheenColor = 1u << 12,
        TB_SheenR = 1u << 13,
        TB_Trans = 1u << 14,
        TB_Thick = 1u << 15,
    };

    static const TexBindDesc kBinds[] =
    {
        { TextureType::BaseColorTexture,        "u_BaseColorTex",       0,  TB_BaseColor  },
        { TextureType::MetallicTexture,         "u_MetallicTex",        1,  TB_Metallic   },
        { TextureType::RoughnessTexture,        "u_RoughnessTex",       2,  TB_Roughness  },
        { TextureType::NormalTexture,           "u_NormalTex",          3,  TB_Normal     },
        { TextureType::AmbientOcclusionTexture, "u_AOTex",              4,  TB_AO         },
        { TextureType::EmissiveTexture,         "u_EmissiveTex",        5,  TB_Emissive   },
        { TextureType::OpacityTexture,          "u_OpacityTex",         6,  TB_Opacity    },

        { TextureType::ORMTexture,              "u_ORMTex",             7,  TB_ORM        },

        { TextureType::ClearcoatTexture,        "u_ClearcoatTex",       8,  TB_Clearcoat  },
        { TextureType::ClearcoatRoughnessTexture,"u_ClearcoatRTex",     9,  TB_ClearcoatR },
        { TextureType::SpecularColorTexture,    "u_SpecularColorTex",   10, TB_SpecColor  },
        { TextureType::SpecularTexture,         "u_SpecularTex",        11, TB_Spec       },
        { TextureType::SheenColorTexture,       "u_SheenColorTex",      12, TB_SheenColor },
        { TextureType::SheenRoughnessTexture,   "u_SheenRTex",          13, TB_SheenR     },
        { TextureType::TransmissionTexture,     "u_TransmissionTex",    14, TB_Trans      },
        { TextureType::ThicknessTexture,        "u_ThicknessTex",       15, TB_Thick      },
    };

    // ─────────────────────────────────────────────────────────────────────────
    // Fallback 2D textures (to avoid sampler type violations)
    // ─────────────────────────────────────────────────────────────────────────
    static std::shared_ptr<ITexture> g_FallbackWhite;   // (1,1,1,1)
    static std::shared_ptr<ITexture> g_FallbackBlack;   // (0,0,0,1)
    static std::shared_ptr<ITexture> g_FallbackGray;    // (0.5,0.5,0.5,1)

    static void EnsureFallbacks()
    {
        if (!g_FallbackWhite) g_FallbackWhite = ITexture::Create(1, 1, { 1.0f, 1.0f, 1.0f });
        if (!g_FallbackBlack) g_FallbackBlack = ITexture::Create(1, 1, { 0.0f, 0.0f, 0.0f });
        if (!g_FallbackGray)  g_FallbackGray = ITexture::Create(1, 1, { 0.5f, 0.5f, 0.5f });
    }

    static std::shared_ptr<ITexture> PickFallback(TextureType t)
    {
        switch (t)
        {
        case TextureType::BaseColorTexture:        return g_FallbackWhite; // albedo 1
        case TextureType::MetallicTexture:         return g_FallbackBlack; // 0 metal
        case TextureType::RoughnessTexture:        return g_FallbackWhite; // 1 rough
        case TextureType::AmbientOcclusionTexture: return g_FallbackWhite; // AO=1
        case TextureType::NormalTexture:           return g_FallbackGray;  // flat normal
        case TextureType::OpacityTexture:          return g_FallbackWhite; // alpha=1
        case TextureType::EmissiveTexture:         return g_FallbackBlack; // 0 emissive
        case TextureType::ORMTexture:              return g_FallbackWhite; // not sampled if mask says absent
        case TextureType::ClearcoatTexture:        return g_FallbackWhite; // clearcoat 1
        case TextureType::ClearcoatRoughnessTexture: return g_FallbackWhite; // clearcoatR 1
        case TextureType::SpecularColorTexture:    return g_FallbackWhite; // specular color 1
        case TextureType::SpecularTexture:         return g_FallbackWhite; // specular 1
        case TextureType::SheenColorTexture:       return g_FallbackWhite; // sheen color 1
        case TextureType::SheenRoughnessTexture:   return g_FallbackWhite; // sheenR 1
        case TextureType::TransmissionTexture:     return g_FallbackWhite; // transmission 1
        case TextureType::ThicknessTexture:        return g_FallbackWhite; // thickness 1
        default:                                   return g_FallbackWhite;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // IBL binding helper
    // ─────────────────────────────────────────────────────────────────────────
    static inline void BindIBL(IShader* shader, IEnvironment* env)
    {
        // Ensure environment binds: irradiance (cube) @0, prefiltered (cube) @1, BRDF LUT (2D) @2
        env->BindIBLAll(TEX_SLOTS::Irradiance, TEX_SLOTS::Prefilter, TEX_SLOTS::BRDFLUT);

        shader->SetUniform("u_IrradianceMap", TEX_SLOTS::Irradiance);
        shader->SetUniform("u_PrefilteredEnvMap", TEX_SLOTS::Prefilter);
        shader->SetUniform("u_BRDFLUT", TEX_SLOTS::BRDFLUT);

        auto I = env->GetIntensity();
        shader->SetUniform("u_IBLIntensity_Diffuse", I.Diffuse);
        shader->SetUniform("u_IBLIntensity_Specular", I.Specular);
    }

    // Effective material texture (instance overrides base)
    static inline std::shared_ptr<ITexture> GetMatTex(PhysicalBasedMaterialInstance* mat, TextureType type)
    {
        if (!mat)
            return nullptr;

        if (mat->Texture.contains(type))
            return mat->Texture.at(type);

        if (mat->BaseMaterial)
        {
            if (mat->BaseMaterial->Texture.contains(type))
                return mat->BaseMaterial->Texture.at(type);
        }

        return nullptr;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Render queue flush
    // ─────────────────────────────────────────────────────────────────────────
    static void FlushQueue() noexcept
    {
        if (s_CommandQueue.empty()) return;

        std::sort(s_CommandQueue.begin(), s_CommandQueue.end());

        AssetManager& assetManager = AssetManager::GetInstance();
        IShader* const PBR = assetManager.Get<IShader>("PBR").get();

        IShader* currentShader = nullptr;
        PhysicalBasedMaterialInstance* currentMaterial = nullptr;
        Mesh* currentMesh = nullptr;
        IEnvironment* currentEnv = nullptr;

        UUID currentViewKey{ 0 };
        bool haveView = false;

        EnsureFallbacks();

        for (const auto& cmd : s_CommandQueue)
        {
            IShader* const nextShader = PBR;
            Mesh* const nextMesh = cmd.MeshPtr;
            auto* const nextMat = cmd.PBR_MatPtr;
            auto* const nextEnv = cmd.EnvironmentPtr;
            if (!nextShader || !nextMesh || !nextMat || !nextEnv) continue;

            // Shader switch
            if (currentShader != nextShader) {
                if (currentShader) currentShader->Unbind();
                currentShader = nextShader;
                currentShader->Bind();
            }

            // Environment / IBL
            if (currentEnv != nextEnv) {
                currentEnv = nextEnv;
                BindIBL(currentShader, currentEnv);
            }

            // Camera & sun per-view
            if (!haveView || currentViewKey != cmd.SortKey) {
                haveView = true;
                currentViewKey = cmd.SortKey;

                currentShader->SetUniform("u_View", cmd.ViewMatrix);
                currentShader->SetUniform("u_Proj", cmd.ProjectionMatrix);
                currentShader->SetUniform("u_CameraWorldPos", cmd.CameraPosition);

                currentShader->SetUniform("u_Sun.direction", cmd.SunDirection);
                currentShader->SetUniform("u_Sun.color", cmd.SunColor);
                currentShader->SetUniform("u_Sun.intensity", cmd.SunIntensity);
            }

            // Mesh switch
            if (currentMesh != nextMesh) {
                if (currentMesh) currentMesh->Unbind();
                currentMesh = nextMesh;
                currentMesh->Bind();
            }

            // Material switch
            if (currentMaterial != nextMat) {
                currentMaterial = nextMat;

                // Base PBR attributes
                currentShader->SetUniform("u_Material_BaseColor", currentMaterial->Attributes.BaseColor);
                currentShader->SetUniform("u_Material_Metallic", currentMaterial->Attributes.Metallic);
                currentShader->SetUniform("u_Material_Roughness", currentMaterial->Attributes.Roughness);
                currentShader->SetUniform("u_Material_Opacity", currentMaterial->Attributes.Opacity);

                // Extended PBR attributes
                const auto& A = currentMaterial->Attributes;
                currentShader->SetUniform("u_Material_ClearcoatFactor", A.ClearcoatFactor);
                currentShader->SetUniform("u_Material_ClearcoatRoughness", A.ClearcoatRoughness);
                currentShader->SetUniform("u_Material_SpecularColor", A.SpecularColor);
                currentShader->SetUniform("u_Material_SpecularLevel", A.SpecularLevel);
                currentShader->SetUniform("u_Material_SheenColor", A.SheenColor);
                currentShader->SetUniform("u_Material_SheenRoughness", A.SheenRoughness);
                currentShader->SetUniform("u_Material_Transmission", A.Transmission);
                currentShader->SetUniform("u_Material_Thickness", A.Thickness);
                currentShader->SetUniform("u_Material_AttenuationColor", A.AttenuationColor);
                currentShader->SetUniform("u_Material_AttenuationDist", A.AttenuationDistance);
                currentShader->SetUniform("u_Material_IOR", A.IOR);

                // Bind all material samplers to fixed slots, real or fallback; build mask
                uint32_t texMask = 0u;
                for (const auto& b : kBinds)
                {
                    const int slot = TEX_SLOTS::Base + b.slotOffset;

                    // ALWAYS set the sampler uniform to its intended slot
                    if (b.uniformName)
                        currentShader->SetUniform(b.uniformName, slot);

                    // Instance overrides base
                    std::shared_ptr<ITexture> tex = GetMatTex(currentMaterial, b.type);
                    if (tex)
                    {
                        tex->Bind(slot);
                        texMask |= b.maskBit;
                    }
                    else
                    {
                        // Bind safe 2D fallback to avoid sampler/cubemap collisions
                        PickFallback(b.type)->Bind(slot);
                    }
                }
                currentShader->SetUniform("u_TexMask", (int)texMask);

                // Per-texture normal Y flip from the active normal map
                float normalYFlip = 0.0f;
                if (auto nrm = GetMatTex(currentMaterial, TextureType::NormalTexture))
                    normalYFlip = nrm->GetSpecification().InvertGreen ? 1.0f : 0.0f;
                currentShader->SetUniform("u_NormalYFlip", normalYFlip);
            }

            // Per-draw transforms
            currentShader->SetUniform("u_Model", cmd.ModelMatrix);
            currentShader->SetUniform("u_NormalMatrix", cmd.NormalMatrix);

            currentMesh->Render();
        }

        if (currentMesh)   currentMesh->Unbind();
        if (currentShader) currentShader->Unbind();
        s_CommandQueue.clear();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Submit all drawables from the scene
    // ─────────────────────────────────────────────────────────────────────────
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
                cmd.PBR_MatPtr = mesh->PhysicalBasedMaterials.get();
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

    // ─────────────────────────────────────────────────────────────────────────
    // End scene: skybox then opaque queue
    // ─────────────────────────────────────────────────────────────────────────
    void SceneRenderer::EndScene() noexcept
    {
        RenderSkyboxPass(s_CurrentScene);
        if (!s_CommandQueue.empty())
            FlushQueue();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Skybox (delegates to engine environment; handles depth state internally)
    // ─────────────────────────────────────────────────────────────────────────
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
