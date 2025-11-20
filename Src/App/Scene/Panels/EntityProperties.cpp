#include "CorePCH.hpp"
#include "EntityProperties.hpp"

namespace Motion
{
    void EntityProperties::OnRender(Scene* scene)
    {
        if(!scene) return;

        auto& context = scene->GetContext();
        if(!context.Panels->ShowEntityComponents) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Entity Properties", &context.Panels->ShowEntityComponents);

        if (context.Entities->SelectedEntity == entt::null)
        {
            ImGui::TextDisabled("No entity selected.");
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        auto* tc = context.Entities->Registry.try_get<TransformComponent>(context.Entities->SelectedEntity);
        auto* cc = context.Entities->Registry.try_get<ColliderComponent>(context.Entities->SelectedEntity);
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(context.Entities->SelectedEntity);
        if (!tc || !cc || !rb)
        {
            ImGui::TextDisabled("This entity does not have any components.");
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        RenderTransform(tc);
        RenderRigidBody(rb);
        RenderCollider(cc);

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void EntityProperties::RenderTransform(TransformComponent* tc)
    {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Spacing();
            
            // Position
            glm::vec3 translation = tc->Translation;
            ImGui::Text("Position (m)");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("The position of the object in 3D space (X, Y, Z coordinates).");
                ImGui::Text("Measured in meters.");
                ImGui::EndTooltip();
            }
            
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##position", &translation.x, 0.1f))
            {
                tc->Translation = translation;
            }
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Rotation
            glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(tc->Rotation));
            ImGui::Text("Rotation (deg)");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("The rotation of the object around each axis.");
                ImGui::Text("Measured in degrees (0-360).");
                ImGui::EndTooltip();
            }
            
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##rotation", &eulerDeg.x, 1.0f))
            {
                tc->Rotation = glm::normalize(glm::quat(glm::radians(eulerDeg)));
            }
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Scale
            glm::vec3 scale = tc->Scale;
            ImGui::Text("Scale");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("The size multiplier for each axis.");
                ImGui::Text("1.0 = original size, 2.0 = double size, 0.5 = half size");
                ImGui::EndTooltip();
            }
            
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##scale", &scale.x, 0.1f, 0.01f, 100.0f))
            {
                tc->Scale = scale;
            }
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
        }
    }
    void EntityProperties::RenderRigidBody(RigidBodyComponent* rb)
    {
        if (ImGui::CollapsingHeader("Rigid Body Properties", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Spacing();
            
            auto* body = rb->PhysicsBody;
            
            // Allow Sleeping
            bool allowSleeping = body->isAllowedToSleep();
            if (ImGui::Checkbox("Allow Sleeping", &allowSleeping))
            {
                body->setIsAllowedToSleep(allowSleeping);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Allow the body to sleep if it is not moving");
                ImGui::EndTooltip();
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Body Type
            ImGui::Text("Body Type");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Static: Immovable objects like walls, floors, and obstacles");
                ImGui::Text("Dynamic: Objects that move and respond to forces like balls, boxes, characters");
                ImGui::EndTooltip();
            }
            
            const char* bodyTypes[] = { "Static", "Dynamic" };
            int currentBodyType = (body->getType() == rp3d::BodyType::DYNAMIC) ? 1 : 0;
            ImGui::PushItemWidth(-1);
            if (ImGui::Combo("##bodytype", &currentBodyType, bodyTypes, IM_ARRAYSIZE(bodyTypes)))
            {
                if (currentBodyType == 0)
                {
                    rb->Type = BodyType::Static;
                    body->setType(rp3d::BodyType::STATIC);
                }
                else if (currentBodyType == 1)
                {
                    rb->Type = BodyType::Dynamic;
                    body->setType(rp3d::BodyType::DYNAMIC);
                }
            }
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Computed Mass (read-only)
            ImGui::Text("Computed Mass (kg)");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("How heavy the object is in kilograms.");
                ImGui::Text("Heavier objects need more force to move and have more momentum.");
                ImGui::Separator();
                ImGui::Text("Examples: Basketball ≈0.6kg, Car ≈1500kg, Person ≈70kg");
                ImGui::EndTooltip();
            }
            
            ImGui::PushItemWidth(-1);
            ImGui::BeginDisabled();
            float mass = static_cast<float>(body->getMass());
            ImGui::DragFloat("##mass", &mass, 0.1f, 0.01f, 10000.0f);
            ImGui::EndDisabled();
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Linear Damping
            ImGui::Text("Linear Damping");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Air resistance for movement. Higher = slows down faster.");
                ImGui::Separator();
                ImGui::BulletText("0 = No air resistance (moves forever like in space)");
                ImGui::BulletText("0.5 = Medium resistance (normal physics)");
                ImGui::BulletText("1.0 = High resistance (moving through water)");
                ImGui::EndTooltip();
            }
            
            ImGui::PushItemWidth(-1);
            float linDamp = static_cast<float>(body->getLinearDamping());
            if (ImGui::SliderFloat("##lindamp", &linDamp, 0.0f, 1.0f, "%.3f"))
            {
                body->setLinearDamping(linDamp);
            }
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Angular Damping
            ImGui::Text("Angular Damping");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Air resistance for rotation/spinning. Higher = stops spinning faster.");
                ImGui::Separator();
                ImGui::BulletText("0 = Spins forever like in space");
                ImGui::BulletText("0.5 = Normal spinning (like a basketball)");
                ImGui::BulletText("1.0 = Stops spinning quickly");
                ImGui::EndTooltip();
            }
            
            ImGui::PushItemWidth(-1);
            float angDamp = static_cast<float>(body->getAngularDamping());
            if (ImGui::SliderFloat("##angdamp", &angDamp, 0.0f, 1.0f, "%.3f"))
            {
                body->setAngularDamping(angDamp);
            }
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
        }
    }

    void EntityProperties::RenderCollider(ColliderComponent* cc)
    {
        if (ImGui::CollapsingHeader("Collider Properties", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Spacing();
            
            // Bounciness (Restitution)
            ImGui::Text("Bounciness");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("How bouncy the object is when it hits something.");
                ImGui::Separator();
                ImGui::BulletText("0.0 = No bounce (like clay or putty)");
                ImGui::BulletText("0.5 = Medium bounce (like a basketball)");
                ImGui::BulletText("0.9 = Very bouncy (like a rubber super ball)");
                ImGui::BulletText("1.0 = Perfect bounce (no energy lost)");
                ImGui::EndTooltip();
            }
            
            ImGui::PushItemWidth(-1);
            float bounce = cc->Restitution;
            if (ImGui::SliderFloat("##bounciness", &bounce, 0.0f, 1.0f, "%.3f"))
            {
                cc->Collider->getMaterial().setBounciness(bounce);
                cc->Restitution = bounce;
            }
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Friction
            ImGui::Text("Friction");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("How much the object resists sliding.");
                ImGui::Separator();
                ImGui::BulletText("0.0 = Ice (super slippery)");
                ImGui::BulletText("0.5 = Wood or plastic");
                ImGui::BulletText("0.8 = Rubber");
                ImGui::BulletText("1.0 = Maximum grip");
                ImGui::EndTooltip();
            }
            
            ImGui::PushItemWidth(-1);
            float friction = cc->Friction;
            if (ImGui::SliderFloat("##friction", &friction, 0.0f, 1.0f, "%.3f"))
            {
                cc->Collider->getMaterial().setFrictionCoefficient(friction);
                cc->Friction = friction;
            }
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Density
            ImGui::Text("Density (kg/m³)");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("How dense the material is (mass per volume).");
                ImGui::Text("This affects the calculated mass based on object size.");
                ImGui::Separator();
                ImGui::Text("Common densities:");
                ImGui::BulletText("Water: 1000 kg/m³");
                ImGui::BulletText("Wood: 500-800 kg/m³");
                ImGui::BulletText("Concrete: 2400 kg/m³");
                ImGui::BulletText("Steel: 7850 kg/m³");
                ImGui::BulletText("Gold: 19300 kg/m³");
                ImGui::EndTooltip();
            }
            
            ImGui::PushItemWidth(-1);
            float density = cc->MassDensity;
            if (ImGui::DragFloat("##density", &density, 1.0f, 0.1f, 10000.0f))
            {
                cc->Collider->getMaterial().setMassDensity(density);
                cc->MassDensity = density;
            }
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Volume (read-only)
            ImGui::Text("Volume (m³)");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("The volume of the object.");
                ImGui::Text("This is automatically calculated based on object size.");
                ImGui::EndTooltip();
            }
            
            ImGui::PushItemWidth(-1);
            ImGui::BeginDisabled();
            float volume = cc->Shape->getVolume();
            ImGui::DragFloat("##volume", &volume, 0.1f, 0.0f, 1000000.0f);
            ImGui::EndDisabled();
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
        }
    }


}