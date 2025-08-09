#include "CorePCH.hpp"
#include "SceneEditorLayer.hpp"

namespace Motion
{
    static std::shared_ptr<ImGuiLayer> s_ImGuiLayer{ nullptr };
    static bool s_RequestLayoutReset = false;

    SceneEditorLayer::SceneEditorLayer(WindowHandle handle, const std::shared_ptr<ImGuiLayer>& imguiLayer) : Layer("EditorLayer")
    {
        s_ImGuiLayer = imguiLayer;
        m_Panels = std::make_shared<ScenePanelManager>();
    }

    void SceneEditorLayer::OnAttach()
    {
        //----------------------------------------------------------------------------
        // TEMP: (Until creating a scene serializer and application config serializer)
        //---------------------------------------------------------------------------

        auto& AM = AssetManager::GetInstance();
        AM.Create<IShader>("ENV", "Assets/Shaders/Environment.glsl");
        AM.Create<IShader>("ENV_IRR", "Assets/Shaders/EnvironmentIrradiance.glsl");
        AM.Create<IShader>("ENV_PRE", "Assets/Shaders/EnvironmentPrefiltered.glsl");
        AM.Create<IShader>("ENV_CUB", "Assets/Shaders/EnvironmentCubeConverter.glsl");
        AM.Create<IShader>("ENV_BRD", "Assets/Shaders/EnvironmentBRDF.glsl");
        AM.Create<IShader>("PBR", "Assets/Shaders/PBR.glsl");
        AM.Create<IShader>("PHONG", "Assets/Shaders/Phong.glsl");

        PhysicalBasedMaterial::Import("Assets/Materials/Base/PBR/Base.yaml");

        ImGuiIO& io = ImGui::GetIO();
        static std::string s_IniPath = "Config/EditorLayout.ini";
        io.IniFilename = s_IniPath.c_str(); // ImGui will auto load/save here

        m_Viewport.FrameSpec.Name = "SceneEditorFrame";
        m_Viewport.FrameSpec.Width = (uint32_t)m_CurrentViewportSize.x;
        m_Viewport.FrameSpec.Height = (uint32_t)m_CurrentViewportSize.y;
        m_Viewport.Size = m_CurrentViewportSize;
        m_Framebuffer = IFrameBuffer::Create(m_Viewport.FrameSpec);

        SceneSpecification spec{};
        spec.Name = "Default Scene";
        spec.IsActive = true;
        spec.Viewport = m_Viewport;
        spec.Environment = SceneEnvironment();
        spec.Environment.Env = IEnvironment::Create("Assets/HDRI/Scene4.hdr");

        if (m_Scenes.empty())
            m_Scenes.push_back(std::make_shared<Scene>(spec));

        m_ActiveScene = m_Scenes[0];
        m_ActiveScene->Activate(true);

        //------------------------------------------------------------------------------------

        m_Panels->Emplace<SceneViewportPanel>();
        m_Panels->Emplace<SceneEntityInspectPanel>();
        m_Panels->Emplace<SceneEntityPropertiesPanel>();
        m_Panels->Emplace<SceneSettingsPanel>();
        m_Panels->Emplace<SceneViewPanel>();
    }

    void SceneEditorLayer::OnDetach()
    {
        m_Framebuffer.reset();
        m_Scenes.clear();
    }

    void SceneEditorLayer::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        // react to viewport size changes from the ImGui panel
        if (m_ActiveScene->GetSpecification().Viewport.Size != m_CurrentViewportSize)
        {
            m_CurrentViewportSize = m_ActiveScene->GetSpecification().Viewport.Size;
            m_Framebuffer->ResizeFrame((uint32_t)m_CurrentViewportSize.x, (uint32_t)m_CurrentViewportSize.y);
            m_ActiveScene->OnViewportSizeChanges(m_CurrentViewportSize);
        }

        m_ActiveScene->OnUpdate(handle, deltaTime);

