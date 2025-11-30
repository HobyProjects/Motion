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
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::Begin("Acceleration Analysis", &context.Panels->ShowAccelerationPanel);
        bool inSimulation = context.Simulation->InSimulation;

        if(!inSimulation)
        {
            HeadingConfig config;
            config.Separator = true;
            Heading("Simulation Mode Required", HeadingLevel::H2, config);

            LabelConfig lblConfig;
            lblConfig.Wrapped = true;
            LabelSimple("Acceleration tracking is only available during active simulation. Please start the simulation to view acceleration data.", lblConfig);
        }

        ImGui::BeginDisabled(!inSimulation);
        auto& accel = context.Physics->PhysicsAnalysis.Acceleration;

        {   
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_SPEED " Current Acceleration Vector", HeadingLevel::H3, config);
            LabelValue("Magnitude", accel.AccelerationMagnitude, "%.2f m/s²");
            LabelValue("Direction", accel.CurrentAcceleration, "%.2f");
        }
        
        ImGui::Spacing();

        {
            if (!accel.AccelerationHistory.empty() && !accel.TimeStamps.empty() && inSimulation)
            {
                std::vector<float> timeData(accel.TimeStamps.begin(), accel.TimeStamps.end());
                std::vector<float> magnitudeData;
                magnitudeData.reserve(accel.AccelerationHistory.size());
                
                for (const auto& accelVec : accel.AccelerationHistory)
                    magnitudeData.push_back(glm::length(accelVec));
                
                size_t dataSize = std::min(timeData.size(), magnitudeData.size());
                
                if (dataSize > 1)
                {
                    float maxAccel = *std::max_element(magnitudeData.begin(), magnitudeData.end());
                    maxAccel = std::max(maxAccel, 1.0f);
                    
                    PlotConfig plotConfig;
                    plotConfig.Size             = ImVec2(-1, 350);
                    plotConfig.XAxis.Label      = "Time (s)";
                    plotConfig.YAxis.Label      = "Acceleration (m/s²)";
                    plotConfig.XAxis.AutoFit    = true;
                    plotConfig.YAxis.AutoFit    = false;
                    plotConfig.YAxis.Min        = 0.0;
                    plotConfig.YAxis.Max        = maxAccel * 1.1;
                    plotConfig.Crosshairs       = true;
                    plotConfig.NoLegend         = false;
                    
                    PlotLineConfig lineStyle;
                    lineStyle.Color         = Colors::MaterialOrange400; 
                    lineStyle.Thickness     = 2.5f;
                    lineStyle.Stems         = true;
                    
                    if(BeginPlot("Acceleration Magnitude", plotConfig))
                    {
                        SetupPlotAxes(plotConfig);
                        PlotLine("Acceleration", timeData, magnitudeData, lineStyle);
                        EndPlot();
                    }
                }
            }
            else
            {
                PlotConfig emptyConfig;
                emptyConfig.Size        = ImVec2(-1, 250);
                emptyConfig.XAxis.Label = "Time (s)";
                emptyConfig.YAxis.Label = "Acceleration (m/s²)";
                
                if(BeginPlot("Acceleration Magnitude", emptyConfig))
                {
                    SetupPlotAxes(emptyConfig);
                    EndPlot();
                }
            }
        }
        
        ImGui::Spacing();
        
        {
            ButtonConfig resetConfig;
            resetConfig.Size = ImVec2(150, 0);
            resetConfig.Style = ButtonStyle::Primary;
            resetConfig.Tooltip = "Clear all recorded acceleration data";
            
            Button("Reset Data", resetConfig, [&]()
            {
                accel.Reset();
            });
        }

        ImGui::EndDisabled();
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
}