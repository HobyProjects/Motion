#include "CorePCH.hpp"
#include "ViewportPanel.hpp"

namespace Motion
{
    static void DrawViewportAxisWidget(const glm::mat4& view, int corner = 2, float baseSize = 64.0f, ImVec2 basePadding = ImVec2(12, 12))
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList(); // above content
        ImGuiIO& io = ImGui::GetIO();

        // Scale for HiDPI
        const float scale = io.FontGlobalScale > 0.0f ? io.FontGlobalScale : 1.0f;
        const float size = baseSize * scale;
        const ImVec2 pad = ImVec2(basePadding.x * scale, basePadding.y * scale);
        const float thick = 3.0f * scale;
        const float ahLen = 8.0f * scale;   // arrowhead length
        const float ahHalf = 4.0f * scale;   // arrowhead half-width
        const float radius = 7.0f * scale;   // origin dot
        const float cardR = 8.0f * scale;   // card rounding
        const float cardPad = 6.0f * scale;

        // Anchor inside current window's content rect
        const ImVec2 winPos = ImGui::GetWindowPos();
        const ImVec2 crMin = ImGui::GetWindowContentRegionMin();
        const ImVec2 crMax = ImGui::GetWindowContentRegionMax();
        ImRect content(ImVec2(winPos.x + crMin.x, winPos.y + crMin.y), ImVec2(winPos.x + crMax.x, winPos.y + crMax.y));

        // Card rectangle
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

        // Background "card" with subtle shadow
        dl->AddRectFilled(ImVec2(cardMin.x, cardMin.y + 2 * scale), ImVec2(cardMax.x, cardMax.y + 2 * scale), IM_COL32(0, 0, 0, 40), cardR);
        dl->AddRectFilled(cardMin, cardMax, IM_COL32(28, 28, 32, 180), cardR);
        dl->AddRect(cardMin, cardMax, IM_COL32(255, 255, 255, 20), cardR);

        // Axis origin
        ImVec2 origin = ImVec2(cardMin.x + cardPad + size * 0.5f, cardMin.y + cardPad + size * 0.5f);

        // Extract camera rotation (upper-left 3x3 of inverse(view))
        // view = R^T * T^-1 for right-handed OpenGL-like conventions.
        glm::mat3 R = glm::mat3(glm::transpose(view)); // matches your original approach (camera basis rows)

        struct Axis { glm::vec3 dir; ImU32 col; const char* lbl; };
        Axis axes[] = {
            { {1,0,0}, IM_COL32(220, 70, 70, 255), "X" },
            { {0,1,0}, IM_COL32(70,220, 70, 255), "Y" },
            { {0,0,1}, IM_COL32(90,150,255,255), "Z" },
        };

        auto draw_axis = [&](const Axis& a)
            {
                // Local axis in camera space (so "toward screen" fades)
                glm::vec3 v = glm::normalize(R * a.dir);

                // Map to 2D inside the square: X to +x, Y to -y to match screen down
                ImVec2 tip = ImVec2(origin.x + v.x * (size * 0.45f),
                    origin.y - v.y * (size * 0.45f));

                // Fade based on Z (positive Z away from camera in view space)—tweak to taste
                float z = v.z; // if axis points out of screen (z<0), brighten, else dim
                float alpha = (z < 0.0f) ? 1.00f : 0.40f;
                ImU32 lineCol = IM_COL32(
                    (int)((a.col >> 0) & 0xFF),
                    (int)((a.col >> 8) & 0xFF),
                    (int)((a.col >> 16) & 0xFF),
                    (int)(255 * alpha)
                );

                // Line
                dl->AddLine(origin, tip, lineCol, thick);

                // Arrowhead (simple isosceles)
                glm::vec2 d = glm::normalize(glm::vec2(tip.x - origin.x, tip.y - origin.y));
                glm::vec2 n = glm::vec2(-d.y, d.x);
                ImVec2 a0 = ImVec2(tip.x - d.x * ahLen + n.x * ahHalf, tip.y - d.y * ahLen + n.y * ahHalf);
                ImVec2 a1 = ImVec2(tip.x - d.x * ahLen - n.x * ahHalf, tip.y - d.y * ahLen - n.y * ahHalf);
                dl->AddTriangleFilled(tip, a0, a1, lineCol);

                // Label near the tip
                ImVec2 labelPos = ImVec2(tip.x + 6.0f * scale, tip.y - 6.0f * scale);
                dl->AddText(labelPos, lineCol, a.lbl);
            };

