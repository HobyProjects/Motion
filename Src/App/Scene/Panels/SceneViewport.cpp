#include "CorePCH.hpp"

#include "Scene.hpp"
#include "SceneUtils.hpp"
#include "SceneCommon.hpp"
#include "SceneViewport.hpp"

namespace Motion
{
    /**
     * @brief Computes the viewport rectangle for the ImGui window
     * @return the viewport rectangle
     * This function computes the viewport rectangle for the ImGui window
     * by getting the window position and content region min/max.
     */
    static SceneViewport::ViewportRect ComputeViewportRect()
    {
        const ImVec2 winPos = ImGui::GetWindowPos();
        const ImVec2 crMin  = ImGui::GetWindowContentRegionMin();
        const ImVec2 crMax  = ImGui::GetWindowContentRegionMax();
        return { { winPos.x + crMin.x, winPos.y + crMin.y }, { winPos.x + crMax.x, winPos.y + crMax.y } };
    }

    /**
     * @brief Computes the screen coordinates for a given point in world space
     * @param p the point in world space
     * @param VP the view projection matrix
     * @param rect the viewport rectangle
     * @param[out] out the screen coordinates
     * @return true if the computation was successful, false otherwise
     * This function computes the screen coordinates for a given point in world space
     * by transforming the point with the view projection matrix and then
     * normalizing the resulting coordinates. The normalized coordinates are then
     * mapped to the viewport rectangle coordinates.
     */
    static bool WorldToScreen(const glm::vec3& p, const glm::mat4& VP, const SceneViewport::ViewportRect& rect, ImVec2& out)
    {
        glm::vec4 clip = VP * glm::vec4(p, 1.0f);
        if (clip.w <= 0.0001f) return false;
        glm::vec3 ndc = glm::vec3(clip) / clip.w;  
        if (ndc.x < -1.2f || ndc.x > 1.2f || ndc.y < -1.2f || ndc.y > 1.2f) return false;

        out.x = rect.min.x + (ndc.x * 0.5f + 0.5f) * rect.width();
        out.y = rect.min.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * rect.height();
        return true;
    }

    /**
     * @brief builds a ray from a mouse position in framebuffer space
     * @param mouseFB the mouse position in framebuffer space
     * @param fbSize the size of the framebuffer
     * @param view the view matrix
     * @param proj the projection matrix
     * @return a ray in world space
     * This function builds a ray from a mouse position in framebuffer space
     * by transforming the mouse position into normalized device coordinates,
     * and then transforming those coordinates into world space using the
     * view and projection matrices.
     */
    static SceneViewport::RayWS BuildMouseRayFromFB(const glm::vec2& mouseFB, const glm::vec2& fbSize, const glm::mat4& view, const glm::mat4& proj)
    {
        const float ndcX =  (mouseFB.x / fbSize.x) * 2.0f - 1.0f;
        const float ndcY =  (mouseFB.y / fbSize.y) * 2.0f - 1.0f;

        const glm::mat4 invVP = glm::inverse(proj * view);

        glm::vec4 pNear = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 pFar  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);
        pNear /= pNear.w;
        pFar  /= pFar.w;

