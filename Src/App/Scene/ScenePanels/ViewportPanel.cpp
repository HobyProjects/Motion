#include "CorePCH.hpp"
#include "ViewportPanel.hpp"

namespace Motion
{
    static void DrawViewportAxisWidget(const glm::mat4& view, int corner = 2, float baseSize = 64.0f, ImVec2 basePadding = ImVec2(12, 12))
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImGuiIO& io = ImGui::GetIO();

        const float scale = io.FontGlobalScale > 0.0f ? io.FontGlobalScale : 1.0f;
        const float size = baseSize * scale;
        const ImVec2 pad = ImVec2(basePadding.x * scale, basePadding.y * scale);
        const float thick = 3.0f * scale;
        const float ahLen = 8.0f * scale;
        const float ahHalf = 4.0f * scale;
        const float radius = 7.0f * scale;
        const float cardR = 8.0f * scale;
        const float cardPad = 6.0f * scale;

        const ImVec2 winPos = ImGui::GetWindowPos();
        const ImVec2 crMin = ImGui::GetWindowContentRegionMin();
        const ImVec2 crMax = ImGui::GetWindowContentRegionMax();
        ImRect content(ImVec2(winPos.x + crMin.x, winPos.y + crMin.y), ImVec2(winPos.x + crMax.x, winPos.y + crMax.y));

        ImVec2 cardSize(size + cardPad * 2, size + cardPad * 2);
        ImVec2 cardMin, cardMax;
        switch (corner)
        {
        case 0:     cardMin = ImVec2(content.Min.x + pad.x, content.Min.y + pad.y); break; // Top-left
        case 1:     cardMin = ImVec2(content.Max.x - pad.x - cardSize.x, content.Min.y + pad.y); break; // Top-right
        case 2:     cardMin = ImVec2(content.Min.x + pad.x, content.Max.y - pad.y - cardSize.y); break; // Bottom-left
        default:    cardMin = ImVec2(content.Max.x - pad.x - cardSize.x, content.Max.y - pad.y - cardSize.y); break; // Bottom-right
        }
        cardMax = ImVec2(cardMin.x + cardSize.x, cardMin.y + cardSize.y);

        dl->AddRectFilled(ImVec2(cardMin.x, cardMin.y + 2 * scale), ImVec2(cardMax.x, cardMax.y + 2 * scale), IM_COL32(0, 0, 0, 40), cardR);
        dl->AddRectFilled(cardMin, cardMax, IM_COL32(28, 28, 32, 180), cardR);
        dl->AddRect(cardMin, cardMax, IM_COL32(255, 255, 255, 20), cardR);


        ImVec2 origin = ImVec2(cardMin.x + cardPad + size * 0.5f, cardMin.y + cardPad + size * 0.5f);
        glm::mat3 R = glm::mat3(glm::transpose(view));

        struct Axis { glm::vec3 dir; ImU32 col; const char* lbl; };
        Axis axes[] = {
            { {1,0,0}, IM_COL32(220, 70, 70, 255), "X" },
            { {0,1,0}, IM_COL32(70,220, 70, 255), "Y" },
            { {0,0,1}, IM_COL32(90,150,255,255), "Z" },
        };

        auto draw_axis = [&](const Axis& a)
            {
                glm::vec3 v = glm::normalize(R * a.dir);
                ImVec2 tip = ImVec2(origin.x + v.x * (size * 0.45f),
                    origin.y - v.y * (size * 0.45f));

                float z = v.z;
                float alpha = (z < 0.0f) ? 1.00f : 0.40f;
                ImU32 lineCol = IM_COL32(
                    (int)((a.col >> 0) & 0xFF),
                    (int)((a.col >> 8) & 0xFF),
                    (int)((a.col >> 16) & 0xFF),
                    (int)(255 * alpha)
                );

                dl->AddLine(origin, tip, lineCol, thick);

                glm::vec2 d = glm::normalize(glm::vec2(tip.x - origin.x, tip.y - origin.y));
                glm::vec2 n = glm::vec2(-d.y, d.x);
                ImVec2 a0 = ImVec2(tip.x - d.x * ahLen + n.x * ahHalf, tip.y - d.y * ahLen + n.y * ahHalf);
                ImVec2 a1 = ImVec2(tip.x - d.x * ahLen - n.x * ahHalf, tip.y - d.y * ahLen - n.y * ahHalf);
                dl->AddTriangleFilled(tip, a0, a1, lineCol);

                ImVec2 labelPos = ImVec2(tip.x + 6.0f * scale, tip.y - 6.0f * scale);
                dl->AddText(labelPos, lineCol, a.lbl);
            };

        for (const Axis& a : axes)
            draw_axis(a);

