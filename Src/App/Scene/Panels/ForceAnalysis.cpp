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
        
        ScenePhysics::ForceVector gravity;
        float mass = rb->PhysicsBody->getMass();
        gravity.Name = "Gravity";
        gravity.Force = glm::vec3(0.0f, -mass * 9.81f, 0.0f);
        gravity.Color = IM_COL32(100, 200, 255, 255);
        analysis.AddForce(gravity);
        
        analysis.Update();
    }

    void ForceAnalysis::OnRender(Scene* scene)
    {
        if(!scene) return;

        auto& context = scene->GetContext();
        if (!context.Panels->ShowForceAnalysisPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Force Analysis", &context.Panels->ShowForceAnalysisPanel);
        bool inSimulation = context.Simulation->InSimulation;

        if(!context.Simulation->InSimulation)
        {
            HeadingConfig config;
            config.Separator = true;
            Heading("Simulation Mode Required", HeadingLevel::H1, config);

            LabelConfig lblConfig;
            lblConfig.Wrapped = true;
            LabelSimple("Force tracking is only available during active simulation. Please start the simulation to view force data.", lblConfig);
        }
        
        ImGui::BeginDisabled(!inSimulation);
        auto& analysis = context.Physics->PhysicsAnalysis.ForceAnalysis;
        
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_AUTO_GRAPH " Visualization Options", HeadingLevel::H2, config);
            
            ToggleSwitch("Show Force Vectors", &analysis.ShowForceVectors, ToggleSwitchPresets::iOS());
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Display arrows representing the magnitude and direction\nof forces acting on the object");
            
            ToggleSwitch("Show Net Force", &analysis.ShowNetForce, ToggleSwitchPresets::iOS());
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("The vector sum of all forces acting on the object\n(resultant force)");
            
            ToggleSwitch("Show Components", &analysis.ShowComponents, ToggleSwitchPresets::iOS());
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Break down forces into X, Y, and Z components");
        }
        
        ImGui::Spacing();
        
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_ROCKET_LAUNCH " Net Force Summary", HeadingLevel::H2, config);
            
            LabelConfig magConfig;
            LabelValue("Magnitude", analysis.NetForceMagnitude, "%.2f N", magConfig);
            
            if (analysis.NetForceMagnitude > EPSILON)
            {
                LabelConfig dirConfig;
                LabelValue("Direction", analysis.NetForce, "%.2f", dirConfig);

                glm::vec3 unitDir = glm::normalize(analysis.NetForce);
                LabelConfig unitConfig;
                LabelValue("Unit Vector", unitDir, "%.2f", unitConfig);
            }
            else
            {
                LabelSimple("Equilibrium State! ΣF = 0");
            }
        }
        
        ImGui::Spacing();
        
        {
            HeadingConfig config;
            config.Separator = true;
            std::string headerText = fmt::format(ICON_MD_ROCKET " Active Forces ({})", analysis.Forces.size());
            Heading(headerText.c_str(), HeadingLevel::H3, config);

            for (size_t i = 0; i < analysis.Forces.size(); ++i)
            {
                auto& force = analysis.Forces[i];
                
                ImGui::PushID(static_cast<int>(i));
                std::string activeForceName = std::format("Force {}: {}", i, force.Name);
                if(ImGui::CollapsingHeader(activeForceName.c_str()))
                {
                    ImGui::BeginGroup();
                    {
                        ImVec4 forceColor = ImGui::ColorConvertU32ToFloat4(force.Color);

                        ColorEditConfig colorConfig;
                        colorConfig.Flags = ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoAlpha;
                        ColorEdit4("Force Display Color", forceColor, colorConfig, [&](glm::vec4 color){
                            force.Color = ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a));
                        });

                        ToggleSwitch("Active", &force.IsActive, ToggleSwitchPresets::iOS());
                        
                        if (force.IsActive)
                        {
                            ImGui::Spacing();
                            LabelConfig detailConfig;
                            LabelValue("Magnitude", force.GetMagnitude(), "%.2f N", detailConfig);
                            LabelValue("Force Vector", force.Force, "%.2f N", detailConfig);
                            
                            if (analysis.ShowComponents && force.GetMagnitude() > EPSILON)
                            {
                                ImGui::Spacing();
                                ImGui::TextDisabled("Components:");
                                ImGui::Indent(20);
                                ImGui::BulletText("Fx: %.2f N", force.Force.x);
                                ImGui::BulletText("Fy: %.2f N", force.Force.y);
                                ImGui::BulletText("Fz: %.2f N", force.Force.z);
                                ImGui::Unindent(20);
                            }
                        }
                    }
                    ImGui::EndGroup();
                }
                
                ImGui::PopID();
                if (i < analysis.Forces.size() - 1) ImGui::Spacing();
            }
        }
        
        ImGui::Spacing();

        {
            auto selectedEntity = context.Simulation->SelectedEntity;
            if (selectedEntity == entt::null)
            {
                HeadingConfig config;
                config.Separator = true;
                Heading(ICON_MD_INFO_OUTLINE " Valid Entity Required!", HeadingLevel::H2, config);

                LabelConfig lblConfig;
                lblConfig.Wrapped = true;
                LabelSimple("Select an object in the scene to apply forces.");
            }
            
            auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(selectedEntity);
            if (!rb)
            {
                HeadingConfig config;
                config.Separator = true;
                Heading(ICON_MD_INFO_OUTLINE " Invalid Entity!", HeadingLevel::H2, config);

                LabelConfig lblConfig;
                lblConfig.Wrapped = true;
                LabelSimple("Selected object has no RigidBody component. Please select a valid physics object.");
            }

            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_GESTURE " Apply Forces & Impulses", HeadingLevel::H2, config);

            static int forceType = 0;
            ComboBoxConfig forceTypeConfig;
            forceTypeConfig.Tooltip = "Select the Force Type";
            ComboBox("Force Type", forceType, { "Force", "Impulse" }, forceTypeConfig);
            
            ImGui::Spacing();
            {
                static glm::vec3 forceDirection(1.0f, 0.0f, 0.0f);
                static float forceMagnitude = 10.0f;
                
                HeadingConfig config;
                config.Separator = true;
                Heading("Direction Vector", HeadingLevel::H3, config);
                
                DragFloatConfig dirConfig;
                dirConfig.Speed = 0.1f;
                dirConfig.MinV = -1.0f;
                dirConfig.MaxV = 1.0f;
                dirConfig.Fmt = "%.2f";
                DragFloat3("Direction", forceDirection, dirConfig);
                
                ImGui::SameLine();
                if(ImGui::Button("Normalize"))
                {
                    float length = glm::length(forceDirection);
                    if(length > EPSILON)
                        forceDirection = glm::normalize(forceDirection);
                }
                
                
                ImGui::TextDisabled("Quick Directions:");
                ImGui::BeginGroup();
                {
                    if(ImGui::Button("+X")) forceDirection = glm::vec3(1, 0, 0);
                    ImGui::SameLine();
                    if(ImGui::Button("-X")) forceDirection = glm::vec3(-1, 0, 0);
                    ImGui::SameLine();
                    if(ImGui::Button("+Y")) forceDirection = glm::vec3(0, 1, 0);
                    ImGui::SameLine();
                    if(ImGui::Button("-Y")) forceDirection = glm::vec3(0, -1, 0);
                    ImGui::SameLine();
                    if(ImGui::Button("+Z")) forceDirection = glm::vec3(0, 0, 1);
                    ImGui::SameLine();
                    if(ImGui::Button("-Z")) forceDirection = glm::vec3(0, 0, -1);
                }
                ImGui::EndGroup();
                

                ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Magnitude");
                Heading(ICON_MD_FLASH_ON " Magnitude", HeadingLevel::H2, config);
                if(ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    if(forceType == 0)
                    {
                        ImGui::Text("Force examples:");
                        ImGui::BulletText("1 N = Weight of a small apple");
                        ImGui::BulletText("10 N = Gentle push");
                        ImGui::BulletText("100 N = Strong push");
                    }
                    else
                    {
                        ImGui::Text("Impulse examples:");
                        ImGui::BulletText("1 N·s = Gentle tap");
                        ImGui::BulletText("10 N·s = Moderate hit");
                        ImGui::BulletText("100 N·s = Strong impact");
                    }
                    ImGui::EndTooltip();
                }
                ImGui::Spacing();


                DragFloatConfig magConfig;
                magConfig.Speed = 1.0f;
                magConfig.MinV = 0.0f;
                magConfig.MaxV = 1000.0f;
                magConfig.Fmt = forceType == 0 ? "%.1f N" : "%.1f N·s";
                DragFloat("Magnitude Value", &forceMagnitude, magConfig);
                
                ImGui::Spacing();
                ImGui::TextDisabled("Preset Magnitudes:");
                ImGui::BeginGroup();
                {
                    if(ImGui::Button("Weak (1)")) forceMagnitude = 1.0f;
                    ImGui::SameLine();
                    if(ImGui::Button("Light (5)")) forceMagnitude = 5.0f;
                    ImGui::SameLine();
                    if(ImGui::Button("Medium (10)")) forceMagnitude = 10.0f;
                    ImGui::SameLine();
                    if(ImGui::Button("Strong (50)")) forceMagnitude = 50.0f;
                    ImGui::SameLine();
                    if(ImGui::Button("Very Strong (100)")) forceMagnitude = 100.0f;
                }
                ImGui::EndGroup();
                ImGui::Spacing();
                

                static bool useLocalPosition = false;
                static glm::vec3 applicationPoint(0.0f);
                static int applicationMode = 0; // 0 = Linear, 1 = Torque, 2 = At Point
                Heading("Application Point", HeadingLevel::H3, config);          
                ToggleSwitch("Use Local Position", &useLocalPosition, ToggleSwitchPresets::iOS());
                if(ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Local: Relative to object's center (moves with object)");
                    ImGui::Text("World: Absolute position in world space (fixed location)");
                    ImGui::Separator();
                    ImGui::Text("Applying force off-center creates torque (rotation)");
                    ImGui::EndTooltip();
                }
                
                ImGui::Spacing();
                
                DragFloatConfig pointConfig;
                pointConfig.Speed = 0.1f;
                pointConfig.MinV = -100.0f;
                pointConfig.MaxV = 100.0f;
                pointConfig.Fmt = "%.2f";
                DragFloat3("Point", applicationPoint, pointConfig);
                
                ImGui::SameLine();
                if(ImGui::Button("Reset##point"))
                    applicationPoint = glm::vec3(0.0f);
                
                ImGui::Spacing();
                

                Heading("Application Mode", HeadingLevel::H2, config);  
                ImGui::Spacing();
                
                ImGui::RadioButton("Linear (Center of Mass)", &applicationMode, 0);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("Applied at center of mass\nCauses only translation (no rotation)");
                
                ImGui::RadioButton("Torque (Rotational)", &applicationMode, 1);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("Causes rotation around the direction vector as axis");
                
                ImGui::RadioButton("At Specific Point", &applicationMode, 2);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("Applied at specified point\nCan cause both translation and rotation");
                
                ImGui::Spacing();

                ImVec4 buttonColor = forceType == 0 ? ImVec4(0.3f, 0.7f, 0.3f, 1.0f) : ImVec4(1.0f, 0.6f, 0.2f, 1.0f);   
                ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonColor.x * 1.2f, buttonColor.y * 1.2f, buttonColor.z * 1.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonColor.x * 0.8f, buttonColor.y * 0.8f, buttonColor.z * 0.8f, 1.0f));
                 
                const char* buttonText = forceType == 0 ? "Apply Force" : "Apply Impulse";
                if (ImGui::Button(buttonText, ImVec2(-1, 45)))
                {
                    glm::vec3 vector = forceDirection * forceMagnitude;
                    reactphysics3d::Vector3 rp3dVector(vector.x, vector.y, vector.z);
                    
                    if (forceType == 0) 
                    {
                        if (applicationMode == 0)
                        {
                            rb->PhysicsBody->applyWorldForceAtCenterOfMass(rp3dVector);
                            MOTION_INFO("Applied force: ({:.2f}, {:.2f}, {:.2f}) N", 
                                    vector.x, vector.y, vector.z);
                        }
                        else if (applicationMode == 1) 
                        {
                            rb->PhysicsBody->applyWorldTorque(rp3dVector);
                            MOTION_INFO("Applied torque: ({:.2f}, {:.2f}, {:.2f}) N·m", 
                                    vector.x, vector.y, vector.z);
                        }
                        else 
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
                    else 
                    {
                        if (applicationMode == 0)
                        {
                            reactphysics3d::Vector3 currentVel = rb->PhysicsBody->getLinearVelocity();
                            reactphysics3d::Vector3 newVel = currentVel + rp3dVector / rb->PhysicsBody->getMass();
                            rb->PhysicsBody->setLinearVelocity(newVel);
                            MOTION_INFO("Applied impulse: ({:.2f}, {:.2f}, {:.2f}) N·s", 
                                    vector.x, vector.y, vector.z);
                        }
                        else if (applicationMode == 1) 
                        {
                            reactphysics3d::Vector3 currentAngVel = rb->PhysicsBody->getAngularVelocity();
                            rb->PhysicsBody->setAngularVelocity(currentAngVel + rp3dVector * 0.1f);
                            MOTION_INFO("Applied angular impulse: ({:.2f}, {:.2f}, {:.2f}) N·m·s", 
                                    vector.x, vector.y, vector.z);
                        }
                        else 
                        {
                            reactphysics3d::Vector3 point(applicationPoint.x, applicationPoint.y, applicationPoint.z);
                            reactphysics3d::Vector3 centerOfMass = rb->PhysicsBody->getTransform().getPosition();
                            reactphysics3d::Vector3 r = useLocalPosition ? 
                                rb->PhysicsBody->getTransform().getOrientation() * point : 
                                point - centerOfMass;
                            
                            reactphysics3d::Vector3 linearImpulse = rp3dVector / rb->PhysicsBody->getMass();
                            rb->PhysicsBody->setLinearVelocity(
                                rb->PhysicsBody->getLinearVelocity() + linearImpulse);
                            
                            reactphysics3d::Vector3 torqueImpulse = r.cross(rp3dVector);
                            rb->PhysicsBody->setAngularVelocity(
                                rb->PhysicsBody->getAngularVelocity() + torqueImpulse * 0.01f);
                            
                            MOTION_INFO("Applied impulse at point");
                        }
                    }
                }
                
                ImGui::PopStyleColor(3);
            }
                
        }
        
        ImGui::EndDisabled();
        ImGui::End();
        ImGui::PopStyleVar();
    }
}