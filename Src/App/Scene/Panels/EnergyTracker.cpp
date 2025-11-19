#include "CorePCH.hpp"
#include "EnergyTracker.hpp"

namespace Motion
{
    void EnergyTracker::OnUpdate(Scene* scene, float dt)
    {
        if (!scene) return;
        
        auto& context = scene->GetContext();
        if (!context.Physics->PhysicsAnalysis.Energy.TrackingEnabled) return;

        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(context.Simulation->SelectedEntity);
        auto* transform = context.Entities->Registry.try_get<TransformComponent>(context.Simulation->SelectedEntity);
        if (!rb || !transform) return;

        auto mass = rb->PhysicsBody->getMass();
        auto linearVelocity = ToVec3(rb->PhysicsBody->getLinearVelocity());
        
        // Calculate kinetic energy: KE = 0.5 * m * v²
        float velocityMag = glm::length(linearVelocity);
        float ke = 0.5f * mass * velocityMag * velocityMag;
        
        // Calculate potential energy: PE = m * g * h
        float pe = mass * context.Physics->PhysicsAnalysis.Energy.GravityMagnitude * transform->Translation.y;
        
        context.Physics->PhysicsAnalysis.Energy.AddSample(ke, pe, dt);
    }

    void EnergyTracker::OnRender(Scene* scene)
    {
        if(!scene) return;

        auto& context = scene->GetContext();
        if (!context.Panels->ShowEnergyPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Energy Analysis", &context.Panels->ShowEnergyPanel);

        if(!context.Simulation->InSimulation)
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Start simulation to record data...");
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }
        
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Energy Tracking");
        ImGui::Separator();
        ImGui::Spacing();
        
        auto& energy = context.Physics->PhysicsAnalysis.Energy;
        
        // Enable/Disable Tracking
        ImGui::Checkbox("Enable Tracking", &energy.TrackingEnabled);
        ImGui::SameLine(); 
        ShowPhysicsTooltip("Energy Tracking", 
            "Record kinetic and potential energy over time to analyze energy conservation");
        
        ImGui::Spacing();
        
        // Current Energy Values - Large Display
        ImGui::BeginGroup();
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.5f, 1.0f));
            ImGui::Text("Kinetic Energy");
            ImGui::PopStyleColor();
            ImGui::SameLine(200);
            ImGui::Text("%.2f J", energy.CurrentKE);
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));
            ImGui::Text("Potential Energy");
            ImGui::PopStyleColor();
            ImGui::SameLine(200);
            ImGui::Text("%.2f J", energy.CurrentPE);
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
            ImGui::Text("Total Energy");
            ImGui::PopStyleColor();
            ImGui::SameLine(200);
            ImGui::Text("%.2f J", energy.CurrentTotal);
        }
        ImGui::EndGroup();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Conservation Analysis
        float conservation = energy.GetEnergyConservation();
        ImGui::Text("Energy Conservation: ");
        ImGui::SameLine();
        
        ImVec4 conservationColor;
        if (conservation > 95.0f)
            conservationColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // Green - excellent
        else if (conservation > 85.0f)
            conservationColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow - good
        else
            conservationColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red - poor
        
        ImGui::TextColored(conservationColor, "%.1f%%", conservation);
        
        if (energy.InitialTotal > EPSILON)
        {
            ImGui::Text("Initial Total: %.2f J", energy.InitialTotal);
            ImGui::Text("Energy Lost: %.2f J", energy.EnergyLoss);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Energy Graph using ImPlot
        ImGui::Text("Energy Over Time");
        
        ImGui::Checkbox("Show KE", &energy.ShowKE);
        ImGui::SameLine();
        ImGui::Checkbox("Show PE", &energy.ShowPE);
        ImGui::SameLine();
        ImGui::Checkbox("Show Total", &energy.ShowTotal);
        
        if (!energy.TimeStamps.empty() && energy.TimeStamps.size() == energy.TotalEnergyHistory.size())
        {
            // Convert deques to vectors for ImPlot
            std::vector<float> timeData(energy.TimeStamps.begin(), energy.TimeStamps.end());
            std::vector<float> keData(energy.KineticEnergyHistory.begin(), energy.KineticEnergyHistory.end());
            std::vector<float> peData(energy.PotentialEnergyHistory.begin(), energy.PotentialEnergyHistory.end());
            std::vector<float> totalData(energy.TotalEnergyHistory.begin(), energy.TotalEnergyHistory.end());
            
            // Find max energy for y-axis limits
            float maxEnergy = 1.0f;
            if (!totalData.empty())
            {
                maxEnergy = *std::max_element(totalData.begin(), totalData.end());
                maxEnergy = std::max(maxEnergy, 1.0f); // Ensure minimum scale
            }
            
            ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, 2.0f);
            
            if (ImPlot::BeginPlot("##EnergyPlot", ImVec2(-1, 250), ImPlotFlags_NoLegend))
            {
                // Setup axes
                ImPlot::SetupAxes("Time (s)", "Energy (J)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, maxEnergy * 1.1, ImPlotCond_Always);
                
                // Plot kinetic energy
                if (energy.ShowKE && keData.size() == timeData.size())
                {
                    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.2f, 1.0f, 0.4f, 1.0f));
                    ImPlot::PlotLine("Kinetic Energy", timeData.data(), keData.data(), 
                                    static_cast<int>(timeData.size()));
                    ImPlot::PopStyleColor();
                }
                
                // Plot potential energy
                if (energy.ShowPE && peData.size() == timeData.size())
                {
                    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
                    ImPlot::PlotLine("Potential Energy", timeData.data(), peData.data(), 
                                    static_cast<int>(timeData.size()));
                    ImPlot::PopStyleColor();
                }
                
                // Plot total energy
                if (energy.ShowTotal && totalData.size() == timeData.size())
                {
                    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                    ImPlot::PlotLine("Total Energy", timeData.data(), totalData.data(), 
                                    static_cast<int>(timeData.size()));
                    ImPlot::PopStyleColor();
                }
                
                ImPlot::EndPlot();
            }
            
            ImPlot::PopStyleVar();
            
            // Legend
            ImGui::Spacing();
            if (energy.ShowKE)
            {
                ImGui::ColorButton("##ke", ImVec4(0.2f, 1.0f, 0.4f, 1.0f), 
                                  ImGuiColorEditFlags_NoTooltip, ImVec2(15, 15));
                ImGui::SameLine();
                ImGui::Text("Kinetic Energy");
                ImGui::SameLine(200);
            }
            if (energy.ShowPE)
            {
                ImGui::ColorButton("##pe", ImVec4(0.4f, 0.6f, 1.0f, 1.0f), 
                                  ImGuiColorEditFlags_NoTooltip, ImVec2(15, 15));
                ImGui::SameLine();
                ImGui::Text("Potential Energy");
                ImGui::SameLine(200);
            }
            if (energy.ShowTotal)
            {
                ImGui::ColorButton("##total", ImVec4(1.0f, 0.8f, 0.2f, 1.0f), 
                                  ImGuiColorEditFlags_NoTooltip, ImVec2(15, 15));
                ImGui::SameLine();
                ImGui::Text("Total Energy");
            }
        }
        else
        {
            // Empty plot placeholder
            if (ImPlot::BeginPlot("##EnergyPlot", ImVec2(-1, 250)))
            {
                ImPlot::SetupAxes("Time (s)", "Energy (J)");
                ImPlot::EndPlot();
            }
            
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                              "Start simulation to track energy data");
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Settings
        ImGui::Text("Settings");
        ImGui::SliderFloat("Gravity", &energy.GravityMagnitude, 0.0f, 20.0f, "%.2f m/s²");
        
        ImGui::Spacing();
        
        // Action buttons
        if (ImGui::Button("Reset", ImVec2(120, 0)))
        {
            energy.Reset();
        }
        ImGui::Spacing();
        
        // Educational Info
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.25f, 0.35f, 0.9f));
        if (ImGui::BeginChild("EnergyInfo", ImVec2(-1, 100), true))
        {
            RenderPhysicsEquation("KE = ½mv²", "Kinetic Energy");
            RenderPhysicsEquation("PE = mgh", "Gravitational Potential Energy");
            ImGui::Spacing();
            ImGui::TextWrapped("Law of Conservation of Energy: In a closed system, "
                              "total energy remains constant. Energy can transform between "
                              "kinetic and potential forms.");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
}