#include "CorePCH.hpp"
#include "MaterialEditorPanel.hpp"
#include <numeric>

namespace Motion
{
    // --- Preview binding contract: all samplers start AFTER 15 -------------------
    namespace MatPreview
    {
        // Base slot (everything at/after 16 as requested)
        static constexpr int TEX_BASE = 16;

        // IBL bindings (cube/cube/2D)
        static constexpr int TEX_IRRADIANCE = TEX_BASE + 0; // samplerCube u_IrradianceMap
        static constexpr int TEX_PREFILTER = TEX_BASE + 1; // samplerCube u_PrefilteredEnvMap
        static constexpr int TEX_BRDFLUT = TEX_BASE + 2; // sampler2D   u_BRDFLUT

        // Material samplers (order is arbitrary as long as you set uniforms accordingly)
        static constexpr int TEX_BASECOLOR = TEX_BASE + 8;
        static constexpr int TEX_ORM = TEX_BASE + 9;
        static constexpr int TEX_NORMAL = TEX_BASE + 10;
        static constexpr int TEX_EMISSIVE = TEX_BASE + 11;
        static constexpr int TEX_OPACITY = TEX_BASE + 12;

        // If you enable extended maps later, just continue: +13, +14, ...
    }

    // Pick instance texture with fallback to base material
    static std::shared_ptr<ITexture> PickTex(const PhysicalBasedMaterialInstance& inst, TextureType tt)
    {
        if (auto it = inst.Texture.find(tt); it != inst.Texture.end() && it->second)
            return it->second;

        if (inst.BaseMaterial) {
            if (auto ib = inst.BaseMaterial->Texture.find(tt);
                ib != inst.BaseMaterial->Texture.end() && ib->second)
                return ib->second;
        }
        return nullptr;
    }

    void MaterialEditorPanel::CollectFromSelection(ScenePanelContext& ctx)
    {
        m_Items.clear();
        m_Selected = -1;
        if (!ctx.ActiveScene) return;

        auto ent = ctx.ActiveScene->GetSelectedEntity();
        if (!ent || ent == EntityFactory::EMPTYENTITY) return;

        if (ent->HasComponent<StaticMeshComponent>())
        {
            auto& sm = ent->GetComponent<StaticMeshComponent>();
            for (auto& mesh : *sm.Model)
            {
                if (mesh->PhysicalBasedMaterials && mesh->PhysicalBasedMaterials->BaseMaterial)
                {
                    ItemRef ref;
                    ref.Instance = mesh->PhysicalBasedMaterials;
                    ref.Owner = ent;
                    ref.MeshIndex = mesh->Index;
                    m_Items.emplace_back(std::move(ref));
                }
            }
            if (!m_Items.empty()) m_Selected = 0;
        }
    }

    void MaterialEditorPanel::ApplyToAllMeshSlots(ScenePanelContext& ctx,
        const std::shared_ptr<PhysicalBasedMaterialInstance>& src)
    {
        if (!ctx.ActiveScene || !src) return;
        auto ent = ctx.ActiveScene->GetSelectedEntity();
        if (!ent || !ent->HasComponent<StaticMeshComponent>()) return;

        auto& sm = ent->GetComponent<StaticMeshComponent>();
        for (auto& mesh : *sm.Model)
        {
            auto clone = std::make_shared<PhysicalBasedMaterialInstance>(src->GetMaterialID(), src->BaseMaterial);
            clone->Attributes = src->Attributes;
            clone->Texture = src->Texture;
            mesh->PhysicalBasedMaterials = std::move(clone);
        }
    }

