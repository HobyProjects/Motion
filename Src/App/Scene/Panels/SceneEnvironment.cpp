#include "CorePCH.hpp"
#include "SceneEnvironment.hpp"

namespace Motion
{
    void SceneEnvironmentSettings::OnRender(Scene* scene)
    {
        if(!scene) return;

        auto& context = scene->GetContext();
        if(!context.Panels->ShowEnvironmentSettings) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Environment Settings", &context.Panels->ShowEnvironmentSettings);

        bool simulationRunning = context.Simulation->InSimulation;
        if (simulationRunning)
        {
            HeadingConfig config;
            config.Separator = true;
            config.Color = ImVec4(0.3f, 0.2f, 0.1f, 0.3f);
            Heading("In Simulation Mode", HeadingLevel::H2, config);

            LabelConfig lblConfig;
            lblConfig.Wrapped = true;
            LabelSimple("Environment settings can only be modified when the simulation is stopped. Please stop the simulation to make changes.", lblConfig);
        }
        
        ImGui::BeginDisabled(simulationRunning);
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_LIGHT_MODE " Lighting", HeadingLevel::H3, config);            

            auto& light = context.PhysicsWorld->SunLight;
            {                    
                DragFloatConfig dirConfig;
                dirConfig.Speed = 0.01f;
                dirConfig.MinV = -1.0f;
                dirConfig.MaxV = 1.0f;
                dirConfig.Fmt = "%.2f";
                dirConfig.ResetValue = -100.0f;
                dirConfig.Tooltip = "The direction vector for the light source\nThink of this as the sun position\n(-100, 0, 0) = Light from left\n(0, -100, 0) = Light from above\n(-100, 0, 0) = Light from right";
                DragFloat3("Direction", light.Direction, dirConfig);

                ColorEditConfig colorConfig;
                colorConfig.Flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_Float;
                ColorEdit3("Light Color", light.Color, colorConfig);

                DragFloatConfig intensityConfig;
                intensityConfig.Speed = 0.1f;
                intensityConfig.MinV = 0.0f;
                intensityConfig.MaxV = 50.0f;
                intensityConfig.Fmt = "%.1f";
                intensityConfig.ResetValue = 1.0f;
                intensityConfig.Tooltip = "How bright the light is, A higher intensity\nmeans a brighter light source\n0.1 = Dim\n1.0 = Normal daylight\n5.0 = Very bright";
                DragFloat("Intensity", &light.Intensity, intensityConfig);

                ToggleSwitch("Show Light Gizmo", &light.ShowGuizmo, ToggleSwitchPresets::iOS());
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("Show a visual indicator for light direction\nin the viewport");
            } 
        }
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_FOREST " Physics World", HeadingLevel::H3, config);           

            auto& world = context.PhysicsWorld->Settings;
            std::string worldName = world.worldName.empty() ? "New World" : world.worldName;
            TextBoxConfig nameConfig;
            nameConfig.ReadOnly = false;
            TextBox("World Name", worldName, nameConfig, [&](const std::string& val){
                world.worldName = val;
            });

            glm::vec3 gravity = ToVec3(world.gravity);
            DragFloatConfig gravityConfig;
            gravityConfig.Speed = 0.1f;
            gravityConfig.MinV = -50.0f;
            gravityConfig.MaxV = 50.0f;
            gravityConfig.Fmt = "%.2f m/s²";
            gravityConfig.ResetValue = 0.0f;
            gravityConfig.Tooltip = "The pull of gravity on all objects\n(0, -9.81, 0) = Earth\n(0, -1.62, 0) = Moon\n(0, -3.71, 0) = Mars\n(0, 0, 0) = Space";
            DragFloat3("Gravity Vector", gravity, gravityConfig, [&](const glm::vec3& val){
                world.gravity = reactphysics3d::Vector3(val.x, val.y, val.z);
            });

            ImGui::Spacing();
            ImGui::TextDisabled("Quick Presets:");
            ImGui::BeginGroup();
            {
                if(ImGui::Button("Earth"))
                {
                    world.gravity = reactphysics3d::Vector3(0.0f, -9.81f, 0.0f);
                }
                ImGui::SameLine();
                if(ImGui::Button("Moon"))
                {
                    world.gravity = reactphysics3d::Vector3(0.0f, -1.62f, 0.0f);
                }
                ImGui::SameLine();
                if(ImGui::Button("Mars"))
                {
                    world.gravity = reactphysics3d::Vector3(0.0f, -3.71f, 0.0f);
                }
                ImGui::SameLine();
                if(ImGui::Button("Zero-G"))
                {
                    world.gravity = reactphysics3d::Vector3(0.0f, 0.0f, 0.0f);
                }
            }
            ImGui::EndGroup();

            float defaultRestitution = world.defaultBounciness;
            SliderFloatConfig bounceConfig;
            bounceConfig.MinV = 0.0f;
            bounceConfig.MaxV = 1.0f;
            bounceConfig.Fmt = "%.3f";
            
            SliderFloat("Default Bounciness", &defaultRestitution, bounceConfig, [&](float val){
                world.defaultBounciness = defaultRestitution;
            });

            float defaultFriction = world.defaultFrictionCoefficient;
            SliderFloatConfig frictionConfig;
            frictionConfig.MinV = 0.0f;
            frictionConfig.MaxV = 1.0f;
            frictionConfig.Fmt = "%.3f";
            
            SliderFloat("Default Friction", &defaultFriction, frictionConfig, [&](float val){
                world.defaultFrictionCoefficient = defaultFriction;
            });

            bool sleeping = world.isSleepingEnabled;
            if(ToggleSwitch("Enable Sleep Mode", &sleeping, ToggleSwitchPresets::iOS()))
                world.isSleepingEnabled = sleeping;
            
            if(ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Allow objects to 'sleep' when not moving");
                ImGui::Separator();
                ImGui::BulletText("Saves CPU by not updating still objects");
                ImGui::BulletText("Turn off for precise simulations");
                ImGui::EndTooltip();
            }
        }
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_LANDSLIDE " Advanced Physics", HeadingLevel::H3, config);
            
            LabelConfig lblConfig;
            lblConfig.Wrapped = true;
            LabelSimple("These settings affect simulation accuracy and performance", lblConfig);
            
            auto& world = context.PhysicsWorld->Settings;
            {
                int velIter = world.defaultVelocitySolverNbIterations;
                float velIterFloat = static_cast<float>(velIter);
                
                DragFloatConfig velConfig;
                velConfig.Speed = 0.1f;
                velConfig.MinV = 1.0f;
                velConfig.MaxV = 50.0f;
                velConfig.Fmt = "%.0f";
                velConfig.Tooltip = "Solver iterations for velocity constraints\n-Higher = More accurate velocities\n-Higher = Slower performance\n-Typical: 10-20 iterations";
                DragFloat("Velocity Iterations", &velIterFloat, velConfig, [&](float val){
                    world.defaultVelocitySolverNbIterations = static_cast<unsigned int>(velIterFloat);
                });

                int posIter = world.defaultPositionSolverNbIterations;
                float posIterFloat = static_cast<float>(posIter);
                
                DragFloatConfig posConfig;
                posConfig.Speed = 0.1f;
                posConfig.MinV = 1.0f;
                posConfig.MaxV = 50.0f;
                posConfig.Fmt = "%.0f";
                posConfig.Tooltip = "Solver iterations for position constraints\n-Higher = More accurate positions\n-Higher = Slower performance\n-Typical: 5-10 iterations";
                DragFloat("Position Iterations", &posIterFloat, posConfig, [&](float val){
                    world.defaultPositionSolverNbIterations = static_cast<unsigned int>(posIterFloat);
                });


                float sleepLinVel = world.defaultSleepLinearVelocity;
                DragFloatConfig sleepLinConfig;
                sleepLinConfig.Speed = 0.01f;
                sleepLinConfig.MinV = 0.0f;
                sleepLinConfig.MaxV = 5.0f;
                sleepLinConfig.Fmt = "%.2f m/s";
                
                DragFloat("Linear Velocity", &sleepLinVel, sleepLinConfig, [&](float val){
                    world.defaultSleepLinearVelocity = sleepLinVel;
                });

                float sleepAngVel = world.defaultSleepAngularVelocity;
                DragFloatConfig sleepAngConfig;
                sleepAngConfig.Speed = 0.01f;
                sleepAngConfig.MinV = 0.0f;
                sleepAngConfig.MaxV = 5.0f;
                sleepAngConfig.Fmt = "%.2f rad/s";
                
                DragFloat("Angular Velocity", &sleepAngVel, sleepAngConfig, [&](float val){
                    world.defaultSleepAngularVelocity = sleepAngVel;
                });

                float timeBeforeSleep = world.defaultTimeBeforeSleep;
                DragFloatConfig timeConfig;
                timeConfig.Speed = 0.1f;
                timeConfig.MinV = 0.0f;
                timeConfig.MaxV = 10.0f;
                timeConfig.Fmt = "%.1f s";
                
                DragFloat("Time Before Sleep", &timeBeforeSleep, timeConfig, [&](float val){
                    world.defaultTimeBeforeSleep = timeBeforeSleep;
                });
            }  
        }

        ImGui::Spacing();
        ImGui::Spacing();

        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_CAMERA " Camera", HeadingLevel::H3, config);

            float translationSpeed = context.View->Camera.TranslationSpeed;
            DragFloatConfig timeConfig;
            timeConfig.Speed = 0.01f;
            timeConfig.MinV = 0.1f;
            timeConfig.MaxV = 10.0f;
            timeConfig.Fmt = "%.2f";
            
            DragFloat("Translation Speed", &translationSpeed, timeConfig, [&](float val){
                context.View->Camera.TranslationSpeed = val;
            });

            float sensitivity = context.View->Camera.Sensitivity;
            DragFloatConfig timeConfig2;
            timeConfig2.Speed = 0.01f;
            timeConfig2.MinV = 0.1f;
            timeConfig2.MaxV = 5.0f;
            timeConfig2.Fmt = "%.2f";
            
            DragFloat("Sensitivity", &sensitivity, timeConfig2, [&](float val){
                context.View->Camera.Sensitivity = val;
            });

            ToggleSwitch("Enable Rotation", &context.View->Camera.RotationEnabled);
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Enables camera rotation with \"Q\" and \"E\" keys");
        }
        ImGui::EndDisabled();

        
        ImGui::End();
        ImGui::PopStyleVar();
    }
}