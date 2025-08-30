#include "CorePCH.hpp"
#include "SceneEditorLayer.hpp"
#include "ViewportPanel.hpp"

namespace Motion
{
    void SceneViewportPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
        ImGui::Begin(std::format("{}##SceneViewport", context.ActiveScene->GetName()).c_str());

        const bool focused_or_hovered = ImGui::IsWindowFocused() || ImGui::IsWindowHovered();
        if (context.UILayerInstance) context.UILayerInstance->AcceptEvents(focused_or_hovered);

        const ImVec2 vp = ImGui::GetContentRegionAvail();
        FrameTextureID tex = context.ActiveViewportTexture;
        if (tex != 0)
        {
            ImGui::Image((ImTextureID)(intptr_t)tex, vp, ImVec2(0, 1), ImVec2(1, 0));
        }
        else
        {
            ImGui::Dummy(vp);
        }

        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 crMin  = ImGui::GetWindowContentRegionMin();
        ImVec2 crMax  = ImGui::GetWindowContentRegionMax();
        ImVec2 vpMin  = { winPos.x + crMin.x, winPos.y + crMin.y };
        ImVec2 vpMax  = { winPos.x + crMax.x, winPos.y + crMax.y };
        ImVec2 mouse  = ImGui::GetMousePos();

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) 
            && ImGui::IsWindowFocused() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing())
        {
            glm::vec2 local = { mouse.x - vpMin.x, mouse.y - vpMin.y };
            local.y = vp.y - local.y;

            glm::vec2 fbSize = 
            {
                (float)context.ActiveSceneSpecification.Viewport.FrameSpec.Width,
                (float)context.ActiveSceneSpecification.Viewport.FrameSpec.Height
            };
            glm::vec2 mouseInFB = 
            {
                local.x * (fbSize.x / vp.x),
                local.y * (fbSize.y / vp.y)
            };

            if (auto picked = context.ActiveScene->PickEntity(mouseInFB, fbSize))
                context.ActiveScene->SelectedEntity(picked);
        }

        ImGuizmo::Enable(true);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());


        static ImGuizmo::OPERATION gizmoOp   = ImGuizmo::TRANSLATE;
        static ImGuizmo::MODE      gizmoMode = ImGuizmo::LOCAL;

        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_E, false))
        {
            switch (gizmoOp)
            {
                case ImGuizmo::TRANSLATE: gizmoOp = ImGuizmo::ROTATE; break;
                case ImGuizmo::ROTATE:    gizmoOp = ImGuizmo::SCALE;  break;
                case ImGuizmo::SCALE:     gizmoOp = ImGuizmo::TRANSLATE; break;
                default: break;
            }
        }

        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_Q, false))
        {
            gizmoMode = (gizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
        }

        const bool snapToggle = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyDown(ImGuiKey_Space);
        float snap[3] = { 0.f, 0.f, 0.f };
        if (snapToggle)
        {
            switch (gizmoOp)
            {
                case ImGuizmo::TRANSLATE: snap[0] = snap[1] = snap[2] = 0.1f; break;
                case ImGuizmo::ROTATE:    snap[0] = snap[1] = snap[2] = 5.0f;  break; // degrees
                case ImGuizmo::SCALE:     snap[0] = snap[1] = snap[2] = 0.05f; break;
                default: break;
            }
        }

        glm::mat4 view       = context.ActiveScene->GetCameraView();
        glm::mat4 projection = context.ActiveScene->GetCameraProjection();

        if (auto sel = context.ActiveScene->GetSelectedEntity();
            sel && sel != EntityFactory::EMPTYENTITY && sel->HasComponent<TransformComponent>())
        {
            auto& TRS = sel->GetComponent<TransformComponent>();
            glm::mat4 transform = TRS.GetTransform();

            ImGuizmo::AllowAxisFlip(false);
            ImGuizmo::Manipulate(glm::value_ptr(view),
                                glm::value_ptr(projection),
                                gizmoOp,
                                gizmoMode,
                                glm::value_ptr(transform),
                                nullptr,
                                snapToggle ? snap : nullptr);

            if (ImGuizmo::IsUsing())
            {
                float T[3], Rdeg[3], S[3];
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform), T, Rdeg, S);
                TRS.Translation = { T[0], T[1], T[2] };
                TRS.Scale       = { S[0], S[1], S[2] };
                TRS.Rotation    = glm::quat(glm::radians(glm::vec3{ Rdeg[0], Rdeg[1], Rdeg[2] }));
            }
        }

        if (focused_or_hovered)
        {
            ImGui::SetCursorPos({8.f, 8.f});
            ImGui::BeginGroup();
            const ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("Mouse: (%.1f, %.1f) Down:%d%d%d  Capt:%d",
                        io.MousePos.x, io.MousePos.y,
                        io.MouseDown[0], io.MouseDown[1], io.MouseDown[2],
                        io.WantCaptureMouse ? 1 : 0);
            ImGui::Text("Gizmo Over:%d Using:%d  Op:%s  Mode:%s",
                        ImGuizmo::IsOver() ? 1 : 0,
                        ImGuizmo::IsUsing() ? 1 : 0,
                        gizmoOp == ImGuizmo::TRANSLATE ? "Translate" :
                        gizmoOp == ImGuizmo::ROTATE    ? "Rotate"    : "Scale",
                        gizmoMode == ImGuizmo::WORLD ? "World" : "Local");
            ImGui::TextUnformatted("Ctrl+E: Cycle | Ctrl+Q: World/Local | Ctrl+Shift: Snap");
            ImGui::EndGroup();
        }

        if (context.EditorLayerInstance && context.ActiveScene)
        {
            auto& viewport = context.ActiveSceneSpecification.Viewport;
            if(viewport.Size.x != vp.x || viewport.Size.y != vp.y)
                context.EditorLayerInstance->SetViewportSize({ vp.x, vp.y });
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}