        SceneViewport::RayWS r;
        r.Origin    = glm::vec3(pNear);
        r.Direction = glm::normalize(glm::vec3(pFar - pNear));
        return r;
    }

    /**
     * @brief creates a rotation matrix from a direction vector and an up hint vector
     * @param dir the direction vector
     * @param upHint the up hint vector, defaults to {0,1,0}
     * @return a rotation matrix
     * This function creates a rotation matrix from a direction vector and an up hint vector.
     * The direction vector is used to compute the forward direction of the rotation matrix.
     * The up hint vector is used to compute the right and up directions of the rotation matrix.
     * If the up hint vector is close to parallel to the direction vector, a default up vector is used instead.
     */
    static glm::mat3 MakeRotationFromDirection(const glm::vec3& dir, const glm::vec3& upHint = {0,1,0})
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

    /**
     * @brief extracts the forward direction from a 4x4 matrix
     * @param M the 4x4 matrix
     * @return the forward direction as a glm::vec3
     * This function extracts the forward direction from a 4x4 matrix by normalizing the second column of the matrix and negating it.
     */
    static glm::vec3 ExtractDirectionFromMatrix(const glm::mat4& M)
    {
        const glm::vec3 fwd = glm::normalize(glm::vec3(M[2]));
        return -fwd;
    }
    
    /**
     * @brief returns a dummy position for a directional light based on the camera's position and direction
     * @param cam the camera
     * @param distance the distance from the camera's position to the dummy position, defaults to 6.0f
     * @return a dummy position for a directional light
     * This function returns a dummy position for a directional light based on the camera's position and direction.
     * The dummy position is computed by moving along the camera's forward direction by the specified distance.
     */
    static glm::vec3 ChooseDummyPosition(const Camera3D& cam, float distance = 6.0f)
    {
        const glm::mat4 invView = glm::inverse(cam.View);
        const glm::vec3 camFwd  = glm::normalize(glm::vec3(invView[2]) * -1.0f);
        return cam.Position + camFwd * distance;
    }


    /**
     * @brief draws a directional light in the scene editor
     * @param light the light to be drawn
     * @param camera the camera used for drawing
     * @param rect the viewport rect
     * @param dl the draw list
     * @param cfg the config for drawing the light
     * @return true if the light was changed, false otherwise
     * This function draws a directional light in the scene editor.
     * It uses ImGuizmo to draw the light and its direction.
     * The light is represented as a billboard with a direction arrow.
     * The direction arrow is drawn from the light's position to a point on the direction vector.
     * The length of the direction arrow is configurable.
     * The function also draws optional rays from the light's position in the direction of the light.
     * The number of rays is configurable.
     * The function returns true if the light was changed, false otherwise.
     */
    static bool DrawDirectionalLight(ScenePhysicsWorld::WorldLighting& light, const Camera3D& camera, const SceneViewport::ViewportRect& rect, ImDrawList* dl, const SceneViewport::LightGizmoConfig& cfg)
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
                const float iw = (clip.w == 0.0f) ? EPSILON : clip.w;
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


    void SceneView::OnRender(Scene* scene)
    {
        if(!scene) return;

        SceneContext& context = scene->GetContext();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        ImGui::Begin("Scene Viewport");
        {
            context.View->ViewportFocusedOrHovered = ImGui::IsWindowFocused() || 
                ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

            const ImVec2 vpAvail = ImGui::GetContentRegionAvail();
            if (FrameTextureID tex = context.View->FrameTexturePtr; tex != 0)
                ImGui::Image((ImTextureID)tex, vpAvail, ImVec2(0, 1), ImVec2(1, 0));
            else
                ImGui::Dummy(vpAvail);

            const SceneViewport::ViewportRect rect = ComputeViewportRect();

            const ImVec2 mouse      = ImGui::GetMousePos();
            ImDrawList* windowDL    = ImGui::GetWindowDrawList();

            const Camera3D& camera      = context.View->Camera;
            const glm::mat4& view       = camera.View;
            const glm::mat4& projection = camera.Projection;

            
            if(!context.Simulation->InSimulation)
            {
                bool canPickEntites = false;
                canPickEntites |= ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                canPickEntites |= ImGui::IsWindowFocused();
                canPickEntites &= ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                canPickEntites |= !ImGuizmo::IsUsing();
    
                if(canPickEntites && context.View->ViewportFocusedOrHovered)
                {
                    glm::vec2 local = { mouse.x - rect.min.x, mouse.y - rect.min.y };
                    local.y = vpAvail.y - local.y;
                    
                    const auto& fbSpecs = context.View->FrameSpecification;
                    const glm::vec2 fbSize = { (float)fbSpecs.Width, (float)fbSpecs.Height };
                    const glm::vec2 mouseMapFB = { local.x * (fbSize.x / vpAvail.x), local.y * (fbSize.y / vpAvail.y) };
                    const SceneViewport::RayWS ray = BuildMouseRayFromFB(mouseMapFB, fbSize, view, projection);
    
                    static constexpr float MAX_DISTANCE = 5000.0f;
                    const glm::vec3 P0 = ray.Origin;
                    const glm::vec3 P1 = ray.Origin + ray.Direction * MAX_DISTANCE;
    
                    RayHitResults result{};
                    const bool hit = RaycastFirstHit(context.PhysicsWorld->World, P0, P1, result);
                    if (hit) 
                    {
                        scene->SelectedEntity(result.Entity);
                    }
                }
                
                static SceneViewport::GizmoState gizmo;
                gizmo.HandleHotkeys();
                ImGuizmo::SetDrawlist(windowDL);
                ImGuizmo::SetRect(rect.min.x, rect.min.y, rect.width(), rect.height());
                ImGuizmo::SetGizmoSizeClipSpace(0.18f);
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::AllowAxisFlip(false);

                bool gizmoConsumedInput = false;
                float snapTriplet[3] = {0,0,0};
                gizmo.FillSnapTriplet(snapTriplet);
                const float* snapPtr = (snapTriplet[0] != 0 || snapTriplet[1] != 0 
                    || snapTriplet[2] != 0) ? snapTriplet : nullptr;

                if (context.Entities->SelectedEntity != entt::null)
                {
                    const bool hasTransform  = context.Entities->Registry.any_of<TransformComponent>(context.Entities->SelectedEntity);
                    TagComponent* tag        = context.Entities->Registry.try_get<TagComponent>(context.Entities->SelectedEntity);
                    const bool isActive      = tag ? tag->IsActive : false;

                    if (canPickEntites && ImGui::IsMouseClicked(ImGuiPopupFlags_MouseButtonMiddle))
                    {
                        ImGui::OpenPopup("ViewportContextMenu");
                    }

                    if (ImGui::BeginPopupContextWindow("ViewportContextMenu", ImGuiPopupFlags_MouseButtonMiddle))
                    {
                        ImGui::Indent(5.0f);
                        if(ImGui::MenuItem("Delete"))
                        {
                            scene->DestroyEntity(context.Entities->SelectedEntity, true);
                            ImGui::CloseCurrentPopup();
                        }

                        if(ImGui::MenuItem("Duplicate"))
                        {
                            scene->DuplicateEntity(context.Entities->SelectedEntity);
                            ImGui::CloseCurrentPopup();
                        }

                        if(tag->IsActive)
                        {
                            ImGui::Separator();
                            if(ImGui::MenuItem("Hide"))
                            {
                                tag->IsActive = false;
                                ImGui::CloseCurrentPopup();
                            }
                        }

                        ImGui::Separator();

                        if (!context.Simulation->IsEntitySimulated(context.Entities->SelectedEntity))
                        {
                            if(ImGui::MenuItem("Add To Watchlist"))
                            {
                                context.Simulation->AddSimulatedEntity(context.Entities->SelectedEntity);
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        else
                        {
                            if(ImGui::MenuItem("Remove From Watchlist"))
                            {
                                context.Simulation->RemoveSimulatedEntity(context.Entities->SelectedEntity);
                                ImGui::CloseCurrentPopup();
                            }
                        }

                        ImGui::Unindent(5.0f);
                        ImGui::EndPopup();
                    }

                    if (isActive && hasTransform)
                    {
                        ImGuizmo::PushID(1);
                        auto& TRS = context.Entities->Registry.get<TransformComponent>(context.Entities->SelectedEntity);

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

                if (context.Entities->SelectedEntity != entt::null && context.Entities->Registry.valid(context.Entities->SelectedEntity))
                {
                    auto* transform = context.Entities->Registry.try_get<TransformComponent>(context.Entities->SelectedEntity);
                    auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(context.Entities->SelectedEntity);
                    
                    if (transform && rb)
                    {
                        // Draw force vectors
                        if(context.Physics->PhysicsAnalysis.ForceAnalysis.ShowForceVectors && 
                            context.Panels->ShowForceAnalysisPanel)
                        {
                            for (const auto& force : context.Physics->PhysicsAnalysis.ForceAnalysis.Forces)
                            {
                                if (force.IsActive)
                                {
                                    DrawForceVector(context, transform->Translation, force.Force, 
                                                force.Color, context.Physics->PhysicsAnalysis.ForceAnalysis.VectorScale);
                                }
                            }
                            
                            // Draw net force
                            if (context.Physics->PhysicsAnalysis.ForceAnalysis.ShowNetForce)
                            {
                                DrawForceVector(context, transform->Translation, 
                                            context.Physics->PhysicsAnalysis.ForceAnalysis.NetForce,
                                            IM_COL32(255, 255, 0, 255), 
                                            context.Physics->PhysicsAnalysis.ForceAnalysis.VectorScale * 1.5f);
                            }
                        }
                        
                        // Draw momentum vector
                        if (context.Panels->ShowMomentumPanel && 
                            context.Physics->PhysicsAnalysis.Momentum.ShowMomentumVector)
                        {
                            DrawMomentumVector(context, transform->Translation, 
                                            context.Physics->PhysicsAnalysis.Momentum.LinearMomentum,
                                            IM_COL32(200, 100, 255, 255),
                                            context.Physics->PhysicsAnalysis.Momentum.VectorScale);
                        }
                        
                        // Draw acceleration vector
                        if (context.Panels->ShowAccelerationPanel && 
                            context.Physics->PhysicsAnalysis.Acceleration.ShowVector)
                        {
                            glm::vec3 accelVector = context.Physics->PhysicsAnalysis.Acceleration.CurrentAcceleration * 
                                                    context.Physics->PhysicsAnalysis.Acceleration.VectorScale;
                            DrawForceVector(context, transform->Translation, accelVector,
                                        IM_COL32(100, 200, 255, 255),
                                        1.0f);
                        }
                        
                        // Draw trajectory path
                        if (context.Panels->ShowTrajectoryPanel && 
                            context.Physics->PhysicsAnalysis.Trajectory.ShowPredictionPath &&
                            !context.Physics->PhysicsAnalysis.Trajectory.PredictedPath.empty())
                        {
                            DrawTrajectoryPath(context, context.Physics->PhysicsAnalysis.Trajectory.PredictedPath,
                                        context.Physics->PhysicsAnalysis.Trajectory.PathColor);
                        }
                    }
                }

                const bool clutchHide = ImGui::IsKeyDown(ImGuiKey_4);
                static SceneViewport::LightGizmoConfig lightCfg;
                lightCfg.Enabled = context.PhysicsWorld->SunLight.ShowGuizmo && !clutchHide;

                if (!gizmoConsumedInput && lightCfg.Enabled)
                {
                    context.PhysicsWorld->SunLight.ShowGuizmo = true;
                    (void)DrawDirectionalLight(context.PhysicsWorld->SunLight, camera, rect, windowDL, lightCfg);
                }
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void SceneView::DrawForceVector(SceneContext& context, const glm::vec3& origin, const glm::vec3& force, const ImU32& color, float scale)
    {
        // Skip if force is too small
        const float forceMagnitude = glm::length(force);
        if (forceMagnitude < EPSILON) return;

        // Get viewport and camera info
        const SceneViewport::ViewportRect rect = ComputeViewportRect();
        const Camera3D& camera = context.View->Camera;
        const glm::mat4 VP = camera.Projection * camera.View;
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Calculate scaled force vector
        const glm::vec3 scaledForce = force * scale;
        const glm::vec3 endPoint = origin + scaledForce;

        // Convert to screen space
        ImVec2 screenOrigin, screenEnd;
        if (!WorldToScreen(origin, VP, rect, screenOrigin)) return;
        if (!WorldToScreen(endPoint, VP, rect, screenEnd)) return;

        // Draw the vector line
        const float thickness = 2.5f;
        drawList->AddLine(screenOrigin, screenEnd, color, thickness);

        // Draw arrowhead
        const float arrowLength = 15.0f;
        const float arrowWidth = 8.0f;
        
        const ImVec2 direction = {screenEnd.x - screenOrigin.x, screenEnd.y - screenOrigin.y};
        const float lineLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        
        if (lineLength > arrowLength)
        {
            const ImVec2 normalizedDir = {direction.x / lineLength, direction.y / lineLength};
            const ImVec2 perpDir = {-normalizedDir.y, normalizedDir.x};
            
            const ImVec2 arrowTip = screenEnd;
            const ImVec2 arrowBase = {screenEnd.x - normalizedDir.x * arrowLength, 
                                      screenEnd.y - normalizedDir.y * arrowLength};
            const ImVec2 arrowLeft = {arrowBase.x + perpDir.x * arrowWidth, 
                                      arrowBase.y + perpDir.y * arrowWidth};
            const ImVec2 arrowRight = {arrowBase.x - perpDir.x * arrowWidth, 
                                       arrowBase.y - perpDir.y * arrowWidth};
            
            drawList->AddTriangleFilled(arrowTip, arrowLeft, arrowRight, color);
        }

        // Draw origin point
        drawList->AddCircleFilled(screenOrigin, 4.0f, color);
    }

    void SceneView::DrawMomentumVector(SceneContext& context, const glm::vec3& position, const glm::vec3& momentum, const ImU32& color, float scale)
    {
        // Skip if momentum is too small
        const float momentumMagnitude = glm::length(momentum);
        if (momentumMagnitude < EPSILON) return;

        // Get viewport and camera info
        const SceneViewport::ViewportRect rect = ComputeViewportRect();
        const Camera3D& camera = context.View->Camera;
        const glm::mat4 VP = camera.Projection * camera.View;
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Calculate scaled momentum vector
        const glm::vec3 scaledMomentum = momentum * scale;
        const glm::vec3 endPoint = position + scaledMomentum;

        // Convert to screen space
        ImVec2 screenOrigin, screenEnd;
        if (!WorldToScreen(position, VP, rect, screenOrigin)) return;
        if (!WorldToScreen(endPoint, VP, rect, screenEnd)) return;

        // Draw the vector line with a slightly thicker line for momentum
        const float thickness = 3.0f;
        drawList->AddLine(screenOrigin, screenEnd, color, thickness);

        // Draw arrowhead
        const float arrowLength = 18.0f;
        const float arrowWidth = 9.0f;
        
        const ImVec2 direction = {screenEnd.x - screenOrigin.x, screenEnd.y - screenOrigin.y};
        const float lineLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        
        if (lineLength > arrowLength)
        {
            const ImVec2 normalizedDir = {direction.x / lineLength, direction.y / lineLength};
            const ImVec2 perpDir = {-normalizedDir.y, normalizedDir.x};
            
            const ImVec2 arrowTip = screenEnd;
            const ImVec2 arrowBase = {screenEnd.x - normalizedDir.x * arrowLength, 
                                      screenEnd.y - normalizedDir.y * arrowLength};
            const ImVec2 arrowLeft = {arrowBase.x + perpDir.x * arrowWidth, 
                                      arrowBase.y + perpDir.y * arrowWidth};
            const ImVec2 arrowRight = {arrowBase.x - perpDir.x * arrowWidth, 
                                       arrowBase.y - perpDir.y * arrowWidth};
            
            drawList->AddTriangleFilled(arrowTip, arrowLeft, arrowRight, color);
        }

        // Draw origin point with a larger circle for momentum
        drawList->AddCircleFilled(screenOrigin, 5.0f, color);
        
        // Draw an outer ring to distinguish momentum vectors
        drawList->AddCircle(screenOrigin, 7.0f, color, 16, 1.5f);
    }

    void SceneView::DrawTrajectoryPath(SceneContext& context, const std::vector<glm::vec3>& path, const ImU32 & color)
    {
        // Need at least 2 points to draw a path
        if (path.size() < 2) return;

        // Get viewport and camera info
        const SceneViewport::ViewportRect rect = ComputeViewportRect();
        const Camera3D& camera = context.View->Camera;
        const glm::mat4 VP = camera.Projection * camera.View;
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Convert all path points to screen space
        std::vector<ImVec2> screenPoints;
        screenPoints.reserve(path.size());
        
        for (const auto& worldPoint : path)
        {
            ImVec2 screenPoint;
            if (WorldToScreen(worldPoint, VP, rect, screenPoint))
            {
                screenPoints.push_back(screenPoint);
            }
            else
            {
                // Mark invalid points with a special flag
                screenPoints.push_back({-1.0f, -1.0f});
            }
        }

        // Draw the trajectory path as connected line segments
        const float thickness = 2.0f;
        
        for (size_t i = 0; i < screenPoints.size() - 1; ++i)
        {
            const ImVec2& p1 = screenPoints[i];
            const ImVec2& p2 = screenPoints[i + 1];
            
            // Skip if either point is invalid
            if (p1.x < 0 || p2.x < 0) continue;
            
            // Calculate alpha for fade effect (fade out as we go further)
            const float t = static_cast<float>(i) / static_cast<float>(screenPoints.size() - 1);
            const float alpha = 1.0f - (t * 0.5f); // Fade from 100% to 50% opacity
            
            // Extract color components and apply alpha
            const ImU32 r = (color >> IM_COL32_R_SHIFT) & 0xFF;
            const ImU32 g = (color >> IM_COL32_G_SHIFT) & 0xFF;
            const ImU32 b = (color >> IM_COL32_B_SHIFT) & 0xFF;
            const ImU32 a = static_cast<ImU32>(((color >> IM_COL32_A_SHIFT) & 0xFF) * alpha);
            const ImU32 fadedColor = IM_COL32(r, g, b, a);
            
            drawList->AddLine(p1, p2, fadedColor, thickness);
        }

        // Draw small circles at each trajectory point
        const float pointRadius = 2.5f;
        for (size_t i = 0; i < screenPoints.size(); ++i)
        {
            const ImVec2& p = screenPoints[i];
            if (p.x < 0) continue; // Skip invalid points
            
            // Fade points along with the lines
            const float t = static_cast<float>(i) / static_cast<float>(screenPoints.size() - 1);
            const float alpha = 1.0f - (t * 0.5f);
            
            const ImU32 r = (color >> IM_COL32_R_SHIFT) & 0xFF;
            const ImU32 g = (color >> IM_COL32_G_SHIFT) & 0xFF;
            const ImU32 b = (color >> IM_COL32_B_SHIFT) & 0xFF;
            const ImU32 a = static_cast<ImU32>(((color >> IM_COL32_A_SHIFT) & 0xFF) * alpha);
            const ImU32 fadedColor = IM_COL32(r, g, b, a);
            
            drawList->AddCircleFilled(p, pointRadius, fadedColor);
        }

        // Draw a larger circle at the end point to show where the object will end up
        if (!screenPoints.empty() && screenPoints.back().x >= 0)
        {
            const ImVec2& endPoint = screenPoints.back();
            drawList->AddCircleFilled(endPoint, 4.0f, color);
            drawList->AddCircle(endPoint, 6.0f, color, 16, 2.0f);
        }
    }


}