    // attributes editor (PBR)
    void MaterialEditorPanel::DrawAttributes(PhysicalBasedMaterialInstance& mat)
    {
        CustomUIControl::ColorEdit3(std::string(ICON_MD_PALETTE "  Base Color").c_str(), mat.Attributes.BaseColor);
        CustomUIControl::DrawFloat(std::string(ICON_MD_TONALITY "  Metallic").c_str(), mat.Attributes.Metallic, 0.0f, 1.0f, 0.005f);
        CustomUIControl::DrawFloat(std::string(ICON_MD_GRAIN "  Roughness").c_str(), mat.Attributes.Roughness, 0.0f, 1.0f, 0.005f);
        CustomUIControl::DrawFloat(std::string(ICON_MD_OPACITY "  Opacity").c_str(), mat.Attributes.Opacity, 0.0f, 1.0f, 0.005f);
    }

    // texture grid editor
    void MaterialEditorPanel::DrawTextures(std::unordered_map<TextureType, std::shared_ptr<ITexture>>& map)
    {
        struct Slot {
            TextureType type;
            const char* pretty;
            const char* icon;
            const char* hint;     // small help text shown as tooltip
        };

        static const Slot kCore[] = {
            { TextureType::BaseColorTexture,  "Base Color",   ICON_MD_PALETTE,       "Albedo/base color. sRGB" },
            { TextureType::ORMTexture,        "ORM (AO/R/M)", ICON_MD_TONALITY,      "Occlusion(R), Roughness(G), Metalness(B). Linear" },
            { TextureType::NormalTexture,     "Normal",       ICON_MD_TERRAIN,       "Tangent-space normal. Linear" },
            { TextureType::EmissiveTexture,   "Emissive",     ICON_MD_BRIGHTNESS_5,  "Self-illumination color. sRGB" },
            { TextureType::OpacityTexture,    "Opacity",      ICON_MD_OPACITY,       "Alpha/opacity. Linear" },
        };

        auto texExists = [&](TextureType t) -> bool {
            auto it = map.find(t);
            return it != map.end() && it->second != nullptr;
            };

        auto badge = [&](bool ok) {
            ImGui::SameLine();
            ImGui::TextColored(ok ? ImVec4(0.33f, 0.78f, 0.47f, 1.0f)   // green
                : ImVec4(0.95f, 0.33f, 0.36f, 1.0f), // red
                ok ? ICON_MD_CHECK_CIRCLE : ICON_MD_ERROR);
            };

        // Loader hook — wire your file dialog here if you have one
        auto loadOrReplace = [&](TextureType t) {
            // TODO: open file dialog and create texture
            // Example (pseudo):
            // std::filesystem::path p = FileDialog::OpenTexture();
            // if (!p.empty()) map[t] = ITexture::Create(p);
            };

        // Clear hook
        auto clearTex = [&](TextureType t) {
            if (auto it = map.find(t); it != map.end()) it->second.reset();
            };

        // ---- Core slots in a table grid -----------------------------------------
        if (ImGui::BeginTable("pbr_tex_grid", 2, ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableNextRow();
            for (int i = 0; i < (int)std::size(kCore); ++i)
            {
                const auto& s = kCore[i];
                ImGui::TableSetColumnIndex(i % 2);
                ImGui::PushID(i);

                // Header line with icon, name, badge
                ImGui::TextUnformatted((std::string(s.icon) + "  " + s.pretty).c_str());
                badge(texExists(s.type));
                if (ImGui::IsItemHovered() && s.hint) ImGui::SetTooltip("%s", s.hint);

                // Card (uses your custom control with inline loader)
                std::shared_ptr<ITexture>& tex = map[s.type];
                CustomUIControl::TextureSlotCard(
                    (std::string(s.icon) + "  " + s.pretty).c_str(),
                    tex,
                    [&]() { loadOrReplace(s.type); } // “Load/Replace” button callback
                );

                // Context menu: Replace / Clear / Reimport
                if (ImGui::BeginPopupContextItem("slot_ctx"))
                {
                    if (ImGui::MenuItem(ICON_MD_FILE_OPEN "  Load / Replace")) { loadOrReplace(s.type); }
                    if (ImGui::MenuItem(ICON_MD_BACKSPACE "  Clear", nullptr, false, tex != nullptr)) { clearTex(s.type); }
                    // Optional: if you track original source file per texture:
                    // if (ImGui::MenuItem(ICON_MD_REFRESH "  Reimport", nullptr, false, tex != nullptr)) { /* reload from source */ }
                    ImGui::EndPopup();
                }

                // Drag-drop: accept a file path payload
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH"))
                    {
                        // payload->Data is const char* to a null-terminated path (define in your DnD source)
                        auto path = std::filesystem::path((const char*)payload->Data);
                        if (!path.empty())
                        {
                            // TODO: replace with your loader
                            // tex = ITexture::Create(path);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Tiny meta line (dimensions / format) if available
                // if (tex)
                // {
                //     auto spec = tex->GetSpecification();
                //     ImGui::BeginDisabled();
                //     ImGui::TextDisabled("%dx%d  %s%s", spec.Width, spec.Height, spec.IsCube ? "Cube " : "", spec.sRGB ? "sRGB" : "Linear");
                //     ImGui::EndDisabled();
                // }

                ImGui::PopID();

                if (i % 2 == 1 && i != (int)std::size(kCore) - 1)
                    ImGui::TableNextRow();
            }
            ImGui::EndTable();
        }

        // ---- Any extra/extended slots (appear after core) -----------------------
        // Show whatever the material already contains but isn’t part of the core set
        auto isCore = [&](TextureType t) {
            for (auto& s : kCore) if (s.type == t) return true;
            return false;
            };

        bool printedHeader = false;
        for (auto& [tt, tex] : map)
        {
            if (isCore(tt)) continue;

            if (!printedHeader)
            {
                ImGui::Separator();
                ImGui::TextUnformatted(ICON_MD_EXTENSION "  Extended");
                printedHeader = true;
            }

            std::string label = std::string(ICON_MD_BROKEN_IMAGE) + "  " + GetTextureTypeString(tt);
            CustomUIControl::TextureSlotCard(
                label.c_str(),
                tex,
                [&]() {
                    // TODO: open file dialog for this specific type
                    // tex = ITexture::Create(FileDialog::OpenTexture());
                }
            );

            // Context: Clear / Replace
            if (ImGui::BeginPopupContextItem("ext_ctx"))
            {
                if (ImGui::MenuItem(ICON_MD_FILE_OPEN "  Load / Replace")) {
                    // tex = ITexture::Create(FileDialog::OpenTexture());
                }
                if (ImGui::MenuItem(ICON_MD_BACKSPACE "  Clear", nullptr, false, tex != nullptr)) {
                    tex.reset();
                }
                ImGui::EndPopup();
            }
        }
    }

    MaterialEditorPanel::MaterialEditorPanel()
    {
        EnsurePreviewFB(512, 512); // default preview size
    }

    void MaterialEditorPanel::EnsurePreviewFB(int w, int h)
    {
        if (w <= 0 || h <= 0) return;

        if (!m_PreviewFB)
        {
            FrameBufferSpecification spec{};
            spec.Name = "MatPreviewFB";
            spec.Width = w;
            spec.Height = h;
            spec.Samples = 1;
            // one color + depth
            spec.Colors.clear();
            spec.Colors.push_back({ 0, 0, FrameBufferColorAttachmentStandards::Standard });
            spec.Depth = { 0, 0, FrameBufferDepthAttachmentStandards::CommonCombined };

            m_PreviewFB = IFrameBuffer::Create(spec);
            m_LastFBSize = { w,h };
        }
        else if (m_LastFBSize.x != w || m_LastFBSize.y != h)
        {
            m_PreviewFB->ResizeFrame(w, h);
            m_LastFBSize = { w,h };
        }

        if (!m_SphereMesh)
        {
            auto& AM = AssetManager::GetInstance();
            m_SphereMesh = AM.Get<StaticMesh>("ENG_SPHERE");
        }

        if (m_PreviewFB)
            m_PreviewTex = m_PreviewFB->GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;
    }

    void MaterialEditorPanel::RenderSphere(PhysicalBasedMaterialInstance* mat, Scene* activeScene, const glm::ivec2& fbSize)
    {
        if (!mat || !activeScene) return;

        // Shader (your AssetManager path)
        auto& AM = AssetManager::GetInstance();
        auto pbr = AM.Get<IShader>("PBR");
        if (!pbr) return;

        // Environment
        auto env = activeScene->GetEnvironment().Env;
        if (!env) return;

        // Bind shader
        pbr->Bind();

        // ── Camera (simple turntable) ────────────────────────────────────────────
        const float aspect = (float)std::max(1, fbSize.x) / (float)std::max(1, fbSize.y);
        const glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.05f, 100.0f);
        const glm::vec3 camPos{ 0.0f, 0.0f, 2000.0f };
        const glm::mat4 view = glm::lookAt(camPos, glm::vec3(0), glm::vec3(0, 1, 0));

        const float t = (float)ImGui::GetTime();
        const glm::mat4 model = glm::rotate(glm::mat4(1.0f), t * 0.5f, glm::vec3(0, 1, 0));
        const glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

        pbr->SetUniform("u_Model", model);
        pbr->SetUniform("u_NormalMatrix", normalMatrix);
        pbr->SetUniform("u_View", view);
        pbr->SetUniform("u_Proj", proj);
        pbr->SetUniform("u_CameraWorldPos", camPos);

        // ── Sun from scene (matches your env data) ───────────────────────────────
        const auto& sun = activeScene->GetEnvironment().Sun;
        pbr->SetUniform("u_Sun.direction", sun.Direction);
        pbr->SetUniform("u_Sun.color", sun.Color);
        pbr->SetUniform("u_Sun.intensity", sun.Intensity);

        // ── IBL at texture units >= 16 ───────────────────────────────────────────
        env->BindIrradianceTexture(MatPreview::TEX_IRRADIANCE);
        pbr->SetUniform("u_IrradianceMap", MatPreview::TEX_IRRADIANCE);

        env->BindPrefilteredTexture(MatPreview::TEX_PREFILTER);
        pbr->SetUniform("u_PrefilteredEnvMap", MatPreview::TEX_PREFILTER);

        env->BindBRDFLUTTexture(MatPreview::TEX_BRDFLUT);
        pbr->SetUniform("u_BRDFLUT", MatPreview::TEX_BRDFLUT);

        const auto intens = env->GetIntensity();
        pbr->SetUniform("u_IBLIntensity_Diffuse", intens.Diffuse);
        pbr->SetUniform("u_IBLIntensity_Specular", intens.Specular);

        // ── Material scalars (fallbacks when maps are absent) ────────────────────
        const auto& A = mat->Attributes;
        pbr->SetUniform("u_Material_BaseColor", A.BaseColor);
        pbr->SetUniform("u_Material_Metallic", A.Metallic);
        pbr->SetUniform("u_Material_Roughness", A.Roughness);
        pbr->SetUniform("u_Material_Opacity", A.Opacity);

        // ── Bind material maps (all at/after 16) + build mask ────────────────────
        int texMask = 0;

        auto bind2D = [&](const char* uName, TextureType tt, int unit, int bitFlag)
            {
                if (auto tex = PickTex(*mat, tt)) {
                    tex->Bind(unit);
                    texMask |= bitFlag;
                }
                else {
                    // Optional: bind a tiny 1x1 fallback here if your shader samples unconditionally
                    Renderer::UnbindTextureUnit(unit);
                }
                pbr->SetUniform(uName, unit); // set regardless—shader expects a value
            };

        // These bit flags must match your PBR.glsl TB_* layout
        constexpr int TB_BaseColor = 1 << 0;
        constexpr int TB_Metallic = 1 << 1;
        constexpr int TB_Roughness = 1 << 2;
        constexpr int TB_Normal = 1 << 3;
        constexpr int TB_AO = 1 << 4;
        constexpr int TB_Emissive = 1 << 5;
        constexpr int TB_Opacity = 1 << 6;
        constexpr int TB_ORM = 1 << 7;

        bind2D("u_BaseColorTex", TextureType::BaseColorTexture, MatPreview::TEX_BASECOLOR, TB_BaseColor);
        bind2D("u_ORMTex", TextureType::ORMTexture, MatPreview::TEX_ORM, TB_ORM);
        bind2D("u_NormalTex", TextureType::NormalTexture, MatPreview::TEX_NORMAL, TB_Normal);
        bind2D("u_EmissiveTex", TextureType::EmissiveTexture, MatPreview::TEX_EMISSIVE, TB_Emissive);
        bind2D("u_OpacityTex", TextureType::OpacityTexture, MatPreview::TEX_OPACITY, TB_Opacity);

        // If you also use separate AO/Metallic/Roughness (non-ORM) in your content, bind them here:
        // bind2D("u_AOTex",       TextureType::AmbientOcclusionTexture, MatPreview::TEX_BASE + 13, TB_AO);
        // bind2D("u_MetallicTex", TextureType::MetallicTexture,         MatPreview::TEX_BASE + 14, TB_Metallic);
        // bind2D("u_RoughnessTex",TextureType::RoughnessTexture,        MatPreview::TEX_BASE + 15, TB_Roughness);

        pbr->SetUniform("u_TexMask", texMask);

        // Normal Y flip based on texture metadata (if you store it)
        float normalYFlip = 0.0f;
        if (auto n = PickTex(*mat, TextureType::NormalTexture)) {
            normalYFlip = n->GetSpecification().InvertGreen ? 1.0f : 0.0f;
        }
        pbr->SetUniform("u_NormalYFlip", normalYFlip);

        // ── Draw your ready-made sphere mesh ─────────────────────────────────────
        m_SphereMesh->Render();
    }

    void MaterialEditorPanel::RenderPreviewScene(ScenePanelContext& ctx, PhysicalBasedMaterialInstance* mat)
    {
        RenderSphere(mat, ctx.ActiveScene.get(), m_LastFBSize);
    }

    void MaterialEditorPanel::RenderUI(ScenePanelContext& ctx)
    {
        ImGui::Begin(ICON_MD_BRUSH "  Material Editor");

        static int s_SortMode = 0; // 0: Name, 1: Has Errors, 2: Recently Added
        // ─────────────────────────────────────────────────────────────────────────
        // Top toolbar
        // ─────────────────────────────────────────────────────────────────────────
        {
            // Left: main actions
            if (ImGui::Button(ICON_MD_ADD "  New"))
            {
                auto& AM = AssetManager::GetInstance();
                auto base = AM.Get<PhysicalBasedMaterial>("PBR_BaseMaterial");
                auto inst = std::make_shared<PhysicalBasedMaterialInstance>(std::format("MatInstance_{}", m_Items.size() + 1), base);
                m_Items.push_back({ inst });
                m_Selected = (int)m_Items.size() - 1;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_FILE_OPEN "  Import YAML"))
            {
                // TODO: open your file dialog, then build base/instance from YAML
                // MaterialIO::ImportYAML(...);
            }
            ImGui::SameLine();
            bool canSave = (m_Selected >= 0 && m_Selected < (int)m_Items.size() && m_Items[m_Selected].Instance);
            ImGui::BeginDisabled(!canSave);
            if (ImGui::Button(ICON_MD_SAVE "  Save"))
            {
                // TODO: MaterialIO::ExportYAML(*m_Items[m_Selected].Instance, outPath);
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_SAVE_AS "  Save As..."))
            {
                // TODO: file dialog then export with new name
            }
            ImGui::EndDisabled();

            // Right: filter + sort
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x * 0.4f + 10.0f); // push to right-ish

            // Filter
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 150.0f);
            ImGui::InputTextWithHint("##filter", ICON_MD_SEARCH "  Filter...", m_Filter, sizeof(m_Filter));
            ImGui::SameLine();