        for (const Axis& a : axes)
            draw_axis(a);

        // Origin dot
        dl->AddCircleFilled(origin, radius, IM_COL32(180, 180, 190, 220));
        dl->AddCircle(origin, radius, IM_COL32(255, 255, 255, 40), 0, 1.5f * scale);
    }

    void SceneViewportPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
        ImGui::Begin(std::format("{}##SceneViewport", context.ActiveScene->GetName()).c_str());
        context.UILayerInstance->AcceptEvents(ImGui::IsWindowFocused() || ImGui::IsWindowHovered());

        // keep framebuffer size in lockstep with the ImGui panel
        ImVec2 vp = ImGui::GetContentRegionAvail();
        if (vp.x != context.ActiveSceneSpecification.Viewport.Size.x || vp.y != context.ActiveSceneSpecification.Viewport.Size.y)
        {
            context.ActiveSceneSpecification.Viewport.Size = { vp.x, vp.y };
        }

        // draw scene texture
        ImGui::Image((ImTextureID)context.ActiveViewportTexture, vp, { 0,1 }, { 1,0 });

        // mouse-pick to select an entity (ignores when gizmo is being used)
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 crMin = ImGui::GetWindowContentRegionMin();
        ImVec2 crMax = ImGui::GetWindowContentRegionMax();
        ImVec2 vpMin = { winPos.x + crMin.x, winPos.y + crMin.y };
        ImVec2 vpMax = { winPos.x + crMax.x, winPos.y + crMax.y };
        ImVec2 mouse = ImGui::GetMousePos();

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
            ImGui::IsWindowFocused() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGuizmo::IsUsing())
        {
            // mouse in panel-local space (origin = content top-left)
            glm::vec2 local = { mouse.x - vpMin.x, mouse.y - vpMin.y };
            local.y = vp.y - local.y; // flip Y for GL

            // scale to framebuffer space (critical when FB != panel size)
            glm::vec2 fbSize = {
                (float)context.ActiveSceneSpecification.Viewport.FrameSpec.Width,
                (float)context.ActiveSceneSpecification.Viewport.FrameSpec.Height
            };
            glm::vec2 mouseInFB = {
                local.x * (fbSize.x / vp.x),
                local.y * (fbSize.y / vp.y)
            };

            if (auto picked = context.ActiveScene->PickEntity(mouseInFB, fbSize))
                context.ActiveScene->SelectedEntity(picked);
        }

        // gizmo: translate/rotate/scale with Ctrl+E to cycle
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(vpMin.x, vpMin.y, vp.x, vp.y);
        glm::mat4 view = context.ActiveScene->GetCameraView();
        glm::mat4 proj = context.ActiveScene->GetCameraProjection();

        static ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_E))
        {
            op = (op == ImGuizmo::TRANSLATE) ? ImGuizmo::ROTATE :
                (op == ImGuizmo::ROTATE) ? ImGuizmo::SCALE :
                ImGuizmo::TRANSLATE;
        }

        if (auto sel = context.ActiveScene->GetSelectedEntity();sel && sel != EntityFactory::EMPTYENTITY && sel->HasComponent<TransformComponent>())
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

        // tiny axis card & view manipulator
        // (same DrawViewportAxisWidget(view) you’ve seen, plus optional ImGuizmo::ViewManipulate block)

        ImGui::End();
        ImGui::PopStyleVar();
    }
}