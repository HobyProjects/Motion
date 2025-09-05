#include "CorePCH.hpp"
#include "SceneEditorLayer.hpp"
#include "ViewportPanel.hpp"

namespace Motion
{
    struct EditorRay 
    {
        glm::vec3 origin;
        glm::vec3 dir; // normalized
    };

    static EditorRay BuildPickingRayFromFramebufferPixel(
        const glm::vec2& mouseInFB,     // pixel in your framebuffer
        const glm::vec2& fbSize,        // framebuffer size in pixels
        const glm::mat4& projection,    // camera projection
        const glm::mat4& view,          // camera view
        const glm::vec3& camPos,        // camera world position
        bool perspective = true         // set false for ortho cameras
    )
    {
        // 1) framebuffer pixel -> Normalized Device Coordinates [-1, +1]
        const float x = (2.0f * mouseInFB.x) / fbSize.x - 1.0f;
        const float y = 1.0f - (2.0f * mouseInFB.y) / fbSize.y; // flip Y

        // 2) Build clip-space points on near (+far) planes
        const glm::vec4 clipNear(x, y, -1.0f, 1.0f);
        const glm::vec4 clipFar (x, y,  1.0f, 1.0f);

        const glm::mat4 invPV = glm::inverse(projection * view);

        // 3) Unproject to world
        glm::vec4 worldNear = invPV * clipNear;
        glm::vec4 worldFar  = invPV * clipFar;
        worldNear /= worldNear.w;
        worldFar  /= worldFar.w;

        EditorRay ray{};
        if (perspective) 
        {
            ray.origin = camPos;
            ray.dir    = glm::normalize(glm::vec3(worldFar - worldNear));
        } 
        else 
        {
            // Orthographic: origin is the unprojected near point; direction is -camera forward
            ray.origin = glm::vec3(worldNear);

            // Forward = -Z in view space -> transform by inverse view (or take from your camera)
            glm::vec3 camForward = glm::normalize(glm::vec3(glm::transpose(glm::mat3(view))[2]) * -1.0f);
            ray.dir = camForward; // already normalized if your camera sets it
        }

        return ray;
    }

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

    static glm::vec3 ChooseDummpyPosition(const Camera3D& cam, float distance = 5.0f)
    {
        glm::mat4 invView = glm::inverse(cam.View);
        glm::vec3 camFwd  = glm::normalize(glm::vec3(invView[2]) * -1.0f); 
        return cam.Position + camFwd * distance;
    }