        dl->AddCircleFilled(origin, radius, IM_COL32(180, 180, 190, 220));
        dl->AddCircle(origin, radius, IM_COL32(255, 255, 255, 40), 0, 1.5f * scale);
    }

    void SceneViewportPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
        ImGui::Begin(std::format("{}##SceneViewport", context.ActiveScene->GetName()).c_str());
        context.UILayerInstance->AcceptEvents(ImGui::IsWindowFocused() || ImGui::IsWindowHovered());

        // Keep the gizmo op accessible to the overlay
        static ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;

        // Viewport size tracking
        ImVec2 vp = ImGui::GetContentRegionAvail();
        if (vp.x != context.ActiveSceneSpecification.Viewport.Size.x || vp.y != context.ActiveSceneSpecification.Viewport.Size.y)
            context.ActiveSceneSpecification.Viewport.Size = { vp.x, vp.y };

        // Draw the actual viewport image
        ImGui::Image((ImTextureID)context.ActiveViewportTexture, vp, { 0,1 }, { 1,0 });

        // Compute the viewport rect in screen space (used by overlay & gizmo)
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 crMin  = ImGui::GetWindowContentRegionMin();
        ImVec2 crMax  = ImGui::GetWindowContentRegionMax();
        ImVec2 vpMin  = { winPos.x + crMin.x, winPos.y + crMin.y };
        ImVec2 vpMax  = { winPos.x + crMax.x, winPos.y + crMax.y };
        ImVec2 mouse  = ImGui::GetMousePos();

        // ---------- Floating top-center toolbar overlay (over the image) ----------
        
        {
            ImGuiIO& io = ImGui::GetIO();
            const float scale     = io.FontGlobalScale > 0.0f ? io.FontGlobalScale : 1.0f;

            const float buttonW   = 30.0f * scale;
            const float buttonH   = 25.0f * scale;
            const float spacing   = 8.0f  * scale;
            const float pad       = 8.0f  * scale;
            const float topOffset = 8.0f  * scale;

            const float totalW = buttonW * 3.0f + spacing * 2.0f;

            // Position the overlay in the top-center of the viewport
            ImVec2 overlayPos  = { vpMin.x + (vp.x - totalW) * 0.5f - pad, vpMin.y + topOffset - pad };
            ImVec2 overlaySize = { totalW + pad * 2.0f, buttonH + pad * 2.0f };

            ImGui::SetNextWindowPos(overlayPos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(overlaySize, ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.0f); // fully transparent
            ImGuiWindowFlags overlayFlags =
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
                | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse
                | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            if (ImGui::Begin("##ViewportToolbarOverlay", nullptr, overlayFlags))
            {
                ImGui::SetCursorPos(ImVec2(pad, pad));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f * scale);

                if (ImGui::Button(ICON_MD_PLAY_ARROW, ImVec2(buttonW, buttonH))) 
                {
                    // action
                }
                ImGui::SameLine(0.0f, spacing);
                if (ImGui::Button(ICON_MD_STOP, ImVec2(buttonW, buttonH))) 
                {
                    // action
                }
                ImGui::SameLine(0.0f, spacing);
                if (ImGui::Button(ICON_MD_PAUSE, ImVec2(buttonW, buttonH))) 
                {
                    // action
                }

                ImGui::PopStyleVar(); // FrameRounding
            }

            ImGui::End();
            ImGui::PopStyleVar(2);
        }

        // ---- Floating top-right overlay (Camera controls) ----
        {
            ImGuiIO& io = ImGui::GetIO();
            const float scale     = io.FontGlobalScale > 0.0f ? io.FontGlobalScale : 1.0f;
            const float pad       = 8.0f  * scale;   // margin from the edges
            const float topOffset = 8.0f  * scale;   // distance from top inside the viewport
            const float gap       = 16.0f * scale;   // gap between groups

            ImVec2 anchor = ImVec2(vpMax.x - pad, vpMin.y + topOffset);
            ImGui::SetNextWindowPos(anchor, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
            ImGui::SetNextWindowBgAlpha(0.0f); // fully transparent
            ImGuiWindowFlags overlayFlags =
                ImGuiWindowFlags_NoDecoration
                | ImGuiWindowFlags_AlwaysAutoResize
                | ImGuiWindowFlags_NoSavedSettings
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoDocking;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
            if (ImGui::Begin("##CameraControlsOverlay", nullptr, overlayFlags))
            {
                ImGui::PushItemWidth(80.0f * scale);

                // Camera Speed
                ImGui::TextUnformatted(ICON_MD_DIRECTIONS);
                ImGui::SameLine();

                float* speedPtr = &context.ActiveCamera.Camera.TranslationSpeed;
                ImGui::DragFloat("##CamSpeed", speedPtr, 0.05f, 0.05f, 10.0f);

                ImGui::SameLine(0.0f, 20.0f * scale); // spacing between groups

                // Camera Sensitivity
                ImGui::TextUnformatted(ICON_MD_LOOKS);
                ImGui::SameLine();

                float* sensPtr = &context.ActiveCamera.Camera.Sensitivity;
                ImGui::DragFloat("##CamSens", sensPtr, 0.05f, 0.05f, 3.0f, "%.2f");

                ImGui::PopItemWidth();
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
    
        // -------------------------------------------------------------------------

        // Entity picking (clicks on the overlay won't trigger this)
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsWindowFocused() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing())
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

        DrawViewportAxisWidget(view, 2, 64.0f, ImVec2(12, 12));

        ImGui::End();
        ImGui::PopStyleVar();
    }
}