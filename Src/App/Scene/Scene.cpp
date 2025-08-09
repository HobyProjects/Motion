#include "CorePCH.hpp"
#include "Scene.hpp"

namespace Motion
{
    //-------------------------------------------------------------------
    // HELPER FUNCTIONS 
    //-------------------------------------------------------------------

    static std::shared_ptr<ITexture> LoadTexture(TextureType type)
    {
        std::filesystem::path file = DialogBoxes::OpenFileDialog();
        if (!file.empty())
        {
            auto newTex = ITexture::Create(file, type, true);
            if (newTex)
            {
                return newTex; // Return the loaded texture
            }
            else
            {
                MOTION_ERROR("Failed to load texture from file: {0}", file.string());
                return nullptr;
            }
        }

        MOTION_ERROR("No file selected for texture loading");
        return nullptr; // Return null if no file was selected
    }

    template<typename T, typename UIFunc>
    static void DrawComponentControls(const std::string& name, const std::shared_ptr<Entity>& entity, UIFunc uiFunc, bool enabled = true)
    {
        static const ImGuiTreeNodeFlags treeNodeFlags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        if (entity->HasComponent<T>())
        {
            auto& component = entity->GetComponent<T>();
            if (ImGui::TreeNodeEx((void*)component.ID, treeNodeFlags, name.c_str()))
            {
                if (!enabled) ImGui::BeginDisabled();

                uiFunc(component);

                if (!enabled) ImGui::EndDisabled();
                ImGui::TreePop();
            }
        }
    }

    namespace MaterialUI
    {
        // Fallback pretty-name helper if you don't already have one
        inline const char* FallbackTexTypeName(TextureType t)
        {
            switch (t)
            {
            case TextureType::BaseColorTexture:         return "Base Color";
            case TextureType::MetallicTexture:          return "Metallic";
            case TextureType::RoughnessTexture:         return "Roughness";
            case TextureType::AmbientOcclusionTexture:  return "Ambient Occlusion";
            case TextureType::NormalTexture:            return "Normal";
            case TextureType::DisplacementTexture:      return "Displacement";
            case TextureType::EmissiveTexture:          return "Emissive";
            case TextureType::OpacityTexture:           return "Opacity";
            case TextureType::ORMTexture:               return "ORM (AO/R/M)";
            case TextureType::ClearcoatTexture:         return "Clearcoat";
            case TextureType::ClearcoatRoughnessTexture:return "Clearcoat Roughness";
            case TextureType::SpecularTexture:          return "Specular";
            case TextureType::SpecularColorTexture:     return "Specular Color";
            case TextureType::SheenColorTexture:        return "Sheen Color";
            case TextureType::SheenRoughnessTexture:    return "Sheen Roughness";
            case TextureType::TransmissionTexture:      return "Transmission";
            case TextureType::ThicknessTexture:         return "Thickness";
            default:                                    return "Unknown";
            }
        }

        inline std::string GetPrettyName(TextureType t)
        {
            // If you already have GetTextureTypeString(t), use that instead:
            // return GetTextureTypeString(t);
            return FallbackTexTypeName(t);
        }

        struct TexSlotDesc {
            TextureType Type;
            const char* Pretty;
            const char* Icon;
        };

        // The “core” PBR slots you want to always show first (add/remove as you like)
        static inline const std::array<TexSlotDesc, 8> kDefaultPBRSlots = { {
            { TextureType::BaseColorTexture,        "Base Color",        ICON_MD_IMAGE      },
            { TextureType::ORMTexture,              "ORM (AO/R/M)",      ICON_MD_AUTO_AWESOME },
            { TextureType::MetallicTexture,         "Metallic",          ICON_MD_TONALITY   },
            { TextureType::RoughnessTexture,        "Roughness",         ICON_MD_GRAIN      },
            { TextureType::AmbientOcclusionTexture, "Ambient Occlusion", ICON_MD_BLUR_ON    },
            { TextureType::NormalTexture,           "Normal",            ICON_MD_GESTURE    },
            { TextureType::EmissiveTexture,         "Emissive",          ICON_MD_BRIGHTNESS_7 },
            { TextureType::OpacityTexture,          "Opacity",           ICON_MD_OPACITY    },
        } };

        // Draw one texture slot (card + “Pick” button that uses your existing loader)
        inline void DrawTexSlot(std::unordered_map<TextureType, std::shared_ptr<ITexture>>& map,
            TextureType t, const char* label, const char* icon)
        {
            std::shared_ptr<ITexture>& current = map[t]; // ensures key exists
            CustomUIControl::TextureSlotCard(
                (std::string(icon) + "  " + label).c_str(),
                current,
                [&]()
                {
                    if (auto newTex = LoadTexture(t)) // your existing helper
                        map[t] = std::move(newTex);
                }
            );
        }

