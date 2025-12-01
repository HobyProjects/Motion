#include "CorePCH.hpp"
#include "EntityProperties.hpp"

namespace Motion
{
    void EntityProperties::OnRender(Scene* scene)
    {
        if(!scene) return;

        auto& context = scene->GetContext();
        if(!context.Panels->ShowEntityComponents) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::Begin("Entity Properties", &context.Panels->ShowEntityComponents);

        if (context.Entities->SelectedEntity == entt::null)
        {
            HeadingConfig selctionConfig{};
            selctionConfig.Separator = true;
            Heading("No Entity Selected", HeadingLevel::H1, selctionConfig);

            LabelConfig lblConfig{};
            lblConfig.Wrapped = true;
            LabelSimple("Select an entity in the viewport to view its properties.", lblConfig);

            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        auto* tc = context.Entities->Registry.try_get<TransformComponent>(context.Entities->SelectedEntity);
        auto* cc = context.Entities->Registry.try_get<ColliderComponent>(context.Entities->SelectedEntity);
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(context.Entities->SelectedEntity);
        
        if (!tc || !cc || !rb)
        {
            HeadingConfig invalidConfig{};
            invalidConfig.Separator = true;
            Heading("Invalid Entity", HeadingLevel::H2, invalidConfig);

            LabelConfig lblConfig{};
            lblConfig.Wrapped = true;
            LabelSimple("This entity is missing required components.", lblConfig);
            
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        RenderTransform(tc);

        ImGui::Spacing();
        ImGui::Spacing();

        
        RenderRigidBody(rb);

        ImGui::Spacing();
        ImGui::Spacing();
        
        RenderCollider(cc);

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void EntityProperties::RenderTransform(TransformComponent* tc)
    {
        HeadingConfig headerConfig;
        headerConfig.Separator = true;
        Heading(ICON_MD_3D_ROTATION " Transform Component", HeadingLevel::H2, headerConfig);

        ImGui::Indent();
        {                
            glm::vec3 translation = tc->Translation;
            
            DragFloatConfig dragConfig;
            dragConfig.Speed = 0.1f;
            dragConfig.Fmt = "%.2f m";
            dragConfig.ResetValue = 0.0f;
            dragConfig.Tooltip = "The position of the object in 3D space (X, Y, Z coordinates)\nMeasured in meters";
            DragFloat3("Position", translation, dragConfig, [&](const glm::vec3& newTranslation){
                tc->Translation = newTranslation;
            });
        }            
        {                
            glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(tc->Rotation));
            
            DragFloatConfig rotConfig;
            rotConfig.Speed = 1.0f;
            rotConfig.Fmt = "%.1f°";
            rotConfig.ResetValue = 0.0f;
            rotConfig.Tooltip = "The rotation of the object around each axis\nMeasured in degrees (0-360)";
            DragFloat3("Rotation", eulerDeg, rotConfig, [&](const glm::vec3& newEuler){
                glm::vec3 radians = glm::radians(newEuler);
                tc->Rotation = glm::quat(radians);
            });
        }
        {                
            glm::vec3 scale = tc->Scale;
            
            DragFloatConfig scaleConfig;
            scaleConfig.Speed = 0.01f;
            scaleConfig.MinV = 1.0f;
            scaleConfig.ResetValue = 1.0f;
            scaleConfig.Fmt = "%.2f";
            scaleConfig.Tooltip = "The size multiplier for each axis\n1.0 = original size\n2.0 = double size\n0.5 = half size";
            DragFloat3("Scale", scale, scaleConfig, [&](const glm::vec3& newScale){
                tc->Scale = newScale;
            });
        }
        ImGui::Unindent();
    }

    void EntityProperties::RenderRigidBody(RigidBodyComponent* rb)
    {
        auto* body = rb->PhysicsBody;

        HeadingConfig headerConfig;
        headerConfig.Separator = true;
        Heading(ICON_MD_NOW_WIDGETS " Rigid Body Properties", HeadingLevel::H2, headerConfig); 


        ImGui::Indent();            
        {
            bool allowSleeping = body->isAllowedToSleep();
            ToggleSwitch("Allow Sleeping", &allowSleeping, ToggleSwitchPresets::iOS(), [&](bool enabled) {
                body->setIsAllowedToSleep(enabled);
            });
            
            if(ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Allow the body to enter sleep mode when stationary\nThis improves performance by not simulating inactive objects");
            }
        }
        {   
            bool isStatic = (body->getType() == rp3d::BodyType::STATIC);

            ComboBoxConfig comboConfig;
            comboConfig.Tooltip = "Select the type of body";
            static int currentTypeIndex = isStatic ? 0 : 1;

            ComboBox("Body Type", currentTypeIndex, {"Static", "Dynamic"}, comboConfig, [&](const std::string& item, int index){
                if(index == 0)
                {
                    rb->Type = BodyType::Static;
                    body->setType(rp3d::BodyType::STATIC);
                }
                else if(index == 1)
                {
                    rb->Type = BodyType::Dynamic;
                    body->setType(rp3d::BodyType::DYNAMIC);
                }
            });
        }
        {
            LabelConfig massConfig{};
            massConfig.Tooltip = "How heavy the object is in kilograms\nHeavier objects need more force to move and have more momentum\nExamples:\n- Basketball ≈ 0.6 kg\n- Person ≈ 70 kg\n- Car ≈ 1500 kg";
            LabelValue("Computed Mass", body->getMass(), "%.2f kg", massConfig);
        }
        {                
            float linDamp = static_cast<float>(body->getLinearDamping());
            
            SliderFloatConfig dampConfig;
            dampConfig.MinV = 0.0f;
            dampConfig.MaxV = 1.0f;
            dampConfig.Fmt = "%.2f";
            dampConfig.Tooltip = "How quickly the object stops moving due to air resistance\nHigher values make it stop faster\nExamples:\n- A hockey puck has low linear damping (glides far on ice)\n- A rolling ball has medium linear damping (slows down over time)\n- A parachute has high linear damping (quickly loses speed)";
            
            SliderFloat("Linear Damping", &linDamp, dampConfig, [&](float value){
                body->setLinearDamping(linDamp);
            });
        }            
        {
            float angDamp = static_cast<float>(body->getAngularDamping());
            
            SliderFloatConfig angDampConfig;
            angDampConfig.MinV = 0.0f;
            angDampConfig.MaxV = 1.0f;
            angDampConfig.Fmt = "%.2f";
            angDampConfig.Tooltip = "How quickly the object stops spinning due to air resistance\nHigher values make it stop faster\nExamples:\n- A spinning top has low angular damping (spins for a long time)\n- A thrown frisbee has medium angular damping (slows rotation moderately)\n- A spinning fan blade has high angular damping (stops quickly when power is off)";
            SliderFloat("Angular Damping", &angDamp, angDampConfig, [&](float value){
                body->setAngularDamping(angDamp);
            });
        }
        ImGui::Unindent();
    }

    void EntityProperties::RenderCollider(ColliderComponent* cc)
    {
        HeadingConfig headerConfig{};
        headerConfig.Separator = true;
        Heading(ICON_MD_VIEW_IN_AR " Collider Properties", HeadingLevel::H2, headerConfig);

        ImGui::Indent();
        {                
            float volume = cc->Shape->getVolume();
            LabelConfig volumeConfig;
            volumeConfig.Tooltip = "The volume of the object, in cubic meters.\nAutomatically calculated based on object size";
            LabelValue("Volume", volume, "%.2f m³", volumeConfig);
        } 
        {                
            float bounce = cc->Restitution;
            
            SliderFloatConfig bounceConfig;
            bounceConfig.MinV = 0.0f;
            bounceConfig.MaxV = 1.0f;
            bounceConfig.Fmt = "%.2f";
            bounceConfig.Tooltip = "How bouncy the object is when it hits something\nBounciness affects how much energy is lost when the object hits something\n- 0.0 = No bounce (like clay or putty)\n- 0.5 = Medium bounce (like a basketball)\n- 0.9 = Very bouncy (like a rubber super ball)\n- 1.0 = Perfect bounce (no energy lost)";
            SliderFloat("Bounciness", &bounce, bounceConfig, [&](float value){
                cc->Collider->getMaterial().setBounciness(bounce);
                cc->Restitution = bounce;
            });
        }            
        {                
            float friction = cc->Friction;

            SliderFloatConfig frictionConfig;
            frictionConfig.MinV = 0.0f;
            frictionConfig.MaxV = 1.0f;
            frictionConfig.Fmt = "%.2f";
            frictionConfig.Tooltip = "How much the object resists sliding against other surfaces\nLower values make it more slippery\nExamples:\n- Ice has low friction (around 0.1)\n- Wood or plastic has medium friction (around 0.5)\n- Rubber has high friction (around 0.8)";
            SliderFloat("Friction", &friction, frictionConfig, [&](float value){
                cc->Collider->getMaterial().setFrictionCoefficient(friction);
                cc->Friction = friction;
            });
        }
        {
            float density = cc->MassDensity;
            
            DragFloatConfig densityConfig;
            densityConfig.Speed = 10.0f;
            densityConfig.MinV = 0.1f;
            densityConfig.MaxV = 20000.0f;
            densityConfig.Fmt = "%.2f kg/m³";
            densityConfig.Tooltip = "The mass per unit volume of the material,\nHigher density materials result in heavier objects\nExamples:\n- Water: 1000 kg/m³\n- Wood: 500-800 kg/m³\n- Concrete: 2400 kg/m³\n- Steel: 7850 kg/m³";
            DragFloat("Density", &density, densityConfig, [&](float value){
                cc->Collider->getMaterial().setMassDensity(density);
                cc->MassDensity = density;
            });
        }
        ImGui::Unindent();
    }
}