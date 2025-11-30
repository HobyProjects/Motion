#include "CorePCH.hpp"
#include "SimulationEntity.hpp"

namespace Motion
{
    void SimulationEntity::OnRender(Scene* scene)
    {
        if (!scene) return;

        auto& context = scene->GetContext();
        if (!context.Panels->ShowEntitySimulated) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Simulation Watch List", &context.Panels->ShowEntitySimulated);

        if (context.Simulation->SimulatedEntities.empty())
        {
            HeadingConfig config;
            config.Separator = true;
            Heading("No Entities Tracked", HeadingLevel::H2, config);

            LabelConfig lblConfig;
            lblConfig.Wrapped = true;
            LabelSimple("No entities are currently being tracked in the simulation watch list. To add entities, select them in the scene and use the 'Add to Watch List' option in the context menu.", lblConfig);
        }
        else
        {
            for (size_t i = 0; i < context.Simulation->SimulatedEntities.size(); ++i)
            {
                auto entity = context.Simulation->SimulatedEntities[i];
                std::string entityName;

                if (context.Entities->Registry.try_get<TagComponent>(entity))
                {
                    auto& tag = context.Entities->Registry.get<TagComponent>(entity);
                    entityName = tag.Tag;
                }
                else
                {
                    entityName = fmt::format("Entity {}", static_cast<uint32_t>(entity));
                }

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed;
                flags |= (context.Simulation->SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0;

                if(ImGui::CollapsingHeader(entityName.c_str(), flags))
                {
                    ImGui::Indent();
                    context.Simulation->SelectedEntity = entity;

                    HeadingConfig config;
                    config.Separator = true;
                    Heading(entityName, HeadingLevel::H3, config);
                
                    auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(entity);
                    if(rb && rb->PhysicsBody)
                    {
                        auto* body = rb->PhysicsBody;
                        {
                            auto linVel = body->getLinearVelocity();
                            glm::vec3 velocity(linVel.x, linVel.y, linVel.z);

                            float speed             = ScenePhysics::GetSpeed(velocity);
                            float speedKmh          = ScenePhysics::MsToKmh(speed);
                            glm::vec3 direction     = ScenePhysics::GetDirection(velocity);

                            if(speed > EPSILON)
                            {
                                LabelConfig speedConfig;
                                speedConfig.Tooltip = "The speed of the object in meters per second (m/s)";
                                LabelValue("Speed (m/s)          ", speed, "%.2f m/s", speedConfig);
    
                                LabelConfig speedKmhConfig;
                                speedKmhConfig.Tooltip = "The speed of the object in kilometers per hour (km/h)";
                                LabelValue("Speed (km/h)         ", speedKmh, "%.1f km/h", speedKmhConfig);
    
                                LabelConfig velocityConfig;
                                velocityConfig.Tooltip = "The velocity vector of the object in meters per second (m/s)";
                                LabelValue("Velocity              ", velocity, "%.2f m/s", velocityConfig);
    
                                LabelConfig directionConfig;
                                directionConfig.Tooltip = "The direction of the object in degrees";
                                LabelValue("Direction             ", direction, "%.2f", directionConfig);
                            }
                        }
                        
                        ImGui::Spacing();
                    
                        {     
                            auto angVel = body->getAngularVelocity();
                            glm::vec3 angularVelocity(angVel.x, angVel.y, angVel.z);

                            float rotSpeed      = ScenePhysics::GetSpeed(angularVelocity);
                            float rpm           = ScenePhysics::RadPerSecToRPM(rotSpeed);
                            glm::vec3 rotAxis   = ScenePhysics::GetDirection(angularVelocity);

                            if(rotSpeed > EPSILON)
                            {
                                LabelConfig spinRateConfig;
                                spinRateConfig.Tooltip = "The rotation speed of the object in radians per second (rad/s)";
                                LabelValue("Spin Rate (rad/s)     ", rotSpeed, "%.2f rad/s", spinRateConfig);
    
                                LabelConfig rpmConfig;
                                rpmConfig.Tooltip = "The rotation speed of the object in revolutions per minute (RPM)";
                                LabelValue("Spin Rate (RPM)       ", rpm, "%.0f RPM", rpmConfig);
    
                                LabelConfig axisConfig;
                                axisConfig.Tooltip = "The axis of rotation of the object in degrees";
                                LabelValue("Spin Axis             ", rotAxis, "%.2f", axisConfig);
                            }
                        }
                        
                        ImGui::Spacing();
                    
                        {  
                            auto linVel = body->getLinearVelocity();
                            glm::vec3 velocity(linVel.x, linVel.y, linVel.z);

                            float speed         = ScenePhysics::GetSpeed(velocity);
                            float mass          = body->getMass();
                            float kineticEnergy = 0.5f * mass * speed * speed;
                            
                            auto* transform         = context.Entities->Registry.try_get<TransformComponent>(entity);
                            float height            = transform ? transform->Translation.y : 0.0f;
                            float potentialEnergy   = mass * 9.81f * height;
                            float totalEnergy       = kineticEnergy + potentialEnergy;

                            LabelConfig kineticConfig;
                            kineticConfig.Tooltip = "The kinetic energy of the object in joules (J)";
                            LabelValue("Kinetic Energy", kineticEnergy, "%.2f J", kineticConfig);
                            
                            LabelConfig potentialConfig;
                            potentialConfig.Tooltip = "The potential energy of the object in joules (J)";
                            LabelValue("Potential Energy", potentialEnergy, "%.2f J", potentialConfig);
                            
                            LabelConfig totalConfig;
                            totalConfig.Tooltip = "The total energy of the object in joules (J)";
                            LabelValue("Total Energy", totalEnergy, "%.2f J", totalConfig);
                        }
                        
                        ImGui::Spacing();
                    
                        {
                            float mass      = body->getMass();
                            bool isSleeping = !body->isActive();

                            LabelConfig massConfig;
                            massConfig.Tooltip = "The mass of the object in kilograms (kg)";
                            LabelValue("Mass", mass, "%.2f kg", massConfig);

                            LabelConfig sleepingConfig;
                            sleepingConfig.Tooltip = "Whether the object is sleeping or not";
                            if(isSleeping)
                                Label("Sleep State", "Sleeping", sleepingConfig);
                            else
                                Label("Sleep State", "Active", sleepingConfig);
                        }
                    }
                    else
                    {
                        HeadingConfig subHeadingConfig;
                        subHeadingConfig.Separator = true;
                        subHeadingConfig.Color = ImVec4(1.0f, 0.6f, 0.2f, 1.0f);
                        Heading("No RigidBody Component", HeadingLevel::H3, subHeadingConfig);
                        LabelSimple("Cannot track motion data");
                    }
                    
                    ImGui::Spacing();

                    {
                        ButtonConfig removeConfig;
                        removeConfig.Style = ButtonStyle::Secondary;
                        removeConfig.Tooltip = "Stop tracking this entity";

                        Button("Remove from Watch List", removeConfig, [&](){
                            context.Simulation->RemoveSimulatedEntity(entity);
                            if (context.Simulation->SelectedEntity == entity)
                            {
                                context.Simulation->SelectedEntity = entt::null;
                            }
                        });
                    }

                    ImGui::Spacing();
                    ImGui::Unindent();
                }
            }
            
            ImGui::Spacing();
            ImGui::Spacing();

            {
                ButtonConfig clearConfig;
                clearConfig.Style = ButtonStyle::Primary;
                clearConfig.Tooltip = "Remove all entities from watch list";

                Button("Clear All", clearConfig, [&](){
                    context.Simulation->ClearSimulatedEntities();
                    context.Simulation->SelectedEntity = entt::null;
                }); 
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}