        // Unified PBR instance inspector (attributes + textures + base material readback)
        inline void DrawPBRInstance(PhysicalBasedMaterialInstance& mat)
        {
            const ImGuiTreeNodeFlags secFlags =
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_AllowItemOverlap |
                ImGuiTreeNodeFlags_FramePadding;

            // ── Attributes
            if (ImGui::CollapsingHeader(std::string(ICON_MD_TUNE "  Material Attributes").c_str(), secFlags))
            {
                CustomUIControl::ColorEdit3(std::string(ICON_MD_PALETTE "  Base Color").c_str(), mat.Attributes.BaseColor);
                CustomUIControl::DrawFloat(std::string(ICON_MD_TONALITY "  Metallic").c_str(), mat.Attributes.Metallic, 0.0f, 1.0f, 0.005f);
                CustomUIControl::DrawFloat(std::string(ICON_MD_GRAIN "  Roughness").c_str(), mat.Attributes.Roughness, 0.0f, 1.0f, 0.005f);
                CustomUIControl::DrawFloat(std::string(ICON_MD_OPACITY "  Opacity").c_str(), mat.Attributes.Opacity, 0.0f, 1.0f, 0.005f);
            }

            // ── Textures
            if (ImGui::CollapsingHeader(std::string(ICON_MD_IMAGE "  Material Textures").c_str(), secFlags))
            {
                // Standard slots first (predictable order)
                for (const auto& slot : kDefaultPBRSlots)
                    DrawTexSlot(mat.Texture, slot.Type, slot.Pretty, slot.Icon);

                // Extended / unknown slots next (auto-list whatever importer provided)
                for (auto& [t, tex] : mat.Texture)
                {
                    bool alreadyStandard = std::any_of(
                        kDefaultPBRSlots.begin(), kDefaultPBRSlots.end(),
                        [&](const TexSlotDesc& s) { return s.Type == t; });
                    if (alreadyStandard) continue;

                    std::string dynLabel = GetPrettyName(t);
                    DrawTexSlot(mat.Texture, t, dynLabel.c_str(), ICON_MD_BROKEN_IMAGE);
                }
            }

            // ── Base Material (read-only mirror)
            if (mat.BaseMaterial)
            {
                if (ImGui::CollapsingHeader(std::string(ICON_MD_TUNE "  Base Material Attributes").c_str(), secFlags))
                {
                    ImGui::BeginDisabled();
                    CustomUIControl::ColorEdit3(std::string(ICON_MD_PALETTE "  Base Color").c_str(), mat.BaseMaterial->Attributes.BaseColor);
                    CustomUIControl::DrawFloat(std::string(ICON_MD_TONALITY "  Metallic").c_str(), mat.BaseMaterial->Attributes.Metallic, 0.0f, 1.0f, 0.005f);
                    CustomUIControl::DrawFloat(std::string(ICON_MD_GRAIN "  Roughness").c_str(), mat.BaseMaterial->Attributes.Roughness, 0.0f, 1.0f, 0.005f);
                    CustomUIControl::DrawFloat(std::string(ICON_MD_OPACITY "  Opacity").c_str(), mat.BaseMaterial->Attributes.Opacity, 0.0f, 1.0f, 0.005f);
                    ImGui::EndDisabled();
                }
                if (ImGui::CollapsingHeader(std::string(ICON_MD_COLLECTIONS "  Base Material Textures").c_str(), secFlags))
                {
                    ImGui::BeginDisabled();
                    for (auto& [type, tex] : mat.BaseMaterial->Texture)
                    {
                        std::string title = std::string(ICON_MD_IMAGE) + "  " + GetPrettyName(type);
                        CustomUIControl::TextureSlotCard(title.c_str(), tex);
                    }
                    ImGui::EndDisabled();
                }
            }
        }