            // Sort dropdown

            if (ImGui::BeginCombo("##sort", s_SortMode == 0 ? "Sort: Name" : s_SortMode == 1 ? "Sort: Issues" : "Sort: Recent"))
            {
                if (ImGui::Selectable("Sort: Name", s_SortMode == 0)) s_SortMode = 0;
                if (ImGui::Selectable("Sort: Issues", s_SortMode == 1)) s_SortMode = 1;
                if (ImGui::Selectable("Sort: Recent", s_SortMode == 2)) s_SortMode = 2;
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_REFRESH "  From Selection"))
            {
                // collect materials on selected entity mesh slots
                CollectFromSelection(ctx);
            }
        }

        ImGui::Separator();

        // ─────────────────────────────────────────────────────────────────────────
        // Three columns: Library | Inspector | Preview (+ Slots)
        // ─────────────────────────────────────────────────────────────────────────
        const float leftW = 320.0f;
        const float rightW = 360.0f;

        if (ImGui::BeginTable("mat_layout", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthFixed, leftW);
            ImGui::TableSetupColumn("mid", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthFixed, rightW);
            ImGui::TableNextRow();

            // ───────────────── Left: Library / Outliner ──────────────────────────
            ImGui::TableSetColumnIndex(0);
            ImGui::BeginChild("lib", ImVec2(0, 0), true);
            {
                // Header row
                ImGui::TextUnformatted(ICON_MD_VIEW_LIST "  Materials");
                ImGui::SameLine();
                if (ImGui::SmallButton(ICON_MD_DELETE_SWEEP "  Clear"))
                {
                    m_Items.clear(); m_Selected = -1;
                }
                ImGui::Separator();

                // Optional: sort view without altering storage
                std::vector<int> order(m_Items.size());
                std::iota(order.begin(), order.end(), 0);
                auto hasIssue = [&](int idx)->bool {
                    auto& it = m_Items[idx];
                    if (!it.Instance) return true;
                    // Example "issue" heuristic: missing ORM or Normal
                    auto& tex = it.Instance->Texture;
                    bool missORM = tex.find(TextureType::ORMTexture) == tex.end();
                    bool missNrm = tex.find(TextureType::NormalTexture) == tex.end();
                    return missORM || missNrm;
                    };
                std::sort(order.begin(), order.end(), [&](int a, int b) {
                    switch (s_SortMode)
                    {
                    case 1: return hasIssue(a) && !hasIssue(b);
                    case 2: return a > b; // recent last → simple reverse index
                    default: {
                        const char* A = m_Items[a].Instance ? m_Items[a].Instance->GetMaterialID().c_str() : "";
                        const char* B = m_Items[b].Instance ? m_Items[b].Instance->GetMaterialID().c_str() : "";
                        return std::string(A) < std::string(B);
                    }
                    }
                    });

                for (int vis = 0; vis < (int)order.size(); ++vis)
                {
                    int i = order[vis];
                    auto& it = m_Items[i];
                    if (!it.Instance) continue;

                    const char* id = it.Instance->GetMaterialID().c_str();
                    if (*m_Filter && std::string(id).find(m_Filter) == std::string::npos)
                        continue;

                    ImGui::PushID(i);
                    bool selected = (m_Selected == i);
                    if (ImGui::Selectable((std::string(ICON_MD_STYLE) + "  " + id).c_str(), selected))
                        m_Selected = i;

                    // Drag source: drag the instance ID
                    if (ImGui::BeginDragDropSource())
                    {
                        ImGui::SetDragDropPayload("MATERIAL_INSTANCE_PTR", &it.Instance, sizeof(std::shared_ptr<PhysicalBasedMaterialInstance>));
                        ImGui::TextUnformatted(id);
                        ImGui::EndDragDropSource();
                    }

                    // Context menu
                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem(ICON_MD_CONTENT_COPY "  Duplicate"))
                        {
                            auto clone = std::make_shared<PhysicalBasedMaterialInstance>(it.Instance->GetMaterialID(), it.Instance->BaseMaterial);
                            clone->Attributes = it.Instance->Attributes;
                            clone->Texture = it.Instance->Texture;
                            m_Items.push_back({ clone });
                        }
                        if (ImGui::MenuItem(ICON_MD_DELETE "  Remove"))
                        {
                            if (m_Selected == i) m_Selected = -1;
                            m_Items.erase(m_Items.begin() + i);
                            ImGui::EndPopup();
                            ImGui::PopID();
                            break;
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();

            // ───────────────── Middle: Inspector ─────────────────────────────────
            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("inspector", ImVec2(0, 0), true);
            {
                if (m_Selected >= 0 && m_Selected < (int)m_Items.size() && m_Items[m_Selected].Instance)
                {
                    auto& mat = *m_Items[m_Selected].Instance;

                    // Title line + quick ops
                    {
                        ImGui::TextUnformatted((std::string(ICON_MD_TUNE) + "  " + mat.GetMaterialID()).c_str());
                        ImGui::SameLine();
                        if (ImGui::SmallButton(ICON_MD_UPLOAD "  Upload")) mat.UploadAttributes();
                        ImGui::SameLine();
                        if (ImGui::SmallButton(ICON_MD_CHECK_CIRCLE "  Apply to Selected Entity"))
                        {
                            // quick apply to active entity’s current mesh slot(s)
                            if (ctx.ActiveScene)
                            {
                                if (auto ent = ctx.ActiveScene->GetSelectedEntity();
                                    ent && ent->HasComponent<StaticMeshComponent>())
                                {
                                    auto& sm = ent->GetComponent<StaticMeshComponent>();
                                    for (auto& sub : *sm.Model)
                                    {
                                        auto clone = std::make_shared<PhysicalBasedMaterialInstance>(mat.GetMaterialID(), mat.BaseMaterial);
                                        clone->Attributes = mat.Attributes;
                                        clone->Texture = mat.Texture;
                                        sub->PhysicalBasedMaterials = std::move(clone);
                                    }
                                }
                            }
                        }
                    }
                    ImGui::Separator();

                    // Attributes (always open)
                    if (ImGui::CollapsingHeader(ICON_MD_TUNE "  Attributes", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        DrawAttributes(mat);
                    }

                    // Textures (always open)
                    if (ImGui::CollapsingHeader(ICON_MD_IMAGE "  Textures", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        DrawTextures(mat.Texture);
                    }

                    // Base material textures (read-only)
                    if (mat.BaseMaterial && ImGui::CollapsingHeader(ICON_MD_COLLECTIONS "  Base Material (read-only)"))
                    {
                        ImGui::BeginDisabled();
                        DrawTextures(mat.BaseMaterial->Texture);
                        ImGui::EndDisabled();
                    }
                }
                else
                {
                    ImGui::TextDisabled("%s  Select a material from the left list.", ICON_MD_INFO);
                }
            }
            ImGui::EndChild();

            // ───────────────── Right: Preview + Slots (drag-apply) ───────────────
            ImGui::TableSetColumnIndex(2);
            ImGui::BeginChild("preview", ImVec2(0, 0), true);
            {
                // Tabs: Preview | Slots
                if (ImGui::BeginTabBar("rightTabs"))
                {
                    if (ImGui::BeginTabItem(ICON_MD_VISIBILITY "  Preview"))
                    {
                        // Allocate/resize FB to fit this pane
                        ImVec2 avail = ImGui::GetContentRegionAvail();
                        int fbW = (int)std::max(8.0f, avail.x);
                        int fbH = (int)std::max(8.0f, avail.y - 6.0f);
                        EnsurePreviewFB(fbW, fbH);

                        // Render
                        if (m_PreviewFB)
                        {
                            m_PreviewFB->Bind();
                            Renderer::SetViewport(0, 0, fbW, fbH);
                            Renderer::Clear();
                            PhysicalBasedMaterialInstance* sel = nullptr;
                            if (m_Selected >= 0 && m_Selected < (int)m_Items.size() && m_Items[m_Selected].Instance)
                                sel = m_Items[m_Selected].Instance.get();
                            RenderPreviewScene(ctx, sel);
                            m_PreviewFB->Unbind();

                            // Show texture (Y-flipped like viewport)
                            ImGui::Image((ImTextureID)(uintptr_t)m_PreviewTex, ImVec2((float)fbW, (float)fbH), ImVec2(0, 1), ImVec2(1, 0));
                        }
                        else
                        {
                            ImGui::TextDisabled("Preview framebuffer not available.");
                        }

                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem(ICON_MD_VIEW_IN_AR "  Slots"))
                    {
                        ImGui::TextDisabled("Drag a material from the left and drop onto a slot.");

                        if (ctx.ActiveScene)
                        {
                            if (auto ent = ctx.ActiveScene->GetSelectedEntity(); ent && ent->HasComponent<StaticMeshComponent>())
                            {
                                std::int32_t count = 0;
                                auto& sm = ent->GetComponent<StaticMeshComponent>();

                                for (auto& mesh : *sm.Model)
                                {
                                    ImGui::PushID(mesh.get());
                                    ImGui::Separator();

                                    std::string label = ICON_MD_LABEL "  Slot " + std::to_string(count);
                                    if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(-1, 0)))
                                    {
                                        // on double-click, pick its material into editor
                                        if (ImGui::IsMouseDoubleClicked(0) && mesh->PhysicalBasedMaterials)
                                        {
                                            // add to library if not present already
                                            bool found = false;
                                            for (int k = 0;k < (int)m_Items.size();++k)
                                            {
                                                if (m_Items[k].Instance == mesh->PhysicalBasedMaterials)
                                                {
                                                    m_Selected = k; found = true; break;
                                                }
                                            }
                                            if (!found)
                                            {
                                                m_Items.push_back({ mesh->PhysicalBasedMaterials, ent, count++ });
                                                m_Selected = (int)m_Items.size() - 1;
                                            }
                                        }
                                    }

                                    // Drag-drop target: accepts MATERIAL_INSTANCE_PTR
                                    if (ImGui::BeginDragDropTarget())
                                    {
                                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL_INSTANCE_PTR"))
                                        {
                                            auto dropped = *(std::shared_ptr<PhysicalBasedMaterialInstance>*)(payload->Data);
                                            if (dropped)
                                            {
                                                auto clone = std::make_shared<PhysicalBasedMaterialInstance>(dropped->GetMaterialID(), dropped->BaseMaterial);
                                                clone->Attributes = dropped->Attributes;
                                                clone->Texture = dropped->Texture;
                                                mesh->PhysicalBasedMaterials = std::move(clone);
                                            }
                                        }
                                        ImGui::EndDragDropTarget();
                                    }

                                    // Small inline info
                                    if (mesh->PhysicalBasedMaterials)
                                    {
                                        ImGui::Indent();
                                        ImGui::TextDisabled("%s  %s", ICON_MD_STYLE, mesh->PhysicalBasedMaterials->GetMaterialID().c_str());
                                        ImGui::Unindent();
                                    }

                                    ImGui::PopID();
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled("Select an entity with a StaticMeshComponent.");
                            }
                        }
                        else
                        {
                            ImGui::TextDisabled("No active scene.");
                        }

                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
            }
            ImGui::EndChild();

            ImGui::EndTable();
        }

        ImGui::End();
    }

}