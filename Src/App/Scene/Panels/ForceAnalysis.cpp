#include "CorePCH.hpp"
#include "ForceAnalysis.hpp"

namespace Motion
{
    void ForceAnalysis::OnUpdate(Scene* scene, float dt)
    {
        if (!scene) return;

        auto& context = scene->GetContext();
        if (!context.Panels->ShowForceAnalysisPanel) return;

        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(context.Simulation->SelectedEntity);
        if (!rb) return;
        
        auto& analysis = context.Physics->PhysicsAnalysis.ForceAnalysis;
        analysis.Forces.clear();
        
        // Add gravity force
        ScenePhysics::ForceVector gravity;
        float mass = rb->PhysicsBody->getMass();
        gravity.Name = "Gravity";
        gravity.Force = glm::vec3(0.0f, -mass * 9.81f, 0.0f);
        gravity.Color = IM_COL32(100, 200, 255, 255);
        analysis.Forces.push_back(gravity);
        
        //TODO: Add applied forces from physics body

        
        // Update net force
        analysis.UpdateNetForce();
    }

    void ForceAnalysis::OnRender(Scene* scene)
    {
        if(!scene) return;

        auto& context = scene->GetContext();
        if (!context.Panels->ShowForceAnalysisPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::SetNextWindowSize(ImVec2(16, 16), ImGuiCond_FirstUseEver);
        ImGui::Begin("Force Analysis", &context.Panels->ShowForceAnalysisPanel);

        if(!context.Simulation->InSimulation)
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Start simulation to record data...");
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }
        
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Force Vector Analysis");
        ImGui::Separator();
        ImGui::Spacing();
        
        auto& analysis = context.Physics->PhysicsAnalysis.ForceAnalysis;
        
        // ============================================================================
        // VISUALIZATION SECTION
        // ============================================================================
        if (ImGui::CollapsingHeader("Visualization Options", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent(10);
            
            ImGui::Checkbox("Show Force Vectors", &analysis.ShowForceVectors);
            ImGui::SameLine(); 
            ShowPhysicsTooltip("Force Vectors", "Display arrows representing the magnitude and direction of forces acting on the object");
            
            ImGui::Checkbox("Show Net Force", &analysis.ShowNetForce);
            ImGui::SameLine(); 
            ShowPhysicsTooltip("Net Force", "The vector sum of all forces acting on the object (resultant force)");
            
            ImGui::Checkbox("Show Components", &analysis.ShowComponents);
            ImGui::SameLine(); 
            ShowPhysicsTooltip("Components", "Break down forces into X, Y, and Z components");
            
            ImGui::Spacing();
            ImGui::SliderFloat("Vector Scale", &analysis.VectorScale, 0.1f, 5.0f, "%.2fx");
            ImGui::SliderFloat("Arrow Size", &analysis.ArrowHeadSize, 0.05f, 0.5f);
            
            ImGui::Unindent(10);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // ============================================================================
        // NET FORCE SUMMARY
        // ============================================================================
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Net Force Summary");
        ImGui::Spacing();
        
        ImGui::Text("Magnitude: %.2f N", analysis.NetForceMagnitude);
        
        if (analysis.NetForceMagnitude > EPSILON)
        {
            ImGui::Text("Direction: (%.2f, %.2f, %.2f)", 
                    analysis.NetForce.x, analysis.NetForce.y, analysis.NetForce.z);
            
            glm::vec3 unitDir = glm::normalize(analysis.NetForce);
            ImGui::Text("Unit Vector: (%.3f, %.3f, %.3f)", 
                    unitDir.x, unitDir.y, unitDir.z);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                            "Object is in equilibrium (ΣF = 0)");
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // ============================================================================
        // ACTIVE FORCES LIST
        // ============================================================================
        ImGui::Text("Active Forces (%zu)", analysis.Forces.size());    
        if (ImGui::BeginChild("ForcesList", ImVec2(0, 250), true))
        {
            for (size_t i = 0; i < analysis.Forces.size(); ++i)
            {
                auto& force = analysis.Forces[i];
                
                ImGui::PushID(static_cast<int>(i));
                ImGui::ColorButton("##color", ImGui::ColorConvertU32ToFloat4(force.Color), 
                                ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20));
                ImGui::SameLine();
                
                ImGui::Checkbox(force.Name.c_str(), &force.IsActive);
                
                if (force.IsActive)
                {
                    ImGui::Indent(30);
                    ImGui::Text("Magnitude: %.2f N", force.GetMagnitude());
                    ImGui::Text("Force: (%.2f, %.2f, %.2f) N", 
                            force.Force.x, force.Force.y, force.Force.z);
                    
                    if (analysis.ShowComponents && force.GetMagnitude() > EPSILON)
                    {
                        ImGui::Text("Components:");
                        ImGui::BulletText("Fx: %.2f N", force.Force.x);
                        ImGui::BulletText("Fy: %.2f N", force.Force.y);
                        ImGui::BulletText("Fz: %.2f N", force.Force.z);
                    }
                    ImGui::Unindent(30);
                }
                
                ImGui::PopID();
                
                if (i < analysis.Forces.size() - 1)
                    ImGui::Separator();
            }
        }
        ImGui::EndChild();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // ============================================================================
        // APPLY FORCES SECTION
        // ============================================================================
        if (ImGui::CollapsingHeader("Apply Forces & Impulses", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent(10);
            
            // Get selected entity
            auto selectedEntity = context.Simulation->SelectedEntity;
            if (selectedEntity == entt::null)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 
                                "Select an object in the scene to apply forces");
                ImGui::Unindent(10);
                ImGui::End();
                ImGui::PopStyleVar();
                return;
            }
            
            auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(selectedEntity);
            if (!rb || !rb->PhysicsBody)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 
                                "Selected object doesn't have a physics body!");
                ImGui::Unindent(10);
                ImGui::End();
                ImGui::PopStyleVar();
                return;
            }
            
