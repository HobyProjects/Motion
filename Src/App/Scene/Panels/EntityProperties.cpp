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
        ImGui::PushID("##transform-properties");
        BeginPropertyGrid("##transform-grid");

        glm::vec3 t = tc->Translation;
        if (DragFloat3("Position (m)", t, 0.1f)) tc->Translation = t;
        HelpMarker("The position of the object in 3D space (X, Y, Z coordinates).\nMeasured in meters.");

        glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(tc->Rotation));
        if (DragFloat3("Rotation (deg)", eulerDeg, 1.0f)) tc->Rotation = glm::normalize(glm::quat(glm::radians(eulerDeg)));
        HelpMarker("The rotation of the object around each axis.\nMeasured in degrees (0-360).");

        glm::vec3 s = tc->Scale;
        if (DragFloat3("Scale", s, 0.1f, 0.01f, 100.0f)) tc->Scale = s;
        HelpMarker("The size multiplier for each axis.\n1.0 = original size, 2.0 = double size, 0.5 = half size");

        EndPropertyGrid();
        ImGui::PopID();
    }

    void EntityProperties::RenderRigidBody(RigidBodyComponent* rb)
    {
        if (ImGui::CollapsingHeader("Rigid Body Properties"))
        {
            ImGui::Indent(10.0f);
            
            if (BeginPropertyGrid("##rigidbody-props"))
            {
                bool allowSleeping = rb->PhysicsBody->isAllowedToSleep();
                if(ToggleSwitch("Allow Sleeping", allowSleeping)){
                    rb->PhysicsBody->setIsAllowedToSleep(allowSleeping);
                }
                HelpMarker("Allow the body to sleep if it is not moving");

                std::int32_t interaction = (rb->PhysicsBody->getType() == rp3d::BodyType::DYNAMIC) ? 1 : 0;
                ComboBox("Body Type", { "Static", "Dynamic" }, interaction,
                    [&](std::int32_t idx, const std::string&)
                    {
                        if (idx == 0) { rb->Type = BodyType::Static;  rb->PhysicsBody->setType(rp3d::BodyType::STATIC); }
                        if (idx == 1) { rb->Type = BodyType::Dynamic; rb->PhysicsBody->setType(rp3d::BodyType::DYNAMIC); }
                    });
                HelpMarker("Static: Immovable objects like walls, floors, and obstacles\n"
                          "Dynamic: Objects that move and respond to forces like balls, boxes, characters");

                auto* body = rb->PhysicsBody;

                // Mass
                ImGui::BeginDisabled(true);
                float mass = static_cast<float>(body->getMass());
                if (DragFloat("Computed Mass (kg)", &mass, 0.1f, 0.01f, 10000.0f))
                HelpMarker("How heavy the object is in kilograms.\n"
                          "Heavier objects need more force to move and have more momentum.\n"
                          "Examples: Basketball ≈0.6kg, Car ≈1500kg, Person ≈70kg");
                ImGui::EndDisabled();

                // Linear Damping
                float linDamp = static_cast<float>(body->getLinearDamping());
                if (SliderFloat("Linear Damping", &linDamp, 0.0f, 1.0f, "%.3f"))
                    body->setLinearDamping(linDamp);
                HelpMarker("Air resistance for movement. Higher = slows down faster.\n"
                          "0 = No air resistance (moves forever like in space)\n"
                          "0.5 = Medium resistance (normal physics)\n"
                          "1.0 = High resistance (moving through water)");

                // Angular Damping
                float angDamp = static_cast<float>(body->getAngularDamping());
                if (SliderFloat("Angular Damping", &angDamp, 0.0f, 1.0f, "%.3f"))
                    body->setAngularDamping(angDamp);
                HelpMarker("Air resistance for rotation/spinning. Higher = stops spinning faster.\n"
                          "0 = Spins forever like in space\n"
                          "0.5 = Normal spinning (like a basketball)\n"
                          "1.0 = Stops spinning quickly");

                EndPropertyGrid();
            }
            
            ImGui::Unindent(10.0f);
        }
    }

    void EntityProperties::RenderCollider(ColliderComponent* cc)
    {
        if (ImGui::CollapsingHeader("Collider Properties"))
        {
            ImGui::Indent(10.0f);
            
            if (BeginPropertyGrid("##collider-props"))
            {
                // Bounciness (Restitution)
                float bounce = cc->Restitution;
                if (SliderFloat("Bounciness", &bounce, 0.0f, 1.0f, "%.3f"))
                {
                    cc->Collider->getMaterial().setBounciness(bounce);
                    cc->Restitution = bounce;
                }
                HelpMarker("How bouncy the object is when it hits something.\n"
                          "0.0 = No bounce (like clay or putty)\n"
                          "0.5 = Medium bounce (like a basketball)\n"
                          "0.9 = Very bouncy (like a rubber super ball)\n"
                          "1.0 = Perfect bounce (no energy lost)");

                // Friction
                float friction = cc->Friction;
                if (SliderFloat("Friction", &friction, 0.0f, 1.0f, "%.3f"))
                {
                    cc->Collider->getMaterial().setFrictionCoefficient(friction);
                    cc->Friction = friction;
                }
                HelpMarker("How much the object resists sliding.\n"
                          "0.0 = Ice (super slippery)\n"
                          "0.5 = Wood or plastic\n"
                          "0.8 = Rubber\n"
                          "1.0 = Maximum grip");

                // Density
                float density = cc->MassDensity;
                if (DragFloat("Density (kg/m³)", &density, 1.0f, 0.1f, 10000.0f))
                {
                    cc->Collider->getMaterial().setMassDensity(density);
                    cc->MassDensity = density;
                }
                HelpMarker("How dense the material is (mass per volume).\n"
                          "This affects the calculated mass based on object size.\n\n"
                          "Common densities:\n"
                          "• Water: 1000 kg/m³\n"
                          "• Wood: 500-800 kg/m³\n"
                          "• Concrete: 2400 kg/m³\n"
                          "• Steel: 7850 kg/m³\n"
                          "• Gold: 19300 kg/m³");

                // Volume
                ImGui::BeginDisabled();
                float volume = cc->Shape->getVolume();
                DragFloat("Volume", &volume, 0.1f, 0.0f, 1000000.0f);
                HelpMarker("The volume of the object.\n"
                          "This is automatically calculated based on object size.");
                ImGui::EndDisabled();

                EndPropertyGrid();
            }
            
            ImGui::Unindent(10.0f);
        }
    }


}