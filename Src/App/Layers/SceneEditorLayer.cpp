#include "CorePCH.hpp"

#include "Panels.hpp"
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
        AM.Create<IShader>("ENV_SKY", "Assets/Shaders/GLSL/Environment/Environment.glsl");
        AM.Create<IShader>("ENV_IRR", "Assets/Shaders/GLSL/Environment/EnvironmentIrradiance.glsl");
        AM.Create<IShader>("ENV_PRE", "Assets/Shaders/GLSL/Environment/EnvironmentPrefiltered.glsl");
        AM.Create<IShader>("ENV_CUB", "Assets/Shaders/GLSL/Environment/EnvironmentCubeConverter.glsl");
        AM.Create<IShader>("ENV_BRD", "Assets/Shaders/GLSL/Environment/EnvironmentBRDF.glsl");
        
        BaseMaterial::Import("Assets/Materials/Metal/Base.yaml");
        BaseMaterial::Import("Assets/Materials/Marble/Base.yaml");
        BaseMaterial::Import("Assets/Materials/Plastic/Base.yaml");
        BaseMaterial::Import("Assets/Materials/Rubber/Base.yaml");
        BaseMaterial::Import("Assets/Materials/Stone/Base.yaml");

        //------------------------------------------------------------------------------------

        m_Viewport.Size                 = m_CurrentViewportSize;
        m_Viewport.FrameSpec.Name       = "SceneEditorFrame";
        m_Viewport.FrameSpec.Width      = (std::uint32_t)m_CurrentViewportSize.x;
        m_Viewport.FrameSpec.Height     = (std::uint32_t)m_CurrentViewportSize.y;
        m_Viewport.FrameSpec.Samples    = 1;
        m_Framebuffer                   = IFrameBuffer::Create(m_Viewport.FrameSpec);

        EnvironmentSpecification specEnv;
        specEnv.UseSHDiffuse = false;
        specEnv.BuildBRDFLUT = true;
        specEnv.HDRfile = "Assets/HDRI/Scene.hdr";

        SceneSpecification spec{};
        spec.Name = "Default Scene";
        spec.IsActive = true;
        spec.Viewport = m_Viewport;
        spec.Environment = SceneEnvironment();
        spec.Environment.EnvironmentInstance = IEnvironment::Create(specEnv);

        if (m_Scenes.empty())
            m_Scenes.push_back(std::make_shared<Scene>(spec));

        m_ActiveScene = m_Scenes[0];
        m_ActiveScene->Activate(true);

        m_SceneTextures[m_ActiveScene] = m_Framebuffer->GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;

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
        m_ActiveScene->OnUpdate(handle, deltaTime);

        m_Framebuffer->Bind();
        Renderer::SetViewport(0, 0, (int)m_CurrentViewportSize.x, (int)m_CurrentViewportSize.y);
        Renderer::ClearColor({ 0.243f, 0.243f, 0.243f, 1.0f });
        Renderer::Clear();

        SceneRenderer::BeginScene();
        SceneRenderer::Submit(m_ActiveScene.get());
        SceneRenderer::EndScene();

        m_Framebuffer->Unbind();
        m_SceneTextures[m_ActiveScene] = m_Framebuffer->GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;
    }

    void SceneEditorLayer::OnEvent(WindowHandle handle, IEvent& e)
    {
        m_ActiveScene->OnEvent(handle, e);
    }

    void SceneEditorLayer::OnUIRender(WindowHandle handle)
    {
        BuildDockspace();
        ImGui::ShowDemoWindow();

        ScenePanelContext panelContext;
        panelContext.ActiveScene                    = m_ActiveScene;
        panelContext.ActiveCamera                   = m_ActiveScene->GetCamera();
        panelContext.ActiveSceneSpecification       = m_ActiveScene->GetSpecification();
        panelContext.ActiveViewportTexture          = m_SceneTextures[m_ActiveScene]; 
        panelContext.UILayerInstance                = s_ImGuiLayer.get();
        panelContext.EditorLayerInstance            = this;

        for (const auto& panel : *m_Panels)
            panel->RenderUI(panelContext);
    }

    void SceneEditorLayer::BuildDockspace()
    {
        ImGuiWindowFlags host =
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_MenuBar;

        ImGuiDockNodeFlags dock = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar;

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
        if(ImGui::Begin("##DockHost", nullptr, host))
        {
            if (ImGui::BeginMenuBar())
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5, 10 });
                if (ImGui::BeginMenu(ICON_MD_FOLDER " Files"))
                {
                    if (ImGui::MenuItem("New Scene")) 
                    { 
                    }

                    if (ImGui::MenuItem("Open...")) 
                    { 
                    }

                    if (ImGui::MenuItem("Save")) 
                    { 
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(ICON_MD_EDIT " Edit"))
                {
                    if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
                    if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}

                    ImGui::Separator();
                    if (ImGui::MenuItem("Preferences...")) {}
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(ICON_MD_VIEW_COMFY " View"))
                {
                    if (ImGui::MenuItem("Reset Layout")) {}
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(ICON_MD_HELP " Help"))
                {
                    if (ImGui::MenuItem("About")) {}
                    ImGui::EndMenu();
                }

                ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

                ImGui::TextUnformatted("Active Scene");
                ImGui::SameLine();
                ImGui::PushItemWidth(220.0f);
                
                std::int32_t selectedIndex = -1;
                std::vector<const char*> sceneNames;
                for (const auto& scene : m_Scenes)
                {
                    sceneNames.push_back(scene->GetName().c_str());
                    if (scene == m_ActiveScene) selectedIndex = (std::int32_t)(sceneNames.size() - 1);
                }

                ImGui::Combo("##scene-list", &selectedIndex, sceneNames.data(), sceneNames.size());
                ImGui::PopItemWidth();

                ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

                ImGui::TextUnformatted("Simulation Controls");

                ImGui::SameLine();
                if (ImGui::Button(ICON_MD_PLAY_ARROW, ImVec2(30, 30)))
                {
                }

                ImGui::SameLine(0, 6);
                if (ImGui::Button(ICON_MD_STOP, ImVec2(30, 30)))
                {
                }

                ImGui::SameLine(0, 6);
                if (ImGui::Button(ICON_MD_PAUSE, ImVec2(30, 30)))
                {
                }

                ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

                ImGui::TextUnformatted("Camera Speed");
                ImGui::SameLine();
                ImGui::PushItemWidth(70.0f);
                ImGui::DragFloat("##camspeed", &m_ActiveScene->GetCamera().Camera.TranslationSpeed, 0.001f);
                ImGui::PopItemWidth();

                ImGui::SameLine();
                ImGui::TextUnformatted("Camera Sensitivity");
                ImGui::SameLine();
                ImGui::PushItemWidth(70.0f);
                ImGui::DragFloat("##camsens", &m_ActiveScene->GetCamera().Camera.Sensitivity, 0.001f);
                ImGui::PopItemWidth();

                ImGui::PopStyleVar();
                ImGui::EndMenuBar();
            }

        }
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
            ImGui::DockSpace(dockspace_id, { 0,0 }, dock);
            ImGui::PopStyleColor();
        }

        ImGui::End();
    }

    void SceneEditorLayer::SetViewportSize(const glm::vec2 & size)
    {
        if (size == m_CurrentViewportSize || size.x <= 1.0f || size.y <= 1.0f) return;
        m_CurrentViewportSize = size;

        if (m_Framebuffer)         m_Framebuffer->ResizeFrame((int)size.x, (int)size.y);
        if (m_ActiveScene)         m_ActiveScene->OnViewportSizeChanges(size);
    }

    void SceneEditorLayer::SetActiveScene(const std::shared_ptr<Scene>& scene)
    {
        if (!scene || scene == m_ActiveScene) return;

        if (m_ActiveScene) m_ActiveScene->Activate(false);
        m_ActiveScene = scene;
        m_ActiveScene->Activate(true);

        m_CurrentViewportSize = m_ActiveScene->GetSpecification().Viewport.Size;
        m_Viewport = m_ActiveScene->GetSpecification().Viewport;
    }

    void SceneEditorLayer::RemoveScene(const std::shared_ptr<Scene>& scene)
    {
        if (!scene) return;

        bool deletingActive = (scene == m_ActiveScene);
        m_SceneTextures.erase(scene);

        auto it = std::find(m_Scenes.begin(), m_Scenes.end(), scene);
        if (it != m_Scenes.end()) m_Scenes.erase(it);

        if (m_Scenes.empty()) {
            // ensure at least one scene exists
            SceneSpecification spec{};
            spec.Name = "New Scene";
            spec.IsActive = true;
            spec.Viewport = m_Viewport;
            spec.Environment = SceneEnvironment();
            m_Scenes.push_back(std::make_shared<Scene>(spec));
        }

        if (deletingActive)
        {
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
}