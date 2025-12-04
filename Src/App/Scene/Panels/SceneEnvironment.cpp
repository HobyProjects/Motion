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
            // ═══════════════════════════════════════════════════════════════
            // LIGHTING SECTION
            // ═══════════════════════════════════════════════════════════════
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
            timeConfig.Speed = 0.001f;
            timeConfig.MinV = 0.001f;
            timeConfig.MaxV = 10.0f;
            timeConfig.Fmt = "%.2f";
            
            DragFloat("Translation Speed", &translationSpeed, timeConfig, [&](float val){
                context.View->Camera.TranslationSpeed = val;
            });

            float sensitivity = context.View->Camera.Sensitivity;
            DragFloatConfig timeConfig2;
            timeConfig2.Speed = 0.01f;
            timeConfig2.MinV = 0.01f;
            timeConfig2.MaxV = 5.0f;
            timeConfig2.Fmt = "%.2f";
            
            DragFloat("Sensitivity", &sensitivity, timeConfig2, [&](float val){
                context.View->Camera.Sensitivity = val;
            });

            ToggleSwitch("Enable Rotation", &context.View->Camera.RotationEnabled);
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Enables camera rotation with \"Q\" and \"E\" keys");
        }
        

        ImGui::Spacing();
        ImGui::Spacing();
        
        RenderPostProcessingSection(scene);
        
        ImGui::EndDisabled();

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void SceneEnvironmentSettings::RenderPostProcessingSection(Scene* scene)
    {
        auto& context = scene->GetContext();
        auto& pp = context.Physics->PostProcessing;
        auto& postProcessStack = scene->GetPostProcessStack();
        
        bool settingsChanged = false;

        HeadingConfig config;
        config.Separator = true;
        Heading(ICON_MD_AUTO_FIX_HIGH " Post-Processing", HeadingLevel::H3, config);
        
        LabelConfig lblConfig;
        lblConfig.Wrapped = true;
        LabelSimple("Visual effects applied to the final rendered image", lblConfig);
        
        ImGui::Spacing();
        
        // Master toggle
        if(ToggleSwitch("Enable Post-Processing", &pp.Enabled, ToggleSwitchPresets::iOS()))
        {
            scene->EnablePostProcessing(pp.Enabled);
            settingsChanged = true;
        }
        
        if(!pp.Enabled)
        {
            ImGui::TextDisabled("Post-processing is disabled");
            return;
        }
        
        ImGui::Spacing();
        
        // Preset selection
        ImGui::TextDisabled("Visual Presets:");
        const char* presets[] = { "Custom", "Cinematic", "Realistic", "Stylized", "Horror", "Sci-Fi", "Fantasy" };
        int currentPreset = static_cast<int>(pp.CurrentPreset);
        
        ImGui::SetNextItemWidth(-1);
        if(ImGui::Combo("##Preset", &currentPreset, presets, IM_ARRAYSIZE(presets)))
        {
            pp.CurrentPreset = static_cast<ScenePhysics::PostProcessingSettings::Preset>(currentPreset);
            
            if(pp.CurrentPreset != ScenePhysics::PostProcessingSettings::Preset::Custom)
            {
                pp.ApplyPreset(pp.CurrentPreset);
                settingsChanged = true;
            }
        }
        
        if(pp.CurrentPreset != ScenePhysics::PostProcessingSettings::Preset::Custom)
        {
            ImGui::SameLine();
            if(ImGui::SmallButton("Customize"))
            {
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            }
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Switch to custom mode to manually adjust settings");
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Individual effect controls
        RenderBloomControls(pp, settingsChanged);
        
        ImGui::Spacing();
        
        RenderToneMappingControls(pp, settingsChanged);
        
        ImGui::Spacing();
        
        RenderColorGradingControls(pp, settingsChanged);
        
        ImGui::Spacing();
        
        RenderVignetteControls(pp, settingsChanged);
        
        ImGui::Spacing();
        
        RenderFXAAControls(pp, settingsChanged);
        
        // Apply settings if changed
        if(settingsChanged)
        {
            pp.ApplyToStack(postProcessStack);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Reset button
        if(ImGui::Button("Reset Post-Processing", ImVec2(-1, 0)))
        {
            pp.Reset();
            pp.ApplyToStack(postProcessStack);
        }
        if(ImGui::IsItemHovered())
            ImGui::SetTooltip("Reset all post-processing settings to defaults");
    }

    void SceneEnvironmentSettings::RenderBloomControls(ScenePhysics::PostProcessingSettings& pp, bool& changed)
    {
        if(ImGui::TreeNodeEx("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if(ToggleSwitch("Enable##Bloom", &pp.Bloom.Enabled, ToggleSwitchPresets::iOS()))
            {
                changed = true;
            }
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Glow effect around bright objects");
            
            ImGui::BeginDisabled(!pp.Bloom.Enabled);
            
            SliderFloatConfig threshConfig;
            threshConfig.MinV = 0.0f;
            threshConfig.MaxV = 5.0f;
            threshConfig.Fmt = "%.2f";
            SliderFloat("Threshold", &pp.Bloom.Threshold, threshConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Brightness level needed for bloom\n0.5-2.0 typical");
            
            SliderFloatConfig intensityConfig;
            intensityConfig.MinV = 0.0f;
            intensityConfig.MaxV = 2.0f;
            intensityConfig.Fmt = "%.2f";
            SliderFloat("Intensity", &pp.Bloom.Intensity, intensityConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Bloom strength\n0.3-0.8 typical");
            
            SliderFloatConfig radiusConfig;
            radiusConfig.MinV = 0.5f;
            radiusConfig.MaxV = 3.0f;
            radiusConfig.Fmt = "%.2f";
            SliderFloat("Radius", &pp.Bloom.Radius, radiusConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Blur spread size");
            
            DragFloatConfig iterConfig;
            iterConfig.Speed = 0.1f;
            iterConfig.MinV = 1.0f;
            iterConfig.MaxV = 10.0f;
            iterConfig.Fmt = "%.0f";
            float iterFloat = static_cast<float>(pp.Bloom.Iterations);
            DragFloat("Iterations", &iterFloat, iterConfig, [&](float val){
                pp.Bloom.Iterations = static_cast<int>(val);
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Quality vs performance\n3-7 recommended");
            
            ImGui::EndDisabled();
            ImGui::TreePop();
        }
    }

    void SceneEnvironmentSettings::RenderToneMappingControls(ScenePhysics::PostProcessingSettings& pp, bool& changed)
    {
        if(ImGui::TreeNodeEx("Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if(ToggleSwitch("Enable##ToneMapping", &pp.ToneMapping.Enabled, ToggleSwitchPresets::iOS()))
            {
                changed = true;
            }
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Converts HDR to displayable range");
            
            ImGui::BeginDisabled(!pp.ToneMapping.Enabled);
            
            const char* operators[] = { "Reinhard", "Reinhard Luminance", "Uncharted 2", "ACES", "Exposure" };
            ImGui::SetNextItemWidth(-1);
            if(ImGui::Combo("Operator", &pp.ToneMapping.OperatorIndex, operators, IM_ARRAYSIZE(operators)))
            {
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            }
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("ACES recommended for most scenes");
            
            SliderFloatConfig expConfig;
            expConfig.MinV = 0.1f;
            expConfig.MaxV = 5.0f;
            expConfig.Fmt = "%.2f";
            SliderFloat("Exposure", &pp.ToneMapping.Exposure, expConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Scene brightness\n1.0 = neutral");
            
            SliderFloatConfig gammaConfig;
            gammaConfig.MinV = 1.0f;
            gammaConfig.MaxV = 3.0f;
            gammaConfig.Fmt = "%.2f";
            SliderFloat("Gamma", &pp.ToneMapping.Gamma, gammaConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Gamma correction\n2.2 is standard");
            
            if(pp.ToneMapping.OperatorIndex == 2) // Uncharted 2
            {
                SliderFloatConfig wpConfig;
                wpConfig.MinV = 1.0f;
                wpConfig.MaxV = 20.0f;
                wpConfig.Fmt = "%.1f";
                SliderFloat("White Point", &pp.ToneMapping.WhitePoint, wpConfig, [&](float val){
                    changed = true;
                    pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
                });
                if(ImGui::IsItemHovered())
                    ImGui::SetTooltip("White point for Uncharted 2");
            }
            
            ImGui::EndDisabled();
            ImGui::TreePop();
        }
    }

    void SceneEnvironmentSettings::RenderColorGradingControls(ScenePhysics::PostProcessingSettings& pp, bool& changed)
    {
        if(ImGui::TreeNode("Color Grading"))
        {
            if(ToggleSwitch("Enable##ColorGrading", &pp.ColorGrading.Enabled, ToggleSwitchPresets::iOS()))
            {
                changed = true;
            }
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Cinematic color adjustments");
            
            ImGui::BeginDisabled(!pp.ColorGrading.Enabled);
            
            ImGui::TextDisabled("Basic Adjustments:");
            
            SliderFloatConfig satConfig;
            satConfig.MinV = 0.0f;
            satConfig.MaxV = 2.0f;
            satConfig.Fmt = "%.2f";
            SliderFloat("Saturation", &pp.ColorGrading.Saturation, satConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("0.0 = grayscale\n1.0 = normal\n2.0 = vibrant");
            
            SliderFloatConfig contrastConfig;
            contrastConfig.MinV = 0.5f;
            contrastConfig.MaxV = 2.0f;
            contrastConfig.Fmt = "%.2f";
            SliderFloat("Contrast", &pp.ColorGrading.Contrast, contrastConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("1.0 = normal contrast");
            
            SliderFloatConfig brightConfig;
            brightConfig.MinV = -1.0f;
            brightConfig.MaxV = 1.0f;
            brightConfig.Fmt = "%.2f";
            SliderFloat("Brightness", &pp.ColorGrading.Brightness, brightConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("0.0 = normal brightness");
            
            ImGui::Spacing();
            ImGui::TextDisabled("Color Wheels:");
            
            ColorEditConfig colorConfig;
            colorConfig.Flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_Float;
            

            ColorEdit3("Shadows", pp.ColorGrading.Shadows, colorConfig, [&]([[maybe_unused]] glm::vec3 val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            
            ColorEdit3("Midtones", pp.ColorGrading.Midtones, colorConfig, [&]([[maybe_unused]] glm::vec3 val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            
            ColorEdit3("Highlights", pp.ColorGrading.Highlights, colorConfig, [&]([[maybe_unused]] glm::vec3 val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            
            ImGui::EndDisabled();
            ImGui::TreePop();
        }
    }

    void SceneEnvironmentSettings::RenderVignetteControls(ScenePhysics::PostProcessingSettings& pp, bool& changed)
    {
        if(ImGui::TreeNode("Vignette"))
        {
            if(ToggleSwitch("Enable##Vignette", &pp.Vignette.Enabled, ToggleSwitchPresets::iOS()))
            {
                changed = true;
            }
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Edge darkening effect");
            
            ImGui::BeginDisabled(!pp.Vignette.Enabled);
            
            SliderFloatConfig intensityConfig;
            intensityConfig.MinV = 0.0f;
            intensityConfig.MaxV = 1.0f;
            intensityConfig.Fmt = "%.2f";
            SliderFloat("Intensity", &pp.Vignette.Intensity, intensityConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Vignette strength\n0.3-0.6 typical");
            
            SliderFloatConfig smoothConfig;
            smoothConfig.MinV = 0.0f;
            smoothConfig.MaxV = 1.0f;
            smoothConfig.Fmt = "%.2f";
            SliderFloat("Smoothness", &pp.Vignette.Smoothness, smoothConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Edge softness");
            
            ColorEditConfig colorConfig;
            colorConfig.Flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_Float;
            ColorEdit3("Color", pp.Vignette.Color, colorConfig, [&]([[maybe_unused]] glm::vec3 val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
                
            });

            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Vignette color\nBlack is typical");
            
            ImGui::EndDisabled();
            ImGui::TreePop();
        }
    }

    void SceneEnvironmentSettings::RenderFXAAControls(ScenePhysics::PostProcessingSettings& pp, bool& changed)
    {
        if(ImGui::TreeNode("FXAA (Anti-Aliasing)"))
        {
            if(ToggleSwitch("Enable##FXAA", &pp.FXAA.Enabled, ToggleSwitchPresets::iOS()))
            {
                changed = true;
            }
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Fast anti-aliasing for smooth edges");
            
            ImGui::BeginDisabled(!pp.FXAA.Enabled);
            
            ImGui::TextDisabled("Quality Settings:");
            
            SliderFloatConfig edgeConfig;
            edgeConfig.MinV = 0.0f;
            edgeConfig.MaxV = 0.5f;
            edgeConfig.Fmt = "%.3f";
            SliderFloat("Edge Threshold", &pp.FXAA.EdgeThreshold, edgeConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Edge detection sensitivity\n0.063-0.333 typical");
            
            SliderFloatConfig edgeMinConfig;
            edgeMinConfig.MinV = 0.0f;
            edgeMinConfig.MaxV = 0.1f;
            edgeMinConfig.Fmt = "%.4f";
            SliderFloat("Edge Threshold Min", &pp.FXAA.EdgeThresholdMin, edgeMinConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Minimum edge threshold");
            
            DragFloatConfig stepsConfig;
            stepsConfig.Speed = 0.1f;
            stepsConfig.MinV = 4.0f;
            stepsConfig.MaxV = 16.0f;
            stepsConfig.Fmt = "%.0f";
            float stepsFloat = static_cast<float>(pp.FXAA.SearchSteps);
            DragFloat("Search Steps", &stepsFloat, stepsConfig, [&](float val){
                pp.FXAA.SearchSteps = static_cast<int>(val);
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Quality vs performance\n8-12 recommended");
            
            SliderFloatConfig subpixelConfig;
            subpixelConfig.MinV = 0.0f;
            subpixelConfig.MaxV = 1.0f;
            subpixelConfig.Fmt = "%.2f";
            SliderFloat("Subpixel Quality", &pp.FXAA.SubpixelQuality, subpixelConfig, [&](float val){
                changed = true;
                pp.CurrentPreset = ScenePhysics::PostProcessingSettings::Preset::Custom;
            });
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Sub-pixel AA strength");
            
            ImGui::EndDisabled();
            ImGui::TreePop();
        }
    }
}