        // Batch assignment for an entire StaticMesh (applies to every Mesh in the model)
        inline void DrawBatchAssignment(StaticMesh& model)
        {
            const ImGuiTreeNodeFlags secFlags =
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_AllowItemOverlap |
                ImGuiTreeNodeFlags_FramePadding;

            if (ImGui::CollapsingHeader(std::string(ICON_MD_BRUSH "  Material Batch Assignment").c_str(), secFlags))
            {
                static PhysicalBasedMaterialAttribute batchAttr{};
                static std::unordered_map<TextureType, std::shared_ptr<ITexture>> batchTex{};

                CustomUIControl::ColorEdit3("Base Color", batchAttr.BaseColor);
                CustomUIControl::DrawFloat("Metallic", batchAttr.Metallic, 0.0f, 1.0f, 0.005f);
                CustomUIControl::DrawFloat("Roughness", batchAttr.Roughness, 0.0f, 1.0f, 0.005f);
                CustomUIControl::DrawFloat("Opacity", batchAttr.Opacity, 0.0f, 1.0f, 0.005f);

                ImGui::Separator();

                for (const auto& slot : kDefaultPBRSlots)
                    DrawTexSlot(batchTex, slot.Type, slot.Pretty, slot.Icon);

                ImGui::Separator();

                if (ImGui::Button(std::string(ICON_MD_DONE "  Apply to all meshes").c_str()))
                {
                    for (auto& mesh : model)
                    {
                        if (!mesh->PhysicalBasedMaterials) continue;
                        auto& inst = *mesh->PhysicalBasedMaterials;
                        inst.Attributes = batchAttr;
                        for (auto& [t, tex] : batchTex)
                            inst.Texture[t] = tex;
                    }
                }
            }
        }
    } // namespace MaterialUI


        //-------------------------------------------------------------------



    Scene::Scene(const SceneSpecification& spec)
    {
        m_Specification = spec;
        m_Camera = SceneCamera(spec.Viewport.Size.x, spec.Viewport.Size.y, false);
    }

    void Scene::OnUpdate(WindowHandle handle, Timer deltaTime) noexcept
    {
        m_Camera.OnUpdate(handle, deltaTime);
    }

    void Scene::OnEvent(WindowHandle handle, IEvent& e) noexcept
    {
        m_Camera.OnEvents(handle, e);
    }

    void Scene::OnViewportSizeChanges(const glm::vec2& size) noexcept
    {
        m_Camera.SetAspectRatio(size.x, size.y);
    }

    void Scene::SelectEntityIf()
    {
        if (m_SelectedEntity != EntityFactory::EMPTYENTITY && !m_Entities.empty())
            m_SelectedEntity = m_Entities.front();
        else
            m_SelectedEntity = EntityFactory::EMPTYENTITY;
    }

    std::shared_ptr<Entity> Scene::PickEntity(const glm::vec2& mousePos, const glm::vec2& viewportSize)
    {
        const glm::mat4& projection = m_Camera.Camera.Projection;
        const glm::mat4& view = m_Camera.Camera.View;

        // 1) Screen -> NDC
        float x = (2.0f * mousePos.x) / viewportSize.x - 1.0f;
        float y = 1.0f - (2.0f * mousePos.y) / viewportSize.y; // GL Y is inverted

        // 2) NDC -> world ray
        glm::vec4 rayStartNDC(x, y, -1.0f, 1.0f);
        glm::vec4 rayEndNDC(x, y, 1.0f, 1.0f);

        glm::mat4 invVP = glm::inverse(projection * view);
        glm::vec4 rayStartWorld = invVP * rayStartNDC; rayStartWorld /= rayStartWorld.w;
        glm::vec4 rayEndWorld = invVP * rayEndNDC;   rayEndWorld /= rayEndWorld.w;

        glm::vec3 rayOrigin = glm::vec3(rayStartWorld);
        glm::vec3 rayDir = glm::normalize(glm::vec3(rayEndWorld - rayStartWorld));

        // 3) Find closest entity hit by ray
        float closestT = std::numeric_limits<float>::infinity();
        std::shared_ptr<Entity> pickedEntity = nullptr;

        for (const auto& entity : m_Entities)
        {
            if (!entity->HasComponent<StaticMeshComponent>() || !entity->HasComponent<TransformComponent>())
                continue;

            auto& meshComp = entity->GetComponent<StaticMeshComponent>();
            auto& transComp = entity->GetComponent<TransformComponent>();
            if (!meshComp.Model) continue;

            // Local-space AABB from model
            const glm::vec3 localMin = meshComp.Model->GetMinBounds();
            const glm::vec3 localMax = meshComp.Model->GetMaxBounds();

            // Build world-space AABB by transforming all 8 corners
            const glm::mat4 model = transComp.GetTransform();

            const glm::vec3 corners[8] = {
                {localMin.x, localMin.y, localMin.z},
                {localMax.x, localMin.y, localMin.z},
                {localMin.x, localMax.y, localMin.z},
                {localMax.x, localMax.y, localMin.z},
                {localMin.x, localMin.y, localMax.z},
                {localMax.x, localMin.y, localMax.z},
                {localMin.x, localMax.y, localMax.z},
                {localMax.x, localMax.y, localMax.z}
            };

            glm::vec3 worldMin(std::numeric_limits<float>::max());
            glm::vec3 worldMax(-std::numeric_limits<float>::max());
            for (int i = 0; i < 8; ++i)
            {
                glm::vec3 w = glm::vec3(model * glm::vec4(corners[i], 1.0f));
                worldMin = glm::min(worldMin, w);
                worldMax = glm::max(worldMax, w);
            }

            float tmin, tmax;
            if (RayIntersectsAABB(rayOrigin, rayDir, worldMin, worldMax, tmin, tmax))
            {
                float hitDist = (tmin > 0.0f) ? tmin : tmax; // prefer the front hit
                if (hitDist > 0.0f && hitDist < closestT)
                {
                    closestT = hitDist;
                    pickedEntity = entity;
                }
            }
        }

        return pickedEntity;
    }

