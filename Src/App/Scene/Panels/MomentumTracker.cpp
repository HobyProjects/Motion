#include "CorePCH.hpp"
#include "MomentumTracker.hpp"

namespace Motion
{
    void MomentumTracker::OnUpdate(Scene* scene, float dt)
    {
        if (!scene) return;
        
        auto& context = scene->GetContext();
        if (!context.Panels->ShowMomentumPanel) return;
        
        // Get selected entity
        auto selectedEntity = context.Simulation->SelectedEntity;
        if (selectedEntity == entt::null) return;
        
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(selectedEntity);
        if (!rb || !rb->PhysicsBody) return;
        
        // Get physics data from ReactPhysics3D
        rp3d::Vector3 rp3dLinearVel = rb->PhysicsBody->getLinearVelocity();
        rp3d::Vector3 rp3dAngularVel = rb->PhysicsBody->getAngularVelocity();
        rp3d::Vector3 rp3dInertia = rb->PhysicsBody->getLocalInertiaTensor();
        float mass = rb->PhysicsBody->getMass();
        
        // Convert to GLM vectors
        glm::vec3 linearVelocity(rp3dLinearVel.x, rp3dLinearVel.y, rp3dLinearVel.z);
        glm::vec3 angularVelocity(rp3dAngularVel.x, rp3dAngularVel.y, rp3dAngularVel.z);
        glm::vec3 inertia(rp3dInertia.x, rp3dInertia.y, rp3dInertia.z);
        
        // Calculate linear momentum: p = m * v
        glm::vec3 linearMomentum = mass * linearVelocity;
        
        // Calculate angular momentum: L = I * ω (simplified)
        glm::vec3 angularMomentum = inertia * angularVelocity;
        
        // Calculate speeds (magnitudes)
        float linearSpeed = glm::length(linearVelocity);
        float angularSpeed = glm::length(angularVelocity);
        
        // Update the momentum tracker with all data
        context.Physics->PhysicsAnalysis.Momentum.Update(
            linearMomentum, 
            angularMomentum,
            linearSpeed,
            angularSpeed,
            dt
        );
    }

