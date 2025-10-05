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
        m_Viewport.Size                 = m_CurrentViewportSize;
        m_Viewport.FrameSpec.Name       = "SceneEditorFrame";
        m_Viewport.FrameSpec.Width      = (std::uint32_t)m_CurrentViewportSize.x;
        m_Viewport.FrameSpec.Height     = (std::uint32_t)m_CurrentViewportSize.y;
        m_Viewport.FrameSpec.Samples    = 1;
        m_Framebuffer                   = IFrameBuffer::Create(m_Viewport.FrameSpec);

        SceneSpecification spec{};
        spec.Name           = "Default Scene";
        spec.IsActive       = true;
        spec.Viewport       = m_Viewport;

        if (m_Scenes.empty())
            m_Scenes.push_back(std::make_shared<Scene>(spec));

        m_ActiveScene = m_Scenes[0];
        m_ActiveScene->Activate(true);
        m_SceneTextures[m_ActiveScene] = m_Framebuffer->GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;

        //------------------------------------------------------------------------------------

        m_Panels->Emplace<SceneViewportPanel>();
        m_Panels->Emplace<SceneViewPanel>();
        m_Panels->Emplace<SimulationPanel>();
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

        Renderer::SetViewport(0, 0,  (std::int32_t)m_CurrentViewportSize.x, (std::int32_t)m_CurrentViewportSize.y);
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

        ScenePanelContext panelContext;
        panelContext.ActiveScene                    = m_ActiveScene;
        panelContext.ViewportTexture          = m_SceneTextures[m_ActiveScene]; 
        panelContext.UIInstance                = s_ImGuiLayer.get();
        panelContext.EditorInstance            = this;

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
                ImGuiStyle& style = ImGui::GetStyle();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 13));     // roomier
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  ImVec2(8, 6));      // breathing room
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);             // soft corners
                ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg)); // flatter bar

                const float full_w   = ImGui::GetContentRegionAvail().x;
                const float start_x  = ImGui::GetCursorPosX();
                const float pad_x    = style.ItemSpacing.x;

                const ImVec2 btnSz(30, 30);
                auto text_w = [](const char* s){ return ImGui::CalcTextSize(s).x; };
                const float sep_w = style.ItemSpacing.x; 
                const float sim_buttons_w = (btnSz.x * 3.0f) + (pad_x * 2.0f);
                const float sim_label_w   = text_w("Simulation State: ");
                const float sim_value_w   = text_w("RUNNING");
                const float sim_center_w  = sim_buttons_w + sep_w + sim_label_w + sim_value_w;

                const float drag_w        = 70.0f;
                const float cam_speed_w   = text_w("Camera Speed");
                const float cam_sens_w    = text_w("Camera Sensitivity");
                const float right_w = cam_speed_w + pad_x + drag_w + pad_x + cam_sens_w + pad_x + drag_w;

                bool left_open = false;
                if (ImGui::BeginMenu(ICON_MD_FOLDER " Files"))
                {
                    left_open = true;
                    if (ImGui::MenuItem("New Scene")) { /* ... */ }
                    if (ImGui::MenuItem("Open..."))   { /* ... */ }
                    if (ImGui::MenuItem("Save"))      { /* ... */ }
                    ImGui::EndMenu();
                }

                const float after_left_x = ImGui::GetCursorPosX();
                float center_x = start_x + (full_w - sim_center_w) * 0.5f;
                center_x = ImMax(center_x, after_left_x + pad_x);

                ImGui::SameLine(0, 0);
                ImGui::SetCursorPosX(center_x);

                ImGui::PushID("simbar");
                if (ImGui::Button(ICON_MD_PLAY_ARROW, btnSz))
                    m_ActiveScene->GotoSimulation(SimulationState::Running);

                ImGui::SameLine(0, pad_x);
                if (ImGui::Button(ICON_MD_STOP, btnSz))
                    m_ActiveScene->GotoSimulation(SimulationState::Stop);

                ImGui::SameLine(0, pad_x);
                if (ImGui::Button(ICON_MD_PAUSE, btnSz))
                    m_ActiveScene->GotoSimulation(SimulationState::Paused);
                ImGui::PopID();

                ImGui::SameLine(0, style.ItemSpacing.x * 1.5f);

                ImGui::TextUnformatted("Simulation State: ");
                ImGui::SameLine();

                switch (m_ActiveScene->GetSimualtionState())
                {
                    case SimulationState::Stop:
                        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), "IDLE");   
                        break;
                    case SimulationState::Paused:
                        ImGui::TextColored(ImVec4(1.0f, 0.72f, 0.0f, 1.0f), "PAUSED");   
                        break;
                    case SimulationState::Running:
                        ImGui::TextColored(ImVec4(0.1f, 0.95f, 0.4f, 1.0f), "RUNNING"); 
                        break;
                    default: break;
                }

                float right_x = start_x + full_w - right_w;
                right_x = ImMax(right_x, ImGui::GetCursorPosX() + pad_x);

                ImGui::SameLine(0, 0);
                ImGui::SetCursorPosX(right_x);

                ImGui::TextUnformatted("Camera Speed");
                ImGui::SameLine();
                ImGui::PushItemWidth(drag_w);
                ImGui::DragFloat("##camspeed", &m_ActiveScene->GetCamera().TranslationSpeed, 0.001f, 0.0f);
                ImGui::PopItemWidth();

                ImGui::SameLine();
                ImGui::TextUnformatted("Camera Sensitivity");
                ImGui::SameLine();
                ImGui::PushItemWidth(drag_w);
                ImGui::DragFloat("##camsens", &m_ActiveScene->GetCamera().Sensitivity, 0.001f);
                ImGui::PopItemWidth();

                ImGui::PopStyleColor(); 
                ImGui::PopStyleVar(3);

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

        if (m_Framebuffer)         m_Framebuffer->ResizeFrame((std::int32_t)size.x, (std::int32_t)size.y);
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

        if (m_Scenes.empty()) 
        {
            SceneSpecification spec{};

            spec.Name           = "New Scene";
            spec.IsActive       = true;
            spec.Viewport       = m_Viewport;

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

        spec.Name           = name.empty() ? "Untitled Scene" : name;
        spec.IsActive       = false;      
        spec.Viewport       = m_Viewport;

        auto s = std::make_shared<Scene>(spec);
        m_Scenes.push_back(s);

        if (makeActive)
            SetActiveScene(s);

        return s;
    }

    void SceneEditorLayer::DeleteScene(UUID id)
    {
        if (m_Scenes.empty()) return;
        bool deletingActive = (m_ActiveScene && m_ActiveScene->GetID() == id);

        for (auto it = m_SceneTextures.begin(); it != m_SceneTextures.end(); )
        {
            if (it->first && it->first->GetID() == id) it = m_SceneTextures.erase(it);
            else ++it;
        }

        for (auto it = m_Scenes.begin(); it != m_Scenes.end(); ++it)
        {
            if ((*it)->GetID() == id)
            {
                m_Scenes.erase(it);
                break;
            }
        }
        
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