    void Scene::RenderEnvironment() const
    {
        m_Specification.Environment.Env->Render(m_Camera.Camera.View, m_Camera.Camera.Projection);
    }

    static void DrawViewportAxisWidget(const glm::mat4& view, int corner = 2, float baseSize = 64.0f, ImVec2 basePadding = ImVec2(12, 12))
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList(); // above content
        ImGuiIO& io = ImGui::GetIO();

        // Scale for HiDPI
        const float scale = io.FontGlobalScale > 0.0f ? io.FontGlobalScale : 1.0f;
        const float size = baseSize * scale;
        const ImVec2 pad = ImVec2(basePadding.x * scale, basePadding.y * scale);
        const float thick = 3.0f * scale;
        const float ahLen = 8.0f * scale;   // arrowhead length
        const float ahHalf = 4.0f * scale;   // arrowhead half-width
        const float radius = 7.0f * scale;   // origin dot
        const float cardR = 8.0f * scale;   // card rounding
        const float cardPad = 6.0f * scale;

        // Anchor inside current window's content rect
        const ImVec2 winPos = ImGui::GetWindowPos();
        const ImVec2 crMin = ImGui::GetWindowContentRegionMin();
        const ImVec2 crMax = ImGui::GetWindowContentRegionMax();
        ImRect content(ImVec2(winPos.x + crMin.x, winPos.y + crMin.y), ImVec2(winPos.x + crMax.x, winPos.y + crMax.y));

        // Card rectangle
        ImVec2 cardSize(size + cardPad * 2, size + cardPad * 2);
        ImVec2 cardMin, cardMax;
        switch (corner)
        {
        case 0:     cardMin = ImVec2(content.Min.x + pad.x, content.Min.y + pad.y); break; // Top-left
        case 1:     cardMin = ImVec2(content.Max.x - pad.x - cardSize.x, content.Min.y + pad.y); break; // Top-right
        case 2:     cardMin = ImVec2(content.Min.x + pad.x, content.Max.y - pad.y - cardSize.y); break; // Bottom-left
        default:    cardMin = ImVec2(content.Max.x - pad.x - cardSize.x, content.Max.y - pad.y - cardSize.y); break; // Bottom-right
        }
        cardMax = ImVec2(cardMin.x + cardSize.x, cardMin.y + cardSize.y);

        // Background "card" with subtle shadow
        dl->AddRectFilled(ImVec2(cardMin.x, cardMin.y + 2 * scale), ImVec2(cardMax.x, cardMax.y + 2 * scale), IM_COL32(0, 0, 0, 40), cardR);
        dl->AddRectFilled(cardMin, cardMax, IM_COL32(28, 28, 32, 180), cardR);
        dl->AddRect(cardMin, cardMax, IM_COL32(255, 255, 255, 20), cardR);

        // Axis origin
        ImVec2 origin = ImVec2(cardMin.x + cardPad + size * 0.5f, cardMin.y + cardPad + size * 0.5f);

        // Extract camera rotation (upper-left 3x3 of inverse(view))
        // view = R^T * T^-1 for right-handed OpenGL-like conventions.
        glm::mat3 R = glm::mat3(glm::transpose(view)); // matches your original approach (camera basis rows)

        struct Axis { glm::vec3 dir; ImU32 col; const char* lbl; };
        Axis axes[] = {
            { {1,0,0}, IM_COL32(220, 70, 70, 255), "X" },
            { {0,1,0}, IM_COL32(70,220, 70, 255), "Y" },
            { {0,0,1}, IM_COL32(90,150,255,255), "Z" },
        };

