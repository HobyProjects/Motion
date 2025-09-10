#include "CorePCH.hpp"
#include "SceneEditorLayer.hpp"
#include "ViewportPanel.hpp"

namespace Motion
{
    static glm::mat3 MakeRotationFromDirection(const glm::vec3& dir, const glm::vec3& upHint = glm::vec3(0,1,0))
    {
        glm::vec3 fwd = glm::normalize(-dir);
        glm::vec3 right = glm::cross(upHint, fwd);
        if (glm::length2(right) < 1e-8f)
        {
            glm::vec3 altUp = std::abs(upHint.y) > 0.5f ? glm::vec3(0,0,1) : glm::vec3(0,1,0);
            right = glm::cross(altUp, fwd);
        }

        right           = glm::normalize(right);
        glm::vec3 up    = glm::normalize(glm::cross(fwd, right));
        glm::mat3 R( right, up, fwd );
        return R;
    }

    static glm::vec3 ExtractDirectionFromMatrix(const glm::mat4& M)
    {
        glm::vec3 fwd = glm::normalize(glm::vec3(M[2]));
        return -fwd;
    }

    static glm::vec3 ChooseDummpyPosition(const Camera3D& cam, float distance = 6.0f)
    {
        glm::mat4 invView = glm::inverse(cam.View);
        glm::vec3 camFwd  = glm::normalize(glm::vec3(invView[2]) * -1.0f);
        return cam.Position + camFwd * distance;
    }

    struct ViewportRect
    {
        ImVec2 min;
        ImVec2 max;
        float  width()  const { return max.x - min.x; }
        float  height() const { return max.y - min.y; }
    };

    static bool WorldToScreen(const glm::vec3& p, const glm::mat4& VP, const ViewportRect& r, ImVec2& out)
    {
        glm::vec4 clip = VP * glm::vec4(p, 1.0f);
        if (clip.w <= 0.0001f) return false;

        glm::vec3 ndc = glm::vec3(clip) / clip.w; // [-1,1]
        if (ndc.x < -1.2f || ndc.x > 1.2f || ndc.y < -1.2f || ndc.y > 1.2f || ndc.z < -1.2f || ndc.z > 1.2f)
            return false;

        out.x = r.min.x + (ndc.x * 0.5f + 0.5f) * r.width();
        out.y = r.min.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * r.height();
        return true;
    }

    static void AddLine3D(ImDrawList* dl, const glm::vec3& a, const glm::vec3& b, const glm::mat4& VP, const ViewportRect& rect, ImU32 col, float thickness = 1.5f)
    {
        ImVec2 pa, pb;
        if (WorldToScreen(a, VP, rect, pa) & WorldToScreen(b, VP, rect, pb))
            dl->AddLine(pa, pb, col, thickness);
    }