        m_Framebuffer->Bind();
        Renderer::ClearColor({ 0.243f, 0.243f, 0.243f, 1.0f });
        Renderer::Clear();
        SceneRenderer::BeginScene();
        SceneRenderer::Submit(m_ActiveScene.get());
        SceneRenderer::EndScene();
        m_Framebuffer->Unbind();

        m_SceneTextures[m_ActiveScene] =
            m_Framebuffer->GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;
    }

    void SceneEditorLayer::OnEvent(WindowHandle handle, IEvent& e)
    {
        m_ActiveScene->OnEvent(handle, e);
    }

    static void DrawViewportAxisWidget(const glm::mat4& view, float size = 60.0f)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Find bottom-left of viewport
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMin = ImVec2(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
        ImVec2 axisOrigin = ImVec2(viewportMin.x + size + 10.0f, viewportMin.y + ImGui::GetWindowSize().y - size - 10.0f);

        // Camera basis (extract from view matrix)
        glm::mat3 camBasis = glm::mat3(glm::transpose(view)); // Row-major, use transpose for camera orientation

        struct Axis {
            glm::vec3 dir;
            ImU32 color;
            const char* label;
        };

        Axis axes[3] = {
            { glm::vec3(1,0,0), IM_COL32(200,60,60,255), "X" },
            { glm::vec3(0,1,0), IM_COL32(60,200,60,255), "Y" },
            { glm::vec3(0,0,1), IM_COL32(80,150,255,255), "Z" }
        };

        for (int i = 0; i < 3; ++i)
        {
            glm::vec3 localDir = camBasis * axes[i].dir; // Camera space to world
            localDir = glm::normalize(localDir);

            float len = size;
            ImVec2 p0 = axisOrigin;
            ImVec2 p1 = ImVec2((axisOrigin.x + localDir.x * len), (axisOrigin.y - localDir.y * len)); // ImGui Y is downward

            drawList->AddLine(p0, p1, axes[i].color, 3.0f);

            // Draw label at the end
            ImVec2 labelPos = ImVec2(p1.x + 5.0f, p1.y - 5.0f); // Offset for better visibility
            drawList->AddText(labelPos, axes[i].color, axes[i].label);
        }

        // Optional: Draw circle at axis origin
        drawList->AddCircleFilled(axisOrigin, 5.0f, IM_COL32(120, 120, 120, 255));
    }

    void SceneEditorLayer::OnUIRender(WindowHandle handle)
    {
        BuildDockspace();

        ScenePanelContext panelContext;
        panelContext.ActiveScene = m_ActiveScene;
        panelContext.ActiveCamera = m_ActiveScene->GetCamera();
        panelContext.ActiveSceneSpecification = m_ActiveScene->GetSpecification();
        panelContext.ActiveViewportTexture = m_SceneTextures[m_ActiveScene];
        panelContext.UILayerInstance = s_ImGuiLayer.get();
        panelContext.EditorLayerInstance = this;

        for (const auto& panel : *m_Panels)
            panel->RenderUI(panelContext);
    }

    void SceneEditorLayer::BuildDockspace()
    {
        ImGuiWindowFlags host = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGuiDockNodeFlags dock = ImGuiDockNodeFlags_PassthruCentralNode |
            ImGuiDockNodeFlags_AutoHideTabBar;

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
        ImGui::Begin("##DockHost", nullptr, host);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
            ImGui::DockSpace(dockspace_id, { 0,0 }, dock);
            ImGui::PopStyleColor();
        }

        // Build default layout only if needed (first run or explicit reset)
        static bool built_once = false;

        auto iniExists =
            []() -> bool
            {
                const ImGuiIO& io = ImGui::GetIO();
                if (!io.IniFilename || !*io.IniFilename) return false;
#if __cpp_lib_filesystem
                return std::filesystem::exists(io.IniFilename);
#else
                // Fallback: try to open
                FILE* f = fopen(io.IniFilename, "rb");
                if (f) { fclose(f); return true; }
                return false;
#endif
            };

        bool need_default_layout = (!built_once && !iniExists()) || s_RequestLayoutReset;
        if (need_default_layout && (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable))
        {
            built_once = true;
            s_RequestLayoutReset = false;

            // Wipe and create the split tree (no docking of windows by title!)
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, vp->WorkSize);

            ImGuiID center = dockspace_id;
            ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.28f, nullptr, &center);
            ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.28f, nullptr, &center);
            ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.28f, nullptr, &center);
            (void)ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.35f, nullptr, &left);
            (void)ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.35f, nullptr, &right);

            // NOTE: No DockBuilderDockWindow() calls here.
            // Panels open themselves and the user places them; ImGui will persist.

            ImGui::DockBuilderFinish(dockspace_id);

            // Optional: immediately save so next launch uses the split tree
            ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
        }

        ImGui::End();
    }


    void SceneEditorLayer::SetActiveScene(const std::shared_ptr<Scene>& scene)
    {
        if (!scene || scene == m_ActiveScene) return;

        if (m_ActiveScene) m_ActiveScene->Activate(false);
        m_ActiveScene = scene;
        m_ActiveScene->Activate(true);

        // Keep viewport/fb in sync on scene swap
        m_CurrentViewportSize = m_ActiveScene->GetSpecification().Viewport.Size;
        m_Viewport = m_ActiveScene->GetSpecification().Viewport;

        // If this scene doesn't have a cached texture yet, next OnUpdate will fill it
    }

    void SceneEditorLayer::RemoveScene(const std::shared_ptr<Scene>& scene)
    {
        if (!scene) return;

        // if deleting active scene, pick a fallback after erase
        bool deletingActive = (scene == m_ActiveScene);

        // drop any cached texture entry
        m_SceneTextures.erase(scene);

        // erase from list
        auto it = std::find(m_Scenes.begin(), m_Scenes.end(), scene);
        if (it != m_Scenes.end()) m_Scenes.erase(it);

        if (m_Scenes.empty()) {
            // ensure at least one scene exists
            SceneSpecification spec{};
            spec.Name = "New Scene";
            spec.IsActive = true;
            spec.Viewport = m_Viewport; // reuse current viewport
            spec.Environment = SceneEnvironment();
            m_Scenes.push_back(std::make_shared<Scene>(spec));
        }

        if (deletingActive) {
            // choose first scene as new active
            SetActiveScene(m_Scenes.front());
        }
    }

    std::shared_ptr<Scene> SceneEditorLayer::AddNewScene(const std::string& name, bool makeActive)
    {
        SceneSpecification spec{};
        spec.Name = name.empty() ? "Untitled Scene" : name;
        spec.IsActive = false;              // will be set via SetActiveScene if makeActive
        spec.Viewport = m_Viewport;         // copy current viewport config
        spec.Environment = SceneEnvironment(); // fresh env

        auto s = std::make_shared<Scene>(spec);
        m_Scenes.push_back(s);

        if (makeActive)
            SetActiveScene(s);

        return s;
    }

    void SceneEditorLayer::DeleteScene(UUID id)
    {
        if (m_Scenes.empty()) return;

        // If deleting the active scene, we’ll swap active afterward
        bool deletingActive = (m_ActiveScene && m_ActiveScene->GetID() == id);

        // erase from texture cache first (safe even if missing)
        for (auto it = m_SceneTextures.begin(); it != m_SceneTextures.end(); )
        {
            if (it->first && it->first->GetID() == id) it = m_SceneTextures.erase(it);
            else ++it;
        }

        // remove from list
        for (auto it = m_Scenes.begin(); it != m_Scenes.end(); ++it)
        {
            if ((*it)->GetID() == id)
            {
                m_Scenes.erase(it);
                break;
            }
        }

        // fixup active scene
        if (deletingActive)
        {
            if (!m_Scenes.empty())
            {
                m_ActiveScene = m_Scenes.front();
                m_ActiveScene->Activate(true);
            }
            else
            {
                m_ActiveScene.reset();
            }
        }
    }

    void SceneViewPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::Begin("Project Scenes");

        // ── Top row: Search box + Add button (same line)
        static char s_SearchBuf[128] = {};
        {
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 38.0f); // leave room for the button
            ImGui::InputTextWithHint("##SearchScenes",
                ICON_MD_SEARCH " Search scenes...",
                s_SearchBuf, sizeof(s_SearchBuf));
            ImGui::PopItemWidth();

            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_ADD "##AddScene"))
            {
                ImGui::OpenPopup("New Scene");
            }
            ImGui::Separator();
        }

        // ── Create New Scene modal
        if (ImGui::BeginPopupModal("New Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char s_NewSceneName[128] = {};
            static bool s_SetActive = true;
            static bool s_Init = true;
            static std::string s_Error;

            if (ImGui::IsWindowAppearing() || s_Init)
            {
                s_Init = false;
                s_Error.clear();
                static int s_Counter = 1;
                std::snprintf(s_NewSceneName, sizeof(s_NewSceneName), "New Scene %d", s_Counter++);
                s_SetActive = true;
                ImGui::SetKeyboardFocusHere();
            }

            ImGui::TextUnformatted("Scene name:");
            ImGui::SetNextItemWidth(320.0f);
            bool enterPressed = ImGui::InputText("##scene_name", s_NewSceneName, sizeof(s_NewSceneName),
                ImGuiInputTextFlags_EnterReturnsTrue);

            ImGui::Checkbox("Set active after creating", &s_SetActive);

            // Validation helpers
            auto isBlank = [](const char* s) {
                for (const char* p = s; *p; ++p) if (!std::isspace((unsigned char)*p)) return false;
                return true;
                };
            auto nameExists = [&](const std::string& n) {
                for (auto& sc : *context.EditorLayerInstance)
                    if (sc->GetSpecification().Name == n) return true;
                return false;
                };

            if (!s_Error.empty())
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1, 0.35f, 0.35f, 1), "%s", s_Error.c_str());
            }

            ImGui::Separator();

            auto tryCreate = [&]() {
                std::string name = s_NewSceneName;
                if (isBlank(name.c_str()))
                {
                    s_Error = "Name cannot be empty.";
                    return false;
                }
                if (nameExists(name))
                {
                    s_Error = "A scene with this name already exists.";
                    return false;
                }
                context.EditorLayerInstance->AddNewScene(name, s_SetActive);
                s_Error.clear();
                s_Init = true;
                ImGui::CloseCurrentPopup();
                return true;
                };

            bool createClicked = ImGui::Button("Create", ImVec2(100, 0));
            if (createClicked || enterPressed)
                tryCreate();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0)))
            {
                s_Error.clear();
                s_Init = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // ── Tree of scenes (icon on the root)
        if (ImGui::TreeNodeEx(
            (void*)context.UILayerInstance,
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_AllowItemOverlap |
            ImGuiTreeNodeFlags_FramePadding,
            "%s  %s", ICON_MD_COLLECTIONS, "Project Scenes"))
        {
            // case-insensitive substring matcher
            auto ci_contains = [](std::string hay, std::string needle)
                {
                    std::transform(hay.begin(), hay.end(), hay.begin(),
                        [](unsigned char c) { return (char)std::tolower(c); });
                    std::transform(needle.begin(), needle.end(), needle.begin(),
                        [](unsigned char c) { return (char)std::tolower(c); });
                    return needle.empty() || (hay.find(needle) != std::string::npos);
                };

            for (auto& scene : *context.EditorLayerInstance)
            {
                ImGui::PushID(scene.get());

                const bool isActive = scene->IsActive();

                // Scene row label with icon + optional active star
                std::string label = std::format("{}  {}[{:X}]{}",
                    ICON_MD_DASHBOARD,                     // scene icon
                    scene->GetName(),
                    scene->GetID(),
                    isActive ? std::string("  ") + ICON_MD_STAR : "");

                // Filter using our local static buffer
                if (!ci_contains(label, std::string(s_SearchBuf)))
                {
                    ImGui::PopID();
                    continue;
                }

                ImGuiTreeNodeFlags hdrFlags =
                    ImGuiTreeNodeFlags_FramePadding |
                    ImGuiTreeNodeFlags_SpanAvailWidth |
                    ImGuiTreeNodeFlags_Framed;

                bool open = ImGui::CollapsingHeader(label.c_str(), hdrFlags);

                // Left click / activate -> set active
                if (ImGui::IsItemClicked() ||
                    ImGui::IsItemActivated() ||
                    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) ||
                    ImGui::IsItemFocused())
                {
                    scene->SelectEntityIf();
                    context.ActiveScene = scene;                        // view-side context
                    context.EditorLayerInstance->SetActiveScene(scene); // editor state
                }

                // Right-click context menu
                if (ImGui::BeginPopupContextItem("SceneCtx"))
                {
                    if (ImGui::MenuItem(ICON_MD_EDIT "  Rename..."))
                    {
                        ImGui::CloseCurrentPopup();
                        ImGui::OpenPopup("RenameScenePopup");
                    }
                    if (ImGui::MenuItem(ICON_MD_DELETE "  Delete..."))
                    {
                        ImGui::CloseCurrentPopup();
                        ImGui::OpenPopup("DeleteScenePopup");
                    }
                    ImGui::EndPopup();
                }

                // Rename modal (unique per scene thanks to PushID)
                if (ImGui::BeginPopupModal("RenameScenePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    static char s_RenameBuf[128] = {};
                    if (ImGui::IsWindowAppearing())
                    {
                        memset(s_RenameBuf, 0, sizeof(s_RenameBuf));
                        const std::string& n = scene->GetSpecification().Name;
                        strncpy(s_RenameBuf, n.c_str(), sizeof(s_RenameBuf) - 1);
                        ImGui::SetKeyboardFocusHere();
                    }

                    ImGui::TextUnformatted("New scene name:");
                    ImGui::InputText("##rename", s_RenameBuf, sizeof(s_RenameBuf));

                    ImGui::Separator();
                    if (ImGui::Button("OK", { 80,0 }))
                    {
                        scene->GetSpecification().Name = std::string(s_RenameBuf);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", { 80,0 }))
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                // Delete confirmation modal
                if (ImGui::BeginPopupModal("DeleteScenePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::TextWrapped("%s  Delete scene \"%s\"?\nThis cannot be undone.",
                        ICON_MD_WARNING, scene->GetName().c_str());
                    ImGui::Separator();

                    if (ImGui::Button("Delete", { 80,0 }))
                    {
                        auto id = scene->GetID();
                        ImGui::CloseCurrentPopup();
                        context.EditorLayerInstance->DeleteScene(id);
                        ImGui::EndPopup(); // avoid touching 'scene' after deletion
                        ImGui::PopID();
                        break; // container mutated; restart next frame
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", { 80,0 }))
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                // Optional: inner Entities list (icon on header + bullets with tags)
                if (open)
                {
                    std::string entitiesHeader = std::string(ICON_MD_LIST) + "  Entities";
                    if (ImGui::CollapsingHeader(entitiesHeader.c_str(), hdrFlags))
                    {
                        for (const auto& entity : *scene)
                        {
                            if (entity->HasComponent<TagComponent>())
                            {
                                const std::string& tag = entity->GetComponent<TagComponent>().Tag;
                                ImGui::BulletText("%s  %s", ICON_MD_LABEL, tag.c_str());
                            }
                            else
                            {
                                ImGui::BulletText("%s  %s", ICON_MD_LABEL_OFF, "Unnamed Entity");
                            }
                        }
                    }
                }

                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        ImGui::End();
    }
}