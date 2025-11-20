#include "CorePCH.hpp"
#include "SimulationEntity.hpp"

namespace Motion
{
    void SimulationEntity::OnRender(Scene* scene)
    {
        if (!scene) return;

        auto& context = scene->GetContext();
        if (!context.Panels->ShowEntitySimulated) return;

        ImGui::Begin("Simulation Watch List", &context.Panels->ShowEntitySimulated);

        if (context.Simulation->SimulatedEntities.empty())
        {
            ImGui::TextDisabled("No entities in watch list");
        }
        else
        {
            for (size_t i = 0; i < context.Simulation->SimulatedEntities.size(); ++i)
            {
                auto entity = context.Simulation->SimulatedEntities[i];
                std::string entityLabel;
                if (context.Entities->Registry.try_get<TagComponent>(entity))
                {
                    auto& tag = context.Entities->Registry.get<TagComponent>(entity);
                    entityLabel = tag.Tag + " (ID: " + std::to_string(static_cast<uint32_t>(entity)) + ")";
                }
                else
                {
                    entityLabel = "Entity " + std::to_string(static_cast<uint32_t>(entity));
                }

                const bool selected = ImGui::CollapsingHeader(entityLabel.c_str(), ImGuiTreeNodeFlags_Framed);
                if(selected)
                {
                    ImGui::Indent(10.0f);
                    context.Simulation->SelectedEntity = entity;
                    auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(context.Simulation->SelectedEntity);
                    if(rb)
                    {
                        auto* body = rb->PhysicsBody;

                        // Linear Velocity
                        auto linVel = body->getLinearVelocity();
                        glm::vec3 velocity(linVel.x, linVel.y, linVel.z);
                        float speed = ScenePhysics::GetSpeed(velocity);
                        float speedKmh = ScenePhysics::MsToKmh(speed);
                        glm::vec3 direction = ScenePhysics::GetDirection(velocity);
                        
                        ImGui::Text("Movement");
                        ImGui::Indent(10.0f);
                        ImGui::BulletText("Speed: %.2f m/s (%.1f km/h)", speed, speedKmh);
                        if (speed > 0.001f)
                        {
                            ImGui::BulletText("Direction: (%.2f, %.2f, %.2f)", direction.x, direction.y, direction.z);
                        }
                        else
                        {
                            ImGui::BulletText("Direction: Not moving");
                        }
                        ImGui::Unindent(10.0f);
                        ImGui::Spacing();
                        
                        // Angular Velocity
                        auto angVel = body->getAngularVelocity();
                        glm::vec3 angularVelocity(angVel.x, angVel.y, angVel.z);
                        float rotSpeed = ScenePhysics::GetSpeed(angularVelocity);
                        float rpm = ScenePhysics::RadPerSecToRPM(rotSpeed);
                        glm::vec3 rotAxis = ScenePhysics::GetDirection(angularVelocity);
                        
                        ImGui::Text("Rotation");
                        ImGui::Indent(10.0f);
                        ImGui::BulletText("Spin Rate: %.2f rad/s (%.0f RPM)", rotSpeed, std::abs(rpm));
                        if (rotSpeed > 0.001f)
                        {
                            ImGui::BulletText("Spin Axis: (%.2f, %.2f, %.2f)", rotAxis.x, rotAxis.y, rotAxis.z);
                        }
                        else
                        {
                            ImGui::BulletText("Spin Axis: Not rotating");
                        }
                        ImGui::Unindent(10.0f);
                        ImGui::Spacing();
                        
                        // Energy Information
                        float kineticEnergy = 0.5f * body->getMass() * speed * speed;
                        ImGui::Text("Energy");
                        ImGui::Indent(10.0f);
                        ImGui::BulletText("Kinetic Energy: %.2f Joules", kineticEnergy);
                        ImGui::Unindent(10.0f);
                    }  
                    ImGui::Unindent(10.0f);
                }

                // Right-click context menu
                if (ImGui::BeginPopupContextItem(("EntityContext_" + std::to_string(i)).c_str()))
                {
                    if (ImGui::MenuItem("Remove from Simulation"))
                    {
                        context.Simulation->RemoveSimulatedEntity(entity);
                        if (context.Simulation->SelectedEntity == entity)
                        {
                            context.Simulation->SelectedEntity = entt::null;
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
        }

        // Clear all button
        if (!context.Simulation->SimulatedEntities.empty())
        {
            ImGui::Separator();
            if (ImGui::Button("Clear All"))
            {
                context.Simulation->ClearSimulatedEntities();
                context.Simulation->SelectedEntity = entt::null;
            }
        }

        ImGui::End();
    }
}