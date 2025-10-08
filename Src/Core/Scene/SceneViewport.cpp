#include "CorePCH.hpp"
#include <reactphysics3d/reactphysics3d.h>

namespace Motion
{
    static constexpr float kEpsilon = 1e-6f;

    struct ViewportRect
    {
        ImVec2 min{}, max{};
        float width()  const { return max.x - min.x; }
        float height() const { return max.y - min.y; }
    };

    static ViewportRect ComputeViewportRect()
    {
        const ImVec2 winPos = ImGui::GetWindowPos();
        const ImVec2 crMin  = ImGui::GetWindowContentRegionMin();
        const ImVec2 crMax  = ImGui::GetWindowContentRegionMax();
        return { { winPos.x + crMin.x, winPos.y + crMin.y }, { winPos.x + crMax.x, winPos.y + crMax.y } };
    }

    static bool WorldToScreen(const glm::vec3& p, const glm::mat4& VP, const ViewportRect& rect, ImVec2& out)
    {
        glm::vec4 clip = VP * glm::vec4(p, 1.0f);
        if (clip.w <= 0.0001f) return false;
        glm::vec3 ndc = glm::vec3(clip) / clip.w;                 // [-1, 1]
        if (ndc.x < -1.2f || ndc.x > 1.2f || ndc.y < -1.2f || ndc.y > 1.2f) return false;

        out.x = rect.min.x + (ndc.x * 0.5f + 0.5f) * rect.width();
        out.y = rect.min.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * rect.height();
        return true;
    }

    struct RayWS { glm::vec3 Origin; glm::vec3 Direction; };

    static RayWS BuildMouseRayFromFB(const glm::vec2& mouseFB, const glm::vec2& fbSize, const glm::mat4& view, const glm::mat4& proj)
    {
        const float ndcX =  (mouseFB.x / fbSize.x) * 2.0f - 1.0f;
        const float ndcY =  (mouseFB.y / fbSize.y) * 2.0f - 1.0f;

        const glm::mat4 invVP = glm::inverse(proj * view);

        glm::vec4 pNear = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 pFar  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);
        pNear /= pNear.w;
        pFar  /= pFar.w;

