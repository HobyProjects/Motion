#include "CorePCH.hpp"
#include "TrajectoryTracker.hpp"

namespace Motion
{
    void TrajectoryTracker::OnRender(Scene* scene)
    {
        if(!scene) return;

        auto& context = scene->GetContext();
        if(!context.Panels->ShowTrajectoryPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Trajectory Prediction", &context.Panels->ShowTrajectoryPanel);

        auto& traj = context.Physics->PhysicsAnalysis.Trajectory;
        {
            HeadingConfig config;
            config.Separator = true;
            Heading("Trajectory Prediction", HeadingLevel::H1, config);
        
            ToggleSwitch("Enable Prediction", &traj.PredictionEnabled, ToggleSwitchPresets::iOS());
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Calculate and display the predicted path\nof the object based on current motion");
            
            ToggleSwitch("Show Path", &traj.ShowPredictionPath, ToggleSwitchPresets::iOS());
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Visualize the predicted trajectory in the 3D viewport");
        }
        
        ImGui::Spacing();
        
        {
            HeadingConfig config;
            config.Separator = true;
            Heading("Prediction Settings", HeadingLevel::H2, config);

            {                    
                float stepsFloat = static_cast<float>(traj.PredictionSteps);
                
                SliderFloatConfig stepsConfig;
                stepsConfig.MinV = 10.0f;
                stepsConfig.MaxV = 200.0f;
                stepsConfig.Fmt = "%.0f";
                stepsConfig.Tooltip = "Number of points to calculate along the path\n-More steps = smoother curve\n-More steps = slower calculation\nRecommended: 50-100 steps";
                
                SliderFloat("Prediction Steps", &stepsFloat, stepsConfig, [&](float value){
                    traj.PredictionSteps = static_cast<int>(value);
                });
            }
                 
            {                    
                SliderFloatConfig timeConfig;
                timeConfig.MinV = 0.01f;
                timeConfig.MaxV = 0.5f;
                timeConfig.Fmt = "%.3f s";
                timeConfig.Tooltip = "Time interval between predicted positions\nSmaller = more detailed prediction\nLarger = longer prediction time\nRecommended: 0.05-0.1 seconds";
                
                SliderFloat("Time Step", &traj.TimeStep, timeConfig);
            }
        }
        
        ImGui::Spacing();
        
        {
            if (traj.PredictionEnabled && !traj.PredictedPath.empty())
            {
                float totalDistance = 0.0f;
                float predictionTime = traj.TimeStep * traj.PredictionSteps;
                int pointCount = static_cast<int>(traj.PredictedPath.size());

                for (size_t i = 1; i < traj.PredictedPath.size(); ++i)
                    totalDistance += glm::length(traj.PredictedPath[i] - traj.PredictedPath[i-1]);
                

                LabelConfig labelConfig;
                labelConfig.Tooltip = "Number of points along the trajectory";
                LabelValue("Path Points", pointCount, "%d", labelConfig);

                labelConfig.Tooltip = "Total path length along the trajectory";
                LabelValue("Total Distance", totalDistance, "%.2f m", labelConfig);

                labelConfig.Tooltip = "Duration of predicted trajectory";
                LabelValue("Prediction Time", predictionTime, "%.2f s", labelConfig);

                if(totalDistance > EPSILON && predictionTime > EPSILON)
                {
                    float avgSpeed = totalDistance / predictionTime;
                    labelConfig.Tooltip = "Average speed along the path";
                    LabelValue("Average Speed", avgSpeed, "%.2f m/s", labelConfig);
                }
        
                if (traj.PredictedPath.size() >= 2)
                {
                    glm::vec3 start = traj.PredictedPath.front();
                    glm::vec3 end = traj.PredictedPath.back();
                    
                    glm::vec3 displacement = end - start;
                    float displacementMag = glm::length(displacement);

                    LabelValue("Start Psition",  glm::length(start), "%.2f m");
                    LabelValue("End Position",  glm::length(end), "%.2f m");
                    LabelValue("Displacement", displacement, "%.2f m");
                }
            }
            else
            {
                HeadingConfig config;
                config.Separator = true;
                Heading("No Trajectory Data", HeadingLevel::H2, config);
                LabelSimple("Enable prediction and select an object");
            }
        }
        
        ImGui::Spacing();
        
        {
            ButtonGroupConfig buttonConfig;
            buttonConfig.SameLine = true;

            ButtonGroup controlButtons(buttonConfig);

            ButtonConfig recalConfig;
            recalConfig.Style = ButtonStyle::Primary;
            recalConfig.Tooltip = "Recalculate trajectory";
            controlButtons.AddButton("Recalculate", recalConfig, [&](){
                if (context.Simulation->SelectedEntity != entt::null)
                    UpdateTrajectoryPrediction(context, context.Simulation->SelectedEntity);
            });

            ButtonConfig clearConfig;
            clearConfig.Style = ButtonStyle::Secondary;
            clearConfig.Tooltip = "Clear all trajectory data";
            controlButtons.AddButton("Clear", clearConfig, [&](){
                traj.Reset();
            });
        }
        
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void TrajectoryTracker::UpdateTrajectoryPrediction(SceneContext& context, entt::entity entity)
    {
        if (!context.Physics->PhysicsAnalysis.Trajectory.PredictionEnabled) return;
        
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(entity);
        auto* transform = context.Entities->Registry.try_get<TransformComponent>(entity);
        if (!rb || !transform) return;
        
        glm::vec3 acceleration = context.Physics->PhysicsAnalysis.Acceleration.CurrentAcceleration;
        auto linearVelocity = ToVec3(rb->PhysicsBody->getLinearVelocity());

        context.Physics->PhysicsAnalysis.Trajectory.PredictTrajectory(
            transform->Translation,
            linearVelocity,
            acceleration
        );
    }
}