    static bool DrawDirectionalLightGizmo(DirectLight& light, const Camera3D& camera, const ImVec2 viewportMin, const ImVec2 viewportMax, float iconScale = 1.0f)
    {
        ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetRect(viewportMin.x, viewportMin.y, viewportMax.x - viewportMin.x, viewportMax.y - viewportMin.y);

        glm::vec3 pos = ChooseDummpyPosition(camera, 6.0f);
        glm::mat3 R   = MakeRotationFromDirection(light.Direction);
        glm::mat4 model(1.0f);

        model[0] = glm::vec4(R[0], 0.0f);
        model[1] = glm::vec4(R[1], 0.0f);
        model[2] = glm::vec4(R[2], 0.0f);

        model[3] = glm::vec4(pos, 1.0f);

        if (iconScale != 1.0f) 
        {
            model = model * glm::scale(glm::mat4(1.0f), glm::vec3(iconScale));
        }

        float modelArr[16], viewArr[16], projArr[16];
        memcpy(modelArr, &model, sizeof(modelArr));
        memcpy(viewArr,  &camera.View, sizeof(viewArr));
        memcpy(projArr,  &camera.Projection, sizeof(projArr));

        bool changed = false;
        if (ImGuizmo::Manipulate(
                viewArr, projArr,
                ImGuizmo::ROTATE,     
                ImGuizmo::LOCAL,      
                modelArr,
                nullptr               
            ))
        {
            glm::mat4 newModel = glm::make_mat4(modelArr);

            glm::vec3 col0 = glm::vec3(newModel[0]);
            glm::vec3 col1 = glm::vec3(newModel[1]);
            glm::vec3 col2 = glm::vec3(newModel[2]);
            if (glm::length2(col0) > 0) newModel[0] = glm::vec4(glm::normalize(col0), 0.0f);
            if (glm::length2(col1) > 0) newModel[1] = glm::vec4(glm::normalize(col1), 0.0f);
            if (glm::length2(col2) > 0) newModel[2] = glm::vec4(glm::normalize(col2), 0.0f);

            glm::vec3 newDir = ExtractDirectionFromMatrix(newModel);
            if (glm::length2(newDir) > 0) {
                light.Direction = glm::normalize(newDir);
                changed = true;
            }
        }

        {
            glm::mat4 invViewProj = glm::inverse(camera.Projection * camera.View);
            auto worldToScreen = [&](const glm::vec3& p) -> ImVec2 {
                glm::vec4 clip = camera.Projection * camera.View * glm::vec4(p, 1.0f);
                if (clip.w == 0.0f) clip.w = 1e-6f;
                glm::vec3 ndc = glm::vec3(clip) / clip.w; // [-1,1]
                ImVec2 out;
                out.x = viewportMin.x + (ndc.x * 0.5f + 0.5f) * (viewportMax.x - viewportMin.x);
                out.y = viewportMin.y + (-ndc.y * 0.5f + 0.5f) * (viewportMax.y - viewportMin.y);
                return out;
            };

            glm::vec3 iconPos   = pos;
            glm::vec3 iconAhead = pos + glm::normalize(-light.Direction) * 1.5f;
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            dl->AddCircleFilled(worldToScreen(iconPos), 4.0f, IM_COL32(255, 255, 0, 255));
            dl->AddLine(worldToScreen(iconPos), worldToScreen(iconAhead), IM_COL32(255, 255, 0, 255), 2.0f);
        }

        return changed; 
    }

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
        context.ActiveSceneSpecification.Viewport.MIN = { vpMin.x, vpMin.y };
        context.ActiveSceneSpecification.Viewport.MAX = { vpMax.x, vpMax.y };
        ImVec2 mouse  = ImGui::GetMousePos();

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) 
            && ImGui::IsWindowFocused()
            && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && !ImGuizmo::IsUsing())
        {
            glm::vec2 local = { mouse.x - vpMin.x, mouse.y - vpMin.y };
            local.y = vp.y - local.y; // already doing the flip inside the viewport

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

            // Build camera ray
            const auto& cam = context.ActiveScene->GetCamera(); // or however you access it
            const glm::mat4 P  = cam.Camera.Projection;
            const glm::mat4 V  = cam.Camera.View;
            const glm::vec3 C  = cam.Camera.Position;
            const EditorRay pickRay = BuildPickingRayFromFramebufferPixel(mouseInFB, fbSize, P, V, C);

            if (auto picked = context.ActiveScene->PickEntityRay(pickRay.origin, pickRay.dir))
                context.ActiveScene->SelectedEntity(picked);
        }


        ImGuizmo::Enable(true);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
        ImGuizmo::SetGizmoSizeClipSpace(0.18f);

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
            ImGuizmo::PushID(1);
            auto& TRS = sel->GetComponent<TransformComponent>();
            glm::vec3 T = TRS.Translation;
            glm::vec3 S = TRS.Scale;
            glm::vec3 Rdeg = glm::degrees(glm::eulerAngles(TRS.Rotation)); 

            glm::mat4 transform{1.0f};
            ImGuizmo::RecomposeMatrixFromComponents(&T.x, &Rdeg.x, &S.x, glm::value_ptr(transform));

            ImGuizmo::AllowAxisFlip(false);
            ImGuizmo::Manipulate(glm::value_ptr(view),
                                glm::value_ptr(projection),
                                gizmoOp, gizmoMode,
                                glm::value_ptr(transform),
                                nullptr,
                                snapToggle ? snap : nullptr);

            if (ImGuizmo::IsUsing())
            {
                float Td[3], RdDeg[3], Sd[3];
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform), Td, RdDeg, Sd);

                TRS.Translation = { Td[0], Td[1], Td[2] };
                TRS.Scale       = { Sd[0], Sd[1], Sd[2] };

                glm::vec3 RdRad = glm::radians(glm::vec3(RdDeg[0], RdDeg[1], RdDeg[2]));
                TRS.Rotation    = glm::quat(RdRad);
            }

            ImGuizmo::PopID();
        }

        if(context.ActiveSceneSpecification.Environment.Sun.ShowLightDirectionGuizmo)
        {
            ImGuizmo::PushID(2);
            DrawDirectionalLightGizmo(context.ActiveScene->GetEnvironment().Sun, context.ActiveCamera.Camera, vpMin, vpMax);
            ImGuizmo::PopID();
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