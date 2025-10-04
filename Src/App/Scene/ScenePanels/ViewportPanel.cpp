#include "CorePCH.hpp"
#include "SceneEditorLayer.hpp"
#include "ViewportPanel.hpp"

namespace Motion
{
    namespace detail
    {
        static constexpr float kEpsilon = 1e-6f;

        inline glm::mat3 MakeRotationFromDirection(const glm::vec3& dir, const glm::vec3& upHint = {0,1,0})
        {
            glm::vec3 fwd   = glm::normalize(-dir);
            glm::vec3 right = glm::cross(upHint, fwd);
            if (glm::length2(right) < 1e-8f)
            {
                const glm::vec3 altUp = std::abs(upHint.y) > 0.5f ? glm::vec3(0,0,1) : glm::vec3(0,1,0);
                right = glm::cross(altUp, fwd);
            }
            right        = glm::normalize(right);
            const glm::vec3 up = glm::normalize(glm::cross(fwd, right));
            return { right, up, fwd };
        }

        inline glm::vec3 ExtractDirectionFromMatrix(const glm::mat4& M)
        {
            // Column 2 is the forward axis of the model matrix
            const glm::vec3 fwd = glm::normalize(glm::vec3(M[2]));
            return -fwd;
        }

        inline glm::vec3 ChooseDummyPosition(const Camera3D& cam, float distance = 6.0f)
        {
            const glm::mat4 invView = glm::inverse(cam.View);
            const glm::vec3 camFwd  = glm::normalize(glm::vec3(invView[2]) * -1.0f);
            return cam.Position + camFwd * distance;
        }
    }

    // ----------------------------- Screen helpers ----------------------------- //
    struct ViewportRect
    {
        ImVec2 min;
        ImVec2 max;
        float  width()  const { return max.x - min.x; }
        float  height() const { return max.y - min.y; }
    };