        auto draw_axis = [&](const Axis& a)
            {
                // Local axis in camera space (so "toward screen" fades)
                glm::vec3 v = glm::normalize(R * a.dir);

                // Map to 2D inside the square: X to +x, Y to -y to match screen down
                ImVec2 tip = ImVec2(origin.x + v.x * (size * 0.45f),
                    origin.y - v.y * (size * 0.45f));

                // Fade based on Z (positive Z away from camera in view space)—tweak to taste
                float z = v.z; // if axis points out of screen (z<0), brighten, else dim
                float alpha = (z < 0.0f) ? 1.00f : 0.40f;
                ImU32 lineCol = IM_COL32(
                    (int)((a.col >> 0) & 0xFF),
                    (int)((a.col >> 8) & 0xFF),
                    (int)((a.col >> 16) & 0xFF),
                    (int)(255 * alpha)
                );

                // Line
                dl->AddLine(origin, tip, lineCol, thick);

                // Arrowhead (simple isosceles)
                glm::vec2 d = glm::normalize(glm::vec2(tip.x - origin.x, tip.y - origin.y));
                glm::vec2 n = glm::vec2(-d.y, d.x);
                ImVec2 a0 = ImVec2(tip.x - d.x * ahLen + n.x * ahHalf, tip.y - d.y * ahLen + n.y * ahHalf);
                ImVec2 a1 = ImVec2(tip.x - d.x * ahLen - n.x * ahHalf, tip.y - d.y * ahLen - n.y * ahHalf);
                dl->AddTriangleFilled(tip, a0, a1, lineCol);

                // Label near the tip
                ImVec2 labelPos = ImVec2(tip.x + 6.0f * scale, tip.y - 6.0f * scale);
                dl->AddText(labelPos, lineCol, a.lbl);
            };

        for (const Axis& a : axes)
            draw_axis(a);