    static void RenderEmptyGraph(const char* plotId, const char* xLabel, const char* yLabel)
    {
        if (ImPlot::BeginPlot(plotId, ImVec2(-1, 220)))
        {
            ImPlot::SetupAxes(xLabel, yLabel);
            ImPlot::EndPlot();
        }
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                        "Start simulation to record data...");
    }

    static void RenderMotionGraph(const char* plotId, const std::deque<ImVec2>& data,
                        float currentTime, float timeWindow,
                        const char* xLabel, const char* yLabel,
                        const ImVec4& color, const char* label)
    {
        if (ImPlot::BeginPlot(plotId, ImVec2(-1, 220)))
        {
            ImPlot::SetupAxes(xLabel, yLabel, ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxisLimits(ImAxis_X1, currentTime - timeWindow, currentTime, ImGuiCond_Always);
            
            ImPlot::PushStyleColor(ImPlotCol_Line, color);
            ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, 2.5f);
            
            // Fill effect
            ImVec4 fillColor = color;
            fillColor.w = 0.2f;
            ImPlot::SetNextFillStyle(fillColor);
            
            std::vector<float> times, values;
            for (const auto& point : data)
            {
                times.push_back(point.x);
                values.push_back(point.y);
            }
            
            if (!times.empty())
            {
                ImPlot::PlotLine(label, times.data(), values.data(), times.size());
                ImPlot::PlotShaded(label, times.data(), values.data(), times.size());
            }
            
            ImPlot::PopStyleVar();
            ImPlot::PopStyleColor();
            ImPlot::EndPlot();
        }
    }

    static void RenderMotionGraphWithReference(const char* plotId, const std::deque<ImVec2>& data,
                                    float currentTime, float timeWindow,
                                    const char* xLabel, const char* yLabel,
                                    const ImVec4& color, const char* label,
                                    float referenceValue, const glm::vec3& initialMomentum)
    {
        if (ImPlot::BeginPlot(plotId, ImVec2(-1, 220)))
        {
            ImPlot::SetupAxes(xLabel, yLabel, ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxisLimits(ImAxis_X1, currentTime - timeWindow, currentTime, ImGuiCond_Always);
            
            ImPlot::PushStyleColor(ImPlotCol_Line, color);
            ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, 2.5f);
            
            // Fill effect
            ImVec4 fillColor = color;
            fillColor.w = 0.2f;
            ImPlot::SetNextFillStyle(fillColor);
            
            std::vector<float> times, values;
            for (const auto& point : data)
            {
                times.push_back(point.x);
                values.push_back(point.y);
            }
            
            if (!times.empty())
            {
                ImPlot::PlotLine(label, times.data(), values.data(), times.size());
                ImPlot::PlotShaded(label, times.data(), values.data(), times.size());
                
                // Add reference line if initial momentum is set
                if (glm::length(initialMomentum) > EPSILON)
                {
                    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.5f, 0.5f, 0.5f, 0.8f));
                    ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, 1.5f);
                    
                    std::vector<float> refTimes = {times.front(), times.back()};
                    std::vector<float> refValues = {referenceValue, referenceValue};
                    
                    ImPlot::PlotLine("Initial", refTimes.data(), refValues.data(), 2);
                    
                    ImPlot::PopStyleVar();
                    ImPlot::PopStyleColor();
                }
            }
            
            ImPlot::PopStyleVar();
            ImPlot::PopStyleColor();
            ImPlot::EndPlot();
        }
    }


    void MomentumTracker::OnRender(Scene* scene)
    {
        if(!scene) return;

        auto& context = scene->GetContext();
        if (!context.Panels->ShowMomentumPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::SetNextWindowSize(ImVec2(16, 16), ImGuiCond_FirstUseEver);
        ImGui::Begin("Momentum Analysis", &context.Panels->ShowMomentumPanel);

        if(!context.Simulation->InSimulation)
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Start simulation to record data...");
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }
        
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "Momentum & Motion Analysis");
        ImGui::Separator();
        ImGui::Spacing();
        
        auto& momentum = context.Physics->PhysicsAnalysis.Momentum;
        
        // Get selected entity
        auto selectedEntity = context.Simulation->SelectedEntity;
        if (selectedEntity == entt::null)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 
                            "Select an object to analyze its momentum and motion");
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }
        
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(selectedEntity);
        if (!rb || !rb->PhysicsBody)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 
                            "Selected object doesn't have a physics body!");
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }
        
        auto* tag = context.Entities->Registry.try_get<TagComponent>(selectedEntity);
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), 
                        "Analyzing: %s", tag ? tag->Tag.c_str() : "Unknown");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Get current physics data from ReactPhysics3D
        rp3d::Vector3 rp3dLinearVel = rb->PhysicsBody->getLinearVelocity();
        rp3d::Vector3 rp3dAngularVel = rb->PhysicsBody->getAngularVelocity();
        float mass = rb->PhysicsBody->getMass();
        rp3d::Vector3 localInertia = rb->PhysicsBody->getLocalInertiaTensor();
        
        glm::vec3 linearVelocity(rp3dLinearVel.x, rp3dLinearVel.y, rp3dLinearVel.z);
        glm::vec3 angularVelocity(rp3dAngularVel.x, rp3dAngularVel.y, rp3dAngularVel.z);
        glm::vec3 inertia(localInertia.x, localInertia.y, localInertia.z);
        
        // Calculate momentum values
        glm::vec3 currentLinearMomentum = linearVelocity * mass;
        glm::vec3 currentAngularMomentum = angularVelocity * inertia; // Simplified L = Iω
        float linearSpeed = glm::length(linearVelocity);
        float angularSpeed = glm::length(angularVelocity);
        
        // Update momentum tracker
        if (context.Simulation->InSimulation)
        {
            momentum.Update(currentLinearMomentum, currentAngularMomentum, 
                        linearSpeed, angularSpeed, ImGui::GetIO().DeltaTime);
        }
        
        // ============================================================================
        // LINEAR MOMENTUM SECTION
        // ============================================================================
        if (ImGui::CollapsingHeader("Linear Momentum", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent(10);
            
            ImGui::Text("Mass: %.2f kg", mass);
            ImGui::Spacing();
            
            ImGui::Text("Velocity: (%.3f, %.3f, %.3f) m/s", 
                    linearVelocity.x, linearVelocity.y, linearVelocity.z);
            ImGui::Text("Speed: %.2f m/s  (%.1f km/h)", linearSpeed, linearSpeed * 3.6f);
            ImGui::Spacing();
            
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Linear Momentum (p = mv)");
            ImGui::Text("Magnitude: %.2f kg⋅m/s", momentum.LinearMagnitude);
            ImGui::Text("Vector: (%.3f, %.3f, %.3f) kg⋅m/s", 
                    momentum.LinearMomentum.x, momentum.LinearMomentum.y, momentum.LinearMomentum.z);
            
            if (momentum.LinearMagnitude > EPSILON)
            {
                glm::vec3 momentumDir = glm::normalize(momentum.LinearMomentum);
                ImGui::Text("Direction: (%.3f, %.3f, %.3f)", 
                        momentumDir.x, momentumDir.y, momentumDir.z);
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Conservation Analysis
            if (glm::length(momentum.InitialLinearMomentum) > EPSILON)
            {
                float conservation = momentum.GetLinearConservationPercentage();
                ImGui::Text("Conservation Status: ");
                ImGui::SameLine();
                
                ImVec4 color = conservation > 95.0f ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : 
                            conservation > 85.0f ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : 
                            ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                
                ImGui::TextColored(color, "%.1f%%", conservation);
                
                ImGui::Text("Initial: %.2f kg⋅m/s", glm::length(momentum.InitialLinearMomentum));
                ImGui::Text("Current: %.2f kg⋅m/s", momentum.LinearMagnitude);
                
                float change = momentum.LinearMagnitude - glm::length(momentum.InitialLinearMomentum);
                float changePercent = glm::length(momentum.InitialLinearMomentum) > EPSILON ? 
                                    (change / glm::length(momentum.InitialLinearMomentum)) * 100.0f : 0.0f;
                ImGui::Text("Change: %.2f kg⋅m/s (%.1f%%)", change, changePercent);
            }
            else
            {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                                "Click 'Set Initial Momentum' to track conservation");
            }
            
            ImGui::Unindent(10);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // ============================================================================
        // ANGULAR MOMENTUM SECTION
        // ============================================================================
        if (ImGui::CollapsingHeader("Angular Momentum", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent(10);
            
            ImGui::Text("Moment of Inertia: (%.2f, %.2f, %.2f) kg⋅m²", 
                    inertia.x, inertia.y, inertia.z);
            ImGui::Spacing();
            
            ImGui::Text("Angular Velocity: (%.3f, %.3f, %.3f) rad/s", 
                    angularVelocity.x, angularVelocity.y, angularVelocity.z);
            float rpm = (angularSpeed * 60.0f) / (2.0f * glm::pi<float>());
            ImGui::Text("Rotation Rate: %.2f rad/s  (%.0f RPM)", angularSpeed, std::abs(rpm));
            ImGui::Spacing();
            
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Angular Momentum (L = Iω)");
            ImGui::Text("Magnitude: %.2f kg⋅m²/s", momentum.AngularMagnitude);
            ImGui::Text("Vector: (%.3f, %.3f, %.3f) kg⋅m²/s", 
                    momentum.AngularMomentum.x, momentum.AngularMomentum.y, momentum.AngularMomentum.z);
            
            if (momentum.AngularMagnitude > EPSILON)
            {
                glm::vec3 angMomDir = glm::normalize(momentum.AngularMomentum);
                ImGui::Text("Axis: (%.3f, %.3f, %.3f)", angMomDir.x, angMomDir.y, angMomDir.z);
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Angular Conservation
            if (glm::length(momentum.InitialAngularMomentum) > EPSILON)
            {
                float conservation = momentum.GetAngularConservationPercentage();
                ImGui::Text("Conservation Status: ");
                ImGui::SameLine();
                
                ImVec4 color = conservation > 95.0f ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : 
                            conservation > 85.0f ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : 
                            ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                
                ImGui::TextColored(color, "%.1f%%", conservation);
                
                ImGui::Text("Initial: %.2f kg⋅m²/s", glm::length(momentum.InitialAngularMomentum));
                ImGui::Text("Current: %.2f kg⋅m²/s", momentum.AngularMagnitude);
            }
            
            ImGui::Unindent(10);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // ============================================================================
        // MOTION GRAPHS SECTION
        // ============================================================================
        if (ImGui::CollapsingHeader("Motion Graphs", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent(10);
            
            // Time window control
            ImGui::Text("Graph Time Window:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            ImGui::SliderFloat("##timewindow", &momentum.GraphTimeWindow, 1.0f, 60.0f, "%.1f sec");
            ImGui::SameLine();
            ShowPhysicsTooltip("Time Window", 
                "How much historical data to display in the graphs\n"
                "• Short (1-10s): See recent details\n"
                "• Long (30-60s): See overall patterns");
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // ========================================================================
            // LINEAR SPEED GRAPH
            // ========================================================================
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Linear Speed Over Time");
            ImGui::SameLine();
            ShowPhysicsTooltip("Linear Speed", 
                "Speed = |v| = √(vx² + vy² + vz²)\n\n"
                "Shows how fast the object is moving\n"
                "Examples:\n"
                "• Walking: ~1.4 m/s (5 km/h)\n"
                "• Running: ~4 m/s (15 km/h)\n"
                "• Car: ~14 m/s (50 km/h)");
            
            if (!momentum.LinearSpeedHistory.empty())
            {
                RenderMotionGraph(
                    "##LinearSpeedPlot",
                    momentum.LinearSpeedHistory,
                    momentum.TimeAccumulator,
                    momentum.GraphTimeWindow,
                    "Time (s)", "Speed (m/s)",
                    ImVec4(0.3f, 0.8f, 0.3f, 1.0f),
                    "Speed"
                );
                
                ImGui::Spacing();
                ImGui::Text("Current Speed: ");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), 
                                "%.2f m/s  (%.1f km/h)", linearSpeed, linearSpeed * 3.6f);
            }
            else
            {
                RenderEmptyGraph("##LinearSpeedPlot", "Time (s)", "Speed (m/s)");
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // ========================================================================
            // ANGULAR SPEED GRAPH
            // ========================================================================
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Rotational Speed Over Time");
            ImGui::SameLine();
            ShowPhysicsTooltip("Angular Speed",
                "Angular Speed = |ω| = √(ωx² + ωy² + ωz²)\n\n"
                "Shows how fast the object is spinning\n"
                "Examples:\n"
                "• Ceiling fan: ~60-300 RPM\n"
                "• Car engine idle: ~800 RPM\n"
                "• Car engine cruising: ~2000-3000 RPM");
            
            if (!momentum.AngularSpeedHistory.empty())
            {
                RenderMotionGraph(
                    "##AngularSpeedPlot",
                    momentum.AngularSpeedHistory,
                    momentum.TimeAccumulator,
                    momentum.GraphTimeWindow,
                    "Time (s)", "Angular Speed (rad/s)",
                    ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                    "Angular Speed"
                );
                
                ImGui::Spacing();
                float currentRPM = (angularSpeed * 60.0f) / (2.0f * glm::pi<float>());
                ImGui::Text("Current Rotation: ");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), 
                                "%.2f rad/s  (%.0f RPM)", angularSpeed, std::abs(currentRPM));
            }
            else
            {
                RenderEmptyGraph("##AngularSpeedPlot", "Time (s)", "Angular Speed (rad/s)");
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // ========================================================================
            // MOMENTUM MAGNITUDE GRAPH
            // ========================================================================
            ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "Momentum Magnitude Over Time");
            ImGui::SameLine();
            ShowPhysicsTooltip("Momentum History",
                "p = mv (momentum = mass × velocity)\n\n"
                "Track momentum changes and conservation\n"
                "In closed systems, momentum is conserved");
            
            if (!momentum.MomentumHistory.empty())
            {
                RenderMotionGraphWithReference(
                    "##MomentumPlot",
                    momentum.MomentumHistory,
                    momentum.TimeAccumulator,
                    momentum.GraphTimeWindow,
                    "Time (s)", "Momentum (kg⋅m/s)",
                    ImVec4(0.8f, 0.4f, 1.0f, 1.0f),
                    "Momentum",
                    glm::length(momentum.InitialLinearMomentum),
                    momentum.InitialLinearMomentum
                );
                
                ImGui::Spacing();
                ImGui::Text("Current Momentum: ");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), 
                                "%.2f kg⋅m/s", momentum.LinearMagnitude);
            }
            else
            {
                RenderEmptyGraph("##MomentumPlot", "Time (s)", "Momentum (kg⋅m/s)");
            }
            
            // Clear data button
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            if (ImGui::Button("Clear Graph Data", ImVec2(150, 0)))
            {
                momentum.ClearGraphData();
            }
            ImGui::SameLine();
            ShowPhysicsTooltip("Clear Data", "Reset all graph data and start recording fresh");
            
            ImGui::Unindent(10);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // ============================================================================
        // VISUALIZATION OPTIONS
        // ============================================================================
        if (ImGui::CollapsingHeader("Visualization Options"))
        {
            ImGui::Indent(10);
            
            ImGui::Checkbox("Show Linear Momentum Vector", &momentum.ShowMomentumVector);
            ImGui::SameLine(); 
            ShowPhysicsTooltip("Linear Momentum Vector", 
                "Display linear momentum vector from the object's center of mass");
            
            ImGui::Checkbox("Show Angular Momentum Vector", &momentum.ShowAngularMomentumVector);
            ImGui::SameLine(); 
            ShowPhysicsTooltip("Angular Momentum Vector",
                "Display angular momentum vector (perpendicular to rotation plane)");
            
            ImGui::Checkbox("Track Conservation", &momentum.TrackConservation);
            ImGui::SameLine(); 
            ShowPhysicsTooltip("Conservation Tracking", 
                "Monitor how well momentum is conserved during collisions and interactions");
            
            ImGui::Spacing();
            ImGui::SliderFloat("Vector Scale", &momentum.VectorScale, 0.1f, 3.0f, "%.2fx");
            
            ImGui::Unindent(10);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // ============================================================================
        // ACTION BUTTONS
        // ============================================================================
        if (ImGui::Button("Set Initial Momentum", ImVec2(180, 0)))
        {
            momentum.InitialLinearMomentum = currentLinearMomentum;
            momentum.InitialAngularMomentum = currentAngularMomentum;
            MOTION_INFO("Initial momentum set - Linear: ({:.2f}, {:.2f}, {:.2f}) kg⋅m/s", 
                    currentLinearMomentum.x, currentLinearMomentum.y, currentLinearMomentum.z);
        }
        ImGui::SameLine();
        ShowPhysicsTooltip("Set Initial", 
            "Capture current momentum as reference point for conservation tracking");
        
        ImGui::SameLine();
        if (ImGui::Button("Reset All", ImVec2(120, 0)))
        {
            momentum.Reset();
        }
        ImGui::SameLine();
        ShowPhysicsTooltip("Reset All", "Clear all momentum data and graphs");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // ============================================================================
        // EDUCATIONAL INFO
        // ============================================================================
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.25f, 0.35f, 0.9f));
        if (ImGui::BeginChild("MomentumInfo", ImVec2(-1, 150), true))
        {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Key Physics Equations:");
            ImGui::Spacing();
            
            RenderPhysicsEquation("p = mv", "Linear Momentum");
            RenderPhysicsEquation("L = Iω", "Angular Momentum");
            RenderPhysicsEquation("Δp = FΔt", "Impulse-Momentum Theorem");
            RenderPhysicsEquation("Στ = dL/dt", "Torque and Angular Momentum");
            
            ImGui::Spacing();
            ImGui::TextWrapped("Conservation of Momentum: In a closed system with no external "
                            "forces, total momentum remains constant. This fundamental principle "
                            "is essential for analyzing collisions and interactions.");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        
        ImGui::End();
        ImGui::PopStyleVar();
    }

}