    static bool WorldToScreen(const glm::vec3& p, const glm::mat4& VP, const ViewportRect& r, ImVec2& out)
    {
        const glm::vec4 clip = VP * glm::vec4(p, 1.0f);
        if (clip.w <= 0.0001f) return false;
        const glm::vec3 ndc = glm::vec3(clip) / clip.w; // [-1,1]
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

    struct GizmoState
    {
        ImGuizmo::OPERATION Operation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE      Mode      = ImGuizmo::LOCAL;

        // Snapping values per-op; zero disables per ImGuizmo
        float TranslateSnap = 0.0f;    // meters
        float RotateSnapDeg = 0.0f;    // degrees
        float ScaleSnap     = 0.0f;    // scale units

        // Hotkeys (ImGuiKey values)
        ImGuiKey KeyTranslate = ImGuiKey_1;
        ImGuiKey KeyRotate    = ImGuiKey_2;
        ImGuiKey KeyScale     = ImGuiKey_3;
        ImGuiKey KeyToggleMode= ImGuiKey_5; // WORLD <-> LOCAL

        // Snap modifiers
        ImGuiKey KeySnap      = ImGuiKey_LeftCtrl;     // hold for coarse snap
        ImGuiKey KeyFineSnap  = ImGuiKey_LeftShift;    // hold for fine snap

        // Two snap tiers
        float TranslateSnapCoarse = 0.1f;
        float RotateSnapCoarseDeg = 5.0f;
        float ScaleSnapCoarse     = 0.05f;

        float TranslateSnapFine   = 0.01f;
        float RotateSnapFineDeg   = 1.0f;
        float ScaleSnapFine       = 0.01f;

        // Compute current snap triplet for ImGuizmo based on held keys
        void FillSnapTriplet(float outSnap[3]) const
        {
            const bool coarse = ImGui::IsKeyDown(KeySnap) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
            const bool fine   = ImGui::IsKeyDown(KeyFineSnap) || ImGui::IsKeyDown(ImGuiKey_RightShift);

            float sT = 0.f, sR = 0.f, sS = 0.f;
            if (coarse)
            {
                sT = TranslateSnapCoarse; sR = RotateSnapCoarseDeg; sS = ScaleSnapCoarse;
            }
            else if (fine)
            {
                sT = TranslateSnapFine;   sR = RotateSnapFineDeg;   sS = ScaleSnapFine;
            }
            else
            {
                sT = TranslateSnap;       sR = RotateSnapDeg;       sS = ScaleSnap;
            }

            switch (Operation)
            {
                case ImGuizmo::TRANSLATE: outSnap[0] = outSnap[1] = outSnap[2] = sT; break;
                case ImGuizmo::ROTATE:    outSnap[0] = outSnap[1] = outSnap[2] = sR; break;
                case ImGuizmo::SCALE:     outSnap[0] = outSnap[1] = outSnap[2] = sS; break;
                default:                  outSnap[0] = outSnap[1] = outSnap[2] = 0.f; break;
            }
        }

        void HandleHotkeys()
        {
            if (!ImGui::IsWindowFocused()) return;
            if (ImGui::IsKeyPressed(KeyTranslate)) Operation = ImGuizmo::TRANSLATE;
            if (ImGui::IsKeyPressed(KeyRotate))    Operation = ImGuizmo::ROTATE;
            if (ImGui::IsKeyPressed(KeyScale))     Operation = ImGuizmo::SCALE;
            if (ImGui::IsKeyPressed(KeyToggleMode))
                Mode = (Mode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
        }
    };

    struct LightGizmoConfig
    {
        int   GizmoId         = 2;
        bool  Enabled         = true;
        float CameraDistance  = 6.0f;    // where the icon is placed
        float IconScale       = 1.0f;    // model scale
        float IconLength      = 1.5f;    // arrow length
        ImU32 IconColor       = IM_COL32(255, 255, 0, 255);
        bool  DrawBillboard   = true;    // simple 2D icon for readability
        bool  LockToViewAxis  = false;   // if true, rotates only around view axis
        bool  AllowAxisFlip   = false;   // ImGuizmo axis flip
        float GizmoSizeClip   = 0.18f;   // ImGuizmo size in clip space
        bool  UseLocalSpace   = true;    // LOCAL vs WORLD for rotation

        bool  DrawRays        = true;
        float RayLength       = 0.75f;
        int   RayCount        = 6;
    };

    static bool DrawDirectionalLight(DirectLight& light, const Camera3D& camera, const ViewportRect& rect, ImDrawList* dl, const LightGizmoConfig& cfg)
    {
        ImGuizmo::PushID(cfg.GizmoId);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(dl);
        ImGuizmo::SetRect(rect.min.x, rect.min.y, rect.width(), rect.height());
        ImGuizmo::AllowAxisFlip(cfg.AllowAxisFlip);
        ImGuizmo::SetGizmoSizeClipSpace(cfg.GizmoSizeClip);

        glm::vec3 pos = detail::ChooseDummyPosition(camera, cfg.CameraDistance);
        glm::mat3 R   = detail::MakeRotationFromDirection(light.Direction);

        glm::mat4 model(1.0f);
        model[0] = glm::vec4(R[0], 0.0f);
        model[1] = glm::vec4(R[1], 0.0f);
        model[2] = glm::vec4(R[2], 0.0f);
        model[3] = glm::vec4(pos,   1.0f);
        if (cfg.IconScale != 1.0f)
            model = model * glm::scale(glm::mat4(1.0f), glm::vec3(cfg.IconScale));

        glm::mat4 view = camera.View;
        glm::mat4 proj = camera.Projection;

        bool changed = false;
        const ImGuizmo::MODE rotMode = cfg.UseLocalSpace ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::ROTATE, rotMode, glm::value_ptr(model)))
        {
            // Re-orthonormalize basis to prevent drift
            glm::vec3 c0 = glm::vec3(model[0]);
            glm::vec3 c1 = glm::vec3(model[1]);
            glm::vec3 c2 = glm::vec3(model[2]);
            if (glm::length2(c0) > 0) model[0] = glm::vec4(glm::normalize(c0), 0.0f);
            if (glm::length2(c1) > 0) model[1] = glm::vec4(glm::normalize(c1), 0.0f);
            if (glm::length2(c2) > 0) model[2] = glm::vec4(glm::normalize(c2), 0.0f);

            glm::vec3 newDir = detail::ExtractDirectionFromMatrix(model);
            if (glm::length2(newDir) > 0.0f)
            {
                if (cfg.LockToViewAxis)
                {
                    // Constrain rotation to only yaw around camera forward (screen-space twist)
                    const glm::vec3 camFwd = -glm::vec3(glm::inverse(view)[2]);
                    glm::vec3 axis = glm::normalize(camFwd);
                    newDir = glm::normalize(newDir - axis * glm::dot(newDir, axis));
                }
                light.Direction = glm::normalize(newDir);
                changed = true;
            }
        }

        // 2D overlay icon for readability (always on top in the viewport)
        if (cfg.DrawBillboard)
        {
            auto worldToScreen = [&](const glm::vec3& p) -> ImVec2 \
            {
                const glm::mat4 VP = proj * view;
                glm::vec4 clip = VP * glm::vec4(p, 1.0f);
                float iw = (clip.w == 0.0f) ? 1e-6f : clip.w;
                glm::vec3 ndc = glm::vec3(clip) / iw;
                ImVec2 out;
                out.x = rect.min.x + (ndc.x * 0.5f + 0.5f) * rect.width();
                out.y = rect.min.y + (-ndc.y * 0.5f + 0.5f) * rect.height();
                return out;
            };

            const glm::vec3 iconPos   = pos;
            const glm::vec3 iconAhead = pos + glm::normalize(-light.Direction) * cfg.IconLength;
            dl->AddCircleFilled(worldToScreen(iconPos), 4.0f * cfg.IconScale, cfg.IconColor);
            dl->AddLine(worldToScreen(iconPos), worldToScreen(iconAhead), cfg.IconColor, 2.0f * cfg.IconScale);

            if (cfg.DrawRays)
            {
                const glm::vec3 fwd = glm::normalize(-light.Direction);
                // Build a small orthonormal basis around fwd to scatter rays
                glm::vec3 t = glm::normalize(glm::cross(fwd, glm::vec3(0,1,0)));
                if (glm::length2(t) < 1e-5f) t = glm::vec3(1,0,0);
                glm::vec3 b = glm::normalize(glm::cross(fwd, t));

                for (int i = 0; i < cfg.RayCount; ++i)
                {
                    const float a = (glm::two_pi<float>() / cfg.RayCount) * i;
                    const glm::vec3 dir = glm::normalize(t * std::cos(a) + b * std::sin(a));
                    const glm::vec3 a0 = iconPos + dir * (0.2f * cfg.IconScale);
                    const glm::vec3 a1 = iconPos + dir * (0.2f + cfg.RayLength) * cfg.IconScale;
                    dl->AddLine(worldToScreen(a0), worldToScreen(a1), cfg.IconColor, 1.0f);
                }
            }
        }

        ImGuizmo::PopID();
        return changed;
    }

