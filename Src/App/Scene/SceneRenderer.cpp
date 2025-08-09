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
        // IBL
        static constexpr int Irradiance = 0;
        static constexpr int Prefilter = 1;
        static constexpr int BRDFLUT = 2;

        // Material (start at 8 to leave room for skybox or extras if you want)
        static constexpr int Base = 8; // first material slot
    }

    // Order of material bindings (expandable)
    struct TexBindDesc
    {
        TextureType type;
        const char* uniformName;  // sampler uniform
        const char* hasFlagName;  // bool flag uniform (optional; we also set a bitmask)
        int         slotOffset;   // TEX_SLOTS::Base + offset
        uint32_t    maskBit;      // bit in u_TexMask
    };

    // Bit layout for u_TexMask (match with shader)
    enum TexBits : uint32_t
    {
        TB_BaseColor = 1u << 0,
        TB_Metallic = 1u << 1,
        TB_Roughness = 1u << 2,
        TB_Normal = 1u << 3,
        TB_AO = 1u << 4,
        TB_Emissive = 1u << 5,
        TB_Opacity = 1u << 6,
        TB_ORM = 1u << 7,   // packed Occlusion-Roughness-Metallic (R,G,B)
        TB_Clearcoat = 1u << 8,
        TB_ClearcoatR = 1u << 9,
        TB_SpecColor = 1u << 10,
        TB_Spec = 1u << 11,
        TB_SheenColor = 1u << 12,
        TB_SheenR = 1u << 13,
        TB_Trans = 1u << 14,
        TB_Thick = 1u << 15,
        // add more bits here if you add more types
    };

    // Declare all supported bindings and their slots (contiguous)
    static const TexBindDesc kBinds[] =
    {
        // offset order drives texture unit index = TEX_SLOTS::Base + slotOffset
        { TextureType::BaseColorTexture,        "u_BaseColorTex",       "u_HasBaseColorTex",   0,  TB_BaseColor  },
        { TextureType::MetallicTexture,         "u_MetallicTex",        "u_HasMetallicTex",    1,  TB_Metallic   },
        { TextureType::RoughnessTexture,        "u_RoughnessTex",       "u_HasRoughnessTex",   2,  TB_Roughness  },
        { TextureType::NormalTexture,           "u_NormalTex",          "u_HasNormalTex",      3,  TB_Normal     },
        { TextureType::AmbientOcclusionTexture, "u_AOTex",              "u_HasAOTex",          4,  TB_AO         },
        { TextureType::EmissiveTexture,         "u_EmissiveTex",        "u_HasEmissiveTex",    5,  TB_Emissive   },
        { TextureType::OpacityTexture,          "u_OpacityTex",         "u_HasOpacityTex",     6,  TB_Opacity    },

        // Packed ORM map (R=AO, G=Roughness, B=Metallic)
        { TextureType::ORMTexture,              "u_ORMTex",             "u_HasORMTex",         7,  TB_ORM        },

        // Extended PBR
        { TextureType::ClearcoatTexture,        "u_ClearcoatTex",       "u_HasClearcoatTex",      8,  TB_Clearcoat  },
        { TextureType::ClearcoatRoughnessTexture,"u_ClearcoatRTex",     "u_HasClearcoatRTex",     9,  TB_ClearcoatR },
        { TextureType::SpecularColorTexture,    "u_SpecularColorTex",   "u_HasSpecularColorTex", 10,  TB_SpecColor  },
        { TextureType::SpecularTexture,         "u_SpecularTex",        "u_HasSpecularTex",      11,  TB_Spec       },
        { TextureType::SheenColorTexture,       "u_SheenColorTex",      "u_HasSheenColorTex",    12,  TB_SheenColor },
        { TextureType::SheenRoughnessTexture,   "u_SheenRTex",          "u_HasSheenRTex",        13,  TB_SheenR     },
        { TextureType::TransmissionTexture,     "u_TransmissionTex",    "u_HasTransmissionTex",  14,  TB_Trans      },
        { TextureType::ThicknessTexture,        "u_ThicknessTex",       "u_HasThicknessTex",     15,  TB_Thick      },
    };

    static inline void BindIBL(IShader* shader, IEnvironment* env)
    {
        env->BindIBLAll(TEX_SLOTS::Irradiance, TEX_SLOTS::Prefilter, TEX_SLOTS::BRDFLUT);
        shader->SetUniform("u_IrradianceMap", TEX_SLOTS::Irradiance);
        shader->SetUniform("u_PrefilteredEnvMap", TEX_SLOTS::Prefilter);
        shader->SetUniform("u_BRDFLUT", TEX_SLOTS::BRDFLUT);

        auto I = env->GetIntensity();
        shader->SetUniform("u_IBLIntensity_Diffuse", I.Diffuse);
        shader->SetUniform("u_IBLIntensity_Specular", I.Specular);
    }

    // Resolve a texture from material instance or its base
    static inline std::shared_ptr<ITexture> GetMatTex(PhysicalBasedMaterialInstance* mat, TextureType type)
    {
        if (!mat) return nullptr;
        if (auto it = mat->Texture.find(type); it != mat->Texture.end() && it->second) return it->second;
        if (mat->BaseMaterial)
        {
            if (auto it = mat->BaseMaterial->Texture.find(type); it != mat->BaseMaterial->Texture.end() && it->second)
                return it->second;
        }
        return nullptr;
    }

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

        for (const auto& cmd : s_CommandQueue)
        {
            IShader* const nextShader = PBR;
            Mesh* const nextMesh = cmd.MeshPtr;
            auto* const nextMat = cmd.PBR_MatPtr;
            auto* const nextEnv = cmd.EnvironmentPtr;
            if (!nextShader || !nextMesh || !nextMat || !nextEnv) continue;

            if (currentShader != nextShader) {
                if (currentShader) currentShader->Unbind();
                currentShader = nextShader;
                currentShader->Bind();
            }

            if (currentEnv != nextEnv) {
                currentEnv = nextEnv;
                BindIBL(currentShader, currentEnv);
            }

            if (!haveView || currentViewKey != cmd.SortKey) {
                haveView = true;
                currentViewKey = cmd.SortKey;

                currentShader->SetUniform("u_View", cmd.ViewMatrix);
                currentShader->SetUniform("u_Proj", cmd.ProjectionMatrix);
                currentShader->SetUniform("u_CameraWorldPos", cmd.CameraPosition);

                // Sun
                currentShader->SetUniform("u_Sun.direction", cmd.SunDirection);
                currentShader->SetUniform("u_Sun.color", cmd.SunColor);
                currentShader->SetUniform("u_Sun.intensity", cmd.SunIntensity);
            }

            if (currentMesh != nextMesh) {
                if (currentMesh) currentMesh->Unbind();
                currentMesh = nextMesh;
                currentMesh->Bind();
            }

            if (currentMaterial != nextMat) {
                currentMaterial = nextMat;

                currentShader->SetUniform("u_Material.BaseColor", currentMaterial->Attributes.BaseColor);
                currentShader->SetUniform("u_Material.Metallic", currentMaterial->Attributes.Metallic);
                currentShader->SetUniform("u_Material.Roughness", currentMaterial->Attributes.Roughness);
                currentShader->SetUniform("u_Material.Opacity", currentMaterial->Attributes.Opacity);

                // Bind all material textures present and build the bitmask
                uint32_t texMask = 0u;
                for (const auto& b : kBinds)
                {
                    const int slot = TEX_SLOTS::Base + b.slotOffset;
                    if (auto tex = GetMatTex(currentMaterial, b.type))
                    {
                        tex->Bind(slot);
                        texMask |= b.maskBit;
                        if (b.uniformName) currentShader->SetUniform(b.uniformName, slot);
                        if (b.hasFlagName) currentShader->SetUniform(b.hasFlagName, true);
                    }
                    else
                    {
                        if (b.hasFlagName) currentShader->SetUniform(b.hasFlagName, false);
                    }
                }
                currentShader->SetUniform("u_TexMask", (int)texMask);
            }

            currentShader->SetUniform("u_Model", cmd.ModelMatrix);
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

    void SceneRenderer::EndScene() noexcept
    {
        if (s_CommandQueue.empty())
            return;

        FlushQueue();
        RenderSkyboxPass(s_CurrentScene);
    }

    void SceneRenderer::RenderSkyboxPass(Scene* scene) noexcept
    {
        if (!scene) return;

        auto& env = scene->GetEnvironment();
        IEnvironment* ibl = env.Env ? env.Env.get() : nullptr;
        if (!ibl) return;

        const auto& cam = scene->GetCamera().Camera;
        // Delegate *all state & draw* to the engine-side environment
        ibl->Render(cam.View, cam.Projection);
    }
}