        // Origin dot
        dl->AddCircleFilled(origin, radius, IM_COL32(180, 180, 190, 220));
        dl->AddCircle(origin, radius, IM_COL32(255, 255, 255, 40), 0, 1.5f * scale);
    }

    void SceneViewportPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
        ImGui::Begin(std::format("{}##SceneViewport", context.ActiveScene->GetName()).c_str());
        context.UILayerInstance->AcceptEvents(ImGui::IsWindowFocused() || ImGui::IsWindowHovered());

        // keep framebuffer size in lockstep with the ImGui panel
        ImVec2 vp = ImGui::GetContentRegionAvail();
        if (vp.x != context.ActiveSceneSpecification.Viewport.Size.x || vp.y != context.ActiveSceneSpecification.Viewport.Size.y)
        {
            context.ActiveSceneSpecification.Viewport.Size = { vp.x, vp.y };
        }

        // draw scene texture
        ImGui::Image((ImTextureID)context.ActiveViewportTexture, vp, { 0,1 }, { 1,0 });

        // mouse-pick to select an entity (ignores when gizmo is being used)
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 crMin = ImGui::GetWindowContentRegionMin();
        ImVec2 crMax = ImGui::GetWindowContentRegionMax();
        ImVec2 vpMin = { winPos.x + crMin.x, winPos.y + crMin.y };
        ImVec2 vpMax = { winPos.x + crMax.x, winPos.y + crMax.y };
        ImVec2 mouse = ImGui::GetMousePos();

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
            ImGui::IsWindowFocused() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGuizmo::IsUsing())
        {
            // mouse in panel-local space (origin = content top-left)
            glm::vec2 local = { mouse.x - vpMin.x, mouse.y - vpMin.y };
            local.y = vp.y - local.y; // flip Y for GL

            // scale to framebuffer space (critical when FB != panel size)
            glm::vec2 fbSize = {
                (float)context.ActiveSceneSpecification.Viewport.FrameSpec.Width,
                (float)context.ActiveSceneSpecification.Viewport.FrameSpec.Height
            };
            glm::vec2 mouseInFB = {
                local.x * (fbSize.x / vp.x),
                local.y * (fbSize.y / vp.y)
            };

            if (auto picked = context.ActiveScene->PickEntity(mouseInFB, fbSize))
                context.ActiveScene->SelectedEntity(picked);
        }

        // gizmo: translate/rotate/scale with Ctrl+E to cycle
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(vpMin.x, vpMin.y, vp.x, vp.y);
        glm::mat4 view = context.ActiveScene->GetCameraView();
        glm::mat4 proj = context.ActiveScene->GetCameraProjection();

        static ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_E))
        {
            op = (op == ImGuizmo::TRANSLATE) ? ImGuizmo::ROTATE :
                (op == ImGuizmo::ROTATE) ? ImGuizmo::SCALE :
                ImGuizmo::TRANSLATE;
        }

        if (auto sel = context.ActiveScene->GetSelectedEntity();sel && sel != EntityFactory::EMPTYENTITY && sel->HasComponent<TransformComponent>())
        {
            auto& tc = sel->GetComponent<TransformComponent>();
            glm::mat4 model = tc.GetTransform();
            float m[16]; memcpy(m, glm::value_ptr(model), sizeof(m));
            if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), op, ImGuizmo::LOCAL, m))
            {
                glm::vec3 t, s; glm::quat r;
                glm::vec3 euler = glm::degrees(glm::eulerAngles(r));
                ImGuizmo::DecomposeMatrixToComponents(m, &t.x, &euler.x, &s.x);
                r = glm::quat(glm::radians(euler));
                tc.Translation = t; tc.Rotation = r; tc.Scale = s;
            }
        }

        // tiny axis card & view manipulator
        // (same DrawViewportAxisWidget(view) you’ve seen, plus optional ImGuizmo::ViewManipulate block)

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void SceneEntityInspectPanel::RenderUI(ScenePanelContext& ctx)
    {
        ImGui::Begin("Scene Entities");

        // ─────────────────────────────────────────────────────────────
        // Window context menu: quick import model -> entity
        // ─────────────────────────────────────────────────────────────
        if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem(ICON_MD_FILE_UPLOAD "  Import StaticMesh"))
            {
                if (auto path = DialogBoxes::OpenFileDialog(); !path.empty())
                {
                    if (auto mesh = Importer::ImportModel(path))
                    {
                        auto& fac = EntityFactory::GetInstance();
                        auto e = fac.CreateEntity(mesh->GetName());
                        e->AddComponent<StaticMeshComponent>(mesh->GetName(), mesh);
                        e->AddComponent<TransformComponent>();
                        ctx.ActiveScene->EmplaceEntity(e);
                    }
                }
            }
            ImGui::EndPopup();
        }

        // Click on empty space to clear selection
        if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
            ctx.ActiveScene->SelectedEntity(EntityFactory::EMPTYENTITY);

        // ─────────────────────────────────────────────────────────────
        // Entity tree
        // ─────────────────────────────────────────────────────────────
        for (auto& ent : *ctx.ActiveScene)
        {
            auto& tag = ent->GetComponent<TagComponent>();

            // Pick an icon based on components (extend if you like)
            const char* iconEntity = ICON_MD_LABEL_OUTLINE;           // default
            if (ent->HasComponent<StaticMeshComponent>()) iconEntity = ICON_MD_VIEW_IN_AR;
            // You can add: if (ent->HasComponent<LightComponent>()) iconEntity = ICON_MD_LIGHTBULB; etc.

            // Tree row flags
            ImGuiTreeNodeFlags flags =
                ((ctx.ActiveScene->GetSelectedEntity() == ent) ? ImGuiTreeNodeFlags_Selected : 0) |
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                ImGuiTreeNodeFlags_SpanFullWidth |
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_FramePadding;

            ImGui::PushID((void*)ent.get());

            // Label with icon + name
            std::string label = std::format("{}  {}", iconEntity, tag.Tag);

            bool open = ImGui::TreeNodeEx("##node", flags, "%s", label.c_str());

            // Click to select (both LMB and RMB to keep parity with your original)
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
                ctx.ActiveScene->SelectedEntity(ent);

            if (open)
            {
                // ── Static Mesh Section(s)
                if (ent->HasComponent<StaticMeshComponent>())
                {
                    auto& model = ent->GetComponent<StaticMeshComponent>().Model;
                    if (model)
                    {
                        ImGuiTreeNodeFlags secFlags =
                            ImGuiTreeNodeFlags_SpanAvailWidth |
                            ImGuiTreeNodeFlags_Framed |
                            ImGuiTreeNodeFlags_FramePadding;

                        // Mesh Details
                        if (ImGui::CollapsingHeader(std::string(ICON_MD_INFO "  Mesh Details").c_str(), secFlags))
                        {
                            std::string meshCount = std::to_string(model->GetMeshesCount());
                            std::string minBounds = glm::to_string(model->GetMinBounds());
                            std::string maxBounds = glm::to_string(model->GetMaxBounds());
                            std::string filePath = model->GetSource();

                            CustomUIControl::TextBox("Mesh Count", meshCount, true);
                            CustomUIControl::TextBox("Min Bounds", minBounds, true);
                            CustomUIControl::TextBox("Max Bounds", maxBounds, true);
                            CustomUIControl::TextBox("File Path", filePath, true);
                        }

                        // Batch assign PBR material
                        if (ImGui::CollapsingHeader(std::string(ICON_MD_BRUSH "  Material Batch Assignment").c_str(), secFlags))
                        {
                            MaterialUI::DrawBatchAssignment(*model);
                        }
                    }
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::End();
    }

    void SceneEntityPropertiesPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::Begin("Properties");

        // Safer selected-entity resolution
        std::shared_ptr<Entity> selectedEntity = context.ActiveScene ? context.ActiveScene->GetSelectedEntity() : nullptr;
        const bool hasSelection = selectedEntity && selectedEntity != EntityFactory::EMPTYENTITY;

        if (!hasSelection)
        {
            ImGui::TextDisabled("%s  No entity selected", ICON_MD_INFO);
            ImGui::End();
            return;
        }

        // ─────────────────────────────────────────────────────────────
        // Tag / Name
        // ─────────────────────────────────────────────────────────────
        if (selectedEntity->HasComponent<TagComponent>())
        {
            auto& tag = selectedEntity->GetComponent<TagComponent>();
            // Label with an icon
            CustomUIControl::TextBox(std::string(ICON_MD_LABEL "  Tag").c_str(), tag.Tag, false, 256, 150.0f);
        }

        // ─────────────────────────────────────────────────────────────
        // Transform (icon on the header)
        // ─────────────────────────────────────────────────────────────
        DrawComponentControls<TransformComponent>(std::string(ICON_MD_OPEN_WITH "  Transform").c_str(), selectedEntity,
            [](TransformComponent& component)
            {
                CustomUIControl::DrawFloat3(std::string(ICON_MD_NEAR_ME "  Translation").c_str(), component.Translation, 0.0f);
                CustomUIControl::DrawQuatEuler(std::string(ICON_MD_ROTATE_90_DEGREES_CW "  Rotation").c_str(), component.Rotation, 0.0f);
                CustomUIControl::DrawFloat3(std::string(ICON_MD_ZOOM_OUT_MAP "  Scale").c_str(), component.Scale, 10.0f);
            }
        );

        // ─────────────────────────────────────────────────────────────
        // Static Mesh / Materials (icon on the header)
        // ─────────────────────────────────────────────────────────────
        DrawComponentControls<StaticMeshComponent>(std::string(ICON_MD_VIEW_IN_AR "  Materials").c_str(), selectedEntity,
            [](StaticMeshComponent& component)
            {
                std::shared_ptr<StaticMesh>& model = component.Model;
                if (!model) return;

                const ImGuiTreeNodeFlags secFlags =
                    ImGuiTreeNodeFlags_Framed |
                    ImGuiTreeNodeFlags_SpanAvailWidth |
                    ImGuiTreeNodeFlags_AllowItemOverlap |
                    ImGuiTreeNodeFlags_FramePadding;

                // Optional: batch apply a look to the whole model
                MaterialUI::DrawBatchAssignment(*model);

                // One collapsible per Mesh
                for (std::shared_ptr<Mesh>& mesh : *model)
                {
                    std::string hdr = std::format("{}  Mesh: {}  [{}]", ICON_MD_DNS, mesh->Name, mesh->Index);
                    if (ImGui::TreeNodeEx(hdr.c_str(), secFlags))
                    {
                        if (model->ModelShadingMethod == ShadingMethod::PhysicalBased && mesh->PhysicalBasedMaterials)
                        {
                            MaterialUI::DrawPBRInstance(*mesh->PhysicalBasedMaterials);
                        }
                        ImGui::TreePop();
                    }
                }
            }
        );

        ImGui::End();
    }


    void SceneSettingsPanel::RenderUI(ScenePanelContext& ctx)
    {
        ImGui::Begin("Scene Settings");

        if (!ctx.ActiveScene)
        {
            ImGui::TextDisabled("%s  No active scene", ICON_MD_INFO);
            ImGui::End();
            return;
        }

        auto& env = ctx.ActiveScene->GetEnvironment();          // SceneEnvironment
        IEnvironment* ibl = env.Env ? env.Env.get() : nullptr;  // IBL backend (may be null)

        // ───────────────────────────────── Environment Lighting (Sun) ─────────────────────────────────
        if (ImGui::TreeNodeEx((void*)1,
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_AllowItemOverlap |
            ImGuiTreeNodeFlags_FramePadding,
            "%s  %s", ICON_MD_WB_SUNNY, "Environment Lighting"))
        {
            const ImGuiTreeNodeFlags secFlags =
                ImGuiTreeNodeFlags_FramePadding |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_Framed;

            if (ImGui::CollapsingHeader(std::string(ICON_MD_LIGHTBULB "  Sun").c_str(), secFlags))
            {
                // Sun direction (unit vector recommended)
                CustomUIControl::DrawFloat3(std::string(ICON_MD_NEAR_ME "  Direction").c_str(),
                    env.Sun.Direction, 0.0f);
                // Normalize to avoid surprises
                if (glm::length2(env.Sun.Direction) > 0.0f)
                    env.Sun.Direction = glm::normalize(env.Sun.Direction);

                // Sun color
                CustomUIControl::ColorEdit3(std::string(ICON_MD_PALETTE "  Color").c_str(),
                    env.Sun.Color);

                // Sun intensity
                CustomUIControl::DrawFloat(std::string(ICON_MD_TUNGSTEN "  Intensity").c_str(),
                    env.Sun.Intensity, 0.0f, 50.0f, 0.01f);
            }

            ImGui::TreePop();
        }

        // ───────────────────────────────── Image-Based Lighting (IBL) ────────────────────────────────
        if (ImGui::TreeNodeEx((void*)2,
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_AllowItemOverlap |
            ImGuiTreeNodeFlags_FramePadding,
            "%s  %s", ICON_MD_HDR_ENHANCED_SELECT, "Image-Based Lighting"))
        {
            if (!ibl)
            {
                ImGui::TextDisabled("%s  No environment loaded (HDR).", ICON_MD_INFO);
                if (ImGui::Button(ICON_MD_ADD_PHOTO_ALTERNATE "  Load HDR…"))
                {
                    std::string path = Motion::DialogBoxes::OpenFileDialog();
                    if (!path.empty())
                    {
                        // Keep current IBL intensity and rotation if any
                        Motion::EnvIntensity keep = ibl ? ibl->GetIntensity() : Motion::EnvIntensity{ 1.0f, 1.0f };
                        float rotY = ibl ? ibl->GetSkyboxRotationY() : 0.0f;

                        auto newEnv = Motion::IEnvironment::Create(path);
                        if (newEnv)
                        {
                            newEnv->SetIntensity(keep);
                            newEnv->SetSkyboxRotationY(rotY);
                            env.Env = std::move(newEnv);
                        }
                        else
                        {
                            MOTION_CORE_ERROR("Failed to create environment from: {}", path);
                        }
                    }
                }
            }
            else
            {
                auto intens = ibl->GetIntensity();

                // Diffuse & Specular IBL intensity
                CustomUIControl::DrawFloat(std::string(ICON_MD_TUNE "  Diffuse Intensity").c_str(),
                    intens.Diffuse, 0.0f, 4.0f, 0.01f);
                CustomUIControl::DrawFloat(std::string(ICON_MD_TUNE "  Specular Intensity").c_str(),
                    intens.Specular, 0.0f, 4.0f, 0.01f);
                ibl->SetIntensity(intens);

                // Skybox rotation (Y)
                float rotY = ibl->GetSkyboxRotationY();
                if (CustomUIControl::DrawFloat(std::string(ICON_MD_ROTATE_90_DEGREES_CW "  Skybox Y Rotation").c_str(),
                    rotY, -glm::pi<float>(), glm::pi<float>(), 0.005f))
                {
                    ibl->SetSkyboxRotationY(rotY);
                }

                // Optional: debug IDs (collapsed)
                if (ImGui::CollapsingHeader(std::string(ICON_MD_DEVELOPER_MODE "  Debug").c_str()))
                {
                    ImGui::TextDisabled("Irradiance ID:   %u", (unsigned)ibl->GetIrradianceTexture());
                    ImGui::TextDisabled("Prefiltered ID:  %u", (unsigned)ibl->GetPrefilteredTexture());
                    ImGui::TextDisabled("BRDF LUT ID:     %u", (unsigned)ibl->GetBRDFLUTTexture());
                    ImGui::TextDisabled("Env Cube ID:     %u", (unsigned)ibl->GetEnvironmentCubeTexture());
                }
            }

            ImGui::TreePop();
        }

        // ───────────────────────────────── Global Appearance ─────────────────────────────────────────
        if (ImGui::TreeNodeEx((void*)3,
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_AllowItemOverlap |
            ImGuiTreeNodeFlags_FramePadding,
            "%s  %s", ICON_MD_STYLE, "Appearance"))
        {
            CustomUIControl::DrawFloat(std::string(ICON_MD_EXPOSURE "  Exposure").c_str(),
                env.Exposure, 0.0f, 8.0f, 0.01f);

            CustomUIControl::DrawFloat(std::string(ICON_MD_CONTRAST "  Gamma").c_str(),
                env.Gamma, 1.0f, 3.0f, 0.01f);

            CustomUIControl::ColorEdit3(std::string(ICON_MD_COLOR_LENS "  Ambient Tint").c_str(),
                env.AmbientTint);

            ImGui::TreePop();
        }

        // ───────────────────────────────── Fog (optional) ────────────────────────────────────────────
        if (ImGui::TreeNodeEx((void*)4,
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_AllowItemOverlap |
            ImGuiTreeNodeFlags_FramePadding,
            "%s  %s", ICON_MD_CLOUD, "Fog"))
        {
            ImGui::Checkbox(std::string(ICON_MD_TOGGLE_ON "  Enabled").c_str(), &env.FogSettings.Enabled);
            CustomUIControl::DrawFloat(std::string(ICON_MD_BLUR_ON "  Density").c_str(),
                env.FogSettings.Density, 0.0f, 1.0f, 0.001f);
            CustomUIControl::ColorEdit3(std::string(ICON_MD_INVERT_COLORS "  Color").c_str(),
                env.FogSettings.Color);

            ImGui::TreePop();
        }

        ImGui::End();
    };
}