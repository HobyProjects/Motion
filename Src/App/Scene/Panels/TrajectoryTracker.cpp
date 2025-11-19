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

        if(!context.Simulation->InSimulation)
        {
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }
        
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), ICON_MD_SHOW_CHART " Trajectory Prediction");
        ImGui::Separator();
        ImGui::Spacing();
        
        auto& traj = context.Physics->PhysicsAnalysis.Trajectory;
        
        // Enable/Disable
        ImGui::Checkbox("Enable Prediction", &traj.EnablePrediction);
        ImGui::SameLine(); ShowPhysicsTooltip("Trajectory Prediction", 
            "Calculate and display the predicted path of the object based on current motion");
        
        ImGui::Checkbox("Show Path", &traj.ShowPredictionPath);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Settings
        ImGui::Text("Prediction Settings");
        ImGui::SliderInt("Steps", &traj.PredictionSteps, 10, 200);
        ImGui::SameLine(); ShowPhysicsTooltip("Prediction Steps", 
            "Number of points to calculate along the predicted path");
        
        ImGui::SliderFloat("Time Step", &traj.TimeStep, 0.01f, 0.5f, "%.3f s");
        ImGui::SameLine(); ShowPhysicsTooltip("Time Step", 
            "Time interval between predicted positions");
        
        ImGui::ColorEdit4("Path Color", (float*)&traj.PathColor, 
                         ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Prediction Info
        if (traj.EnablePrediction && !traj.PredictedPath.empty())
        {
            ImGui::Text("Predicted Path: %zu points", traj.PredictedPath.size());
            
            float totalDistance = 0.0f;
            for (size_t i = 1; i < traj.PredictedPath.size(); ++i)
            {
                totalDistance += glm::length(traj.PredictedPath[i] - traj.PredictedPath[i-1]);
            }
            
            ImGui::Text("Total Distance: %.2f m", totalDistance);
            ImGui::Text("Prediction Time: %.2f s", traj.TimeStep * traj.PredictionSteps);
            
            if (traj.PredictedPath.size() >= 2)
            {
                glm::vec3 start = traj.PredictedPath.front();
                glm::vec3 end = traj.PredictedPath.back();
                ImGui::Text("End Position: (%.2f, %.2f, %.2f)", end.x, end.y, end.z);
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                              "Enable prediction and select an object to see trajectory");
        }
        
        ImGui::Spacing();
        
        // Action Buttons
        if (ImGui::Button(ICON_MD_REFRESH " Recalculate", ImVec2(140, 0)))
        {
            if (context.Simulation->SelectedEntity != entt::null)
                UpdateTrajectoryPrediction(context, context.Simulation->SelectedEntity);
        }
        ImGui::SameLine();
        
        if (ImGui::Button(ICON_MD_CLEAR " Clear", ImVec2(140, 0)))
        {
            traj.Clear();
        }
        
        ImGui::Spacing();
        
        // Educational Info
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.25f, 0.35f, 0.9f));
        if (ImGui::BeginChild("TrajInfo", ImVec2(-1, 100), true))
        {
            ImGui::TextWrapped("Trajectory prediction uses kinematic equations to calculate "
                              "the future path of an object based on its current velocity and "
                              "acceleration. Useful for projectile motion analysis.");
            ImGui::Spacing();
            ImGui::BulletText("Parabolic paths indicate constant downward acceleration");
            ImGui::BulletText("Predictions assume no air resistance");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void TrajectoryTracker::UpdateTrajectoryPrediction(SceneContext& context, entt::entity entity)
    {
        if (!context.Physics->PhysicsAnalysis.Trajectory.EnablePrediction) return;
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(entity);
        auto* transform = context.Entities->Registry.try_get<TransformComponent>(entity);
        if (!rb || !transform) return;
        
        // Use gravity as acceleration
        glm::vec3 acceleration(0.0f, -9.81f, 0.0f);
        auto mass = rb->PhysicsBody->getMass();
        auto linearVelocity = ToVec3(rb->PhysicsBody->getLinearVelocity());
        
        context.Physics->PhysicsAnalysis.Trajectory.PredictTrajectory(
            transform->Translation,
            linearVelocity,
            acceleration,
            mass
        );
    }

}