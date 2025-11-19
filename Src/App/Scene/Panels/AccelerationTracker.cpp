#include "CorePCH.hpp"
#include "AccelerationTracker.hpp"

namespace Motion
{
    void AccelerationTracker::OnUpdate(Scene* scene, float dt)
    {
        if (!scene) return;
        
        auto& context = scene->GetContext();
        if (!context.Panels->ShowAccelerationPanel) return;

        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(context.Simulation->SelectedEntity);
        if (!rb) return;

        auto linearVelocity = ToVec3(rb->PhysicsBody->getLinearVelocity());
        context.Physics->PhysicsAnalysis.Acceleration.Update(linearVelocity, dt);
    }

    void AccelerationTracker::OnRender(Scene* scene)
    {
        if(!scene) return;

        auto& context = scene->GetContext();
        if (!context.Panels->ShowAccelerationPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Acceleration Analysis", &context.Panels->ShowAccelerationPanel);

        if(!context.Simulation->InSimulation)
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Start simulation to record data...");
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }
        
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Acceleration Tracking");
        ImGui::Separator();
        ImGui::Spacing();
        
        auto& accel = context.Physics->PhysicsAnalysis.Acceleration;
        
        // Current Acceleration
        ImGui::Text("Current Acceleration");
        ImGui::Text("Magnitude: %.2f m/s²", accel.AccelerationMagnitude);
        ImGui::Text("Direction: (%.2f, %.2f, %.2f)", 
                   accel.CurrentAcceleration.x, accel.CurrentAcceleration.y, accel.CurrentAcceleration.z);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Visualization
        ImGui::Text("Visualization");
        ImGui::Checkbox("Show Acceleration Vector", &accel.ShowVector);
        ImGui::SliderFloat("Vector Scale", &accel.VectorScale, 0.1f, 5.0f);
        ImGui::SameLine();
        ImGui::ColorEdit4("Vector Color", (float*)&accel.VectorColor, 
                         ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Acceleration Graph using ImPlot
        ImGui::Text("Acceleration Magnitude Over Time");
        
        if (!accel.AccelerationHistory.empty() && !accel.TimeStamps.empty())
        {
            // Calculate acceleration magnitudes
            std::vector<float> timeData(accel.TimeStamps.begin(), accel.TimeStamps.end());
            std::vector<float> magnitudeData;
            magnitudeData.reserve(accel.AccelerationHistory.size());
            
            for (const auto& accelVec : accel.AccelerationHistory)
            {
                magnitudeData.push_back(glm::length(accelVec));
            }
            
            // Ensure data sizes match
            size_t dataSize = std::min(timeData.size(), magnitudeData.size());
            
            if (dataSize > 1)
            {
                // Find max acceleration for y-axis scaling
                float maxAccel = *std::max_element(magnitudeData.begin(), magnitudeData.end());
                maxAccel = std::max(maxAccel, 1.0f); // Ensure minimum scale
                
                ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, 2.5f);
                
                if (ImPlot::BeginPlot("##AccelPlot", ImVec2(-1, 200), ImPlotFlags_NoLegend))
                {
                    // Setup axes
                    ImPlot::SetupAxes("Time (s)", "Acceleration (m/s²)", 
                                     ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, maxAccel * 1.1, ImPlotCond_Always);
                    
                    // Plot acceleration magnitude
                    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                    ImPlot::PlotLine("Acceleration", timeData.data(), magnitudeData.data(), 
                                    static_cast<int>(dataSize));
                    ImPlot::PopStyleColor();
                    
                    ImPlot::EndPlot();
                }
                
                ImPlot::PopStyleVar();
                
                // Legend
                ImGui::Spacing();
                ImGui::ColorButton("##accel", ImVec4(1.0f, 0.8f, 0.2f, 1.0f), 
                                  ImGuiColorEditFlags_NoTooltip, ImVec2(15, 15));
                ImGui::SameLine();
                ImGui::Text("Acceleration Magnitude");
            }
        }
        else
        {
            // Empty plot placeholder
            if (ImPlot::BeginPlot("##AccelPlot", ImVec2(-1, 200)))
            {
                ImPlot::SetupAxes("Time (s)", "Acceleration (m/s²)");
                ImPlot::EndPlot();
            }
            
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                              "Start simulation to track acceleration data");
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Action Button
        if (ImGui::Button(ICON_MD_REFRESH " Reset", ImVec2(120, 0)))
        {
            accel.Reset();
        }
        
        ImGui::Spacing();
        
        // Educational Info
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.25f, 0.35f, 0.9f));
        if (ImGui::BeginChild("AccelInfo", ImVec2(-1, 100), true))
        {
            RenderPhysicsEquation("a = Δv/Δt", "Acceleration is change in velocity over time");
            RenderPhysicsEquation("a = F/m", "Acceleration from Newton's Second Law");
            ImGui::Spacing();
            ImGui::TextWrapped("Acceleration is a vector quantity showing how quickly "
                              "velocity changes. It has both magnitude and direction.");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
}