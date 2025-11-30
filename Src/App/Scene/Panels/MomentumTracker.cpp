#include "CorePCH.hpp"
#include "MomentumTracker.hpp"

namespace Motion
{
    void MomentumTracker::OnUpdate(Scene* scene, float dt)
    {
        if (!scene) return;
        
        auto& context = scene->GetContext();
        if (!context.Panels->ShowMomentumPanel) return;
        
        auto selectedEntity = context.Simulation->SelectedEntity;
        if (selectedEntity == entt::null) return;
        
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(selectedEntity);
        if (!rb || !rb->PhysicsBody) return;
        
        rp3d::Vector3 rp3dLinearVel = rb->PhysicsBody->getLinearVelocity();
        rp3d::Vector3 rp3dAngularVel = rb->PhysicsBody->getAngularVelocity();
        rp3d::Vector3 rp3dInertia = rb->PhysicsBody->getLocalInertiaTensor();
        float mass = rb->PhysicsBody->getMass();
        
        glm::vec3 linearVelocity(rp3dLinearVel.x, rp3dLinearVel.y, rp3dLinearVel.z);
        glm::vec3 angularVelocity(rp3dAngularVel.x, rp3dAngularVel.y, rp3dAngularVel.z);
        glm::vec3 inertia(rp3dInertia.x, rp3dInertia.y, rp3dInertia.z);
        
        glm::vec3 linearMomentum = mass * linearVelocity;
        glm::vec3 angularMomentum = inertia * angularVelocity;
        
        float linearSpeed = glm::length(linearVelocity);
        float angularSpeed = glm::length(angularVelocity);
        context.Physics->PhysicsAnalysis.Momentum.Update(
            linearMomentum, 
            angularMomentum,
            linearSpeed,
            angularSpeed,
            dt
        );
    }