        RayWS r;
        r.Origin    = glm::vec3(pNear);
        r.Direction = glm::normalize(glm::vec3(pFar - pNear));
        return r;
    }

    inline glm::mat3 MakeRotationFromDirection(const glm::vec3& dir, const glm::vec3& upHint = {0,1,0})
    {
        glm::vec3 fwd   = glm::normalize(-dir);
        glm::vec3 right = glm::cross(upHint, fwd);
        if (glm::length2(right) < 1e-8f)
        {
            const glm::vec3 altUp = std::abs(upHint.y) > 0.5f ? glm::vec3(0,0,1) : glm::vec3(0,1,0);
            right = glm::cross(altUp, fwd);
        }
        right = glm::normalize(right);
        const glm::vec3 up = glm::normalize(glm::cross(fwd, right));
        return { right, up, fwd };
    }

    static glm::vec3 ExtractDirectionFromMatrix(const glm::mat4& M)
    {
        const glm::vec3 fwd = glm::normalize(glm::vec3(M[2]));
        return -fwd;
    }

    static glm::vec3 ChooseDummyPosition(const Camera3D& cam, float distance = 6.0f)
    {
        const glm::mat4 invView = glm::inverse(cam.View);
        const glm::vec3 camFwd  = glm::normalize(glm::vec3(invView[2]) * -1.0f);
        return cam.Position + camFwd * distance;
    }

    struct GizmoState
    {
        ImGuizmo::OPERATION Operation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE      Mode      = ImGuizmo::LOCAL;

        float TranslateSnap = 0.0f;
        float RotateSnapDeg = 0.0f;
        float ScaleSnap     = 0.0f;

        ImGuiKey KeyTranslate = ImGuiKey_1;
        ImGuiKey KeyRotate    = ImGuiKey_2;
        ImGuiKey KeyScale     = ImGuiKey_3;
        ImGuiKey KeyToggleMode= ImGuiKey_5;

        ImGuiKey KeySnap      = ImGuiKey_LeftCtrl;
        ImGuiKey KeyFineSnap  = ImGuiKey_LeftShift;

        float TranslateSnapCoarse = 0.1f;
        float RotateSnapCoarseDeg = 5.0f;
        float ScaleSnapCoarse     = 0.05f;

        float TranslateSnapFine   = 0.01f;
        float RotateSnapFineDeg   = 1.0f;
        float ScaleSnapFine       = 0.01f;

        void FillSnapTriplet(float outSnap[3]) const
        {
            const bool coarse = ImGui::IsKeyDown(KeySnap) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
            const bool fine   = ImGui::IsKeyDown(KeyFineSnap) || ImGui::IsKeyDown(ImGuiKey_RightShift);

            float sT = 0.f, sR = 0.f, sS = 0.f;
            if (coarse)      { sT = TranslateSnapCoarse; sR = RotateSnapCoarseDeg; sS = ScaleSnapCoarse; }
            else if (fine)   { sT = TranslateSnapFine;   sR = RotateSnapFineDeg;   sS = ScaleSnapFine;   }
            else             { sT = TranslateSnap;       sR = RotateSnapDeg;       sS = ScaleSnap;       }

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
        float CameraDistance  = 6.0f;
        float IconScale       = 1.0f;
        float IconLength      = 1.5f;
        ImU32 IconColor       = IM_COL32(255, 255, 0, 255);
        bool  DrawBillboard   = true;
        bool  LockToViewAxis  = false;
        bool  AllowAxisFlip   = false;
        float GizmoSizeClip   = 0.18f;
        bool  UseLocalSpace   = true;

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

        const glm::vec3 pos = ChooseDummyPosition(camera, cfg.CameraDistance);
        const glm::mat3 R   = MakeRotationFromDirection(light.Direction);

        glm::mat4 model(1.0f);
        model[0] = glm::vec4(R[0], 0.0f);
        model[1] = glm::vec4(R[1], 0.0f);
        model[2] = glm::vec4(R[2], 0.0f);
        model[3] = glm::vec4(pos,   1.0f);
        if (cfg.IconScale != 1.0f)
            model = model * glm::scale(glm::mat4(1.0f), glm::vec3(cfg.IconScale));

        const glm::mat4 view = camera.View;
        const glm::mat4 proj = camera.Projection;

        bool changed = false;
        const ImGuizmo::MODE rotMode = cfg.UseLocalSpace ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::ROTATE, rotMode, glm::value_ptr(model)))
        {
            glm::vec3 c0 = glm::vec3(model[0]);
            glm::vec3 c1 = glm::vec3(model[1]);
            glm::vec3 c2 = glm::vec3(model[2]);

            if (glm::length2(c0) > 0) model[0] = glm::vec4(glm::normalize(c0), 0.0f);
            if (glm::length2(c1) > 0) model[1] = glm::vec4(glm::normalize(c1), 0.0f);
            if (glm::length2(c2) > 0) model[2] = glm::vec4(glm::normalize(c2), 0.0f);

            glm::vec3 newDir = ExtractDirectionFromMatrix(model);
            if (glm::length2(newDir) > 0.0f)
            {
                if (cfg.LockToViewAxis)
                {
                    const glm::vec3 camFwd = -glm::vec3(glm::inverse(view)[2]);
                    const glm::vec3 axis   = glm::normalize(camFwd);
                    newDir = glm::normalize(newDir - axis * glm::dot(newDir, axis));
                }

                light.Direction = glm::normalize(newDir);
                changed = true;
            }
        }

        if (cfg.DrawBillboard)
        {
            const glm::mat4 VP = proj * view;

            auto worldToScreen = [&](const glm::vec3& p) -> ImVec2
            {
                glm::vec4 clip = VP * glm::vec4(p, 1.0f);
                const float iw = (clip.w == 0.0f) ? kEpsilon : clip.w;
                const glm::vec3 ndc = glm::vec3(clip) / iw;
                ImVec2 out{};
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
                glm::vec3 t = glm::normalize(glm::cross(fwd, glm::vec3(0,1,0)));
                if (glm::length2(t) < 1e-5f) t = glm::vec3(1,0,0);
                const glm::vec3 b = glm::normalize(glm::cross(fwd, t));

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

    static void DrawSelectedOutline_UsingRp3dDebug(ScenePanelContext& context, const ViewportRect& rect, ImDrawList* dl)
    {

    }

    void SceneViewportPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});

        const std::string title = std::format("{}##SceneViewport", context.ScenePointer->GetName());
        ImGui::Begin(title.c_str());

        const ImVec2 vpAvail = ImGui::GetContentRegionAvail();
        if (FrameTextureID tex = context.FrameTexture; tex != 0)
            ImGui::Image((ImTextureID)tex, vpAvail, ImVec2(0, 1), ImVec2(1, 0));
        else
            ImGui::Dummy(vpAvail);

        const ViewportRect rect = ComputeViewportRect();
        ImDrawList* windowDL = ImGui::GetWindowDrawList();

        const ImVec2 mouse = ImGui::GetMousePos();
        if (!context.ScenePointer->InSimulation())
        {
            const Camera3D& camera      = context.ScenePointer->GetCamera();
            const glm::mat4 view        = camera.View;
            const glm::mat4 projection  = camera.Projection;

            const bool canPick =
                ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
                ImGui::IsWindowFocused() &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !ImGuizmo::IsUsing();

            if (canPick)
            {
                glm::vec2 local = { mouse.x - rect.min.x, mouse.y - rect.min.y };
                local.y         = vpAvail.y - local.y;

                const auto& fbSpec          = context.ScenePointer->GetSceneFrameSpecification();
                const glm::vec2 fbSize      = { (float)fbSpec.Width, (float)fbSpec.Height };
                const glm::vec2 mouseInFB   = { local.x * (fbSize.x / vpAvail.x), local.y * (fbSize.y / vpAvail.y) };
                const RayWS ray             = BuildMouseRayFromFB(mouseInFB, fbSize, view, projection);

                constexpr float kMaxDistance    = 1000.0f;
                const glm::vec3 P0              = ray.Origin;
                const glm::vec3 P1              = ray.Origin + ray.Direction * kMaxDistance;

                RayHitResults result{};
                const bool hit = RaycastFirstHit(context.PhysicsWorld, P0, P1, result);
                if (hit) context.ScenePointer->SelectedEntity(result.Entity);
            }

            static GizmoState gizmo;
            gizmo.HandleHotkeys();
            ImGuizmo::SetDrawlist(windowDL);
            ImGuizmo::SetRect(rect.min.x, rect.min.y, rect.width(), rect.height());
            ImGuizmo::SetGizmoSizeClipSpace(0.18f);
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::AllowAxisFlip(false);

            bool gizmoConsumedInput = false;
            float snapTriplet[3] = {0,0,0};
            gizmo.FillSnapTriplet(snapTriplet);
            const float* snapPtr = (snapTriplet[0] != 0 || snapTriplet[1] != 0 || snapTriplet[2] != 0) ? snapTriplet : nullptr;

            const entt::entity selection = context.ScenePointer->GetSelectedEntity();
            if (selection != entt::null)
            {
                const bool hasTransform  = context.SceneRegistry->any_of<TransformComponent>(selection);
                TagComponent* tag        = context.SceneRegistry->try_get<TagComponent>(selection);
                const bool isActive      = tag ? tag->IsActive : false;

                if (isActive && hasTransform)
                {
                    ImGuizmo::PushID(1);
                    auto& TRS = context.SceneRegistry->get<TransformComponent>(selection);

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

                    glm::mat4 transform{1.0f};
                    ImGuizmo::RecomposeMatrixFromComponents(&T.x, &eulerDeg.x, &S.x, glm::value_ptr(transform));

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
            }

            const bool clutchHide = ImGui::IsKeyDown(ImGuiKey_4);
            static LightGizmoConfig lightCfg;
            lightCfg.Enabled = context.ScenePointer->GetEnvironment().Sun.ShowDir && !clutchHide;
            if (!gizmoConsumedInput && lightCfg.Enabled)
            {
                context.ScenePointer->GetEnvironment().Sun.ShowDir = true;
                (void)DrawDirectionalLight(context.ScenePointer->GetEnvironment().Sun, context.ScenePointer->GetCamera(), rect, windowDL, lightCfg);
            }

            //DrawSelectedOutline_UsingRp3dDebug(context, rect, windowDL);
        }

        if (context.ScenePointer)
        {
            auto& viewport = context.ScenePointer->GetSceneFrameSpecification();
            if ((float)viewport.Width != vpAvail.x || (float)viewport.Height != vpAvail.y)
                context.ScenePointer->SetApectRatio({ vpAvail.x, vpAvail.y });
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

} 
