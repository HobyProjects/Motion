#include "CorePCH.hpp"
#include "SceneEditorLayer.hpp"

namespace Motion
{
    void SceneEditorLayer::OnAttach()
    {
        // TODO: Make this load a scene
        m_Scene = std::make_shared<Scene>(UniqueIdentity::GetUniqueID(), "Default Scene");
    }

    void SceneEditorLayer::OnDetach()
    {

    }

    void SceneEditorLayer::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        m_Scene->OnUpdate(handle, deltaTime);
        m_Scene->Submit();
    }

    void SceneEditorLayer::OnEvent(WindowHandle handle, IEvent& e)
    {
        m_Scene->OnEvent(handle, e);
    }

    void SceneEditorLayer::OnUIRender(WindowHandle handle)
    {
        BuildDockspace();
        m_Scene->OnUIRender(handle);
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
                    m_Scene->StartSimulation();

                ImGui::SameLine(0, pad_x);
                if (ImGui::Button(ICON_MD_STOP, btnSz))
                    m_Scene->StopSimulation();

                ImGui::SameLine(0, pad_x);
                if (ImGui::Button(ICON_MD_PAUSE, btnSz))
                    m_Scene->PauseSimulation();

                ImGui::PopID();

                ImGui::SameLine(0, style.ItemSpacing.x * 1.5f);

                ImGui::TextUnformatted("Simulation State: ");
                ImGui::SameLine();

                switch (m_Scene->GetSimulationState())
                {
                    case Scene::Simulation::IDLE:
                        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), "IDLE");   
                        break;
                    case Scene::Simulation::PAUSE:
                        ImGui::TextColored(ImVec4(1.0f, 0.72f, 0.0f, 1.0f), "PAUSED");   
                        break;
                    case Scene::Simulation::RUNNING:
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
                ImGui::DragFloat("##camspeed", &m_Scene->GetCamera().TranslationSpeed, 0.001f, 0.0f);
                ImGui::PopItemWidth();

                ImGui::SameLine();
                ImGui::TextUnformatted("Camera Sensitivity");
                ImGui::SameLine();
                ImGui::PushItemWidth(drag_w);
                ImGui::DragFloat("##camsens", &m_Scene->GetCamera().Sensitivity, 0.001f);
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
}