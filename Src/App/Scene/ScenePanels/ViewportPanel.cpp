#include "CorePCH.hpp"
#include "SceneEditorLayer.hpp"
#include "ViewportPanel.hpp"

namespace Motion
{

    void SceneViewportPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
        ImGui::Begin(std::format("{}##SceneViewport", context.ActiveScene->GetName()).c_str());
        context.UILayerInstance->AcceptEvents(ImGui::IsWindowFocused() || ImGui::IsWindowHovered());

        // Keep the gizmo op accessible to the overlay
        static ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;

        ImVec2 vp = ImGui::GetContentRegionAvail();
        if (context.EditorLayerInstance && context.ActiveScene)
        {
            context.EditorLayerInstance->SetViewportSize({ vp.x, vp.y });
        }

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

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(vpMin.x, vpMin.y, vp.x, vp.y);

        glm::mat4 view = context.ActiveScene->GetCameraView();
        glm::mat4 proj = context.ActiveScene->GetCameraProjection();
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_E))
        {
            op = (op == ImGuizmo::TRANSLATE) ? ImGuizmo::ROTATE :
                (op == ImGuizmo::ROTATE)    ? ImGuizmo::SCALE  :
                                            ImGuizmo::TRANSLATE;
        }

        if (auto sel = context.ActiveScene->GetSelectedEntity();
            sel && sel != EntityFactory::EMPTYENTITY && sel->HasComponent<TransformComponent>())
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

        ImGui::End();
        ImGui::PopStyleVar();
    }
}