    void SceneViewportPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        ImGui::Begin(std::format("{}##SceneViewport", context.ActiveScene->GetName()).c_str());

        const ImVec2 vpAvail = ImGui::GetContentRegionAvail();
        const bool focused_or_hovered = ImGui::IsWindowFocused() || ImGui::IsWindowHovered();
        if (context.UIInstance) context.UIInstance->AcceptEvents(focused_or_hovered);

        {
            FrameTextureID tex = context.ViewportTexture;
            if (tex != 0)
                ImGui::Image((ImTextureID)(intptr_t)tex, vpAvail, ImVec2(0, 1), ImVec2(1, 0));
            else
                ImGui::Dummy(vpAvail);
        }

        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 crMin  = ImGui::GetWindowContentRegionMin();
        ImVec2 crMax  = ImGui::GetWindowContentRegionMax();
        ImVec2 vpMin  = { winPos.x + crMin.x, winPos.y + crMin.y };
        ImVec2 vpMax  = { winPos.x + crMax.x, winPos.y + crMax.y };
        ViewportRect rect{ vpMin, vpMax };
        ImDrawList* windowDL = ImGui::GetWindowDrawList();

        context.ActiveScene->GetSpecification().Viewport.MIN = { vpMin.x, vpMin.y };
        context.ActiveScene->GetSpecification().Viewport.MAX = { vpMax.x, vpMax.y };

