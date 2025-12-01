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
        
        float velocityMag = glm::length(linearVelocity);
        float ke = 0.5f * mass * velocityMag * velocityMag;
        float pe = mass * context.Physics->PhysicsAnalysis.Energy.GravityMagnitude * transform->Translation.y;
        
        context.Physics->PhysicsAnalysis.Energy.Update(ke, pe, dt);
    }

    void EnergyTracker::OnRender(Scene* scene)
    {
        if(!scene) return;

        auto& context = scene->GetContext();
        if (!context.Panels->ShowEnergyPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::Begin("Energy Analysis", &context.Panels->ShowEnergyPanel);
        bool inSimulation = context.Simulation->InSimulation;

        if(!inSimulation)
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_INFO_OUTLINE " Simulation Mode Required", HeadingLevel::H1, config);

            LabelConfig lblConfig;
            lblConfig.Wrapped = true;
            LabelSimple("Energy tracking is only available during active simulation. Please start the simulation to view energy data.", lblConfig);
        }
        
        auto& energy = context.Physics->PhysicsAnalysis.Energy;
        
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_ENERGY_SAVINGS_LEAF " Energy Tracking", HeadingLevel::H2, config);
            ToggleSwitch("Enable Tracking", &energy.TrackingEnabled, ToggleSwitchPresets::iOS());
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Record kinetic and potential energy over time to analyze energy conservation");
        }
        
        ImGui::BeginDisabled(!inSimulation);
        ImGui::Spacing();
        
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_WIND_POWER " Current Energy Values", HeadingLevel::H2, config);

            LabelConfig kineticConfig;
            kineticConfig.Color = ImVec4(0.3f, 1.0f, 0.5f, 1.0f);
            kineticConfig.Tooltip = "Energy of motion: KE = ½mv²";
            LabelValue("Kinetic Energy  ", energy.CurrentKE, "%.2f J", kineticConfig);
            
            LabelConfig potentialConfig;
            potentialConfig.Color = ImVec4(0.5f, 0.7f, 1.0f, 1.0f);
            potentialConfig.Tooltip = "Stored energy due to position: PE = mgh";
            LabelValue("Potential Energy  ", energy.CurrentPE, "%.2f J", potentialConfig);
            
            LabelConfig totalConfig;
            totalConfig.Color = ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
            totalConfig.Tooltip = "Total energy: KE + PE";
            LabelValue("Total Energy  ", energy.CurrentTotal, "%.2f J", totalConfig);
        }
        
        ImGui::Spacing();
        
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_TIMELINE " Energy Conservation Status", HeadingLevel::H2, config);

            float conservation = energy.GetConservationPercentage();
            ImVec4 conservationColor;
            const char* conservationLabel;
            if (conservation > 95.0f)
            {
                conservationColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // Green - excellent
                conservationLabel = "Excellent";
            }
            else if (conservation > 85.0f)
            {
                conservationColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow - good
                conservationLabel = "Good";
            }
            else
            {
                conservationColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red - poor
                conservationLabel = "Poor";
            }

            LabelConfig energyConfig;
            energyConfig.Color = conservationColor;
            Label("Energy Conservation ", conservationLabel, energyConfig);
            if (energy.InitialTotal > EPSILON)
            {
                ImGui::Separator();
                
                LabelConfig metricsConfig;
                metricsConfig.Disabled = false;
                LabelValue("Initial Total ", energy.InitialTotal, "%.2f J", metricsConfig);
                
                LabelConfig lossConfig;
                lossConfig.Color = energy.EnergyLoss > 0.1f ? ImVec4(1.0f, 0.5f, 0.3f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                LabelValue("Energy Lost ", energy.EnergyLoss, "%.2f J", lossConfig);
            }
        }
        
        ImGui::Spacing();
        
        {
            if (!energy.TimeStamps.empty() && energy.TimeStamps.size() == energy.TotalEnergyHistory.size() && inSimulation)
            {

                std::vector<float> timeData(energy.TimeStamps.begin(), energy.TimeStamps.end());
                std::vector<float> keData(energy.KineticEnergyHistory.begin(), energy.KineticEnergyHistory.end());
                std::vector<float> peData(energy.PotentialEnergyHistory.begin(), energy.PotentialEnergyHistory.end());
                std::vector<float> totalData(energy.TotalEnergyHistory.begin(), energy.TotalEnergyHistory.end());
                
                float maxEnergy = 1.0f;
                if (!totalData.empty())
                {
                    maxEnergy = *std::max_element(totalData.begin(), totalData.end());
                    maxEnergy = std::max(maxEnergy, 1.0f);
                }
                
                PlotConfig plotConfig;
                plotConfig.Size             = ImVec2(-1, 350);
                plotConfig.XAxis.Label      = "Time (s)";
                plotConfig.YAxis.Label      = "Energy (J)";
                plotConfig.XAxis.AutoFit    = true;
                plotConfig.YAxis.AutoFit    = false;
                plotConfig.YAxis.Min        = 0.0;
                plotConfig.YAxis.Max        = maxEnergy * 1.1;
                plotConfig.NoLegend         = false;
                
                if(BeginPlot("Energy Analysis", plotConfig))
                {
                    SetupPlotAxes(plotConfig);
                    if (energy.ShowKE && keData.size() == timeData.size())
                    {
                        PlotLineConfig keStyle;
                        keStyle.Color = Colors::DarkSlateBlue;
                        keStyle.Thickness = 1.5f;
                        keStyle.MarkerStyle = ImPlotMarker_Circle;
                        keStyle.MarkerSize = 1.5f;
                        keStyle.Stems = true;
                        PlotLine("Kinetic Energy", timeData, keData, keStyle);
                    }
                    
                    if (energy.ShowPE && peData.size() == timeData.size())
                    {
                        PlotLineConfig peStyle;
                        peStyle.Color = Colors::DarkMagenta;
                        peStyle.Thickness = 1.5f;
                        peStyle.MarkerStyle = ImPlotMarker_Square;
                        peStyle.MarkerSize = 1.5f;
                        peStyle.Stems = true;
                        PlotLine("Potential Energy", timeData, peData, peStyle);
                    }
                    
                    if (energy.ShowTotal && totalData.size() == timeData.size())
                    {
                        PlotLineConfig totalStyle;
                        totalStyle.Color = Colors::DarkOrchid;
                        totalStyle.Thickness = 2.0f;
                        totalStyle.MarkerStyle = ImPlotMarker_Diamond;
                        totalStyle.MarkerSize = 2.0f;
                        totalStyle.Stems = true;
                        PlotLine("Total Energy", timeData, totalData, totalStyle);
                    }
                    
                    if(energy.InitialTotal > EPSILON)
                    {
                        PlotLineConfig refStyle;
                        refStyle.Color = Colors::MediumSlateBlue;
                        refStyle.Thickness = 1.5f;
                        refStyle.Stems = true;
                        PlotHLine("Initial Energy", energy.InitialTotal, refStyle);
                    }
                    
                    EndPlot();
                }
            }
            else
            {
                PlotConfig emptyConfig;
                emptyConfig.Size = ImVec2(-1, 280);
                emptyConfig.XAxis.Label = "Time (s)";
                emptyConfig.YAxis.Label = "Energy (J)";
                
                if(BeginPlot("Energy Analysis", emptyConfig))
                {
                    SetupPlotAxes(emptyConfig);
                    EndPlot();
                }
            }

            ToggleSwitch("Show Kinetic ", &energy.ShowKE, ToggleSwitchPresets::iOS());
            ToggleSwitch("Show Potential ", &energy.ShowPE, ToggleSwitchPresets::iOS());
            ToggleSwitch("Show Total ", &energy.ShowTotal, ToggleSwitchPresets::iOS());
        }
        
        ImGui::Spacing();
        
        {
            ButtonConfig btnConfig;
            btnConfig.Size = ImVec2(150, 0);
            btnConfig.Style = ButtonStyle::Primary;
            btnConfig.Tooltip = "Clear all recorded energy data";

            Button("Reset Data", btnConfig, [&](){
                energy.Reset();
            });
        }
        
        ImGui::EndDisabled();

        ImGui::End();
        ImGui::PopStyleVar();
    }
}