#include "CorePCH.hpp"
#include "SceneEnvironment.hpp"

namespace Motion
{
    void SceneEnvironmentSettings::OnRender(Scene * scene)
    {
        if(!scene) return;

        const ImGuiTreeNodeFlags flags =
              ImGuiTreeNodeFlags_Framed
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_AllowItemOverlap
            | ImGuiTreeNodeFlags_FramePadding
            | ImGuiTreeNodeFlags_DefaultOpen;

        auto& context = scene->GetContext();
        if(!context.Panels->ShowEnvironmentSettings) return;

        ImGui::Begin("Environment Settings", &context.Panels->ShowEnvironmentSettings);
        {
            if (ImGui::TreeNodeEx("##environment", flags, "Environment"))
            {
                ImGui::Indent();

                // Lighting Section
                if (ImGui::CollapsingHeader("Lighting"))
                {
                    ImGui::Indent(10.0f);
                    auto& light = context.PhysicsWorld->SunLight;
                    
                    if (BeginPropertyGrid("##sun-properties"))
                    {
                        ImGui::BeginDisabled(context.Simulation->InSimulation);
                        DragFloat3("Light Direction", light.Direction, 0.01f);
                        HelpMarker("The direction the main light comes from.\n"
                                "Think of this as the sun position.\n"
                                "(-1,0,0) = light from left, (0,-1,0) = light from above");
                        
                        ColorEdit3("Light Color", light.Color);
                        HelpMarker("The color of the light source.\n"
                                "White = natural sunlight, Yellow = warm light, Blue = cold light");
                        
                        DragFloat("Intensity", &light.Intensity, 0.1f, 0.0f, 50.0f);
                        HelpMarker("How bright the light is.\n"
                                "1.0 = normal daylight, 5.0 = very bright, 0.1 = dim");
                        
                        ToggleSwitch("Show Gizmo", light.ShowGuizmo);
                        HelpMarker("Show a visual indicator for light direction in the viewport");

                        ImGui::EndDisabled();
                        EndPropertyGrid();
                    }
                    ImGui::Unindent(10.0f);
                }

                // Physics World Settings
                if (ImGui::CollapsingHeader("Physics World"))
                {
                    ImGui::Indent(10.0f);
                    auto& world = context.PhysicsWorld->Settings;
                    
                    if (BeginPropertyGrid("##world-properties"))
                    {
                        ImGui::BeginDisabled(context.Simulation->InSimulation);
                        std::string worldName = world.worldName.empty() ? "New World" : world.worldName;
                        TextBox("World Name", worldName);
                        HelpMarker("A name for your physics world");

                        glm::vec3 gravity = ToVec3(world.gravity);
                        if (DragFloat3("Gravity (m/s²)", gravity, 0.1f, -50.0f, 50.0f))
                            world.gravity = ToVec3(gravity);
                        HelpMarker("The pull of gravity on all objects.\n"
                                "Earth = (0, -9.81, 0) downward\n"
                                "Moon = (0, -1.62, 0) weaker gravity\n"
                                "Space = (0, 0, 0) zero gravity");

                        float defaultRestitution = world.defaultBounciness;
                        if (SliderFloat("Default Bounciness", &defaultRestitution, 0.0f, 1.0f, "%.3f"))
                            world.defaultBounciness = defaultRestitution;
                        HelpMarker("Default bounciness for new objects");

                        float defaultFriction = world.defaultFrictionCoefficient;
                        if (SliderFloat("Default Friction", &defaultFriction, 0.0f, 1.0f, "%.3f"))
                            world.defaultFrictionCoefficient = defaultFriction;
                        HelpMarker("Default friction for new objects");

                        bool sleeping = world.isSleepingEnabled;
                        if (ToggleSwitch("Enable Sleep", sleeping))
                            world.isSleepingEnabled = sleeping;
                        HelpMarker("Allow objects to 'sleep' when not moving.\n"
                                "This saves CPU by not updating still objects.\n"
                                "Turn off for precise simulations.");

                        ImGui::EndDisabled();
                        EndPropertyGrid();
                    }
                    
                    // Advanced Physics Settings
                    if (ImGui::TreeNode("Advanced Settings"))
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
                        ImGui::TextWrapped("Advanced: These settings affect simulation accuracy and performance");
                        ImGui::PopStyleColor();
                        ImGui::Spacing();
                        
                        if (BeginPropertyGrid("##advanced-physics"))
                        {
                            int velIter = world.defaultVelocitySolverNbIterations;
                            if (DragFloat("Velocity Iterations", (float*)&velIter, 0.1f, 1.0f, 50.0f))
                                world.defaultVelocitySolverNbIterations = (unsigned int)velIter;
                            HelpMarker("Higher = more accurate velocity calculations but slower.\n"
                                    "Typical: 10-20 iterations");

                            int posIter = world.defaultPositionSolverNbIterations;
                            if (DragFloat("Position Iterations", (float*)&posIter, 0.1f, 1.0f, 50.0f))
                                world.defaultPositionSolverNbIterations = (unsigned int)posIter;
                            HelpMarker("Higher = objects penetrate less but slower.\n"
                                    "Typical: 5-10 iterations");

                            float sleepLinVel = world.defaultSleepLinearVelocity;
                            if (DragFloat("Sleep Linear Velocity", &sleepLinVel, 0.01f, 0.0f, 5.0f))
                                world.defaultSleepLinearVelocity = sleepLinVel;
                            HelpMarker("Objects slower than this can go to sleep");

                            float sleepAngVel = world.defaultSleepAngularVelocity;
                            if (DragFloat("Sleep Angular Velocity", &sleepAngVel, 0.01f, 0.0f, 5.0f))
                                world.defaultSleepAngularVelocity = sleepAngVel;
                            HelpMarker("Objects rotating slower than this can go to sleep");

                            float timeBeforeSleep = world.defaultTimeBeforeSleep;
                            if (DragFloat("Time Before Sleep (s)", &timeBeforeSleep, 0.1f, 0.0f, 10.0f))
                                world.defaultTimeBeforeSleep = timeBeforeSleep;
                            HelpMarker("How long an object must be still before sleeping");

                            EndPropertyGrid();
                        }
                        
                        ImGui::TreePop();
                    }
                    
                    ImGui::Unindent(10.0f);
                }

                ImGui::Unindent();
                ImGui::TreePop();
            }

        }
        ImGui::End();
    }


}