        const ImVec2 mouse = ImGui::GetMousePos();

        if (!context.ActiveScene->InSimulationMode())
        {
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsWindowFocused() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing())
            {
                glm::vec2 local = { mouse.x - vpMin.x, mouse.y - vpMin.y };
                local.y = vpAvail.y - local.y;

                const glm::vec2 fbSize = 
                {
                    (float)context.ActiveScene->GetSpecification().Viewport.FrameSpec.Width,
                    (float)context.ActiveScene->GetSpecification().Viewport.FrameSpec.Height
                };

                const glm::vec2 mouseInFB = 
                {
                    local.x * (fbSize.x / vpAvail.x),
                    local.y * (fbSize.y / vpAvail.y)
                };

                if (auto picked = context.ActiveScene->PickEntity(mouseInFB, fbSize))
                    context.ActiveScene->SelectedEntity(picked);
            }

            glm::mat4 view       = context.ActiveScene->GetCameraView();
            glm::mat4 projection = context.ActiveScene->GetCameraProjection();

            static GizmoState gizmo;
            gizmo.HandleHotkeys();

            ImGuizmo::SetDrawlist(windowDL);
            ImGuizmo::SetRect(vpMin.x, vpMin.y, rect.width(), rect.height());
            ImGuizmo::SetGizmoSizeClipSpace(0.18f);
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::AllowAxisFlip(false);

            bool gizmoConsumedInput = false;

            float snapTriplet[3] = {0,0,0};
            gizmo.FillSnapTriplet(snapTriplet);
            const float* snapPtr = (snapTriplet[0] != 0 || snapTriplet[1] != 0 || snapTriplet[2] != 0) ? snapTriplet : nullptr;
            if (auto sel = context.ActiveScene->GetSelectedEntity(); sel && sel != Entity::Empty() && sel->Has<TransformComponent>() && sel->Get<TagComponent>().IsActive)
            {
                ImGuizmo::PushID(1);
                auto& TRS = sel->Get<TransformComponent>();
                glm::vec3 T = TRS.Translation;
                glm::vec3 S = TRS.Scale;
                glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(TRS.Rotation));
                auto wrap180 = [](float a)
                {
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

                if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), gizmo.Operation, gizmo.Mode, glm::value_ptr(transform), nullptr, snapPtr))
                {
                    gizmoConsumedInput = true;
                    float Td[3], RdDeg[3], Sd[3];
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform), Td, RdDeg, Sd);

                    TRS.Translation = { Td[0], Td[1], Td[2] };
                    TRS.Scale       = { Sd[0], Sd[1], Sd[2] };

                    const glm::vec3 RdRad = glm::radians(glm::vec3(RdDeg[0], RdDeg[1], RdDeg[2]));
                    const glm::quat q     = glm::normalize(glm::quat(RdRad));
                    if (glm::any(glm::epsilonNotEqual(q, TRS.Rotation, 1e-6f))) TRS.Rotation = q;
                }

                ImGuizmo::PopID();
            }
            {
                const bool clutchHide = ImGui::IsKeyDown(ImGuiKey_4);
                static LightGizmoConfig lightCfg;
                lightCfg.Enabled = context.ActiveScene->GetEnvironment().Sun.ShowLightDirectionGuizmo && !clutchHide;

                if (!gizmoConsumedInput && lightCfg.Enabled)
                {
                    context.ActiveScene->GetEnvironment().Sun.ShowLightDirectionGuizmo = true;
                    DrawDirectionalLight(context.ActiveScene->GetEnvironment().Sun, context.ActiveScene->GetCamera(), rect, windowDL, lightCfg);
                }
            }
        }

        if (context.EditorInstance && context.ActiveScene)
        {
            auto& viewport = context.ActiveScene->GetSpecification().Viewport;
            if (viewport.Size.x != vpAvail.x || viewport.Size.y != vpAvail.y)
                context.EditorInstance->SetViewportSize({ vpAvail.x, vpAvail.y });
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