            auto* tag = context.Entities->Registry.try_get<TagComponent>(selectedEntity);
            ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), 
                            "Target: %s", tag ? tag->Tag.c_str() : "Unknown");
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Force/Impulse type selection
            static int forceType = 0; // 0 = Force, 1 = Impulse
            ImGui::Text("Application Type:");
            ImGui::RadioButton("Continuous Force", &forceType, 0);
            ImGui::SameLine();
            ShowPhysicsTooltip("Continuous Force", 
                "Applied every frame while active\n"
                "F = ma (Force = mass × acceleration)\n"
                "Good for: Wind, thrust, sustained pushes");
            
            ImGui::SameLine(0, 20);
            ImGui::RadioButton("Instant Impulse", &forceType, 1);
            ImGui::SameLine();
            ShowPhysicsTooltip("Instant Impulse",
                "Applied once, instant velocity change\n"
                "J = mΔv (Impulse = mass × change in velocity)\n"
                "Good for: Collisions, explosions, kicks");
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Direction controls
            static glm::vec3 forceDirection(0.0f, 1.0f, 0.0f);
            ImGui::Text("Direction:");
            ImGui::SetNextItemWidth(280.0f);
            ImGui::DragFloat3("##force_dir", &forceDirection.x, 0.01f, -1.0f, 1.0f, "%.2f");
            ImGui::SameLine();
            if (ImGui::Button("Normalize"))
            {
                if (glm::length(forceDirection) > EPSILON)
                    forceDirection = glm::normalize(forceDirection);
            }
            ImGui::SameLine();
            ShowPhysicsTooltip("Direction", "The direction in which the force/impulse will be applied");
            
            // Direction presets
            ImGui::Spacing();
            ImGui::Text("Quick Directions:");
            ImGui::Indent(10);
            if (ImGui::Button("Up (Y+)")) forceDirection = glm::vec3(0, 1, 0);
            ImGui::SameLine();
            if (ImGui::Button("Down (Y-)")) forceDirection = glm::vec3(0, -1, 0);
            ImGui::SameLine();
            if (ImGui::Button("Right (X+)")) forceDirection = glm::vec3(1, 0, 0);
            ImGui::SameLine();
            if (ImGui::Button("Left (X-)")) forceDirection = glm::vec3(-1, 0, 0);
            
            if (ImGui::Button("Forward (Z+)")) forceDirection = glm::vec3(0, 0, 1);
            ImGui::SameLine();
            if (ImGui::Button("Back (Z-)")) forceDirection = glm::vec3(0, 0, -1);
            ImGui::Unindent(10);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Magnitude control
            static float forceMagnitude = 10.0f;
            ImGui::Text("Magnitude:");
            ImGui::SetNextItemWidth(350.0f);
            
            if (forceType == 0)
            {
                ImGui::SliderFloat("##force_mag", &forceMagnitude, 0.1f, 1000.0f, 
                                "%.2f N", ImGuiSliderFlags_Logarithmic);
                ImGui::SameLine();
                ShowPhysicsTooltip("Force Magnitude", 
                    "Strength of the force in Newtons (N)\n\n"
                    "Reference values:\n"
                    "• 1 N = ~100g weight\n"
                    "• 10 N = ~1kg weight\n"
                    "• 100 N = Strong push");
            }
            else
            {
                ImGui::SliderFloat("##impulse_mag", &forceMagnitude, 0.1f, 1000.0f, 
                                "%.2f N·s", ImGuiSliderFlags_Logarithmic);
                ImGui::SameLine();
                ShowPhysicsTooltip("Impulse Magnitude",
                    "Strength of the impulse in Newton-seconds (N·s)\n\n"
                    "Reference values:\n"
                    "• 1 N·s = Gentle tap\n"
                    "• 10 N·s = Moderate hit\n"
                    "• 100 N·s = Strong impact");
            }
            
            // Magnitude presets
            ImGui::Spacing();
            ImGui::Text("Preset Magnitudes:");
            ImGui::Indent(10);
            if (ImGui::Button("Weak (1)")) forceMagnitude = 1.0f;
            ImGui::SameLine();
            if (ImGui::Button("Light (5)")) forceMagnitude = 5.0f;
            ImGui::SameLine();
            if (ImGui::Button("Medium (10)")) forceMagnitude = 10.0f;
            ImGui::SameLine();
            if (ImGui::Button("Strong (50)")) forceMagnitude = 50.0f;
            ImGui::SameLine();
            if (ImGui::Button("Very Strong (100)")) forceMagnitude = 100.0f;
            ImGui::Unindent(10);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Application point
            static bool useLocalPosition = false;
            static glm::vec3 applicationPoint(0.0f);
            
            ImGui::Text("Application Point:");
            ImGui::Checkbox("Use Local Position", &useLocalPosition);
            ImGui::SameLine();
            ShowPhysicsTooltip("Local vs World",
                "Local: Relative to object's center (moves with object)\n"
                "World: Absolute position in world space (fixed location)\n\n"
                "Applying force off-center creates torque (rotation)");
            
            ImGui::SetNextItemWidth(280.0f);
            ImGui::DragFloat3("##app_point", &applicationPoint.x, 0.1f, -100.0f, 100.0f, "%.2f");
            ImGui::SameLine();
            if (ImGui::Button("Reset##point"))
                applicationPoint = glm::vec3(0.0f);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Application mode
            static int applicationMode = 0; // 0 = Linear, 1 = Torque, 2 = At Point
            ImGui::Text("Application Mode:");
            ImGui::RadioButton("Linear (Center of Mass)", &applicationMode, 0);
            ImGui::SameLine();
            ShowPhysicsTooltip("Linear Force", "Applied at center of mass, causes only translation (no rotation)");
            
            ImGui::RadioButton("Torque (Rotational)", &applicationMode, 1);
            ImGui::SameLine();
            ShowPhysicsTooltip("Torque", "Causes rotation around the direction vector as axis");
            
            ImGui::RadioButton("At Specific Point", &applicationMode, 2);
            ImGui::SameLine();
            ShowPhysicsTooltip("Force at Point", "Applied at specified point, can cause both translation and rotation");
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Apply buttons
            ImVec4 buttonColor = forceType == 0 ? 
                ImVec4(0.3f, 0.7f, 0.3f, 1.0f) : ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
            
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 
                ImVec4(buttonColor.x * 1.2f, buttonColor.y * 1.2f, buttonColor.z * 1.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                ImVec4(buttonColor.x * 0.8f, buttonColor.y * 0.8f, buttonColor.z * 0.8f, 1.0f));
            
            const char* buttonText = forceType == 0 ? "Apply Force" : "Apply Impulse";
            if (ImGui::Button(buttonText, ImVec2(-1, 40)))
            {
                glm::vec3 vector = forceDirection * forceMagnitude;
                reactphysics3d::Vector3 rp3dVector(vector.x, vector.y, vector.z);
                
                if (forceType == 0) // Continuous Force
                {
                    if (applicationMode == 0) // Linear
                    {
                        rb->PhysicsBody->applyWorldForceAtCenterOfMass(rp3dVector);
                        MOTION_INFO("Applied force: ({:.2f}, {:.2f}, {:.2f}) N", 
                                vector.x, vector.y, vector.z);
                    }
                    else if (applicationMode == 1) // Torque
                    {
                        rb->PhysicsBody->applyWorldTorque(rp3dVector);
                        MOTION_INFO("Applied torque: ({:.2f}, {:.2f}, {:.2f}) N·m", 
                                vector.x, vector.y, vector.z);
                    }
                    else // At Point
                    {
                        reactphysics3d::Vector3 point(applicationPoint.x, applicationPoint.y, applicationPoint.z);
                        if (useLocalPosition)
                            rb->PhysicsBody->applyWorldForceAtLocalPosition(rp3dVector, point);
                        else
                            rb->PhysicsBody->applyWorldForceAtWorldPosition(rp3dVector, point);
                        
                        MOTION_INFO("Applied force at point ({:.2f}, {:.2f}, {:.2f})", 
                                applicationPoint.x, applicationPoint.y, applicationPoint.z);
                    }
                }
                else // Instant Impulse
                {
                    if (applicationMode == 0) // Linear
                    {
                        reactphysics3d::Vector3 currentVel = rb->PhysicsBody->getLinearVelocity();
                        reactphysics3d::Vector3 newVel = currentVel + rp3dVector / rb->PhysicsBody->getMass();
                        rb->PhysicsBody->setLinearVelocity(newVel);
                        MOTION_INFO("Applied impulse: ({:.2f}, {:.2f}, {:.2f}) N·s", 
                                vector.x, vector.y, vector.z);
                    }
                    else if (applicationMode == 1) // Angular Impulse
                    {
                        reactphysics3d::Vector3 currentAngVel = rb->PhysicsBody->getAngularVelocity();
                        // Simplified angular impulse (you may need proper inertia tensor calculation)
                        rb->PhysicsBody->setAngularVelocity(currentAngVel + rp3dVector * 0.1f);
                        MOTION_INFO("Applied angular impulse: ({:.2f}, {:.2f}, {:.2f}) N·m·s", 
                                vector.x, vector.y, vector.z);
                    }
                    else 
                    {
                        // Calculate impulse components
                        reactphysics3d::Vector3 point(applicationPoint.x, applicationPoint.y, applicationPoint.z);
                        reactphysics3d::Vector3 centerOfMass = rb->PhysicsBody->getTransform().getPosition();
                        reactphysics3d::Vector3 r = useLocalPosition ? 
                            rb->PhysicsBody->getTransform().getOrientation() * point : 
                            point - centerOfMass;
                        
                        // Linear impulse
                        reactphysics3d::Vector3 linearImpulse = rp3dVector / rb->PhysicsBody->getMass();
                        rb->PhysicsBody->setLinearVelocity(
                            rb->PhysicsBody->getLinearVelocity() + linearImpulse);
                        
                        // Angular impulse (torque = r × F)
                        reactphysics3d::Vector3 torqueImpulse = r.cross(rp3dVector);
                        rb->PhysicsBody->setAngularVelocity(
                            rb->PhysicsBody->getAngularVelocity() + torqueImpulse * 0.01f);
                        
                        MOTION_INFO("Applied impulse at point");
                    }
                }
            }
            
            ImGui::PopStyleColor(3);
            
            ImGui::Spacing();
            
            // Info about selected mode
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            if (forceType == 0)
            {
                ImGui::TextWrapped("Note: Continuous forces are applied every physics step while the simulation runs. "
                                "They will continue until you stop the simulation or the object leaves the force field.");
            }
            else
            {
                ImGui::TextWrapped("Note: Impulses provide an instant change in velocity. "
                                "Click the button each time you want to apply an impulse.");
            }
            ImGui::PopStyleColor();
            
            ImGui::Unindent(10);
        }
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
}