    void MomentumTracker::OnRender(Scene* scene)
    {
        if(!scene) return;

        auto& context = scene->GetContext();
        if (!context.Panels->ShowMomentumPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Momentum Analysis", &context.Panels->ShowMomentumPanel); 
        bool inSimulation = context.Simulation->InSimulation;

        if(!inSimulation)
        {
            HeadingConfig config;
            config.Separator = true;
            Heading("Simulation Mode Required", HeadingLevel::H2, config);

            LabelConfig lblConfig;
            lblConfig.Wrapped = true;
            LabelSimple("Momentum tracking is only available during active simulation. Please start the simulation to view momentum data.", lblConfig);
        }
        

        auto& momentum = context.Physics->PhysicsAnalysis.Momentum;
        auto selectedEntity = context.Simulation->SelectedEntity;

        if (selectedEntity == entt::null)
        {
            HeadingConfig config;
            config.Separator = true;
            Heading("No Entity", HeadingLevel::H2, config);

            LabelConfig lblConfig;
            lblConfig.Wrapped = true;
            LabelSimple("No Object Selected, Select an object to analyze its momentum", lblConfig);
        }
        
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(selectedEntity);
        if (!rb || !rb->PhysicsBody)
        {
            HeadingConfig config;
            config.Separator = true;
            Heading("Invalid Entity", HeadingLevel::H2, config);

            LabelConfig lblConfig;
            lblConfig.Wrapped = true;
            LabelSimple("No RigidBody Component, Selected object has no physics simulation", lblConfig);
        }
        
        ImGui::BeginDisabled(!inSimulation);
        glm::vec3 currentLinearMomentum     = momentum.LinearMomentum;
        glm::vec3 currentAngularMomentum    = momentum.AngularMomentum;
        float linearSpeed                   = momentum.LinearMagnitude;
        float angularSpeed                  = momentum.AngularMagnitude;
        
        ImGui::Spacing();
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_LINE_AXIS " Motion Analysis Over Time", HeadingLevel::H3, config);

            {
                if (!momentum.LinearSpeedHistory.empty() && inSimulation)
                {
                    LabelValue("Linear Speed", linearSpeed, "%.2f ms^-1");
                    LabelValue("Linear Momentum", glm::length(currentLinearMomentum), "%.2f kg m/s");
                    LabelValue("Time", momentum.TimeAccumulator, "%.2f s");
                    ImGui::Spacing();

                    PlotConfig config;
                    config.Size             = ImVec2(-1, 350);
                    config.XAxis.Label      = "Time (s)";
                    config.YAxis.Label      = "Speed (m/s)";
                    config.XAxis.AutoFit    = true;
                    config.YAxis.AutoFit    = false;
                    config.XAxis.Min        = momentum.TimeAccumulator - momentum.GraphTimeWindow;
                    config.XAxis.Max        = momentum.TimeAccumulator;

                    PlotData speedData;
                    speedData.Label             = "Linear Speed";
                    speedData.Style.Color       = Colors::DodgerBlue;
                    speedData.Style.Thickness   = 2.5f;
                    speedData.Style.Stems       = true;
                    for (const auto& point : momentum.LinearSpeedHistory) speedData.AddPoint(point.x, point.y);
                    
                    if(BeginPlot("Linear Speed", config))
                    {
                        SetupPlotAxes(config);
                        PlotLine(speedData);
                        EndPlot();
                    }
                }
                else
                {
                    PlotConfig emptyConfig;
                    emptyConfig.Size = ImVec2(-1, 250);
                    emptyConfig.XAxis.Label = "Time (s)";
                    emptyConfig.YAxis.Label = "Speed (m/s)";
                    
                    if(BeginPlot("Linear Speed", emptyConfig))
                    {
                        SetupPlotAxes(emptyConfig);
                        EndPlot();
                    }
                }

                ImGui::Spacing();
            }
        
            {
                if (!momentum.AngularSpeedHistory.empty() && inSimulation)
                {
                    LabelValue("Angular Speed", angularSpeed, "%.2f rad/s");
                    LabelValue("Angular Momentum", glm::length(currentAngularMomentum), "%.2f kg m^2/s");
                    LabelValue("Time", momentum.TimeAccumulator, "%.2f s");
                    ImGui::Spacing();
                    
                    PlotConfig config;
                    config.Size = ImVec2(-1, 350);
                    config.XAxis.Label = "Time (s)";
                    config.YAxis.Label = "Angular Speed (rad/s)";
                    config.XAxis.AutoFit = true;
                    config.YAxis.AutoFit = false;
                    config.XAxis.Min = momentum.TimeAccumulator - momentum.GraphTimeWindow;
                    config.XAxis.Max = momentum.TimeAccumulator;

                    PlotData angularData;
                    angularData.Label = "Angular Speed";
                    angularData.Style.Color = Colors::RoyalBlue;
                    angularData.Style.Stems = true;
                    angularData.Style.Thickness = 2.5f;
                    for (const auto& point : momentum.AngularSpeedHistory) angularData.AddPoint(point.x, point.y);
                    
                    if(BeginPlot("Angular Speed", config))
                    {
                        SetupPlotAxes(config);
                        PlotLine(angularData);
                        EndPlot();
                    }
                }
                else
                {
                    PlotConfig emptyConfig;
                    emptyConfig.Size = ImVec2(-1, 250);
                    emptyConfig.XAxis.Label = "Time (s)";
                    emptyConfig.YAxis.Label = "Angular Speed (rad/s)";
                    
                    if(BeginPlot("Angular Speed", emptyConfig))
                    {
                        SetupPlotAxes(emptyConfig);
                        EndPlot();
                    }
                }
            }
            
            ImGui::Separator();
            
            {
                if (!momentum.MomentumMagnitudeHistory.empty() && inSimulation)
                {
                    PlotConfig config;
                    config.Size = ImVec2(-1, 350);
                    config.XAxis.Label = "Time (s)";
                    config.YAxis.Label = "Momentum (kgm/s)";
                    config.XAxis.AutoFit = true;
                    config.YAxis.AutoFit = false;
                    config.XAxis.Min = momentum.TimeAccumulator - momentum.GraphTimeWindow;
                    config.XAxis.Max = momentum.TimeAccumulator;

                    PlotData momentumData;
                    momentumData.Label = "Momentum";
                    momentumData.Style.Color = Colors::DarkMagenta;
                    momentumData.Style.Stems = true;
                    momentumData.Style.Thickness = 2.5f;
                    for (const auto& point : momentum.MomentumMagnitudeHistory) momentumData.AddPoint(point.x, point.y);
                    
                    if(BeginPlot("Momentum", config))
                    {
                        SetupPlotAxes(config);
                        PlotLine(momentumData);
                        EndPlot();
                    }
                }
                else
                {
                    PlotConfig emptyConfig;
                    emptyConfig.Size = ImVec2(-1, 250);
                    emptyConfig.XAxis.Label = "Time (s)";
                    emptyConfig.YAxis.Label = "Momentum (kgm/s)";
                    
                    if(BeginPlot("Momentum", emptyConfig))
                    {
                        SetupPlotAxes(emptyConfig);
                        EndPlot();
                    }
                }
            }
            
            ImGui::Spacing();
            
            if (ImGui::Button("Clear Graph Data", ImVec2(150, 30)))
            {
                momentum.ClearHistory();
            }
            
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Reset all graph data and start recording fresh");
        }
            
        ImGui::Spacing();
        
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_CREATE " Actions", HeadingLevel::H3, config);

            ImGui::Columns(2, nullptr, false);
            
            if (ImGui::Button("Set Initial Momentum", ImVec2(-1, 35)))
            {
                momentum.InitialLinearMomentum = currentLinearMomentum;
                momentum.InitialAngularMomentum = currentAngularMomentum;
            }
            
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Capture current momentum as reference point\nfor conservation tracking");
            
            ImGui::NextColumn();
            
            if (ImGui::Button("Reset All", ImVec2(-1, 35))) momentum.Reset();
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Clear all momentum data and graphs");
            
            ImGui::Columns(1);
        }
        
        ImGui::EndDisabled();
        ImGui::End();
        ImGui::PopStyleVar();
    }
}