    static bool DrawDirectionalLight(DirectLight& light, const Camera3D& camera, const ViewportRect& rect, ImDrawList* dl, int gizmoId = 2, float iconScale = 1.0f)
    {
        // ImGuizmo setup (use the same window drawlist & exact rect)
        ImGuizmo::PushID(gizmoId);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(dl);
        ImGuizmo::SetRect(rect.min.x, rect.min.y, rect.width(), rect.height());
        ImGuizmo::AllowAxisFlip(false);
        ImGuizmo::SetGizmoSizeClipSpace(0.18f);

        glm::vec3 pos = ChooseDummpyPosition(camera, 6.0f);
        glm::mat3 R   = MakeRotationFromDirection(light.Direction);
        glm::mat4 model(1.0f);
        model[0] = glm::vec4(R[0], 0.0f);
        model[1] = glm::vec4(R[1], 0.0f);
        model[2] = glm::vec4(R[2], 0.0f);
        model[3] = glm::vec4(pos,   1.0f);
        if (iconScale != 1.0f)
            model = model * glm::scale(glm::mat4(1.0f), glm::vec3(iconScale));

        glm::mat4 view = camera.View;
        glm::mat4 proj = camera.Projection;

        bool changed = false;
        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::ROTATE, ImGuizmo::LOCAL, glm::value_ptr(model)))
        {
            glm::vec3 col0 = glm::vec3(model[0]);
            glm::vec3 col1 = glm::vec3(model[1]);
            glm::vec3 col2 = glm::vec3(model[2]);
            if (glm::length2(col0) > 0) model[0] = glm::vec4(glm::normalize(col0), 0.0f);
            if (glm::length2(col1) > 0) model[1] = glm::vec4(glm::normalize(col1), 0.0f);
            if (glm::length2(col2) > 0) model[2] = glm::vec4(glm::normalize(col2), 0.0f);

            glm::vec3 newDir = ExtractDirectionFromMatrix(model);
            if (glm::length2(newDir) > 0.0f) 
            {
                light.Direction = glm::normalize(newDir);
                changed = true;
            }
        }

        {
            auto worldToScreen = [&](const glm::vec3& p) -> ImVec2 
            {
                glm::vec4 clip = proj * view * glm::vec4(p, 1.0f);
                float iw = (clip.w == 0.0f) ? 1e-6f : clip.w;
                glm::vec3 ndc = glm::vec3(clip) / iw;
                ImVec2 out;
                out.x = rect.min.x + (ndc.x * 0.5f + 0.5f) * rect.width();
                out.y = rect.min.y + (-ndc.y * 0.5f + 0.5f) * rect.height();
                return out;
            };

            glm::vec3 iconPos   = pos;
            glm::vec3 iconAhead = pos + glm::normalize(-light.Direction) * 1.5f;
            dl->AddCircleFilled(worldToScreen(iconPos), 4.0f, IM_COL32(255, 255, 0, 255));
            dl->AddLine(worldToScreen(iconPos), worldToScreen(iconAhead), IM_COL32(255, 255, 0, 255), 2.0f);
        }

        ImGuizmo::PopID();
        return changed;
    }

    void SceneViewportPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        ImGui::Begin(std::format("{}##SceneViewport", context.ActiveScene->GetName()).c_str());

        const bool focused_or_hovered = ImGui::IsWindowFocused() || ImGui::IsWindowHovered();
        if (context.UILayerInstance) context.UILayerInstance->AcceptEvents(focused_or_hovered);

        const ImVec2 vpAvail = ImGui::GetContentRegionAvail();
        FrameTextureID tex = context.ActiveViewportTexture;
        if (tex != 0)
            ImGui::Image((ImTextureID)(intptr_t)tex, vpAvail, ImVec2(0, 1), ImVec2(1, 0));
        else
            ImGui::Dummy(vpAvail);

        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 crMin  = ImGui::GetWindowContentRegionMin();
        ImVec2 crMax  = ImGui::GetWindowContentRegionMax();
        ImVec2 vpMin  = { winPos.x + crMin.x, winPos.y + crMin.y };
        ImVec2 vpMax  = { winPos.x + crMax.x, winPos.y + crMax.y };
        ViewportRect rect{ vpMin, vpMax };
        ImDrawList* windowDL = ImGui::GetWindowDrawList();

        context.ActiveSceneSpecification.Viewport.MIN = { vpMin.x, vpMin.y };
        context.ActiveSceneSpecification.Viewport.MAX = { vpMax.x, vpMax.y };

        ImVec2 mouse = ImGui::GetMousePos();
        if (!context.ActiveScene->InSimulationMode())
        {
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
                ImGui::IsWindowFocused() &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !ImGuizmo::IsUsing()) 
            {
                glm::vec2 local = { mouse.x - vpMin.x, mouse.y - vpMin.y };
                local.y = vpAvail.y - local.y;

                glm::vec2 fbSize = {
                    (float)context.ActiveSceneSpecification.Viewport.FrameSpec.Width,
                    (float)context.ActiveSceneSpecification.Viewport.FrameSpec.Height
                };
                glm::vec2 mouseInFB = {
                    local.x * (fbSize.x / vpAvail.x),
                    local.y * (fbSize.y / vpAvail.y)
                };

                if (auto picked = context.ActiveScene->PickEntity(mouseInFB, fbSize))
                    context.ActiveScene->SelectedEntity(picked);
            }

            glm::mat4 view       = context.ActiveScene->GetCameraView();
            glm::mat4 projection = context.ActiveScene->GetCameraProjection();

            ImGuizmo::SetDrawlist(windowDL);
            ImGuizmo::SetRect(vpMin.x, vpMin.y, rect.width(), rect.height());
            ImGuizmo::SetGizmoSizeClipSpace(0.18f);
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::AllowAxisFlip(false);

            static ImGuizmo::OPERATION gizmoOp   = ImGuizmo::TRANSLATE;
            static ImGuizmo::MODE      gizmoMode = ImGuizmo::LOCAL;

            if (ImGui::IsWindowFocused())
            {
                if (ImGui::IsKeyDown(ImGuiKey_1)) gizmoOp = ImGuizmo::TRANSLATE;
                if (ImGui::IsKeyDown(ImGuiKey_2)) gizmoOp = ImGuizmo::ROTATE;
                if (ImGui::IsKeyDown(ImGuiKey_3)) gizmoOp = ImGuizmo::SCALE;
                if (ImGui::IsKeyDown(ImGuiKey_5)) gizmoMode = (gizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            }

            const bool snapToggle = (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) && (ImGui::IsKeyDown(ImGuiKey_Space));
            float snap[3] = { 0.f, 0.f, 0.f };
            if (snapToggle)
            {
                switch (gizmoOp)
                {
                    case ImGuizmo::TRANSLATE: snap[0] = snap[1] = snap[2] = 0.1f; break;
                    case ImGuizmo::ROTATE:    snap[0] = snap[1] = snap[2] = 5.0f;  break;
                    case ImGuizmo::SCALE:     snap[0] = snap[1] = snap[2] = 0.05f; break;
                    default: break;
                }
            }

            bool gizmoConsumedInput = false;
            if (auto sel = context.ActiveScene->GetSelectedEntity();
                sel && sel != EntityFactory::EMPTYENTITY &&
                sel->HasComponent<TransformComponent>() &&
                sel->GetComponent<TagComponent>().IsActive)
            {
                ImGuizmo::PushID(1); // IMPORTANT: give entity gizmo its own ID

                auto& TRS = sel->GetComponent<TransformComponent>();
                glm::vec3 T = TRS.Translation;
                glm::vec3 S = TRS.Scale;
                glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(TRS.Rotation));
                auto wrap180 = [](float a)
                {
                    // Wrap to (-180, 180]
                    a = std::fmod(a + 180.0f, 360.0f);
                    if (a < 0) a += 360.0f;
                    return a - 180.0f;
                };
                eulerDeg.x = wrap180(eulerDeg.x);
                eulerDeg.y = wrap180(eulerDeg.y);
                eulerDeg.z = wrap180(eulerDeg.z);
                glm::vec3 Rdeg = eulerDeg;

                glm::mat4 transform{1.0f};
                ImGuizmo::RecomposeMatrixFromComponents(&T.x, &Rdeg.x, &S.x, glm::value_ptr(transform));

                if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), gizmoOp, gizmoMode, glm::value_ptr(transform), nullptr, snapToggle ? snap : nullptr))
                {
                    gizmoConsumedInput = true;
                    float Td[3], RdDeg[3], Sd[3];
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform), Td, RdDeg, Sd);

                    TRS.Translation = { Td[0], Td[1], Td[2] };
                    TRS.Scale       = { Sd[0], Sd[1], Sd[2] };

                    glm::vec3 RdRad     = glm::radians(glm::vec3(RdDeg[0], RdDeg[1], RdDeg[2]));
                    glm::quat q         = glm::normalize(glm::quat(RdRad));
                    if (glm::any(glm::epsilonNotEqual(q, TRS.Rotation, 1e-6f))) TRS.Rotation = q;

                    if(sel->HasComponent<ColliderComponent>() && sel->HasComponent<RigidBodyComponent>())
                    {
                        auto& col   = sel->GetComponent<ColliderComponent>();
                        auto& rb    = sel->GetComponent<RigidBodyComponent>();
                        
                        col.CalcMassFromColliders(rb, rb.Density);
                        col.UpdateWorldAABB(TRS);
                        rb.SyncInertia(TRS);
                    }
                }

                ImGuizmo::PopID();
            }

            bool lightGuizmoEnabled =  ImGui::IsKeyDown(ImGuiKey_4);
            if (!gizmoConsumedInput && context.ActiveSceneSpecification.Environment.Sun.ShowLightDirectionGuizmo)
            {
                context.ActiveSceneSpecification.Environment.Sun.ShowLightDirectionGuizmo = !lightGuizmoEnabled;
                DrawDirectionalLight(context.ActiveScene->GetEnvironment().Sun, context.ActiveCamera.Camera, rect, windowDL, 2, 1.0f);
            }
        }

        if (context.EditorLayerInstance && context.ActiveScene)
        {
            auto& viewport = context.ActiveSceneSpecification.Viewport;
            if (viewport.Size.x != vpAvail.x || viewport.Size.y != vpAvail.y)
                context.EditorLayerInstance->SetViewportSize({ vpAvail.x, vpAvail.y });
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
