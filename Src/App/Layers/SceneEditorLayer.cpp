#include "CorePCH.hpp"

#include "SceneUtils.hpp"
#include "SceneSerializer.hpp"
#include "SceneEditorLayer.hpp"

namespace Motion
{
    namespace PhysicsUI
    {

        static float GetSpeed(const glm::vec3& velocity)
        {
            return glm::length(velocity);
        }
        
        static glm::vec3 GetDirection(const glm::vec3& velocity)
        {
            float speed = GetSpeed(velocity);
            if (speed < 0.0001f) return glm::vec3(0.0f);
            return velocity / speed;
        }
        
        static float RadPerSecToRPM(float radPerSec)
        {
            return radPerSec * (60.0f / (2.0f * glm::pi<float>()));
        }
        
        static float MsToKmh(float ms)
        {
            return ms * 3.6f;
        }
        
        static const char* GetBodyTypeDescription(BodyType type)
        {
            switch (type)
            {
                case BodyType::Static:  return "Static (immovable, like walls or ground)";
                case BodyType::Dynamic: return "Dynamic (moves and collides with forces)";
                default:                return "Unknown";
            }
        }
        
        static void StatusIndicator(const char* label, bool active, const char* tooltip = nullptr)
        {
            ImVec4 color = active ? ImVec4(0.1f, 0.9f, 0.3f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
            ImGui::TextColored(color, "%s %s", active ? "●" : "○", label);
            if (tooltip && ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(tooltip);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
    }

    /**
     * @brief Called when the layer is attached to the application.
     * @details This method is used to load the base materials of the scene.
     * @note It is not recommended to load assets in this method, but rather in the OnUpdate method.
     * @see OnUpdate
     */
    void SceneEditorLayer::OnAttach()
    {
        static const std::array<const char*, 5> kPaths = 
        {
            "Assets/Materials/Metal/Base.yaml",
            "Assets/Materials/Marble/Base.yaml",
            "Assets/Materials/Plastic/Base.yaml",
            "Assets/Materials/Rubber/Base.yaml",
            "Assets/Materials/Stone/Base.yaml",
        };

        m_BaseMaterial.reserve(kPaths.size());
        
        for (auto* p : kPaths) 
            m_BaseMaterial.push_back(Material::CreateBase(p));
        
        std::memset(m_SearchBuf, 0, sizeof(m_SearchBuf));
        m_PlotExporter = IPlotExporter::Create();
        m_ToastManager = std::make_unique<ToastManager>(5.0f, 0.5f);
    }

    /**
     * @brief Called when the layer is detached from the application.
     * @details This method is used to reset any in-progress operations when the layer is detached.
     * @note It is not recommended to load assets in this method, but rather in the OnUpdate method.
     * @see OnUpdate
     */
    void SceneEditorLayer::OnDetach()
    {
        if (m_SceneCreationOp.State == AsyncOperationState::InProgress) m_SceneCreationOp.Reset();
        if (m_SceneLoadOp.State == AsyncOperationState::InProgress) m_SceneLoadOp.Reset();
        if (m_EntityImportOp.State == AsyncOperationState::InProgress) m_EntityImportOp.Reset();
    }

    /**
     * @brief Called on every frame update.
     *
     * This function is responsible for updating the scene and submitting any changes to the rendering pipeline.
     *
     * @param handle The window handle of the application.
     * @param deltaTime The time elapsed since the last frame.
     * @note This function will return immediately if the scene is not valid.
     */
    void SceneEditorLayer::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        if (!m_Scene) return;

        m_ToastManager->Update();
        m_Scene->OnUpdate(handle, deltaTime);

        auto& context = m_Scene->GetContext();
        if (m_PhysicsAnalysis.SelectedEntity != entt::null && context.Entities->Registry.valid(m_PhysicsAnalysis.SelectedEntity))
        {
            float dt = deltaTime.GetDeltaTimeSeconds();
            
            if (m_PhysicsAnalysis.ShowForceAnalysisPanel)
                UpdateForceAnalysis(m_PhysicsAnalysis.SelectedEntity, dt);
            
            if (m_PhysicsAnalysis.ShowEnergyPanel && m_PhysicsAnalysis.Energy.TrackingEnabled)
                UpdateEnergyTracking(m_PhysicsAnalysis.SelectedEntity, dt);
            
            if (m_PhysicsAnalysis.ShowMomentumPanel)
                UpdateMomentumTracking(m_PhysicsAnalysis.SelectedEntity, dt);
            
            if (m_PhysicsAnalysis.ShowAccelerationPanel)
                UpdateAcceleration(m_PhysicsAnalysis.SelectedEntity, dt);
            
            if (m_PhysicsAnalysis.ShowTrajectoryPanel && m_PhysicsAnalysis.Trajectory.EnablePrediction)
                UpdateTrajectoryPrediction(m_PhysicsAnalysis.SelectedEntity);
        }
        
        if (m_PhysicsAnalysis.ShowCollisionPanel && m_PhysicsAnalysis.Collisions.EnableAnalysis)
        {
            DetectAndAnalyzeCollisions(context);
        }

        m_Scene->Submit();
    }

    /**
     * @brief Called when an event happens on the scene editor layer.
     *
     * This function is responsible for passing the event to the scene object, if it exists.
     *
     * @param handle The window handle of the application.
     * @param e The event that happened.
     * @note This function will return immediately if the scene is not valid.
     */
    void SceneEditorLayer::OnEvent(WindowHandle handle, IEvent& e)
    {
        if (m_Scene) m_Scene->OnEvent(handle, e);
    }

    /**
     * @brief Called when the scene editor layer needs to render its UI components.
     *
     * This function is responsible for rendering the dockspace, handling scene creation, scene loading, and entity importing.
     * It also renders a loading overlay if any of the operations above are in progress, and an error modal if any of them have failed.
     *
     * @param handle The window handle of the application.
     */
    void SceneEditorLayer::OnUIRender(WindowHandle handle)
    {
        BuildDockspace();
        
        m_ToastManager->Render();
        HandleSceneCreation();
        HandleSceneLoading();
        HandleEntityImport();

        if (m_Scene) 
            RenderScene();

        if (m_SceneCreationOp.State == AsyncOperationState::InProgress ||
            m_SceneLoadOp.State == AsyncOperationState::InProgress ||
            m_EntityImportOp.State == AsyncOperationState::InProgress)
        {
            RenderLoadingOverlay();
        }

        if (m_SceneCreationOp.State == AsyncOperationState::Failed ||
            m_SceneLoadOp.State == AsyncOperationState::Failed ||
            m_EntityImportOp.State == AsyncOperationState::Failed)
        {
            RenderErrorModal();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_F) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            m_PhysicsAnalysis.ShowForceAnalysisPanel = !m_PhysicsAnalysis.ShowForceAnalysisPanel;
        }
        
        if (ImGui::IsKeyPressed(ImGuiKey_E) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            m_PhysicsAnalysis.ShowEnergyPanel = !m_PhysicsAnalysis.ShowEnergyPanel;
        }
        
        if (ImGui::IsKeyPressed(ImGuiKey_M) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            m_PhysicsAnalysis.ShowMomentumPanel = !m_PhysicsAnalysis.ShowMomentumPanel;
        }
        
        if (ImGui::IsKeyPressed(ImGuiKey_C) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            m_PhysicsAnalysis.ShowCollisionPanel = !m_PhysicsAnalysis.ShowCollisionPanel;
        }
        
        if (ImGui::IsKeyPressed(ImGuiKey_N) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            m_PhysicsAnalysis.ShowNewtonsLawsPanel = !m_PhysicsAnalysis.ShowNewtonsLawsPanel;
        }
        
        if (ImGui::IsKeyPressed(ImGuiKey_A) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            m_PhysicsAnalysis.ShowAccelerationPanel = !m_PhysicsAnalysis.ShowAccelerationPanel;
        }
        
        if (ImGui::IsKeyPressed(ImGuiKey_T) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            m_PhysicsAnalysis.ShowTrajectoryPanel = !m_PhysicsAnalysis.ShowTrajectoryPanel;
        }

        auto& context = m_Scene->GetContext();
        RenderForceAnalysisPanel(context);
        RenderEnergyTrackingPanel(context);
        RenderMomentumPanel(context);
        RenderCollisionAnalysisPanel(context);
        RenderNewtonsLawsPanel(context);
        RenderAccelerationPanel(context);
        RenderTrajectoryPanel(context);
    }

    /**
     * @brief Handles the scene creation operation.
     *
     * This function is responsible for rendering the scene creation dialog, and handling the scene creation operation.
     *
     * It will open the scene creation dialog if m_SceneCreationRequest.ShowDialog is true, and will reset it after the dialog is closed.
     *
     * It will also start the scene creation operation if the user has entered a valid scene name and path, and will set the scene object to the result of the operation.
     *
     * If the scene creation operation is in progress, it will render a loading overlay.
     * If the scene creation operation has failed, it will render an error modal.
     */
    void SceneEditorLayer::HandleSceneCreation()
    {
        const ImVec4 CARD_BG        = ImVec4(255.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f, 0.98f);
        const ImVec4 CONTROL_BG     = ImVec4(251.0f/255.0f, 251.0f/255.0f, 251.0f/255.0f, 1.0f);
        const ImVec4 HOVER_BG       = ImVec4(246.0f/255.0f, 246.0f/255.0f, 246.0f/255.0f, 1.0f);
        const ImVec4 TEXT_PRIMARY   = ImVec4(32.0f/255.0f, 33.0f/255.0f, 36.0f/255.0f, 1.0f);
        const ImVec4 TEXT_SECONDARY = ImVec4(96.0f/255.0f, 94.0f/255.0f, 92.0f/255.0f, 1.0f);
        const ImVec4 TEXT_DISABLED  = ImVec4(161.0f/255.0f, 159.0f/255.0f, 157.0f/255.0f, 1.0f);
        const ImVec4 BORDER         = ImVec4(229.0f/255.0f, 229.0f/255.0f, 229.0f/255.0f, 0.50f);
        const ImVec4 SUCCESS        = ImVec4(16.0f/255.0f, 137.0f/255.0f, 62.0f/255.0f, 1.0f);
        const ImVec4 WARNING        = ImVec4(255.0f/255.0f, 185.0f/255.0f, 0.0f/255.0f, 1.0f);
        const ImVec4 ERROR_COLOR    = ImVec4(232.0f/255.0f, 17.0f/255.0f, 35.0f/255.0f, 1.0f);
        
        ImVec4 ACCENT = ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
        auto MixColors = [](const ImVec4& a, const ImVec4& b, float t) -> ImVec4 {
            return ImVec4(
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t,
                a.w + (b.w - a.w) * t
            );
        };
        
        const ImVec4 ACCENT_HOVER = MixColors(ACCENT, ImVec4(1, 1, 1, 1), 0.12f);
        const ImVec4 ACCENT_ACTIVE = MixColors(ACCENT, ImVec4(0, 0, 0, 1), 0.15f);

        if (m_SceneCreationRequest.ShowDialog)
        {
            ImGui::OpenPopup("Create New Scene");
            m_SceneCreationRequest.ShowDialog = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 12));
        
        if (ImGui::BeginPopupModal("Create New Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
        {
            std::vector<std::string> errors;
            std::vector<std::string> warnings;
            
            auto trim = [](std::string& s)
            {
                const auto wsfront = s.find_first_not_of(" \t\r\n");
                const auto wsback  = s.find_last_not_of(" \t\r\n");
                if (wsfront == std::string::npos) { s.clear(); return; }
                s = s.substr(wsfront, wsback - wsfront + 1);
            };

            auto has_invalid_win_chars = [](const std::string& s)
            {
    #ifdef MOTION_PLATFORM_WINDOWS
                static const char* bad = "<>:\"/\\|?*";
                return s.find_first_of(bad) != std::string::npos;
    #else
                (void)s;
                return false;
    #endif
            };

            ImGui::PushStyleColor(ImGuiCol_Text, ACCENT);
            ImGui::Text("Create a New Physics Scene");
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
            ImGui::TextWrapped("Set up a new scene to start building and simulating your physics experiments.");
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Scene Name:");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            HelpMarker("Choose a descriptive name for your scene.\n\n"
                    "Good examples:\n"
                    "• 'Projectile Motion Experiment'\n"
                    "• 'Collision Test Scene'\n"
                    "• 'Pendulum Simulation'\n\n"
                    "The name should only contain letters, numbers,\n"
                    "spaces, hyphens, and underscores.");
            
            ImGui::SetNextItemWidth(450.0f);
            std::string name = m_SceneCreationRequest.Name;
            trim(name);
            bool nameHasError = false;
            
            if (name.empty() || has_invalid_win_chars(name))
            {
                nameHasError = true;
                ImVec4 errorBg = MixColors(ERROR_COLOR, CONTROL_BG, 0.90f);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, errorBg);
                ImGui::PushStyleColor(ImGuiCol_Border, MixColors(ERROR_COLOR, BORDER, 0.30f));
            }
            
            ImGui::InputTextWithHint("##scenename", "Enter scene name...", 
                                    m_SceneCreationRequest.Name, 
                                    sizeof(m_SceneCreationRequest.Name));
            
            if (nameHasError)
                ImGui::PopStyleColor(2);
            
            ImGui::SameLine();
            int nameLen = (int)strlen(m_SceneCreationRequest.Name);
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DISABLED);
            ImGui::Text("(%d chars)", nameLen);
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Save Location:");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            HelpMarker("Choose where to save your scene files.\n\n"
                    "The scene folder will contain:\n"
                    "• Scene data file (.mes)\n"
                    "• Assets folder (for textures, models, etc.)\n"
                    "• Temporary files folder\n\n"
                    "Make sure you have write permissions for the selected location.");
            
            std::string pathStr = m_SceneCreationRequest.FilePath.string();
            bool pathHasError = m_SceneCreationRequest.FilePath.empty() || 
                                !std::filesystem::exists(m_SceneCreationRequest.FilePath);
            
            if (pathHasError)
            {
                ImVec4 errorBg = MixColors(ERROR_COLOR, CONTROL_BG, 0.90f);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, errorBg);
                ImGui::PushStyleColor(ImGuiCol_Border, MixColors(ERROR_COLOR, BORDER, 0.30f));
            }
            
            ImGui::SetNextItemWidth(340.0f);
            if (ImGui::InputTextWithHint("##path", "Click Browse to select a folder...", 
                                        &pathStr, ImGuiInputTextFlags_ReadOnly))
            {
                m_SceneCreationRequest.FilePath = std::filesystem::path(pathStr);
            }
            
            if (pathHasError)
                ImGui::PopStyleColor(2);
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ACCENT);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ACCENT_HOVER);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ACCENT_ACTIVE);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
            
            if (ImGui::Button("Browse", ImVec2(120, 0)))
            {
                DialogBoxes::InitializeCOM();
                if (auto folder = DialogBoxes::SelectFolderDialog(L"Select where to save your scene"); !folder.empty())
                {
                    m_SceneCreationRequest.FilePath = folder;
                }
                DialogBoxes::UninitializeCOM();
            }
            ImGui::PopStyleColor(4);

            ImGui::Spacing();
            ImGui::Spacing();

            if (name.empty())
            {
                errors.emplace_back("Scene name is required");
            }
            else 
            {
                if (has_invalid_win_chars(name))
                {
                    errors.emplace_back("Name contains invalid characters: < > : \" / \\ | ? *");
                }

    #ifdef MOTION_PLATFORM_WINDOWS
                if (!name.empty() && (name.back() == ' ' || name.back() == '.'))
                {
                    errors.emplace_back("Name cannot end with a space or period on Windows");
                }
    #endif

                // Check if scene already exists
                if (!m_SceneCreationRequest.FilePath.empty())
                {
                    auto potentialPath = m_SceneCreationRequest.FilePath / name;
                    if (std::filesystem::exists(potentialPath))
                    {
                        warnings.emplace_back("A folder with this name already exists at this location");
                    }
                }
            }

            if (m_SceneCreationRequest.FilePath.empty())
            {
                errors.emplace_back("Please select a save location");
            }
            else if (!std::filesystem::exists(m_SceneCreationRequest.FilePath))
            {
                errors.emplace_back("The selected folder does not exist or is not accessible");
            }
            else if (!std::filesystem::is_directory(m_SceneCreationRequest.FilePath))
            {
                errors.emplace_back("The selected path is not a valid folder");
            }

            // Preview the final path
            if (errors.empty())
            {
                ImGui::Spacing();
                ImVec4 successBg = MixColors(SUCCESS, CARD_BG, 0.93f);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, successBg);
                ImGui::PushStyleColor(ImGuiCol_Border, MixColors(SUCCESS, BORDER, 0.40f));
                ImGui::BeginChild("PathPreview", ImVec2(450, 65), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);
                
                ImGui::PushStyleColor(ImGuiCol_Text, MixColors(SUCCESS, TEXT_PRIMARY, 0.30f));
                ImGui::Text("Scene will be created at:");
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                auto fullPath = m_SceneCreationRequest.FilePath / name;
                ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                ImGui::TextWrapped("%s", fullPath.string().c_str());
                ImGui::PopStyleColor();
                
                ImGui::EndChild();
                ImGui::PopStyleColor(2);
                ImGui::Spacing();
            }

            // Display warnings
            if (!warnings.empty())
            {
                ImGui::Separator();
                ImGui::Spacing();
                
                ImVec4 warningBg = MixColors(WARNING, CARD_BG, 0.95f);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, warningBg);
                ImGui::PushStyleColor(ImGuiCol_Border, MixColors(WARNING, BORDER, 0.40f));
                ImGui::BeginChild("WarningList", ImVec2(450, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);
                
                ImGui::PushStyleColor(ImGuiCol_Text, MixColors(WARNING, TEXT_PRIMARY, 0.20f));
                for (const auto& warning : warnings)
                {
                    ImGui::TextWrapped("%s", warning.c_str());
                }
                ImGui::PopStyleColor();
                
                ImGui::EndChild();
                ImGui::PopStyleColor(2);
                ImGui::Spacing();
            }

            // Display errors
            if (!errors.empty())
            {
                ImGui::Separator();
                ImGui::Spacing();
                
                ImVec4 errorBg = MixColors(ERROR_COLOR, CARD_BG, 0.95f);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, errorBg);
                ImGui::PushStyleColor(ImGuiCol_Border, MixColors(ERROR_COLOR, BORDER, 0.40f));
                ImGui::BeginChild("ErrorList", ImVec2(450, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);
                
                ImGui::PushStyleColor(ImGuiCol_Text, MixColors(ERROR_COLOR, TEXT_PRIMARY, 0.20f));
                ImGui::TextWrapped("Please fix the following issues:");
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                ImGui::Indent(10.0f);
                
                ImGui::PushStyleColor(ImGuiCol_Text, MixColors(ERROR_COLOR, TEXT_PRIMARY, 0.30f));
                for (const auto& error : errors)
                {
                    ImGui::BulletText("%s", error.c_str());
                }
                ImGui::PopStyleColor();
                
                ImGui::Unindent(10.0f);
                ImGui::Spacing();
                
                ImGui::EndChild();
                ImGui::PopStyleColor(2);
                
                ImGui::Spacing();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Action Buttons
            bool canCreate = errors.empty();
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 270) * 0.5f);
            
            if (!canCreate) 
            {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                ImGui::BeginDisabled();
            }
        
            ImVec4 successHover = MixColors(SUCCESS, ImVec4(1, 1, 1, 1), 0.12f);
            ImVec4 successActive = MixColors(SUCCESS, ImVec4(0, 0, 0, 1), 0.15f);
            ImGui::PushStyleColor(ImGuiCol_Button, SUCCESS);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, successHover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, successActive);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1)); 
            
            if (ImGui::Button("Create Scene", ImVec2(140, 35)))
            {
                std::string sceneName = name;
                auto scenePath = m_SceneCreationRequest.FilePath / sceneName;
                m_SceneCreationOp.Start(LOADER::Submit([sceneName, scenePath]() -> std::shared_ptr<Scene>
                {
                    try
                    {
                        SceneSpecification spec;
                        spec.ID = UniqueIdentity::GetUniqueID();
                        spec.Name = sceneName;
                        spec.SavedPath = scenePath;
                        auto scene = std::make_shared<Scene>(spec);
                        
                        if (!scene)
                            throw std::runtime_error("Failed to create scene object");

                        // Create directory structure
                        std::filesystem::create_directories(scenePath);
                        std::filesystem::create_directory(scenePath / "Assets");
                        std::filesystem::create_directory(scenePath / ".motion_temp");
                        
                        return scene;
                    }
                    catch (const std::exception& e)
                    {
                        MOTION_CORE_ERROR("Scene creation error: {}", e.what());
                        throw;
                    }
                }));

                m_ScenePath = scenePath;
                m_SceneName = sceneName;
                m_SceneCreationRequest.Reset();
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::PopStyleColor(4);
            
            if (!canCreate) 
            {
                ImGui::EndDisabled();
                ImGui::PopStyleVar();
            }
            
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !canCreate)
            {
                ImGui::BeginTooltip();
                ImGui::PushStyleColor(ImGuiCol_Text, ERROR_COLOR);
                ImGui::TextWrapped("Please fix all errors before creating the scene");
                ImGui::PopStyleColor();
                ImGui::EndTooltip();
            }
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, CONTROL_BG);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HOVER_BG);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, MixColors(ACCENT, CONTROL_BG, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
            
            if (ImGui::Button("Cancel", ImVec2(120, 35)))
            {
                m_SceneCreationRequest.Reset();
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::PopStyleColor(4);
            ImGui::Spacing();
            ImGui::EndPopup();
        }
        
        ImGui::PopStyleVar(2);
        if (m_SceneCreationOp.IsReady())
        {
            try
            {
                m_Scene = m_SceneCreationOp.GetResult();
                
                if (m_Scene)
                {
                    // Save ImGui layout
                    ImGuiIO& io = ImGui::GetIO();
                    io.IniFilename = nullptr;
                    std::string layoutFile = std::format("{}/mes-config.ini", m_ScenePath.string());
                    ImGui::SaveIniSettingsToDisk(layoutFile.c_str());

                    // Serialize the scene
                    std::string sceneName = m_SceneName;
                    std::filesystem::path savePath = m_ScenePath / std::format("{}.mes", sceneName);
                    
                    try
                    {
                        SceneSerializer::Serialize(m_Scene.get(), savePath);
                        MOTION_CORE_INFO("Scene created successfully: {}", m_ScenePath.string());
                    }
                    catch (const std::exception& e)
                    {
                        MOTION_CORE_ERROR("Failed to serialize scene: {}", e.what());
                        ImGui::OpenPopup("SceneCreationError");
                    }
                }
                else
                {
                    throw std::runtime_error("Scene creation returned null");
                }
            }
            catch (const std::exception& e)
            {
                MOTION_CORE_ERROR("Failed to create scene: {}", e.what());
                ImGui::OpenPopup("SceneCreationError");
            }
        }
        
        // Error Modal
        ImVec2 center2 = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center2, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        
        if (ImGui::BeginPopupModal("SceneCreationError", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ERROR_COLOR);
            ImGui::Text("Scene Creation Failed");
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
            ImGui::TextWrapped("An error occurred while creating the scene.");
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
            ImGui::TextWrapped("Please check the following:");
            ImGui::BulletText("You have write permissions for the selected folder");
            ImGui::BulletText("There is enough disk space available");
            ImGui::BulletText("The folder path is valid and accessible");
            ImGui::BulletText("No other application is blocking the folder");
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 120) * 0.5f);
            
            ImGui::PushStyleColor(ImGuiCol_Button, ACCENT);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ACCENT_HOVER);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ACCENT_ACTIVE);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
            
            if (ImGui::Button("OK", ImVec2(120, 30)))
            {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::PopStyleColor(4);
            
            ImGui::EndPopup();
        }
        
        ImGui::PopStyleVar();
    }

    /**
     * @brief Handles the scene loading process.
     *
     * This function shows an open dialog when the user wants to load a scene, and then loads the scene using the file path returned from the dialog.
     * If the scene loading process fails, it sets the show dialog flag to false and shows an error message.
     */
    void SceneEditorLayer::HandleSceneLoading()
    {
        if (m_SceneLoadRequest.ShowDialog)
        {
            OpenDialogOptions options{};
            options.Title = L"Open Scene";
            options.DefaultExtension = L"mes";
            options.AllowMultiSelect = false;
            options.InitialDirectory = DialogBoxes::GetSystemFolder(SystemFolder::Desktop);
            options.Filters = { {L"Scene File", L"*.mes"} };
            
            DialogBoxes::InitializeCOM();
            
            if (auto path = DialogBoxes::OpenFileDialog(options); !path.empty())
            {
                m_SceneLoadRequest.FilePath = path;       
                if (!std::filesystem::exists(path))
                {
                    DialogBoxes::UninitializeCOM();
                    MOTION_CORE_ERROR("Scene file does not exist: {}", path.string());
                    m_SceneLoadRequest.ShowDialog = false;
                    return;
                }
                
                m_SceneLoadOp.Start(LOADER::Submit([path]() -> std::shared_ptr<Scene>
                {
                    auto scene = SceneSerializer::Deserialize(path);
                    if (!scene)
                        throw std::runtime_error("Failed to deserialize scene file");
                    
                    return scene;
                }));

                m_ScenePath = path.parent_path();
            }
            
            DialogBoxes::UninitializeCOM();
            m_SceneLoadRequest.ShowDialog = false;
        }

        if (m_SceneLoadOp.IsReady())
        {
            try
            {
                m_Scene = m_SceneLoadOp.GetResult();
                
                if (m_Scene)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    io.IniFilename = nullptr;
                    std::string layoutFile = std::format("{}/mes-config.ini", m_SceneLoadRequest.FilePath.string());
                    ImGui::LoadIniSettingsFromDisk(layoutFile.c_str());
                    
                    MOTION_CORE_INFO("{} loaded successfully", m_SceneLoadRequest.FilePath.string());
                }
                else
                {
                    throw std::runtime_error("Scene loading returned null");
                }
            }
            catch (const std::exception& e)
            {
                MOTION_CORE_ERROR("Failed to load scene: {}", e.what());
            }
        }
    }
  
    /**
     * @brief Requests the scene editor layer to import an entity from a file.
     * @param showDialog Whether to show an open dialog to select the file to import.
     * @param shouldExport Whether the imported entity should be exported to a file.
     * @param path The path to the file to import.
     * @return True if the request was successful, false otherwise.
     *
     * If showDialog is true, an open dialog will be shown to select the file to import.
     * If shouldExport is true, the imported entity will be exported to a file at the scene's assets path.
     * If the file does not exist or the import process fails, an error message will be logged and false will be returned.
     */
    bool SceneEditorLayer::RequestEntityImport(bool showDialog, bool shouldExport, std::filesystem::path path)
    {
        if (!m_Scene) 
            return false;

        m_EntityImportRequest.ShowDialog = showDialog;
        m_EntityImportRequest.ShouldExport = shouldExport;
        m_EntityImportRequest.FilePath = path;

        if (showDialog)
        {
            OpenDialogOptions options{};
            options.Title = L"Import Model";
            options.DefaultExtension = L"obj";
            options.AllowMultiSelect = false;
            options.InitialDirectory = std::filesystem::current_path();
            options.Filters = { { L"Mesh Files", L"*.fbx;*.obj;*.gltf;*.glb" } };
            
            DialogBoxes::InitializeCOM();
            
            if (auto filePath = DialogBoxes::OpenFileDialog(options); !filePath.empty())
            {
                m_EntityImportRequest.FilePath = filePath;
            }
            else
            {
                DialogBoxes::UninitializeCOM();
                return false;
            }
            
            DialogBoxes::UninitializeCOM();
        }

        if (!std::filesystem::exists(m_EntityImportRequest.FilePath))
        {
            MOTION_CORE_ERROR("Import file does not exist: {}", m_EntityImportRequest.FilePath.string());
            return false;
        }

        ImportSettings settings{};
        settings.FilePath = m_EntityImportRequest.FilePath;
        settings.ShouldExport = m_EntityImportRequest.ShouldExport;
        settings.ExportPath = m_ScenePath / "Assets";

        m_EntityImportOp.Start(LOADER::Submit([settings]() -> std::shared_ptr<ImportedResults>
        {
            return Importer::ImportEntity(settings);
        }));

        return true;
    }


    /**
     * @brief Handles the result of an entity import operation.
     *
     * This function is called when the entity import operation is complete.
     * It retrieves the result of the import operation and adds the imported entity to the scene.
     * If the import operation fails, an error message is logged.
     */
    void SceneEditorLayer::HandleEntityImport()
    {
        if (m_EntityImportOp.IsReady())
        {
            try
            {
                auto results = m_EntityImportOp.GetResult();
                
                if (results && m_Scene)
                {
                    auto& context = m_Scene->GetContext();
                    
                    const BufferLayout layout
                    {
                        { "a_Position",   BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Position)    },
                        { "a_TexCoords",  BufferComponents::UV,   BufferStride::F2, false, offsetof(Vertex, TexCoord)    },
                        { "a_Normals",    BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Normal)      },
                        { "a_Tangents",   BufferComponents::XYZW, BufferStride::F4, false, offsetof(Vertex, Tangent)     },
                        { "a_Bitangents", BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Bitangent)   },
                    };

                    entt::entity root = context.Entities->Registry.create();
                    context.Entities->Registry.emplace<TagComponent>(root, results->Name);
                    context.Entities->Registry.emplace<TransformComponent>(root);

                    auto& modelCompo     = context.Entities->Registry.emplace<ModelComponent>(root);
                    modelCompo.FilePath  = results->FilePath;
                    modelCompo.MeshCount = results->MeshCount;
                    modelCompo.MaxBounds = results->MAX;
                    modelCompo.MinBounds = results->MIN;

                    std::vector<entt::entity> children;
                    children.reserve(results->MeshCount);

                    for (const auto& [index, mesh] : results->Meshes)
                    {
                        entt::entity e = context.Entities->Registry.create();
                        context.Entities->Registry.emplace<TagComponent>(e, mesh.Name);
                        context.Entities->Registry.emplace<TransformComponent>(e);
                        context.Entities->Registry.emplace<RigidBodyComponent>(e);
                        context.Entities->Registry.emplace<ColliderComponent>(e);

                        auto& meshCompo = context.Entities->Registry.emplace<MeshComponent>(e);
                        meshCompo.MeshPointer = Mesh::Create(mesh.Vertices.data(), mesh.Vertices.size(), 
                                                             mesh.Indices.data(), mesh.Indices.size(), layout);
                        
                        meshCompo.MeshIndex = index;
                        meshCompo.Name      = mesh.Name;
                        meshCompo.MaxBounds = mesh.MAX;
                        meshCompo.MinBounds = mesh.MIN;

                        auto& materialCompo = context.Entities->Registry.emplace<MaterialComponent>(e);
                        materialCompo.MaterialPointer = Material::Create();

                        CreateRigidBody(context.Physics->World, &context.Entities->Registry, e);

                        std::vector<glm::vec3> verts;
                        verts.reserve(mesh.Vertices.size());
                        std::transform(mesh.Vertices.begin(), mesh.Vertices.end(), std::back_inserter(verts),
                                      [](const Vertex& v) { return v.Position; });

                        CreateConvexCollider(&context.Physics->Properties, &context.Entities->Registry, e, verts);
                        children.push_back(e);
                    }

                    entt::entity prev = entt::null;
                    for (std::size_t i = 0; i < children.size(); ++i)
                    {
                        auto e = children[i];
                        context.Entities->Registry.emplace<HierarchyComponent>(e, root, entt::null, entt::null);
                        if (i > 0) context.Entities->Registry.get<HierarchyComponent>(prev).NextSibling = e;
                        prev = e;
                    }

                    context.Entities->Registry.emplace<HierarchyComponent>(root, entt::null,
                        children.empty() ? entt::null : children.front(), entt::null);

                    m_Scene->EmplaceEntity(root);
                    
                    MOTION_CORE_INFO("Entity imported successfully: {}", 
                                    m_EntityImportRequest.FilePath.string());
                }
            }
            catch (const std::exception& e)
            {
                MOTION_CORE_ERROR("Failed to import entity: {}", e.what());
            }
        }
    }

    /**
     * @brief Renders a loading overlay for the scene editor.
     *
     * This function renders a semi-transparent overlay with a loading animation and a text describing the current operation.
     * The overlay is rendered over the entire window, and is meant to be used when the scene editor is performing an operation that
     * takes a significant amount of time, such as creating a scene or importing a model.
     */
    void SceneEditorLayer::RenderLoadingOverlay()
    {
        const ImVec4 OVERLAY_DIM    = ImVec4(0.0f, 0.0f, 0.0f, 0.50f);
        const ImVec4 CARD_BG        = ImVec4(255.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f, 0.98f);
        const ImVec4 TEXT_PRIMARY   = ImVec4(32.0f/255.0f, 33.0f/255.0f, 36.0f/255.0f, 1.0f);
        const ImVec4 TEXT_SECONDARY = ImVec4(96.0f/255.0f, 94.0f/255.0f, 92.0f/255.0f, 1.0f);
        const ImVec4 BORDER         = ImVec4(229.0f/255.0f, 229.0f/255.0f, 229.0f/255.0f, 0.50f);
        
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, OVERLAY_DIM);
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;
        
        if (ImGui::Begin("LoadingOverlay", nullptr, flags))
        {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(50, 45));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
            
            ImGui::PushStyleColor(ImGuiCol_WindowBg, CARD_BG);
            ImGui::PushStyleColor(ImGuiCol_Border, BORDER);
            
            if (ImGui::BeginChild("LoadingContent", ImVec2(520, 320), true, 
                                ImGuiWindowFlags_NoScrollbar))
            {
                const float time = ImGui::GetTime();
                const float contentWidth = 520.0f;
                
                // Determine what operation is in progress
                const char* loadingTitle = "Processing";
                const char* loadingDescription = "Please wait while the operation completes.";
                const char* operationIcon = ICON_MD_HOURGLASS_EMPTY;
                ImVec4 accentColor = ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
                float elapsedTime = 0.0f;
                
                if (m_SceneCreationOp.State == AsyncOperationState::InProgress)
                {
                    loadingTitle = "Creating Your Scene";
                    loadingDescription = "Setting up the scene structure, creating folders, and initializing physics simulation...";
                    operationIcon = ICON_MD_CREATE_NEW_FOLDER;
                    accentColor = ImVec4(16.0f/255.0f, 137.0f/255.0f, 62.0f/255.0f, 1.0f); // SUCCESS color
                    elapsedTime = m_SceneCreationOp.GetElapsedSeconds();
                }
                else if (m_SceneLoadOp.State == AsyncOperationState::InProgress)
                {
                    loadingTitle = "Loading Scene";
                    loadingDescription = "Reading scene data, loading assets, and preparing the physics simulation environment...";
                    operationIcon = ICON_MD_FOLDER_OPEN;
                    elapsedTime = m_SceneLoadOp.GetElapsedSeconds();
                }
                else if (m_EntityImportOp.State == AsyncOperationState::InProgress)
                {
                    loadingTitle = "Importing 3D Model";
                    loadingDescription = "Processing geometry, materials, and textures from the imported model file...";
                    operationIcon = ICON_MD_INVENTORY_2;
                    accentColor = ImVec4(255.0f/255.0f, 185.0f/255.0f, 0.0f/255.0f, 1.0f); // WARNING color
                    elapsedTime = m_EntityImportOp.GetElapsedSeconds();
                }
                
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();
                
                // Animated Loading Spinner
                const float radius = 38.0f;
                const float thickness = 3.5f;
                const ImVec2 spinnerPos = ImVec2(contentWidth * 0.5f, 90.0f);
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 windowPos = ImGui::GetCursorScreenPos();
                
                // Draw outer rotating arc
                const int num_segments = 48;
                const float angle_offset = time * 5.0f;
                const float arc_length = 0.75f; // 75% of circle
                
                for (int i = 0; i < num_segments; i++)
                {
                    float t = (float)i / (float)num_segments;
                    if (t > arc_length) break;
                    
                    const float a_start = (t * 2.0f * 3.14159f) + angle_offset;
                    const float a_end = ((t + 0.02f) * 2.0f * 3.14159f) + angle_offset;
                    
                    // Gradient alpha from bright to dim
                    const float alpha = 0.2f + (0.8f * (1.0f - t / arc_length));
                    
                    ImVec4 segmentColor = accentColor;
                    segmentColor.w = alpha;
                    const ImU32 col = ImGui::GetColorU32(segmentColor);
                    
                    draw_list->PathArcTo(
                        ImVec2(windowPos.x + spinnerPos.x, windowPos.y + spinnerPos.y),
                        radius, a_start, a_end, 6
                    );
                    draw_list->PathStroke(col, 0, thickness);
                }
                
                // Draw inner subtle circle
                ImVec4 innerCircleColor = accentColor;
                innerCircleColor.w = 0.08f;
                draw_list->AddCircleFilled(
                    ImVec2(windowPos.x + spinnerPos.x, windowPos.y + spinnerPos.y),
                    radius - thickness * 2, 
                    ImGui::GetColorU32(innerCircleColor),
                    48
                );
                
                // Draw icon in center
                ImGui::SetCursorPosY(55.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, accentColor);
                float iconWidth = ImGui::CalcTextSize(operationIcon).x;
                ImGui::SetCursorPosX((contentWidth - iconWidth) * 0.5f);
                ImGui::Text("%s", operationIcon);
                ImGui::PopStyleColor();
                
                ImGui::SetCursorPosY(160.0f);
                ImGui::Spacing();
                ImGui::Spacing();
                
                // Title
                ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                float titleWidth = ImGui::CalcTextSize(loadingTitle).x;
                ImGui::SetCursorPosX((contentWidth - titleWidth) * 0.5f);
                ImGui::Text("%s", loadingTitle);
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                
                // Description
                ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 440.0f);
                ImGui::SetCursorPosX(40.0f);
                ImGui::TextWrapped("%s", loadingDescription);
                ImGui::PopTextWrapPos();
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();
                
                // Separator
                ImGui::PushStyleColor(ImGuiCol_Separator, BORDER);
                ImGui::Separator();
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                
                // Animated status indicator
                int numDots = ((int)(time * 2.5f)) % 4;
                char dots[5] = "";
                for (int i = 0; i < numDots; i++)
                    dots[i] = '.';
                dots[numDots] = '\0';
                
                char statusText[64];
                snprintf(statusText, sizeof(statusText), "Processing%s", dots);
                
                ImGui::PushStyleColor(ImGuiCol_Text, accentColor);
                float statusWidth = ImGui::CalcTextSize(statusText).x;
                ImGui::SetCursorPosX((contentWidth - statusWidth) * 0.5f);
                ImGui::Text("%s", statusText);
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                
                // Elapsed time
                char timeBuffer[64];
                if (elapsedTime < 60.0f)
                {
                    snprintf(timeBuffer, sizeof(timeBuffer), "%.1f seconds elapsed", elapsedTime);
                }
                else
                {
                    int minutes = (int)(elapsedTime / 60.0f);
                    int seconds = (int)(elapsedTime) % 60;
                    snprintf(timeBuffer, sizeof(timeBuffer), "%d:%02d elapsed", minutes, seconds);
                }
                
                ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                float timeWidth = ImGui::CalcTextSize(timeBuffer).x;
                ImGui::SetCursorPosX((contentWidth - timeWidth) * 0.5f);
                ImGui::TextDisabled("%s", timeBuffer);
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                
                // Optional: Progress hint
                if (elapsedTime > 5.0f)
                {
                    ImGui::Spacing();
                    const char* hintText = "This is taking longer than usual...";
                    ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                    float hintWidth = ImGui::CalcTextSize(hintText).x;
                    ImGui::SetCursorPosX((contentWidth - hintWidth) * 0.5f);
                    ImGui::TextDisabled("%s", hintText);
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndChild();
            
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(3);
        }
        ImGui::End();
        
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    }

    /**
     * @brief Renders an error modal in the event of an operation failure.
     * 
     * This function will render a popup modal with an error message
     * and a button to close the modal. If the operation that failed
     * is a scene creation operation, loading operation, or entity
     * import operation, the error message will correspond to the
     * correct operation.
     * 
     * @note This function is intended to be called from the main loop of
     * the application, and should not be called from any other thread.
     */
    void SceneEditorLayer::RenderErrorModal()
    {
        ImGui::OpenPopup("Operation Failed");
        
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal("Operation Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::Text("Error");
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            std::string errorMsg;
            if (m_SceneCreationOp.State == AsyncOperationState::Failed) errorMsg = m_SceneCreationOp.ErrorMessage;
            else if (m_SceneLoadOp.State == AsyncOperationState::Failed) errorMsg = m_SceneLoadOp.ErrorMessage;
            else if (m_EntityImportOp.State == AsyncOperationState::Failed) errorMsg = m_EntityImportOp.ErrorMessage;
            
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 400);
            ImGui::TextWrapped("%s", errorMsg.c_str());
            ImGui::PopTextWrapPos();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                if (m_SceneCreationOp.State == AsyncOperationState::Failed)
                    m_SceneCreationOp.Reset();
                if (m_SceneLoadOp.State == AsyncOperationState::Failed)
                    m_SceneLoadOp.Reset();
                if (m_EntityImportOp.State == AsyncOperationState::Failed)
                    m_EntityImportOp.Reset();
                
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }
 
    /**
     * @brief Draws the main menu bar of the scene editor layer
     * 
     * This function draws the main menu bar of the scene editor layer, which
     * includes the following items:
     *  - Files: New Scene, Open...
     *  - Shapes: Cube, Cone, Cylinder, Plane, Sphere, Torus
     *  - Simulation Controls: Play, Pause, Stop
     * 
     * This function should be called from the main loop of the application,
     * and should not be called from any other thread.
     * 
     * @note This function will only render the menu items that are valid
     * given the current state of the scene editor layer. For example, if
     * there is no scene loaded, then the "Save" menu item will not be
     * rendered.
     * 
     * @see SceneEditorLayer::DrawMenuBar
     */
    void SceneEditorLayer::DrawMenuBar()
    {
        if (!ImGui::BeginMenuBar())
            return;

        ImGuiStyle& style = ImGui::GetStyle();
        if (ImGui::BeginMenu("Files"))
        {
            if (ImGui::MenuItem("  New Scene ", "Ctrl+N"))
            {
                m_SceneCreationRequest.ShowDialog = true;
            }

            if (ImGui::MenuItem("  Open... ", "Ctrl+O"))
            {
                m_SceneLoadRequest.ShowDialog = true;
            }

            ImGui::Separator();
            
            ImGui::BeginDisabled(m_Scene == nullptr);
            if (ImGui::MenuItem("  Save ", "Ctrl+S"))
            {
                if (m_Scene && !m_ScenePath.empty())
                {
                    auto scene = m_Scene.get();
                    auto path = m_ScenePath;
                    auto sceneName = m_SceneName;
                    std::string filename = std::format("{}.mes", sceneName);
                    SceneSerializer::Serialize(scene, path / filename);
                }
            }
            ImGui::EndDisabled();

            ImGui::Separator();
            
            if (ImGui::MenuItem("  Quit ", "Alt+F4"))
            {

            }

            ImGui::EndMenu();
        }

        ImGui::BeginDisabled(m_Scene == nullptr);
        if (ImGui::BeginMenu("Shapes"))
        {
            if (ImGui::MenuItem("  Cube"))
                RequestEntityImport(false, true, "Assets/Primitives/Cube.obj");
            if (ImGui::MenuItem("  Cone"))
                RequestEntityImport(false, true, "Assets/Primitives/Cone.obj");
            if (ImGui::MenuItem("  Cylinder"))
                RequestEntityImport(false, true, "Assets/Primitives/Cylinder.obj");
            if (ImGui::MenuItem("  Plane"))
                RequestEntityImport(false, true, "Assets/Primitives/Plane.obj");
            if (ImGui::MenuItem("  Sphere"))
                RequestEntityImport(false, true, "Assets/Primitives/Sphere.obj");
            if (ImGui::MenuItem("  Torus"))
                RequestEntityImport(false, true, "Assets/Primitives/Torus.obj");
            ImGui::EndMenu();
        }
        ImGui::EndDisabled();

        if (ImGui::BeginMenu("Physics Analysis"))
        {
            ImGui::MenuItem(ICON_MD_ARCHITECTURE " Force Analysis", "Ctrl+F", 
                          &m_PhysicsAnalysis.ShowForceAnalysisPanel);
            
            ImGui::MenuItem(ICON_MD_BOLT " Energy Tracking", "Ctrl+E", 
                          &m_PhysicsAnalysis.ShowEnergyPanel);
            
            ImGui::MenuItem(ICON_MD_SPEED " Momentum Analysis", "Ctrl+M", 
                          &m_PhysicsAnalysis.ShowMomentumPanel);
            
            ImGui::MenuItem(ICON_MD_ADJUST " Collision Analysis", "Ctrl+C", 
                          &m_PhysicsAnalysis.ShowCollisionPanel);
            
            ImGui::Separator();
            
            ImGui::MenuItem(ICON_MD_SCHOOL " Newton's Laws Demo", "Ctrl+N", 
                          &m_PhysicsAnalysis.ShowNewtonsLawsPanel);
            
            ImGui::MenuItem(ICON_MD_TRENDING_UP " Acceleration", "Ctrl+A", 
                          &m_PhysicsAnalysis.ShowAccelerationPanel);
            
            ImGui::MenuItem(ICON_MD_SHOW_CHART " Trajectory Prediction", "Ctrl+T", 
                          &m_PhysicsAnalysis.ShowTrajectoryPanel);
            
            ImGui::EndMenu();
        }

        const float pad_x = style.ItemSpacing.x;
        const float content_min_x = ImGui::GetWindowContentRegionMin().x;
        const float content_max_x = ImGui::GetWindowContentRegionMax().x;
        const float bar_w = content_max_x - content_min_x;
        const float cur_x = ImGui::GetCursorPosX();

        const char* kPlay  = ICON_MD_PLAY_ARROW;
        const char* kPause = ICON_MD_PAUSE;
        const char* kStop  = ICON_MD_STOP;

        const float button_h = ImGui::GetFrameHeight();
        const float pad_w   = style.FramePadding.x * 2.0f;
        const float w_play  = ImGui::CalcTextSize(kPlay).x   + pad_w;
        const float w_pause = ImGui::CalcTextSize(kPause).x  + pad_w;
        const float w_stop  = ImGui::CalcTextSize(kStop).x   + pad_w;

        const float min_w   = 72.0f;
        const float button_w = ImMax(min_w, ImMax(w_play, ImMax(w_pause, w_stop)));
        const ImVec2 btnSz(button_w, button_h);

        const float label_w       = ImGui::CalcTextSize("Simulation:").x + pad_x * 0.5f;
        const float state_w       = ImGui::CalcTextSize("RUNNING").x;
        const float sim_buttons_w = (btnSz.x * 3.0f) + (pad_x * 2.0f);
        const float center_w      = sim_buttons_w + pad_x * 1.5f + label_w + state_w;

        float desired_center_x = content_min_x + (bar_w - center_w) * 0.5f;
        desired_center_x = ImClamp(desired_center_x, cur_x + pad_x, content_max_x - center_w);

        ImGui::SameLine(0, 0);
        ImGui::SetCursorPosX(desired_center_x);

        SceneSimulation::SimulationState simState = SceneSimulation::SimulationState::IDLE;
        auto* sim = (m_Scene ? m_Scene->GetContext().Simulation : nullptr);
        if (sim) simState = sim->State;

        bool canPlay  = (sim != nullptr) && (simState == SceneSimulation::SimulationState::IDLE || 
                                              simState == SceneSimulation::SimulationState::PAUSED);
        bool canPause = (sim != nullptr) && (simState == SceneSimulation::SimulationState::RUNNING);
        bool canStop  = (sim != nullptr) && (simState == SceneSimulation::SimulationState::RUNNING || 
                                              simState == SceneSimulation::SimulationState::PAUSED);

        ImGui::PushID("transport");

        ImGui::BeginDisabled(!canPlay);
        if (ImGui::Button(kPlay, btnSz) && sim && canPlay)
        {
            SceneSerializer::SerializeRuntime(m_Scene.get(), m_ScenePath / ".motion_temp"/ "scene_sim.mes");
            sim->State = SceneSimulation::SimulationState::RUNNING;
            sim->InSimulation = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(canPlay ? "Play / Resume" : "Play (disabled)");
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!canPause);
        if (ImGui::Button(kPause, btnSz) && sim && canPause)
        {
            sim->State = SceneSimulation::SimulationState::PAUSED;
            sim->InSimulation = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(canPause ? "Pause" : "Pause (disabled)");
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!canStop);
        if (ImGui::Button(kStop, btnSz) && sim && canStop)
        {
            sim->InSimulation = false;
            sim->State = SceneSimulation::SimulationState::IDLE;
            SceneSerializer::DeserializeRuntime(m_Scene.get(), m_ScenePath / ".motion_temp" / "scene_sim.mes");
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(canStop ? "Stop" : "Stop (disabled)");
        ImGui::EndDisabled();

        ImGui::PopID();

        ImGui::SameLine(0.0f, style.ItemSpacing.x * 1.5f);
        ImGui::TextUnformatted("Simulation:");
        ImGui::SameLine();

        switch (simState)
        {
            case SceneSimulation::SimulationState::IDLE:
                ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.65f, 1.0f), "IDLE");
                break;
            case SceneSimulation::SimulationState::PAUSED:
                ImGui::TextColored(ImVec4(1.00f, 0.78f, 0.10f, 1.0f), "PAUSED");
                break;
            case SceneSimulation::SimulationState::RUNNING:
                ImGui::TextColored(ImVec4(0.10f, 0.95f, 0.40f, 1.0f), "RUNNING");
                break;
            default:
                ImGui::TextUnformatted("UNKNOWN");
                break;
        }

        ImGui::EndMenuBar();
    }

    /**
     * @brief Builds the dockspace for the scene editor layer.
     * @details This function sets up the dockspace for the scene editor layer, which includes the scene viewport, scene properties, and other widgets.
     * @note This function is only called once, when the scene editor layer is first initialized.
     */
    void SceneEditorLayer::BuildDockspace()
    {
        ImGuiWindowFlags host =
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_MenuBar;

        ImGuiDockNodeFlags dock = ImGuiDockNodeFlags_PassthruCentralNode
                                | ImGuiDockNodeFlags_AutoHideTabBar;

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});

        if (ImGui::Begin("##DockHost", nullptr, host))
            DrawMenuBar();
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MainDockspace");

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
            ImGui::DockSpace(dockspace_id, ImVec2(0,0), dock);
            ImGui::PopStyleColor();

            static bool first_time = true;        
            if (first_time)
            {
                first_time = false;
                ImGui::DockBuilderRemoveNode(dockspace_id);
                ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, vp->WorkSize);

                ImGuiID dock_main_id  = dockspace_id;
                ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.30f, nullptr, &dock_main_id);
                ImGuiID dock_rbottom_id = ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.30f, nullptr, &dock_right_id);
                //ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.30f, nullptr, &dock_main_id);

                ImGui::DockBuilderDockWindow("Scene Viewport",   dock_main_id);
                ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_right_id);
                ImGui::DockBuilderDockWindow("Physics Simulation Watchlist", dock_right_id);
                ImGui::DockBuilderDockWindow("Statistical Analysis", dock_right_id);

                ImGui::DockBuilderDockWindow("Environment Settings", dock_rbottom_id);
                ImGui::DockBuilderDockWindow("Entity Properties", dock_rbottom_id);

                //ImGui::DockBuilderDockWindow("Console", dock_bottom_id);
                ImGui::DockBuilderFinish(dockspace_id);
            }
        }

        ImGui::End();
    }


    /**
     * @brief Render the scene properties panel and viewport.
     * 
     * This function renders the scene properties panel, which includes the toolbar and search bar, entity hierarchy, and environment settings.
     * It also renders the viewport, which shows the scene as it is being edited.
     * 
     * @note This function will return early if the scene is not valid.
     */
    void SceneEditorLayer::RenderScene()
    {
        if(!m_Scene) return;

        auto& context = m_Scene->GetContext();
        ImGui::Begin("Scene Hierarchy");
        {
            RenderToolbarAndSearch();
            RenderEntityHierarchy(context);
        }
        ImGui::End();
        
        RenderViewport(context);
        RenderSimulationWatchList(context);
        RenderStatisticalAnalysisPanel(context);
        RenderEnvironmentSettings(context);
    }

    /**
     * @brief Renders the material properties panel and texture slots.
     * 
     * This function renders a window that displays the properties of a material, including its attributes and texture slots.
     * The material properties panel is a collapsible tree node that displays the material's attributes, such as its name, diffuse color, metallic factor, etc.
     * The texture slots panel is also a collapsible tree node that displays the material's texture slots, such as its diffuse texture, normal map, etc.
     * 
     * @param mat The material to render the properties for.
     * @return None.
     */
    void SceneEditorLayer::DrawMaterialUI(std::shared_ptr<Material>& mat)
    {
        if (!mat) return;

        if (ImGui::CollapsingHeader("What are Materials?", ImGuiTreeNodeFlags_None))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
            ImGui::TextWrapped(
                "Materials control how objects look when light hits them. Think of it like "
                "the 'skin' of your object - determining if it looks shiny like metal, rough "
                "like stone, or translucent like plastic."
            );
            ImGui::Spacing();
            
            ImGui::TextWrapped("Key Concepts:");
            ImGui::BulletText("Base Color: The main color of the object");
            ImGui::BulletText("Metallic: How metal-like it looks (0=plastic/wood, 1=pure metal)");
            ImGui::BulletText("Roughness: How shiny or matte (0=mirror, 1=rough surface)");
            ImGui::BulletText("Normal Maps: Add surface detail without extra geometry");
            ImGui::BulletText("Emissive: Makes objects glow (like LEDs or screens)");
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        if (ImGui::TreeNodeEx("Material Properties", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawAttributes(mat);
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Texture Slots", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawTexturesSlots(mat);
            ImGui::TreePop();
        }
    }

    /**
     * @brief Draws the material properties panel and texture slots.
     * 
     * This function renders a window that displays the properties of a material, including its attributes and texture slots.
     * The material properties panel is a collapsible tree node that displays the material's attributes, such as its name, diffuse color, metallic factor, etc.
     * The texture slots panel is also a collapsible tree node that displays the material's texture slots, such as its diffuse texture, normal map, etc.
     * 
     * @param mat The material to render the properties for.
     * @return None.
     */
    void SceneEditorLayer::DrawAttributes(std::shared_ptr<Material>& mat)
    {
        if (BeginPropertyGrid("##base-material"))
        {
            auto base = mat->GetBaseMaterial();
            std::vector<std::string> names;
            names.reserve(m_BaseMaterial.size() + 1);
            names.push_back("None");
            std::ranges::transform(m_BaseMaterial, std::back_inserter(names),
                [](const auto& m) { return m->Name; });

            std::int32_t index = 0;
            if (base)
            {
                if (auto it = std::ranges::find(names, base->Name); it != names.end())
                    index = static_cast<std::int32_t>(std::distance(names.begin(), it));
            }

            ComboBox("Base Material", names, index,
                [&](std::int32_t, const std::string& selectedName)
                {
                    if (selectedName == "None") { mat->SetBaseMaterial(nullptr); return; }
                    if (auto it = std::ranges::find_if(m_BaseMaterial, [&](const auto& m){return m->Name == selectedName;}); it != m_BaseMaterial.end())
                    {
                        mat->SetBaseMaterial(*it);
                    }
                });
            HelpMarker("Base materials are presets that give you a starting point for common materials like Metal, Wood, Plastic, etc.");

            EndPropertyGrid();
        }

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

        if (mat->Has<CoreMaterialComponents>())
        {
            auto& C = mat->Get<CoreMaterialComponents>();
            if (BeginPropertyGrid("##core-pbr"))
            {
                ColorEdit4("Base Color", C.BaseColorFactor);
                HelpMarker("The main color of your object. Think of it as painting the object.");
                
                SliderFloat("Metallic", &C.MetallicFactor, 0.0f, 1.0f, "%.3f");
                HelpMarker("0 = Non-metal (plastic, wood, fabric)\n1 = Pure metal (gold, steel, chrome)\n"
                          "Metals reflect environment strongly, non-metals don't.");
                
                SliderFloat("Roughness", &C.RoughnessFactor, 0.0f, 1.0f, "%.3f");
                HelpMarker("0 = Mirror-smooth (polished, shiny)\n1 = Very rough (matte, diffuse)\n"
                          "Controls how blurry reflections are.");
                
                SliderFloat("Normal Strength", &C.NormalScale, 0.0f, 1.0f, "%.3f");
                HelpMarker("Controls how pronounced surface details from the normal map appear.\n"
                          "Higher = more bumpy/detailed, Lower = smoother");
                
                SliderFloat("Ambient Occlusion", &C.OcclusionStrength, 0.0f, 1.0f, "%.3f");
                HelpMarker("Darkens crevices and corners where light doesn't reach easily.\n"
                          "Makes surfaces look more realistic with subtle shadows.");
                
                ColorEdit3("Emissive Color", C.EmissiveFactor);
                HelpMarker("Color of light the object emits. Makes objects 'glow' without affecting other objects.");
                
                SliderFloat("Emissive Strength", &C.EmissiveStrength, 0.0f, 1.0f, "%.3f");
                HelpMarker("How bright the emissive glow is. 0 = off, 1 = full brightness");
                
                SliderFloat("Opacity", &C.OpacityFactor, 0.0f, 1.0f, "%.3f");
                HelpMarker("0 = Fully transparent (invisible)\n1 = Fully opaque (solid)\n"
                          "Values between make the object see-through.");
                
                EndPropertyGrid();
            }
        }
        else
        {
            ImGui::TextDisabled("No materials assigned");
        }
    }

    /**
     * @brief Draw a table of texture slots for the given material.
     * @param mat The material to draw the texture slots for.
     * @details This function will draw a table of texture slots for the given material. The table will have one row per texture slot, and each row will have one column per texture slot. The columns will be labeled "Base Color", "Metallic", "Roughness", "Normal", "Occlusion", and "Emissive". If the material does not have any texture slots, this function will draw a disabled text that says "No textures assigned".
     */
    void SceneEditorLayer::DrawTexturesSlots(std::shared_ptr<Material>& mat)
    {
        if (mat->Has<CoreMaterialComponents>())
        {
            auto& C = mat->Get<CoreMaterialComponents>();

            // Texture explanation
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.6f, 1.0f));
            ImGui::TextWrapped("Tip: Textures add visual detail. Click slots to load images. Right-click for more options.");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            struct Row { const char* Label; std::shared_ptr<ITexture>& Tex; TextureType Type; const char* Help; };
            std::vector<Row> textures =
            {
                {"Base Color",  C.BaseColorTexture, TextureType::BaseColorTexture, 
                 "RGB image that defines the object's color pattern"},
                {"Metallic",    C.MetallicTexture,  TextureType::MetallicTexture, 
                 "Grayscale: White=metal, Black=non-metal"},
                {"Roughness",   C.RoughnessTexture, TextureType::RoughnessTexture, 
                 "Grayscale: White=rough, Black=smooth"},
                {"Normal",      C.NormalTexture,    TextureType::NormalTexture, 
                 "RGB map that fakes surface bumps and details"},
                {"Occlusion",   C.OcclusionTexture, TextureType::AmbientOcclusionTexture, 
                 "Grayscale: Darker areas receive less ambient light"},
                {"Emissive",    C.EmissiveTexture,  TextureType::EmissiveTexture, 
                 "RGB: Defines which parts of the object glow"},
            };

            const int columns = 3;
            ImGui::BeginTable("##texture-grid", columns, ImGuiTableFlags_NoBordersInBody);
            for (size_t i = 0; i < textures.size(); ++i)
            {
                if (i % columns == 0) ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                ImGui::BeginGroup();
                TextureSlot(textures[i].Label, textures[i].Tex, textures[i].Type);
                
                // Show help text below each slot
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 150.0f);
                ImGui::Text("%s", textures[i].Help);
                ImGui::PopTextWrapPos();
                ImGui::PopStyleColor();
                ImGui::EndGroup();
            }
            ImGui::EndTable();
        }
        else
        {
            ImGui::TextDisabled("No textures assigned");
        }
    }

    static std::unordered_map<entt::entity, EntityPlotData> s_EntityPlotData;
    static float s_PlotHistory = 25.0f;
    static bool s_PauseRecording = false;

    /**
     * Draws a watchlist for a simulation, allowing the user to view and
     * manipulate the rigidbodies in the simulation.
     *
     * This function is only called if there is a valid scene and the scene is
     * currently in simulation mode.
     *
     * @param context The scene context.
     */
    void SceneEditorLayer::RenderSimulationWatchList(SceneContext& context)
    {
        const ImVec4 CARD_BG = ImVec4(255.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f, 0.90f);
        const ImVec4 CONTROL_BG = ImVec4(251.0f/255.0f, 251.0f/255.0f, 251.0f/255.0f, 1.0f);
        const ImVec4 HOVER_BG = ImVec4(246.0f/255.0f, 246.0f/255.0f, 246.0f/255.0f, 1.0f);
        const ImVec4 ACTIVE_BG = ImVec4(243.0f/255.0f, 243.0f/255.0f, 243.0f/255.0f, 1.0f);
        const ImVec4 TEXT_PRIMARY = ImVec4(32.0f/255.0f, 33.0f/255.0f, 36.0f/255.0f, 1.0f);
        const ImVec4 TEXT_SECONDARY = ImVec4(96.0f/255.0f, 94.0f/255.0f, 92.0f/255.0f, 1.0f);
        const ImVec4 TEXT_DISABLED = ImVec4(161.0f/255.0f, 159.0f/255.0f, 157.0f/255.0f, 1.0f);
        const ImVec4 BORDER = ImVec4(229.0f/255.0f, 229.0f/255.0f, 229.0f/255.0f, 0.50f);
        const ImVec4 SUCCESS = ImVec4(16.0f/255.0f, 137.0f/255.0f, 62.0f/255.0f, 1.0f);
        const ImVec4 WARNING = ImVec4(255.0f/255.0f, 185.0f/255.0f, 0.0f/255.0f, 1.0f);
        const ImVec4 ERROR_COLOR = ImVec4(232.0f/255.0f, 17.0f/255.0f, 35.0f/255.0f, 1.0f);
        
        ImVec4 ACCENT = ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
        auto MixColors = [](const ImVec4& a, const ImVec4& b, float t) -> ImVec4 {
            return ImVec4(
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t,
                a.w + (b.w - a.w) * t
            );
        };
        
        const ImVec4 ACCENT_HOVER = MixColors(ACCENT, ImVec4(1, 1, 1, 1), 0.12f);
        const ImVec4 ACCENT_ACTIVE = MixColors(ACCENT, ImVec4(0, 0, 0, 1), 0.15f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

        ImGui::PushStyleColor(ImGuiCol_Header, HOVER_BG);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, MixColors(ACCENT, HOVER_BG, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, MixColors(ACCENT, ACTIVE_BG, 0.85f));

        ImGui::SetNextWindowSize(ImVec2(950, 820), ImGuiCond_FirstUseEver);
        ImGui::Begin("Physics Simulation Watchlist", nullptr, ImGuiWindowFlags_MenuBar);

        if (!m_Scene || !context.Simulation->InSimulation || m_SimulationWatchList.empty())
        {
            ImGui::TextDisabled("Waiting for Simulation to Start!");
            ImGui::TextDisabled("Make sure you're simulation watch list is not empty!");
            ImGui::End();
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);
            return;
        }
        
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Options"))
            {
                if (ImGui::MenuItem("Clear All Graph Data"))
                {
                    s_EntityPlotData.clear();
                }
                ImGui::MenuItem("Pause Data Recording", nullptr, &s_PauseRecording);
                ImGui::Separator();
                if (ImGui::MenuItem("Show Help"))
                {
                    ImGui::OpenPopup("WatchlistHelp");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        
        if (ImGui::BeginPopupModal("WatchlistHelp", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ACCENT);
            ImGui::Text("Welcome to the Physics Simulation Watchlist!");
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
            ImGui::TextWrapped("This tool helps you monitor and analyze physics objects in real-time:");
            ImGui::PopStyleColor();
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
            ImGui::BulletText("Track Position & Movement: See where objects are and how they're moving");
            ImGui::BulletText("Monitor Speed: View velocity in both m/s and km/h for easy understanding");
            ImGui::BulletText("Analyze Rotation: Watch how fast objects spin (shown in RPM like a car engine)");
            ImGui::BulletText("Apply Forces: Push, pull, or spin objects to test physics behavior");
            ImGui::BulletText("Create Graphs: Visualize motion over time and export as images");
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImVec4 successBg = MixColors(SUCCESS, CARD_BG, 0.93f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, successBg);
            ImGui::PushStyleColor(ImGuiCol_Border, MixColors(SUCCESS, BORDER, 0.40f));
            ImGui::BeginChild("HelpTip", ImVec2(0, 50), true);
            
            ImGui::PushStyleColor(ImGuiCol_Text, MixColors(SUCCESS, TEXT_PRIMARY, 0.30f));
            ImGui::TextWrapped("Getting Started: Right-click any object in your scene and select 'Add to Watchlist' to begin monitoring it.");
            ImGui::PopStyleColor();
            
            ImGui::EndChild();
            ImGui::PopStyleColor(2);
            
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_Button, ACCENT);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ACCENT_HOVER);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ACCENT_ACTIVE);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
            
            if (ImGui::Button("Got it!", ImVec2(150, 0)))
                ImGui::CloseCurrentPopup();
            
            ImGui::PopStyleColor(4);
            ImGui::EndPopup();
        }
        
        ImGui::PushStyleColor(ImGuiCol_Header, CONTROL_BG);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HOVER_BG);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ACTIVE_BG);
        
        if (ImGui::CollapsingHeader("Global Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PopStyleColor(3);
            ImGui::Indent(15.0f);
            
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, MixColors(WARNING, TEXT_PRIMARY, 0.30f));
            ImGui::TextWrapped("Configure how the watchlist displays and records data for all objects:");
            ImGui::PopStyleColor();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
            ImGui::Text("Graph Time Window:");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::SetNextItemWidth(250.0f);
            ImGui::SliderFloat("##PlotHistory", &s_PlotHistory, 1.0f, 60.0f, "%.1f seconds");
            ImGui::SameLine();
            HelpMarker("Controls how much historical data to show in graphs.\n\n"
                    "• Short window (1-10s): See recent detail\n"
                    "• Long window (30-60s): See overall patterns\n\n"
                    "Note: Longer windows may slow down performance slightly.");
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::AlignTextToFramePadding();
            ImGui::Checkbox("Freeze Data Recording", &s_PauseRecording);
            if(context.Simulation->State == SceneSimulation::SimulationState::PAUSED)
                s_PauseRecording = (s_PauseRecording ? false : true);
            ImGui::SameLine();
            HelpMarker("Pause recording to freeze all graphs at the current moment.\n\n"
                    "Useful for:\n"
                    "• Analyzing specific events (like collisions)\n"
                    "• Taking screenshots of interesting patterns\n"
                    "• Comparing before/after measurements\n\n"
                    "Click again to resume recording.");
            
            ImGui::Spacing();
            ImGui::Unindent(15.0f);
        }
        else
        {
            ImGui::PopStyleColor(3);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();

        for (auto& e : m_SimulationWatchList)
        {
            auto* tag = context.Entities->Registry.try_get<TagComponent>(e);
            if (!tag) continue;
            auto& plotData = s_EntityPlotData[e];

            ImGui::PushID(static_cast<std::int32_t>(entt::to_integral(e)));

            ImVec4 entityHeaderBg = MixColors(ACCENT, CONTROL_BG, 0.92f);
            ImVec4 entityHeaderHover = MixColors(ACCENT, HOVER_BG, 0.85f);
            ImVec4 entityHeaderActive = MixColors(ACCENT, ACTIVE_BG, 0.80f);
            
            ImGui::PushStyleColor(ImGuiCol_Header, entityHeaderBg);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, entityHeaderHover);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, entityHeaderActive);
            
            bool nodeOpen = ImGui::CollapsingHeader(tag->Tag.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            ImGui::PopStyleColor(3);

            if (nodeOpen)
            {
                ImGui::Indent(20.0f);
                ImGui::Spacing();
                
                // Transform Section
                if (auto* tr = context.Entities->Registry.try_get<TransformComponent>(e))
                {
                    ImGui::PushStyleColor(ImGuiCol_Header, CONTROL_BG);
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HOVER_BG);
                    
                    if (ImGui::TreeNodeEx("Position & Orientation", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PopStyleColor(2);
                        ImGui::Spacing();
                        
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                        ImGui::TextWrapped("Where the object is located and how it's rotated in 3D space:");
                        ImGui::PopStyleColor();
                        
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        
                        ImGui::Columns(2, "transform_cols", false);
                        ImGui::SetColumnWidth(0, 180);
                        
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Location (XYZ):"); 
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("Position in meters from the world origin (0,0,0).\n\n"
                                "• X: Left(-) / Right(+)\n"
                                "• Y: Down(-) / Up(+)\n"
                                "• Z: Back(-) / Forward(+)");
                        ImGui::NextColumn();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                        ImGui::Text("(%.3f, %.3f, %.3f) meters", tr->Translation.x, tr->Translation.y, tr->Translation.z);
                        ImGui::PopStyleColor();
                        ImGui::NextColumn();
                        
                        ImGui::Spacing();
                        
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Rotation (Angles):"); 
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("How much the object is rotated around each axis.\n\n"
                                "Measured in degrees (0° to 360°).\n"
                                "• 0°: No rotation\n"
                                "• 90°: Quarter turn\n"
                                "• 180°: Half turn\n"
                                "• 360°: Full turn");
                        ImGui::NextColumn();
                        glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(tr->Rotation));
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                        ImGui::Text("X: %.1f°  Y: %.1f°  Z: %.1f°", eulerDeg.x, eulerDeg.y, eulerDeg.z);
                        ImGui::PopStyleColor();
                        ImGui::NextColumn();
                        
                        ImGui::Spacing();
                        
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Size (Scale):"); 
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("How much the object is stretched or shrunk.\n\n"
                                "• 1.0: Normal size\n"
                                "• 2.0: Twice as big\n"
                                "• 0.5: Half the size");
                        ImGui::NextColumn();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                        ImGui::Text("(%.3f, %.3f, %.3f)", tr->Scale.x, tr->Scale.y, tr->Scale.z);
                        ImGui::PopStyleColor();
                        
                        ImGui::Columns(1);
                        ImGui::Spacing();
                        ImGui::TreePop();
                    }
                    else
                    {
                        ImGui::PopStyleColor(2);
                    }
                    ImGui::Spacing();
                }

                // Rigidbody Section
                if (auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(e))
                {
                    if (!rb->PhysicsBody) 
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ERROR_COLOR);
                        ImGui::TextWrapped("Physics Error: This object doesn't have a valid physics body!");
                        ImGui::PopStyleColor();
                        ImGui::Unindent(20.0f);
                        ImGui::PopID();
                        continue;
                    }

                    // Velocity Data Section
                    ImGui::PushStyleColor(ImGuiCol_Header, CONTROL_BG);
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HOVER_BG);
                    
                    if (ImGui::TreeNodeEx("Motion Data (Speed & Rotation)", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PopStyleColor(2);
                        ImGui::Spacing();
                        
                        glm::vec3 velocity = ToVec3(rb->PhysicsBody->getLinearVelocity());
                        glm::vec3 angularVelocity = ToVec3(rb->PhysicsBody->getAngularVelocity());
                        float speed = glm::length(velocity);
                        float speedKmh = PhysicsUI::MsToKmh(speed);
                        float angSpeed = glm::length(angularVelocity);
                        float rpm = PhysicsUI::RadPerSecToRPM(angSpeed);

                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                        ImGui::TextWrapped("How fast the object is moving and spinning:");
                        ImGui::PopStyleColor();
                        
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        
                        ImGui::Columns(2, "velocity_cols", false);
                        ImGui::SetColumnWidth(0, 200);
                        
                        // Linear Motion Section
                        ImVec4 linearColor = MixColors(ACCENT, TEXT_PRIMARY, 0.30f);
                        ImGui::PushStyleColor(ImGuiCol_Text, linearColor);
                        ImGui::Text("LINEAR MOTION");
                        ImGui::PopStyleColor();
                        ImGui::NextColumn();
                        ImGui::NextColumn();
                        
                        ImGui::Spacing();
                        
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Velocity Vector:"); 
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("The object's velocity broken down by direction.\n\n"
                                "This shows how much the object is moving\n"
                                "in each direction (X, Y, Z) in meters per second.");
                        ImGui::NextColumn();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                        ImGui::Text("(%.3f, %.3f, %.3f) m/s", velocity.x, velocity.y, velocity.z);
                        ImGui::PopStyleColor();
                        ImGui::NextColumn();
                        
                        ImGui::Spacing();
                        
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Overall Speed:"); 
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("How fast the object is traveling overall.\n\n"
                                "Examples of speeds:\n"
                                "• Walking: ~5 km/h (1.4 m/s)\n"
                                "• Running: ~15 km/h (4.2 m/s)\n"
                                "• Car in city: ~50 km/h (14 m/s)\n"
                                "• Highway speed: ~100 km/h (28 m/s)");
                        ImGui::NextColumn();
                        ImGui::PushStyleColor(ImGuiCol_Text, SUCCESS);
                        ImGui::Text("%.3f m/s  ≈  %.1f km/h", speed, speedKmh);
                        ImGui::PopStyleColor();
                        ImGui::NextColumn();
                        
                        ImGui::Spacing();
                        
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Direction of Travel:"); 
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("Which way the object is moving.\n\n"
                                "This is a normalized direction vector\n"
                                "showing the movement direction.");
                        ImGui::NextColumn();
                        if (speed > 0.001f)
                        {
                            glm::vec3 dir = PhysicsUI::GetDirection(velocity);
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                            ImGui::Text("(%.2f, %.2f, %.2f)", dir.x, dir.y, dir.z);
                            ImGui::PopStyleColor();
                        }
                        else
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DISABLED);
                            ImGui::Text("Object is stationary");
                            ImGui::PopStyleColor();
                        }
                        ImGui::NextColumn();
                        
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        
                        // Angular Motion Section
                        ImVec4 angularColor = MixColors(WARNING, TEXT_PRIMARY, 0.30f);
                        ImGui::PushStyleColor(ImGuiCol_Text, angularColor);
                        ImGui::Text("ROTATIONAL MOTION");
                        ImGui::PopStyleColor();
                        ImGui::NextColumn();
                        ImGui::NextColumn();
                        
                        ImGui::Spacing();
                        
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Angular Velocity:"); 
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("How fast the object is rotating around each axis.\n\n"
                                "Measured in radians per second (rad/s).\n"
                                "This shows the spin speed in each direction.");
                        ImGui::NextColumn();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                        ImGui::Text("(%.3f, %.3f, %.3f) rad/s", angularVelocity.x, angularVelocity.y, angularVelocity.z);
                        ImGui::PopStyleColor();
                        ImGui::NextColumn();
                        
                        ImGui::Spacing();
                        
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Spin Rate:"); 
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("How fast the object is spinning overall.\n\n"
                                "RPM = Revolutions Per Minute (like a car engine)\n\n"
                                "Examples:\n"
                                "• Slow ceiling fan: ~60 RPM\n"
                                "• Fast ceiling fan: ~300 RPM\n"
                                "• Car engine idle: ~800 RPM\n"
                                "• Car engine cruising: ~2000-3000 RPM");
                        ImGui::NextColumn();
                        ImGui::PushStyleColor(ImGuiCol_Text, WARNING);
                        ImGui::Text("%.3f rad/s  ≈  %.0f RPM", angSpeed, std::abs(rpm));
                        ImGui::PopStyleColor();
                        ImGui::NextColumn();
                        
                        ImGui::Spacing();
                        
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Axis of Rotation:"); 
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("The axis the object is spinning around.\n\n"
                                "Think of this like the axle of a wheel -\n"
                                "it shows which line the object rotates around.");
                        ImGui::NextColumn();
                        if (angSpeed > 0.001f)
                        {
                            glm::vec3 axis = PhysicsUI::GetDirection(angularVelocity);
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                            ImGui::Text("(%.2f, %.2f, %.2f)", axis.x, axis.y, axis.z);
                            ImGui::PopStyleColor();
                        }
                        else
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DISABLED);
                            ImGui::Text("Object is not rotating");
                            ImGui::PopStyleColor();
                        }
                        
                        ImGui::Columns(1);
                        ImGui::Spacing();
                        ImGui::TreePop();
                    }
                    else
                    {
                        ImGui::PopStyleColor(2);
                    }
                    ImGui::Spacing();

                    // Velocity Graphs Section
                    ImGui::PushStyleColor(ImGuiCol_Header, CONTROL_BG);
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HOVER_BG);

                    if (ImGui::TreeNodeEx("Motion Graphs (Visual Analysis)", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PopStyleColor(2);
                        ImGui::Spacing();
                        
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                        ImGui::TextWrapped("Visualize how the object's motion changes over time with interactive graphs:");
                        ImGui::PopStyleColor();
                        
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        
                        // Recording logic
                        if (!s_PauseRecording || context.Simulation->State != SceneSimulation::SimulationState::PAUSED)
                        {
                            plotData.TimeAccumulator += ImGui::GetIO().DeltaTime;
                            
                            float linearVel = rb->PhysicsBody->getLinearVelocity().length();
                            float angularVel = rb->PhysicsBody->getAngularVelocity().length();
                            
                            plotData.LinearVelocity.AddPoint(plotData.TimeAccumulator, linearVel);
                            plotData.AngularVelocity.AddPoint(plotData.TimeAccumulator, angularVel);
                        }

                        float currentTime = plotData.TimeAccumulator;                
                        ImGui::PushStyleColor(ImGuiCol_Text, ACCENT);
                        ImGui::Text("Linear Speed Over Time");
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("This graph shows how the object's speed changes over time.\n\n"
                                "Reading the graph:\n"
                                "• Horizontal axis: Time in seconds\n"
                                "• Vertical axis: Speed in meters per second\n"
                                "• Peaks: When the object is moving fastest\n"
                                "• Valleys: When the object slows down\n"
                                "• Flat lines: Constant speed (steady motion)\n"
                                "• Rising lines: Acceleration\n"
                                "• Falling lines: Deceleration\n\n"
                                "Useful for understanding:\n"
                                "• How forces affect motion\n"
                                "• Energy transfer during collisions\n"
                                "• Friction and air resistance effects");
                        
                        if (ImPlot::BeginPlot("##LinearVelocityPlot", ImVec2(-1, 280)))
                        {
                            ImPlot::SetupAxes("Time (seconds)", "Speed (m/s)", 0, 0);
                            ImPlot::SetupAxisLimits(ImAxis_X1, currentTime - s_PlotHistory, currentTime, ImGuiCond_Always);
                            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 20, ImGuiCond_Once);
                            
                            ImPlot::PushStyleColor(ImPlotCol_Line, ACCENT);
                            ImVec4 fillColor = ACCENT;
                            fillColor.w = 0.25f;
                            ImPlot::SetNextFillStyle(fillColor);
                            ImPlot::SetNextLineStyle(ACCENT, 2.5f);
                            
                            if (!plotData.LinearVelocity.Data.empty())
                            {
                                ImPlot::PlotLine(
                                    "Speed",
                                    &plotData.LinearVelocity.Data[0].x,
                                    &plotData.LinearVelocity.Data[0].y,
                                    plotData.LinearVelocity.GetSize(),
                                    0,
                                    plotData.LinearVelocity.Offset,
                                    2 * sizeof(float)
                                );
                            }
                            
                            // Store plot position and size for PNG export
                            plotData.LinearPlotPos = ImPlot::GetPlotPos();
                            plotData.LinearPlotSize = ImPlot::GetPlotSize();
                            
                            ImPlot::PopStyleColor();
                            ImPlot::EndPlot();
                        }
                        
                        ImGui::Spacing();                      
                        ImGui::PushStyleColor(ImGuiCol_Button, ACCENT);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ACCENT_HOVER);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ACCENT_ACTIVE);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                        
                        if (ImGui::Button("Export to CSV", ImVec2(180, 0)))
                        {
                            ImGui::OpenPopup("ExportLinearCSV");
                        }
                        ImGui::PopStyleColor(4);
                        ImGui::SameLine();
                        HelpMarker("Export the raw data to a CSV spreadsheet file.\n\n"
                                "Use CSV files for:\n"
                                "• Further analysis in Excel or Google Sheets\n"
                                "• Creating custom graphs\n"
                                "• Statistical calculations\n"
                                "• Comparing with other datasets");
                        
                        ImGui::Spacing();
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        // ============================================================================
                        // ANGULAR VELOCITY PLOT
                        // ============================================================================
                        
                        ImGui::PushStyleColor(ImGuiCol_Text, WARNING);
                        ImGui::Text("Rotational Speed Over Time");
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("This graph shows how fast the object is spinning over time.\n\n"
                                "Reading the graph:\n"
                                "• Horizontal axis: Time in seconds\n"
                                "• Vertical axis: Rotation speed in radians per second\n"
                                "• Peaks: Fastest spinning moments\n"
                                "• Valleys: Slowest spinning moments\n"
                                "• Flat lines: Constant spin rate\n\n"
                                "Useful for analyzing:\n"
                                "• Torque effects\n"
                                "• Angular momentum conservation\n"
                                "• Rotational stability and wobbling\n"
                                "• Gyroscopic effects");
                        
                        if (ImPlot::BeginPlot("##AngularVelocityPlot", ImVec2(-1, 280)))
                        {
                            ImPlot::SetupAxes("Time (seconds)", "Rotation Speed (rad/s)", 0, 0);
                            ImPlot::SetupAxisLimits(ImAxis_X1, currentTime - s_PlotHistory, currentTime, ImGuiCond_Always);
                            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 10, ImGuiCond_Once);
                            
                            ImPlot::PushStyleColor(ImPlotCol_Line, WARNING);
                            ImVec4 fillColor2 = WARNING;
                            fillColor2.w = 0.25f;
                            ImPlot::SetNextFillStyle(fillColor2);
                            ImPlot::SetNextLineStyle(WARNING, 2.5f);
                            
                            if (!plotData.AngularVelocity.Data.empty())
                            {
                                ImPlot::PlotLine(
                                    "Spin Rate",
                                    &plotData.AngularVelocity.Data[0].x,
                                    &plotData.AngularVelocity.Data[0].y,
                                    plotData.AngularVelocity.GetSize(),
                                    0,
                                    plotData.AngularVelocity.Offset,
                                    2 * sizeof(float)
                                );
                            }
                            
                            // Store plot position and size for PNG export
                            plotData.AngularPlotPos = ImPlot::GetPlotPos();
                            plotData.AngularPlotSize = ImPlot::GetPlotSize();
                            
                            ImPlot::PopStyleColor();
                            ImPlot::EndPlot();
                        }
                        
                        ImGui::Spacing();
                        ImVec4 warningHover = MixColors(WARNING, ImVec4(1, 1, 1, 1), 0.12f);
                        ImVec4 warningActive = MixColors(WARNING, ImVec4(0, 0, 0, 1), 0.15f);            
                        ImGui::PushStyleColor(ImGuiCol_Button, WARNING);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, warningHover);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, warningActive);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                        
                        if (ImGui::Button("Export to CSV##Angular", ImVec2(180, 0)))
                        {
                            ImGui::OpenPopup("ExportAngularCSV");
                        }
                        ImGui::PopStyleColor(4);
                        ImGui::SameLine();
                        HelpMarker("Export rotation data to CSV for detailed analysis in spreadsheet applications.");
                        
                        ImGui::SameLine(0.0f, 30.0f);
                        
                        // Clear Data Button
                        ImGui::PushStyleColor(ImGuiCol_Button, ERROR_COLOR);
                        ImVec4 errorHover = MixColors(ERROR_COLOR, ImVec4(1, 1, 1, 1), 0.12f);
                        ImVec4 errorActive = MixColors(ERROR_COLOR, ImVec4(0, 0, 0, 1), 0.15f);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, errorHover);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, errorActive);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                        
                        if (ImGui::Button("Clear All Data", ImVec2(170, 0)))
                        {
                            plotData.LinearVelocity.Erase();
                            plotData.AngularVelocity.Erase();
                            plotData.TimeAccumulator = 0.0f;
                            
                            MOTION_INFO("Cleared all plot data for entity");
                        }
                        ImGui::PopStyleColor(4);
                        ImGui::SameLine();
                        HelpMarker("Erase all recorded data and start with fresh, empty graphs.\n\n"
                                "Use this when:\n"
                                "• Starting a new experiment\n"
                                "• Graphs become cluttered\n"
                                "• You want to reset and try again");

                        // ============================================================================
                        // EXPORT POPUPS
                        // ============================================================================

                        // Linear Velocity CSV Export Popup
                        if (ImGui::BeginPopup("ExportLinearCSV"))
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ACCENT);
                            ImGui::Text("Export Linear Speed Data");
                            ImGui::PopStyleColor();
                            ImGui::Separator();
                            ImGui::Spacing();
                            
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                            ImGui::TextWrapped("Export raw data to CSV format for analysis in Excel or other tools:");
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                            
                            static char linearCsvFilename[128] = "linear_velocity_data.csv";
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                            ImGui::Text("Filename:");
                            ImGui::PopStyleColor();
                            ImGui::SetNextItemWidth(350.0f);
                            ImGui::InputText("##linearCsvFilename", linearCsvFilename, sizeof(linearCsvFilename));
                            
                            ImGui::Spacing();
                            
                            // Show data preview
                            const int dataCount = plotData.LinearVelocity.GetSize();
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                            ImGui::Text("Data points: %d", dataCount);
                            if (dataCount > 0)
                            {
                                const ImVec2 firstPoint = plotData.LinearVelocity.GetPoint(0);
                                const ImVec2 lastPoint = plotData.LinearVelocity.GetPoint(dataCount - 1);
                                ImGui::Text("Time range: %.2f - %.2f seconds", firstPoint.x, lastPoint.x);
                            }
                            ImGui::PopStyleColor();
                            
                            ImGui::Spacing();
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DISABLED);
                            ImGui::TextDisabled("CSV files can be opened in Excel, Google Sheets, or any spreadsheet application");
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::Spacing();
                            
                            ImGui::PushStyleColor(ImGuiCol_Button, SUCCESS);
                            ImVec4 successHover = MixColors(SUCCESS, ImVec4(1, 1, 1, 1), 0.12f);
                            ImVec4 successActive = MixColors(SUCCESS, ImVec4(0, 0, 0, 1), 0.15f);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, successHover);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, successActive);
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                            
                            if (ImGui::Button("Export", ImVec2(140, 0)))
                            {
                                DialogBoxes::InitializeCOM();
                                if (auto path = DialogBoxes::SelectFolderDialog(L"Choose where to save the CSV file", m_ScenePath); !path.empty())
                                {
                                    std::filesystem::path savePath = path / linearCsvFilename;
                                    
                                    if (m_PlotExporter->ExportPlotDataToCSV(savePath.string(), plotData.LinearVelocity, "Time (s)", "Speed (m/s)"))
                                    {
                                        MOTION_INFO("Linear velocity data exported to: {}", savePath.string());
                                    }
                                    else
                                    {
                                        MOTION_ERROR("Failed to export linear velocity data");
                                    }
                                }
                                DialogBoxes::UninitializeCOM();
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::PopStyleColor(4);
                            ImGui::SameLine();
                            
                            ImGui::PushStyleColor(ImGuiCol_Button, CONTROL_BG);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HOVER_BG);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ACTIVE_BG);
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                            
                            if (ImGui::Button("Cancel", ImVec2(140, 0)))
                                ImGui::CloseCurrentPopup();
                            
                            ImGui::PopStyleColor(4);
                            
                            ImGui::EndPopup();
                        }
                        
                        // Angular Velocity CSV Export Popup
                        if (ImGui::BeginPopup("ExportAngularCSV"))
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, WARNING);
                            ImGui::Text("Export Rotational Speed Data");
                            ImGui::PopStyleColor();
                            ImGui::Separator();
                            ImGui::Spacing();
                            
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                            ImGui::TextWrapped("Export raw rotation data to CSV format for detailed analysis:");
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                            
                            static char angularCsvFilename[128] = "angular_velocity_data.csv";
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                            ImGui::Text("Filename:");
                            ImGui::PopStyleColor();
                            ImGui::SetNextItemWidth(350.0f);
                            ImGui::InputText("##angularCsvFilename", angularCsvFilename, sizeof(angularCsvFilename));
                            
                            ImGui::Spacing();
                            
                            // Show data preview
                            const int dataCount = plotData.AngularVelocity.GetSize();
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                            ImGui::Text("Data points: %d", dataCount);
                            if (dataCount > 0)
                            {
                                const ImVec2 firstPoint = plotData.AngularVelocity.GetPoint(0);
                                const ImVec2 lastPoint = plotData.AngularVelocity.GetPoint(dataCount - 1);
                                ImGui::Text("Time range: %.2f - %.2f seconds", firstPoint.x, lastPoint.x);
                            }
                            ImGui::PopStyleColor();
                            
                            ImGui::Spacing();
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DISABLED);
                            ImGui::TextDisabled("CSV files can be opened in Excel, Google Sheets, or any spreadsheet application");
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::Spacing();
                            
                            ImGui::PushStyleColor(ImGuiCol_Button, SUCCESS);
                            ImVec4 successHover = MixColors(SUCCESS, ImVec4(1, 1, 1, 1), 0.12f);
                            ImVec4 successActive = MixColors(SUCCESS, ImVec4(0, 0, 0, 1), 0.15f);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, successHover);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, successActive);
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                            
                            if (ImGui::Button("Export", ImVec2(140, 0)))
                            {
                                DialogBoxes::InitializeCOM();
                                if (auto path = DialogBoxes::SelectFolderDialog(L"Choose where to save the CSV file", m_ScenePath); !path.empty())
                                {
                                    std::filesystem::path savePath = path / angularCsvFilename;
                                    
                                    if (m_PlotExporter->ExportPlotDataToCSV(savePath.string(), plotData.AngularVelocity, "Time (s)", "Rotation Speed (rad/s)"))
                                    {
                                        MOTION_INFO("Angular velocity data exported to: {}", savePath.string());
                                    }
                                    else
                                    {
                                        MOTION_ERROR("Failed to export angular velocity data");
                                    }
                                }
                                DialogBoxes::UninitializeCOM();
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::PopStyleColor(4);
                            ImGui::SameLine();
                            
                            ImGui::PushStyleColor(ImGuiCol_Button, CONTROL_BG);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HOVER_BG);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ACTIVE_BG);
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                            
                            if (ImGui::Button("Cancel", ImVec2(140, 0)))
                                ImGui::CloseCurrentPopup();
                            
                            ImGui::PopStyleColor(4);
                            
                            ImGui::EndPopup();
                        }

                        ImGui::TreePop();
                    }
                    else
                    {
                        ImGui::PopStyleColor(2);
                    }
                    ImGui::Spacing();

                    // Apply Forces Section
                    ImGui::PushStyleColor(ImGuiCol_Header, CONTROL_BG);
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HOVER_BG);
                    
                    if (ImGui::TreeNodeEx("Physics Controls (Apply Forces)", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PopStyleColor(2);
                        ImGui::Spacing();
                        
                        ImGui::PushStyleColor(ImGuiCol_Text, MixColors(WARNING, TEXT_PRIMARY, 0.30f));
                        ImGui::TextWrapped("Experiment with physics by applying forces to see how objects react!");
                        ImGui::PopStyleColor();
                        
                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                        ImGui::TextWrapped("Use these controls to push, pull, or spin the object. Watch how different forces affect motion in real-time.");
                        ImGui::PopStyleColor();
                        
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        
                        // Direction Controls
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Force Direction:");
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("Choose which direction to apply the force.\n\n"
                                "• Use the sliders to set a custom direction\n"
                                "• Or click the preset buttons below for common directions\n"
                                "• Click 'Normalize' to make it a unit direction vector\n\n"
                                "Think of this like choosing which way to push an object.");
                        
                        ImGui::SetNextItemWidth(280.0f);
                        ImGui::DragFloat3("##impulse_dir", &plotData.ImpulseDirection.x, 0.01f, -1.0f, 1.0f, "%.2f");
                        ImGui::SameLine();
                        
                        ImGui::PushStyleColor(ImGuiCol_Button, CONTROL_BG);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HOVER_BG);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ACTIVE_BG);
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        
                        if (ImGui::Button("Normalize Direction", ImVec2(160, 0)))
                        {
                            plotData.ImpulseDirection = glm::normalize(plotData.ImpulseDirection);
                        }
                        ImGui::PopStyleColor(4);
                        ImGui::SameLine();
                        HelpMarker("Convert to a unit vector (length = 1.0)\n"
                                "This makes the direction 'pure' without affecting magnitude.");
                        
                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Quick Direction Presets:");
                        ImGui::PopStyleColor();
                        ImGui::Indent(10.0f);
                        
                        ImGui::PushStyleColor(ImGuiCol_Button, CONTROL_BG);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HOVER_BG);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, MixColors(ACCENT, CONTROL_BG, 0.80f));
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        
                        if (ImGui::Button("Up", ImVec2(90, 0))) 
                            plotData.ImpulseDirection = glm::vec3(0, 1, 0);
                        ImGui::SameLine();
                        if (ImGui::Button("Down", ImVec2(90, 0))) 
                            plotData.ImpulseDirection = glm::vec3(0, -1, 0);
                        ImGui::SameLine();
                        if (ImGui::Button("Right", ImVec2(90, 0))) 
                            plotData.ImpulseDirection = glm::vec3(1, 0, 0);
                        ImGui::SameLine();
                        if (ImGui::Button("Left", ImVec2(90, 0))) 
                            plotData.ImpulseDirection = glm::vec3(-1, 0, 0);
                        
                        if (ImGui::Button("Forward", ImVec2(90, 0))) 
                            plotData.ImpulseDirection = glm::vec3(0, 0, 1);
                        ImGui::SameLine();
                        if (ImGui::Button("Backward", ImVec2(90, 0))) 
                            plotData.ImpulseDirection = glm::vec3(0, 0, -1);
                        
                        ImGui::PopStyleColor(4);
                        ImGui::Unindent(10.0f);
                        
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        
                        // Magnitude Control
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Force Strength (Magnitude):");
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("How strong the force should be, measured in Newton-seconds (N·s).\n\n"
                                "What's a Newton?\n"
                                "• 1 N = Force needed to lift ~100 grams\n"
                                "• 10 N = Force to lift ~1 kilogram\n\n"
                                "Suggested values:\n"
                                "• Light objects (ball, phone): 1-10 N·s\n"
                                "• Medium objects (chair, toolbox): 10-50 N·s\n"
                                "• Heavy objects (car, boulder): 50-100 N·s\n\n"
                                "Experiment with different values to see the effects!");
                        
                        ImGui::SetNextItemWidth(400.0f);
                        ImGui::SliderFloat("##impulse_mag", &plotData.ImpulseMagnitude, 0.1f, 1000.0f, "%.2f N·s", ImGuiSliderFlags_Logarithmic);
                        
                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Preset Strengths:");
                        ImGui::PopStyleColor();
                        ImGui::Indent(10.0f);
                        
                        ImGui::PushStyleColor(ImGuiCol_Button, CONTROL_BG);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HOVER_BG);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, MixColors(ACCENT, CONTROL_BG, 0.80f));
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        
                        if (ImGui::Button("Gentle (1 N·s)", ImVec2(150, 0))) 
                            plotData.ImpulseMagnitude = 1.0f;
                        ImGui::SameLine();
                        if (ImGui::Button("Moderate (10 N·s)", ImVec2(150, 0))) 
                            plotData.ImpulseMagnitude = 10.0f;
                        ImGui::SameLine();
                        if (ImGui::Button("Strong (50 N·s)", ImVec2(150, 0))) 
                            plotData.ImpulseMagnitude = 50.0f;
                        ImGui::SameLine();
                        if (ImGui::Button("Very Strong (100 N·s)", ImVec2(150, 0))) 
                            plotData.ImpulseMagnitude = 100.0f;
                        
                        ImGui::PopStyleColor(4);
                        ImGui::Unindent(10.0f);
                        
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        
                        // Application Point
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::Text("Where to Apply Force:");
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        HelpMarker("Choose where on the object to apply the force.\n\n"
                                "Local Position:\n"
                                "• Relative to the object's center\n"
                                "• (0, 0, 0) = center of object\n"
                                "• Moves with the object as it rotates\n\n"
                                "World Position:\n"
                                "• Absolute position in world space\n"
                                "• Fixed location regardless of object rotation\n\n"
                                "Tip: Pushing off-center creates both linear motion\n"
                                "and rotation (torque)!");
                        
                        ImGui::Checkbox("Use Local Position (relative to object center)", &plotData.UseLocalPosition);
                        ImGui::SetNextItemWidth(280.0f);
                        ImGui::DragFloat3("##impulse_pos", &plotData.ImpulsePosition.x, 0.1f, 0.0f, 10000.0f, "%.2f");
                        ImGui::SameLine();
                        
                        ImGui::PushStyleColor(ImGuiCol_Button, CONTROL_BG);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HOVER_BG);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ACTIVE_BG);
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        
                        if (ImGui::Button("Reset to Center", ImVec2(140, 0)))
                        {
                            plotData.ImpulsePosition = glm::vec3(0.0f);
                        }
                        ImGui::PopStyleColor(4);
                        
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        
                        // Apply Buttons
                        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                        ImGui::TextWrapped("Ready to apply forces? Click a button below:");
                        ImGui::PopStyleColor();
                        
                        ImGui::Spacing();
                        
                        // Linear force button
                        ImGui::PushStyleColor(ImGuiCol_Button, ACCENT);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ACCENT_HOVER);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ACCENT_ACTIVE);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                        
                        if (ImGui::Button("Apply Linear Force (Push/Pull)", ImVec2(-1, 35)))
                        {
                            glm::vec3 impulse = plotData.ImpulseDirection * plotData.ImpulseMagnitude;
                            reactphysics3d::Vector3 rp3dImpulse(impulse.x, impulse.y, impulse.z);
                            rb->PhysicsBody->applyWorldForceAtCenterOfMass(rp3dImpulse);
                        }
                        ImGui::PopStyleColor(4);
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                            ImGui::TextWrapped("Pushes or pulls the object in the chosen direction.");
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                            ImGui::TextDisabled("This creates linear motion (straight-line movement)");
                            ImGui::PopStyleColor();
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::Spacing();
                        
                        // Angular force button
                        ImGui::PushStyleColor(ImGuiCol_Button, WARNING);
                        ImVec4 warningHover = MixColors(WARNING, ImVec4(1, 1, 1, 1), 0.12f);
                        ImVec4 warningActive = MixColors(WARNING, ImVec4(0, 0, 0, 1), 0.15f);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, warningHover);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, warningActive);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                        
                        if (ImGui::Button("Apply Rotational Force (Spin/Torque)", ImVec2(-1, 35)))
                        {
                            glm::vec3 torque = plotData.ImpulseDirection * plotData.ImpulseMagnitude;
                            reactphysics3d::Vector3 rp3dTorque(torque.x, torque.y, torque.z);
                            rb->PhysicsBody->applyWorldTorque(rp3dTorque);
                        }
                        ImGui::PopStyleColor(4);
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                            ImGui::TextWrapped("Makes the object spin around the chosen axis.");
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                            ImGui::TextDisabled("This creates angular motion (rotation)");
                            ImGui::PopStyleColor();
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::Spacing();
                        
                        // Force at point button
                        ImGui::PushStyleColor(ImGuiCol_Button, SUCCESS);
                        ImVec4 successHover = MixColors(SUCCESS, ImVec4(1, 1, 1, 1), 0.12f);
                        ImVec4 successActive = MixColors(SUCCESS, ImVec4(0, 0, 0, 1), 0.15f);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, successHover);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, successActive);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                        
                        if (ImGui::Button("Apply Force at Specific Point", ImVec2(-1, 35)))
                        {
                            glm::vec3 impulse = plotData.ImpulseDirection * plotData.ImpulseMagnitude;
                            reactphysics3d::Vector3 rp3dImpulse(impulse.x, impulse.y, impulse.z);
                            reactphysics3d::Vector3 rp3dPoint(
                                plotData.ImpulsePosition.x, 
                                plotData.ImpulsePosition.y, 
                                plotData.ImpulsePosition.z
                            );
                            
                            if (plotData.UseLocalPosition)
                                rb->PhysicsBody->applyWorldForceAtLocalPosition(rp3dImpulse, rp3dPoint);
                            else
                                rb->PhysicsBody->applyWorldForceAtWorldPosition(rp3dImpulse, rp3dPoint);
                        }
                        ImGui::PopStyleColor(4);
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                            ImGui::TextWrapped("Pushes the object at the specified location.");
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                            ImGui::TextDisabled("Creates both linear motion AND rotation!");
                            ImGui::Spacing();
                            ImGui::Text("Perfect for:");
                            ImGui::BulletText("Simulating impacts");
                            ImGui::BulletText("Creating realistic collisions");
                            ImGui::BulletText("Understanding torque");
                            ImGui::PopStyleColor();
                            ImGui::EndTooltip();
                        }

                        ImGui::TreePop();
                    }
                    else
                    {
                        ImGui::PopStyleColor(2);
                    }
                }
                
                ImGui::Unindent(20.0f);
            }
            
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::PopID();
        }

        ImGui::End();
        
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
    }

    /**
     * @brief Renders a window for statistical analysis of physics data.
     *
     * This window allows the user to analyze physics data with statistical tools.
     * Features include mean, median, standard deviation, min/max value detection,
     * peak detection for oscillations, numerical integration (area under curve),
     * sample statistics and distributions.
     *
     * The user can select an object in the simulation watchlist to analyze its data.
     * The analysis is split into two sections: linear motion and rotational motion.
     * The linear motion section shows the statistical analysis of the object's linear velocity.
     * The rotational motion section shows the statistical analysis of the object's angular velocity.
     * A third section allows the user to compare the two types of motion.
     *
     * @param[in] context The current scene context.
     */
    void SceneEditorLayer::RenderStatisticalAnalysisPanel(SceneContext& context)
    {
        const ImVec4 CARD_BG        = ImVec4(255.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f, 0.90f);
        const ImVec4 CONTROL_BG     = ImVec4(251.0f/255.0f, 251.0f/255.0f, 251.0f/255.0f, 1.0f);
        const ImVec4 HOVER_BG       = ImVec4(246.0f/255.0f, 246.0f/255.0f, 246.0f/255.0f, 1.0f);
        const ImVec4 TEXT_PRIMARY   = ImVec4(32.0f/255.0f, 33.0f/255.0f, 36.0f/255.0f, 1.0f);
        const ImVec4 TEXT_SECONDARY = ImVec4(96.0f/255.0f, 94.0f/255.0f, 92.0f/255.0f, 1.0f);
        const ImVec4 TEXT_DISABLED  = ImVec4(161.0f/255.0f, 159.0f/255.0f, 157.0f/255.0f, 1.0f);
        const ImVec4 BORDER         = ImVec4(229.0f/255.0f, 229.0f/255.0f, 229.0f/255.0f, 0.50f);
        const ImVec4 SUCCESS        = ImVec4(16.0f/255.0f, 137.0f/255.0f, 62.0f/255.0f, 1.0f);
        const ImVec4 WARNING        = ImVec4(255.0f/255.0f, 185.0f/255.0f, 0.0f/255.0f, 1.0f);
        
        ImVec4 ACCENT = ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
        
        auto MixColors = [](const ImVec4& a, const ImVec4& b, float t) -> ImVec4 {
            return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                        a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
        };

        ImGui::SetNextWindowSize(ImVec2(700, 800), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Statistical Analysis", nullptr))
        {
            if(m_SimulationWatchList.empty() || !context.Simulation->InSimulation)
            {
                ImGui::TextDisabled("No simulation data available.");
                ImGui::End();
                return;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, ACCENT);
            ImGui::Text("Statistical Data Analysis");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            HelpMarker("Analyze physics data with statistical tools.\n\n"
                    "Features:\n"
                    "• Mean, median, standard deviation\n"
                    "• Min/max value detection\n"
                    "• Peak detection for oscillations\n"
                    "• Numerical integration (area under curve)\n"
                    "• Sample statistics and distributions");
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
            ImGui::TextWrapped("Statistical analysis of motion data helps identify patterns, trends, and key moments in your physics experiments.");
            ImGui::PopStyleColor();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Object selector
            static int selectedEntityIndex = 0;
            const char* objectNames[100];
            int objectCount = 0;
            
            for (auto& e : m_SimulationWatchList)
            {
                if (auto* tag = context.Entities->Registry.try_get<TagComponent>(e))
                {
                    objectNames[objectCount] = tag->Tag.c_str();
                    objectCount++;
                }
            }
            
            if (objectCount == 0)
            {
                ImGui::TextWrapped("No objects in watchlist. Add objects to the simulation watchlist to analyze their data.");
                ImGui::End();
                return;
            }
            
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
            ImGui::Text("Select Object:");
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(300.0f);
            ImGui::Combo("##ObjectSelect", &selectedEntityIndex, objectNames, objectCount);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        
            auto entityIter = m_SimulationWatchList.begin();
            std::advance(entityIter, selectedEntityIndex);
            auto& plotData = s_EntityPlotData[*entityIter];
            
            static StatisticalData linearStats;
            static StatisticalData angularStats;
            
            std::vector<ImVec2> linearData(plotData.LinearVelocity.Data.begin(), plotData.LinearVelocity.Data.end());
            std::vector<ImVec2> angularData(plotData.AngularVelocity.Data.begin(), plotData.AngularVelocity.Data.end());

            linearStats.Calculate(linearData);
            angularStats.Calculate(angularData);
            
            if (ImGui::BeginTabBar("AnalysisTabs"))
            {
                if (ImGui::BeginTabItem("Linear Motion"))
                {
                    ImGui::Spacing();
                    RenderStatisticsSection("Linear Velocity (m/s)", linearStats, ACCENT);
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("Rotational Motion"))
                {
                    ImGui::Spacing();
                    RenderStatisticsSection("Angular Velocity (rad/s)", angularStats, WARNING);
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("Compare"))
                {
                    ImGui::Spacing();
                    RenderComparisonSection(linearStats, angularStats);
                    ImGui::EndTabItem();
                }
                
                ImGui::EndTabBar();
            }
        }

        ImGui::End();
    }

    /**
     * @brief Renders a statistics section of the scene editor layer.
     * 
     * This section displays a range of statistics about the linear motion of the object.
     * It displays the mean, median, standard deviation, maximum, minimum, range, and integral
     * values of the linear motion. It also displays the number of peaks detected for the linear motion.
     * 
     * @param title The title to display above the statistics section.
     * @param stats The statistical data to display.
     * @param color The accent color to use for the section.
     */
    void SceneEditorLayer::RenderStatisticsSection(const char* title, const StatisticalData& stats, const ImVec4& color)
    {
        const ImVec4 CARD_BG        = ImVec4(255.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f, 0.90f);
        const ImVec4 CONTROL_BG     = ImVec4(251.0f/255.0f, 251.0f/255.0f, 251.0f/255.0f, 1.0f);
        const ImVec4 TEXT_PRIMARY   = ImVec4(32.0f/255.0f, 33.0f/255.0f, 36.0f/255.0f, 1.0f);
        const ImVec4 TEXT_SECONDARY = ImVec4(96.0f/255.0f, 94.0f/255.0f, 92.0f/255.0f, 1.0f);
        const ImVec4 TEXT_DISABLED  = ImVec4(161.0f/255.0f, 159.0f/255.0f, 157.0f/255.0f, 1.0f);
        const ImVec4 BORDER         = ImVec4(229.0f/255.0f, 229.0f/255.0f, 229.0f/255.0f, 0.50f);
        const ImVec4 SUCCESS        = ImVec4(16.0f/255.0f, 137.0f/255.0f, 62.0f/255.0f, 1.0f);
        
        auto MixColors = [](const ImVec4& a, const ImVec4& b, float t) -> ImVec4 {
            return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                        a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
        };
        
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        ImGui::Spacing();
        
        if (stats.SampleCount == 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DISABLED);
            ImGui::TextWrapped("No data available. Run the simulation to collect data.");
            ImGui::PopStyleColor();
            return;
        }
        
        // Basic Statistics Card
        ImVec4 cardBg = MixColors(color, CARD_BG, 0.95f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, cardBg);
        ImGui::PushStyleColor(ImGuiCol_Border, MixColors(color, BORDER, 0.40f));
        
        ImGui::BeginChild("BasicStats", ImVec2(0, 180), true);
        
        ImGui::PushStyleColor(ImGuiCol_Text, MixColors(color, TEXT_PRIMARY, 0.30f));
        ImGui::Text("Basic Statistics");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Columns(2, "stats_cols", false);
        ImGui::SetColumnWidth(0, 200);
        
        // Mean
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
        ImGui::Text("Mean (Average):");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        HelpMarker("The average value of all data points.\n\nCalculated as: Σx / n");
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
        ImGui::Text("%.4f", stats.Mean);
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        
        ImGui::Spacing();
        
        // Median
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
        ImGui::Text("Median (Middle Value):");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        HelpMarker("The middle value when data is sorted.\n\nLess affected by outliers than mean.");
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
        ImGui::Text("%.4f", stats.Median);
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        
        ImGui::Spacing();
        
        // Standard Deviation
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
        ImGui::Text("Standard Deviation:");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        HelpMarker("Measures spread of data around the mean.\n\n"
                "• Low value = data clustered near mean\n"
                "• High value = data widely spread");
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
        ImGui::Text("%.4f", stats.StdDev);
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        
        ImGui::Spacing();
        
        // Sample Count
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
        ImGui::Text("Sample Count:");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        HelpMarker("Total number of data points collected.");
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
        ImGui::Text("%d points", stats.SampleCount);
        ImGui::PopStyleColor();
        
        ImGui::Columns(1);
        
        ImGui::EndChild();
        ImGui::PopStyleColor(2);
        
        ImGui::Spacing();
        
        // Range Statistics Card
        cardBg = MixColors(SUCCESS, CARD_BG, 0.95f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, cardBg);
        ImGui::PushStyleColor(ImGuiCol_Border, MixColors(SUCCESS, BORDER, 0.40f));
        
        ImGui::BeginChild("RangeStats", ImVec2(0, 140), true);
        
        ImGui::PushStyleColor(ImGuiCol_Text, MixColors(SUCCESS, TEXT_PRIMARY, 0.30f));
        ImGui::Text("Range & Extremes");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Columns(2, "range_cols", false);
        ImGui::SetColumnWidth(0, 200);
        
        // Minimum
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
        ImGui::Text("Minimum Value:");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        HelpMarker("Lowest value in the dataset.");
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
        ImGui::Text("%.4f", stats.Min);
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        
        ImGui::Spacing();
        
        // Maximum
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
        ImGui::Text("Maximum Value:");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        HelpMarker("Highest value in the dataset.");
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
        ImGui::Text("%.4f", stats.Max);
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        
        ImGui::Spacing();
        
        // Range
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
        ImGui::Text("Range:");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        HelpMarker("Difference between max and min.\n\nRange = Max - Min");
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
        ImGui::Text("%.4f", stats.Range);
        ImGui::PopStyleColor();
        
        ImGui::Columns(1);
        
        ImGui::EndChild();
        ImGui::PopStyleColor(2);
        
        ImGui::Spacing();
        
        // Peak Detection Card
        if (!stats.Peaks.empty())
        {
            const ImVec4 WARNING = ImVec4(255.0f/255.0f, 185.0f/255.0f, 0.0f/255.0f, 1.0f);
            cardBg = MixColors(WARNING, CARD_BG, 0.95f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, cardBg);
            ImGui::PushStyleColor(ImGuiCol_Border, MixColors(WARNING, BORDER, 0.40f));
            
            ImGui::BeginChild("PeakStats", ImVec2(0, 200), true);
            
            ImGui::PushStyleColor(ImGuiCol_Text, MixColors(WARNING, TEXT_PRIMARY, 0.30f));
            ImGui::Text("Peak Detection");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            HelpMarker("Automatically detected peaks (local maxima) in the data.\n\n"
                    "Useful for:\n"
                    "• Finding maximum speeds\n"
                    "• Counting oscillations\n"
                    "• Identifying impact moments");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
            ImGui::Text("Detected Peaks: %d", (int)stats.Peaks.size());
            ImGui::PopStyleColor();
            ImGui::Spacing();
            
            // Show first few peaks
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
            int peaksToShow = std::min(5, (int)stats.Peaks.size());
            for (int i = 0; i < peaksToShow; i++)
            {
                ImGui::Text("  Peak %d: %.3f at t=%.2fs", i+1, stats.Peaks[i], stats.PeakTimes[i]);
            }
            if (stats.Peaks.size() > 5)
            {
                ImGui::Text("  ... and %d more", (int)stats.Peaks.size() - 5);
            }
            ImGui::PopStyleColor();
            
            ImGui::EndChild();
            ImGui::PopStyleColor(2);
            
            ImGui::Spacing();
        }
        
        // Integration Card
        ImVec4 accentCard = MixColors(color, CARD_BG, 0.93f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, accentCard);
        ImGui::PushStyleColor(ImGuiCol_Border, MixColors(color, BORDER, 0.40f));
        
        ImGui::BeginChild("IntegralStats", ImVec2(0, 120), true);
        
        ImGui::PushStyleColor(ImGuiCol_Text, MixColors(color, TEXT_PRIMARY, 0.30f));
        ImGui::Text("Numerical Integration");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        HelpMarker("Area under the curve using trapezoidal rule.\n\n"
                "For velocity data:\n"
                "• Integral of velocity = displacement\n"
                "• Shows total distance traveled");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
        ImGui::Text("Integral Value:");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%.4f", stats.IntegralValue);
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
        ImGui::TextWrapped("This represents the accumulated value over time (area under the graph).");
        ImGui::PopStyleColor();
        
        ImGui::EndChild();
        ImGui::PopStyleColor(2);
    }


    /**
     * @brief Renders a comparison section of the scene editor layer.
     * 
     * This section compares the linear and angular motion of the object.
     * It displays a table with the mean, median, standard deviation, maximum, minimum, range, and integral
     * values of the linear and angular motion. It also displays the number of peaks detected for the linear and angular motion.
     * 
     * @param linear Linear motion data.
     * @param angular Angular motion data.
     */
    void SceneEditorLayer::RenderComparisonSection(const StatisticalData & linear, const StatisticalData & angular)
    {
        const ImVec4 TEXT_PRIMARY = ImVec4(32.0f/255.0f, 33.0f/255.0f, 36.0f/255.0f, 1.0f);
        const ImVec4 TEXT_SECONDARY = ImVec4(96.0f/255.0f, 94.0f/255.0f, 92.0f/255.0f, 1.0f);
        ImVec4 ACCENT = ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
        const ImVec4 WARNING = ImVec4(255.0f/255.0f, 185.0f/255.0f, 0.0f/255.0f, 1.0f);
        
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
        ImGui::Text("Linear vs Angular Motion Comparison");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Comparison table
        if (ImGui::BeginTable("ComparisonTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Linear Motion");
            ImGui::TableSetupColumn("Angular Motion");
            ImGui::TableHeadersRow();
            
            auto AddRow = [&](const char* metric, float linearVal, float angularVal, const char* units)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                ImGui::Text("%s", metric);
                ImGui::PopStyleColor();
                
                ImGui::TableSetColumnIndex(1);
                ImGui::PushStyleColor(ImGuiCol_Text, ACCENT);
                ImGui::Text("%.4f %s", linearVal, units);
                ImGui::PopStyleColor();
                
                ImGui::TableSetColumnIndex(2);
                ImGui::PushStyleColor(ImGuiCol_Text, WARNING);
                ImGui::Text("%.4f %s", angularVal, units);
                ImGui::PopStyleColor();
            };
            
            AddRow("Mean", linear.Mean, angular.Mean, "");
            AddRow("Median", linear.Median, angular.Median, "");
            AddRow("Std Deviation", linear.StdDev, angular.StdDev, "");
            AddRow("Maximum", linear.Max, angular.Max, "");
            AddRow("Minimum", linear.Min, angular.Min, "");
            AddRow("Range", linear.Range, angular.Range, "");
            AddRow("Integral", linear.IntegralValue, angular.IntegralValue, "");
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
            ImGui::Text("Peaks Detected");
            ImGui::PopStyleColor();
            
            ImGui::TableSetColumnIndex(1);
            ImGui::PushStyleColor(ImGuiCol_Text, ACCENT);
            ImGui::Text("%d", (int)linear.Peaks.size());
            ImGui::PopStyleColor();
            
            ImGui::TableSetColumnIndex(2);
            ImGui::PushStyleColor(ImGuiCol_Text, WARNING);
            ImGui::Text("%d", (int)angular.Peaks.size());
            ImGui::PopStyleColor();
            
            ImGui::EndTable();
        }
    }


    /**
     * Renders a node entity in the scene hierarchy.
     * This includes rendering the entity's tag name and model, as well as its transform and physics components.
     * If the entity is inactive, a disabled text is rendered instead.
     * Additionally, a right-click context menu is rendered with options to delete, duplicate, add to watchlist, or remove from watchlist.
     * @param context The scene context.
     * @param root The root entity of the node.
     */
    void SceneEditorLayer::RenderNodeEntities(SceneContext& context, entt::entity root)
    {
        const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth |
                                        ImGuiTreeNodeFlags_AllowItemOverlap | 
                                        ImGuiTreeNodeFlags_FramePadding;

        m_Scene->ForEachNodeEntity(root, [&](entt::entity e)
        {
            if (e == root) return;
            const TagComponent* tagOpt = context.Entities->Registry.try_get<TagComponent>(e);
            const char* label = tagOpt ? tagOpt->Tag.c_str() : "Unnamed";
            ImGui::PushID(static_cast<std::int32_t>(entt::to_integral(e)));

            const bool open = ImGui::TreeNodeEx("##node", flags, "%s %s", label, (e == context.Entities->SelectedEntity) ? "*" : "");
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) 
            {
                m_Scene->SelectedEntity(e);
                m_PhysicsAnalysis.SelectedEntity = e;

                if (m_PhysicsAnalysis.Energy.TrackingEnabled)
                {
                    m_PhysicsAnalysis.Energy.Reset();
                }
                if (m_PhysicsAnalysis.Momentum.TrackConservation)
                {
                    m_PhysicsAnalysis.Momentum.Reset();
                }
                if (m_PhysicsAnalysis.Acceleration.ShowVector)
                {
                    m_PhysicsAnalysis.Acceleration.Reset();
                }
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) ImGui::OpenPopup("EntityContextMenu");

            if (ImGui::BeginPopupContextWindow("EntityContextMenu"))
            {
                if (ImGui::MenuItem("Delete Entity"))
                {
                    m_Scene->DestroyEntity(e, true);
                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::MenuItem("Duplicate Entity"))
                {
                    m_Scene->DuplicateEntity(e);
                    ImGui::CloseCurrentPopup();
                }

                ImGui::Separator();

                if (std::find(m_SimulationWatchList.begin(), m_SimulationWatchList.end(), e) == m_SimulationWatchList.end())
                {
                    if(ImGui::MenuItem("Add To Watchlist"))
                    {
                        m_SimulationWatchList.push_back(e);
                        ImGui::CloseCurrentPopup();
                    }
                }
                else
                {
                    if(ImGui::MenuItem("Remove From Watchlist"))
                    {
                        m_SimulationWatchList.erase(std::remove(m_SimulationWatchList.begin(), m_SimulationWatchList.end(), e), m_SimulationWatchList.end());
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::Separator();
                if(ImGui::MenuItem("Open In Material Editor"))
                {
                    if (context.Entities->Registry.try_get<MaterialComponent>(e))
                    {
                        openMaterialEditors[e] = true;
                    }
                }

                for (auto it = openMaterialEditors.begin(); it != openMaterialEditors.end();)
                {
                    entt::entity entity = it->first;
                    bool& isOpen = it->second;
                
                    auto* material = context.Entities->Registry.try_get<MaterialComponent>(entity);
                    if (!material)
                    {
                        it = openMaterialEditors.erase(it);
                        continue;
                    }
                    
                    std::string windowName = "Material Editor##" + std::to_string((uint32_t)entity);
                    ImGui::SetNextWindowSize(ImVec2(600.0f, 400.0f), ImGuiCond_FirstUseEver);
                    
                    if (ImGui::Begin(windowName.c_str(), &isOpen, ImGuiWindowFlags_NoDocking))
                    {
                        ImGui::Text("Entity: %u", (uint32_t)entity);
                        ImGui::Separator();
                        
                        if (ImGui::BeginChild("##inspector-area", ImVec2(0.0f, 0.0f)))
                        {
                            if (material->MaterialPointer)
                                DrawMaterialUI(material->MaterialPointer);
                            else
                                ImGui::TextDisabled("No material assigned");
                            
                            ImGui::EndChild();
                        }
                        ImGui::End();
                    }
                    
                    if (!isOpen)
                    {
                        it = openMaterialEditors.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }

                ImGui::EndPopup();
            }

            if (open)
            {
                RenderTagAndModel(context, e);

                if (tagOpt && tagOpt->IsActive)
                {
                    ImGui::BeginDisabled(context.Simulation->InSimulation);

                    RenderTransform(context, e);
                    RenderPhysics(context, e);

                    ImGui::EndDisabled();
                }
                else
                {
                    ImGui::TextDisabled("Entity is inactive");
                }
                ImGui::TreePop();
            }
        
            ImGui::PopID();
        });
    }

    /**
     * Renders the tag and model of an entity in the scene hierarchy.
     * This includes rendering the entity's tag name and model, as well as its transform and physics components.
     * If the entity is inactive, a disabled text is rendered instead.
     * Additionally, a right-click context menu is rendered with options to delete, duplicate, add to watchlist, or remove from watchlist.
     * @param context The scene context.
     * @param root The root entity of the node.
     */
    void SceneEditorLayer::RenderTagAndModel(SceneContext& context, entt::entity e)
    {
        if (!context.Entities->Registry.any_of<TagComponent>(e)) return;
        auto& tag = context.Entities->Registry.get<TagComponent>(e);

        ImGui::PushID("##tag-inspector");
        BeginPropertyGrid("##tag-grid");

        TextBox("Name Tag", tag.Tag, false);
        ToggleSwitch("Is Active", tag.IsActive);
        if (context.Entities->Registry.any_of<ModelComponent>(e))
        {
            const auto& model = context.Entities->Registry.get<ModelComponent>(e);

            std::string file  = model.FilePath.filename().string();
            std::string mesh  = std::to_string(model.MeshCount);
            TextBox("File Path", file, true);
            TextBox("Mesh Count", mesh, true);
        }

        EndPropertyGrid();
        ImGui::PopID();
    }

    /**
     * Renders the transform component of an entity in the scene hierarchy.
     * This includes rendering the entity's position, rotation, and scale, with drag float widgets to adjust them.
     * If the entity does not have a transform component, nothing is rendered.
     * @param context The scene context.
     * @param e The entity to render the transform component for.
     */
    void SceneEditorLayer::RenderTransform(SceneContext& context, entt::entity e)
    {
        if (auto* tr = context.Entities->Registry.try_get<TransformComponent>(e))
        {
            ImGui::PushID("##transform-properties");
            BeginPropertyGrid("##transform-grid");

            glm::vec3 t = tr->Translation;
            if (DragFloat3("Position (m)", t, 0.1f)) tr->Translation = t;
            HelpMarker("The position of the object in 3D space (X, Y, Z coordinates).\nMeasured in meters.");

            glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(tr->Rotation));
            if (DragFloat3("Rotation (deg)", eulerDeg, 1.0f)) tr->Rotation = glm::normalize(glm::quat(glm::radians(eulerDeg)));
            HelpMarker("The rotation of the object around each axis.\nMeasured in degrees (0-360).");

            glm::vec3 s = tr->Scale;
            if (DragFloat3("Scale", s, 0.1f, 0.01f, 100.0f)) tr->Scale = s;
            HelpMarker("The size multiplier for each axis.\n1.0 = original size, 2.0 = double size, 0.5 = half size");

            EndPropertyGrid();
            ImGui::PopID();
        }
    }

    /**
     * Renders the physics components of an entity in the scene hierarchy.
     * This includes rendering the entity's rigid body and collider, with drag float widgets to adjust their properties.
     * If the entity does not have a rigid body or collider, nothing is rendered.
     * @param context The scene context.
     * @param e The entity to render the physics components for.
     */
    void SceneEditorLayer::RenderPhysics(SceneContext& context, entt::entity e)
    {
        auto* cc = context.Entities->Registry.try_get<ColliderComponent>(e);
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(e);
        if (!cc || !rb) return;

        ImGui::PushID("##physics-properties");
        if (ImGui::CollapsingHeader("Physics Status", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent(10.0f);
            
            bool isDynamic = rb->PhysicsBody->getType() == rp3d::BodyType::DYNAMIC;
            bool isAwake = isDynamic && rb->PhysicsBody->isActive();
            bool isSleeping = isDynamic && rb->PhysicsBody->isSleeping();
            
            PhysicsUI::StatusIndicator("Active", isAwake, "Object is currently being updated by physics simulation");
            PhysicsUI::StatusIndicator("Sleeping", isSleeping, "Object has stopped moving and is temporarily paused to save performance");
            PhysicsUI::StatusIndicator("Collisions Enabled", true, "Object can collide with other physics objects");
            
            ImGui::Unindent(10.0f);
            ImGui::Spacing();
        }

        DrawRigidBodyUI(*rb);
        DrawColliderUI(*cc);

        if (context.Simulation->InSimulation && rb->PhysicsBody->getType() == rp3d::BodyType::DYNAMIC)
        {
            ImGui::BeginDisabled(false);

            DrawLivePhysicsData(*rb);
            
            ImGui::EndDisabled();
        }

        ImGui::PopID();
    }

    /**
     * Renders a user interface for adjusting the properties of a rigid body component.
     * This includes a combo box for selecting whether the body is static or dynamic, and
     * drag float widgets for adjusting the body's mass, linear damping, and angular damping.
     * @param rb The rigid body component to render the user interface for.
     */
    void SceneEditorLayer::DrawRigidBodyUI(RigidBodyComponent & rb)
    {
        if (ImGui::CollapsingHeader("Rigid Body Properties"))
        {
            ImGui::Indent(10.0f);
            
            if (BeginPropertyGrid("##rigidbody-props"))
            {
                bool allowSleeping = rb.PhysicsBody->isAllowedToSleep();
                if(ToggleSwitch("Allow Sleeping", allowSleeping)){
                    rb.PhysicsBody->setIsAllowedToSleep(allowSleeping);
                }
                HelpMarker("Allow the body to sleep if it is not moving");

                std::int32_t interaction = (rb.PhysicsBody->getType() == rp3d::BodyType::DYNAMIC) ? 1 : 0;
                ComboBox("Body Type", { "Static", "Dynamic" }, interaction,
                    [&](std::int32_t idx, const std::string&)
                    {
                        if (idx == 0) { rb.Type = BodyType::Static;  rb.PhysicsBody->setType(rp3d::BodyType::STATIC); }
                        if (idx == 1) { rb.Type = BodyType::Dynamic; rb.PhysicsBody->setType(rp3d::BodyType::DYNAMIC); }
                    });
                HelpMarker("Static: Immovable objects like walls, floors, and obstacles\n"
                          "Dynamic: Objects that move and respond to forces like balls, boxes, characters");

                auto* body = rb.PhysicsBody;

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

    /**
     * Renders a user interface for adjusting the properties of a collider component.
     * This includes drag float widgets for adjusting the collider's restitution (bounciness),
     * friction coefficient, and mass density.
     * @param cc The collider component to render the user interface for.
     */
    void SceneEditorLayer::DrawColliderUI(ColliderComponent& cc)
    {
        if (ImGui::CollapsingHeader("Collider Properties"))
        {
            ImGui::Indent(10.0f);
            
            if (BeginPropertyGrid("##collider-props"))
            {
                // Bounciness (Restitution)
                float bounce = cc.Restitution;
                if (SliderFloat("Bounciness", &bounce, 0.0f, 1.0f, "%.3f"))
                {
                    cc.Collider->getMaterial().setBounciness(bounce);
                    cc.Restitution = bounce;
                }
                HelpMarker("How bouncy the object is when it hits something.\n"
                          "0.0 = No bounce (like clay or putty)\n"
                          "0.5 = Medium bounce (like a basketball)\n"
                          "0.9 = Very bouncy (like a rubber super ball)\n"
                          "1.0 = Perfect bounce (no energy lost)");

                // Friction
                float friction = cc.Friction;
                if (SliderFloat("Friction", &friction, 0.0f, 1.0f, "%.3f"))
                {
                    cc.Collider->getMaterial().setFrictionCoefficient(friction);
                    cc.Friction = friction;
                }
                HelpMarker("How much the object resists sliding.\n"
                          "0.0 = Ice (super slippery)\n"
                          "0.5 = Wood or plastic\n"
                          "0.8 = Rubber\n"
                          "1.0 = Maximum grip");

                // Density
                float density = cc.MassDensity;
                if (DragFloat("Density (kg/m³)", &density, 1.0f, 0.1f, 10000.0f))
                {
                    cc.Collider->getMaterial().setMassDensity(density);
                    cc.MassDensity = density;
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
                float volume = cc.Shape->getVolume();
                DragFloat("Volume", &volume, 0.1f, 0.0f, 1000000.0f);
                HelpMarker("The volume of the object.\n"
                          "This is automatically calculated based on object size.");
                ImGui::EndDisabled();

                EndPropertyGrid();
            }
            
            ImGui::Unindent(10.0f);
        }
    }

    /**
     * Draws a user interface for displaying live physics data of a rigid body component.
     * This includes the current linear velocity (speed and direction), angular velocity (spin rate and axis),
     * and kinetic energy of the body.
     * @param rb The rigid body component to display the live physics data for.
     */
    void SceneEditorLayer::DrawLivePhysicsData(RigidBodyComponent& rb)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.7f, 0.4f, 0.5f));
        if (ImGui::CollapsingHeader("Live Physics Data", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PopStyleColor();
            ImGui::Indent(10.0f);
            
            auto* body = rb.PhysicsBody;
            
            // Linear Velocity
            auto linVel = body->getLinearVelocity();
            glm::vec3 velocity(linVel.x, linVel.y, linVel.z);
            float speed = PhysicsUI::GetSpeed(velocity);
            float speedKmh = PhysicsUI::MsToKmh(speed);
            glm::vec3 direction = PhysicsUI::GetDirection(velocity);
            
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
            float rotSpeed = PhysicsUI::GetSpeed(angularVelocity);
            float rpm = PhysicsUI::RadPerSecToRPM(rotSpeed);
            glm::vec3 rotAxis = PhysicsUI::GetDirection(angularVelocity);
            
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
            ImGui::SameLine();
            HelpMarker("Energy of motion. Higher = more force in collisions.\n"
                      "A 1kg object at 10 m/s has 50 Joules.");
            ImGui::Unindent(10.0f);
            
            ImGui::Unindent(10.0f);
        }
        else
        {
            ImGui::PopStyleColor();
        }
    }

    /**
     * Renders the toolbar and search field for the scene editor layer.
     * This includes a text field for searching for entities in the scene, and a button for importing entities from external files.
     */
    void SceneEditorLayer::RenderToolbarAndSearch()
    {
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 100.0f);
        ImGui::InputTextWithHint("##SearchScenes", "Search scenes...", m_SearchBuf, sizeof(m_SearchBuf));
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("Import##Button", ImVec2(120.0f, 0.0f)))
        {
            RequestEntityImport(true, true);
        }

        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Physics Analysis");
        
        // Quick toggle buttons with icons
        bool forceActive = m_PhysicsAnalysis.ShowForceAnalysisPanel;
        if (ImGui::Button(ICON_MD_ARCHITECTURE, ImVec2(40, 40)))
        {
            m_PhysicsAnalysis.ShowForceAnalysisPanel = !m_PhysicsAnalysis.ShowForceAnalysisPanel;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Force Analysis (Ctrl+F)");
        
        bool energyActive = m_PhysicsAnalysis.ShowEnergyPanel;
        if (ImGui::Button(ICON_MD_BOLT, ImVec2(40, 40)))
        {
            m_PhysicsAnalysis.ShowEnergyPanel = !m_PhysicsAnalysis.ShowEnergyPanel;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Energy Tracking (Ctrl+E)");
        
        bool momentumActive = m_PhysicsAnalysis.ShowMomentumPanel;
        if (ImGui::Button(ICON_MD_SPEED, ImVec2(40, 40)))
        {
            m_PhysicsAnalysis.ShowMomentumPanel = !m_PhysicsAnalysis.ShowMomentumPanel;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Momentum Analysis (Ctrl+M)");

        ImGui::Separator();
    }

    /**
     * Renders the entity hierarchy for the scene editor layer.
     * This includes rendering the entities in a tree view, with the ability to expand and collapse entities.
     * If an entity is inactive, a disabled text is rendered instead.
     * @param context The scene context.
     */
    void SceneEditorLayer::RenderEntityHierarchy(SceneContext& context)
    {
        const ImGuiTreeNodeFlags flags =
              ImGuiTreeNodeFlags_Framed
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_AllowItemOverlap
            | ImGuiTreeNodeFlags_FramePadding;

        if (ImGui::TreeNodeEx("##entities", flags, "Entities"))
        {
            m_Scene->ForEachRootEntity([&](entt::entity e)
            {
                ImGui::PushID(static_cast<std::int32_t>(entt::to_integral(e)));
                const TagComponent* tagOpt = context.Entities->Registry.try_get<TagComponent>(e);
                const char* label = tagOpt ? tagOpt->Tag.c_str() : "Unnamed";

                if (ImGui::TreeNodeEx("##root-node", flags, label))
                {       
                    if (tagOpt && tagOpt->IsActive)
                        RenderNodeEntities(context, e);
                    else
                        ImGui::TextDisabled("Entity is inactive");

                    ImGui::TreePop();
                }

                ImGui::PopID();
            });

            ImGui::TreePop();
        }
    }


    /**
     * Renders the environment settings for the scene editor layer.
     * This includes rendering the environment properties, such as the light direction and color, and the physics world settings.
     * The environment properties include the light direction, color, and intensity, as well as whether the light direction should be shown in the gizmo.
     * The physics world settings include the gravity, default restitution, default friction coefficient, and whether sleeping is enabled.
     * Additionally, advanced settings are available for the physics world, including the number of velocity solver iterations, position solver iterations, restitution velocity threshold, sleep linear velocity, sleep angular velocity, and time before sleep.
     * @param context The scene context.
     */
    void SceneEditorLayer::RenderEnvironmentSettings(SceneContext& context)
    {
        const ImGuiTreeNodeFlags flags =
              ImGuiTreeNodeFlags_Framed
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_AllowItemOverlap
            | ImGuiTreeNodeFlags_FramePadding
            | ImGuiTreeNodeFlags_DefaultOpen;

        ImGui::PushID("##environment-settings");
        ImGui::Begin("Environment Settings");
        {
            if (ImGui::TreeNodeEx("##environment", flags, "Environment"))
            {
                ImGui::Indent();

                // Lighting Section
                if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Indent(10.0f);
                    auto& light = context.Physics->SunLight;
                    
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
                if (ImGui::CollapsingHeader("Physics World", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Indent(10.0f);
                    auto& world = context.Physics->Settings;
                    
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
        ImGui::PopID();
    }

    /**
     * @brief Computes the viewport rectangle for the ImGui window
     * @return the viewport rectangle
     * This function computes the viewport rectangle for the ImGui window
     * by getting the window position and content region min/max.
     */
    static SceneEditorLayer::ViewportRect ComputeViewportRect()
    {
        const ImVec2 winPos = ImGui::GetWindowPos();
        const ImVec2 crMin  = ImGui::GetWindowContentRegionMin();
        const ImVec2 crMax  = ImGui::GetWindowContentRegionMax();
        return { { winPos.x + crMin.x, winPos.y + crMin.y }, { winPos.x + crMax.x, winPos.y + crMax.y } };
    }

    /**
     * @brief Computes the screen coordinates for a given point in world space
     * @param p the point in world space
     * @param VP the view projection matrix
     * @param rect the viewport rectangle
     * @param[out] out the screen coordinates
     * @return true if the computation was successful, false otherwise
     * This function computes the screen coordinates for a given point in world space
     * by transforming the point with the view projection matrix and then
     * normalizing the resulting coordinates. The normalized coordinates are then
     * mapped to the viewport rectangle coordinates.
     */
    static bool WorldToScreen(const glm::vec3& p, const glm::mat4& VP, const SceneEditorLayer::ViewportRect& rect, ImVec2& out)
    {
        glm::vec4 clip = VP * glm::vec4(p, 1.0f);
        if (clip.w <= 0.0001f) return false;
        glm::vec3 ndc = glm::vec3(clip) / clip.w;  
        if (ndc.x < -1.2f || ndc.x > 1.2f || ndc.y < -1.2f || ndc.y > 1.2f) return false;

        out.x = rect.min.x + (ndc.x * 0.5f + 0.5f) * rect.width();
        out.y = rect.min.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * rect.height();
        return true;
    }

    /**
     * @brief builds a ray from a mouse position in framebuffer space
     * @param mouseFB the mouse position in framebuffer space
     * @param fbSize the size of the framebuffer
     * @param view the view matrix
     * @param proj the projection matrix
     * @return a ray in world space
     * This function builds a ray from a mouse position in framebuffer space
     * by transforming the mouse position into normalized device coordinates,
     * and then transforming those coordinates into world space using the
     * view and projection matrices.
     */
    static SceneEditorLayer::RayWS BuildMouseRayFromFB(const glm::vec2& mouseFB, const glm::vec2& fbSize, const glm::mat4& view, const glm::mat4& proj)
    {
        const float ndcX =  (mouseFB.x / fbSize.x) * 2.0f - 1.0f;
        const float ndcY =  (mouseFB.y / fbSize.y) * 2.0f - 1.0f;

        const glm::mat4 invVP = glm::inverse(proj * view);

        glm::vec4 pNear = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 pFar  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);
        pNear /= pNear.w;
        pFar  /= pFar.w;

        SceneEditorLayer::RayWS r;
        r.Origin    = glm::vec3(pNear);
        r.Direction = glm::normalize(glm::vec3(pFar - pNear));
        return r;
    }

    /**
     * @brief creates a rotation matrix from a direction vector and an up hint vector
     * @param dir the direction vector
     * @param upHint the up hint vector, defaults to {0,1,0}
     * @return a rotation matrix
     * This function creates a rotation matrix from a direction vector and an up hint vector.
     * The direction vector is used to compute the forward direction of the rotation matrix.
     * The up hint vector is used to compute the right and up directions of the rotation matrix.
     * If the up hint vector is close to parallel to the direction vector, a default up vector is used instead.
     */
    static glm::mat3 MakeRotationFromDirection(const glm::vec3& dir, const glm::vec3& upHint = {0,1,0})
    {
        glm::vec3 fwd   = glm::normalize(-dir);
        glm::vec3 right = glm::cross(upHint, fwd);
        if (glm::length2(right) < 1e-8f)
        {
            const glm::vec3 altUp = std::abs(upHint.y) > 0.5f ? glm::vec3(0,0,1) : glm::vec3(0,1,0);
            right = glm::cross(altUp, fwd);
        }
        right = glm::normalize(right);
        const glm::vec3 up = glm::normalize(glm::cross(fwd, right));
        return { right, up, fwd };
    }

    /**
     * @brief extracts the forward direction from a 4x4 matrix
     * @param M the 4x4 matrix
     * @return the forward direction as a glm::vec3
     * This function extracts the forward direction from a 4x4 matrix by normalizing the second column of the matrix and negating it.
     */
    static glm::vec3 ExtractDirectionFromMatrix(const glm::mat4& M)
    {
        const glm::vec3 fwd = glm::normalize(glm::vec3(M[2]));
        return -fwd;
    }
    
    /**
     * @brief returns a dummy position for a directional light based on the camera's position and direction
     * @param cam the camera
     * @param distance the distance from the camera's position to the dummy position, defaults to 6.0f
     * @return a dummy position for a directional light
     * This function returns a dummy position for a directional light based on the camera's position and direction.
     * The dummy position is computed by moving along the camera's forward direction by the specified distance.
     */
    static glm::vec3 ChooseDummyPosition(const Camera3D& cam, float distance = 6.0f)
    {
        const glm::mat4 invView = glm::inverse(cam.View);
        const glm::vec3 camFwd  = glm::normalize(glm::vec3(invView[2]) * -1.0f);
        return cam.Position + camFwd * distance;
    }


    /**
     * @brief draws a directional light in the scene editor
     * @param light the light to be drawn
     * @param camera the camera used for drawing
     * @param rect the viewport rect
     * @param dl the draw list
     * @param cfg the config for drawing the light
     * @return true if the light was changed, false otherwise
     * This function draws a directional light in the scene editor.
     * It uses ImGuizmo to draw the light and its direction.
     * The light is represented as a billboard with a direction arrow.
     * The direction arrow is drawn from the light's position to a point on the direction vector.
     * The length of the direction arrow is configurable.
     * The function also draws optional rays from the light's position in the direction of the light.
     * The number of rays is configurable.
     * The function returns true if the light was changed, false otherwise.
     */
    static bool DrawDirectionalLight(ScenePhysicsWorld::WorldLighting& light, const Camera3D& camera, const SceneEditorLayer::ViewportRect& rect, ImDrawList* dl, const SceneEditorLayer::LightGizmoConfig& cfg)
    {
        ImGuizmo::PushID(cfg.GizmoId);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(dl);
        ImGuizmo::SetRect(rect.min.x, rect.min.y, rect.width(), rect.height());
        ImGuizmo::AllowAxisFlip(cfg.AllowAxisFlip);
        ImGuizmo::SetGizmoSizeClipSpace(cfg.GizmoSizeClip);

        const glm::vec3 pos = ChooseDummyPosition(camera, cfg.CameraDistance);
        const glm::mat3 R   = MakeRotationFromDirection(light.Direction);

        glm::mat4 model(1.0f);
        model[0] = glm::vec4(R[0], 0.0f);
        model[1] = glm::vec4(R[1], 0.0f);
        model[2] = glm::vec4(R[2], 0.0f);
        model[3] = glm::vec4(pos,   1.0f);
        if (cfg.IconScale != 1.0f)
            model = model * glm::scale(glm::mat4(1.0f), glm::vec3(cfg.IconScale));

        const glm::mat4 view = camera.View;
        const glm::mat4 proj = camera.Projection;

        bool changed = false;
        const ImGuizmo::MODE rotMode = cfg.UseLocalSpace ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::ROTATE, rotMode, glm::value_ptr(model)))
        {
            glm::vec3 c0 = glm::vec3(model[0]);
            glm::vec3 c1 = glm::vec3(model[1]);
            glm::vec3 c2 = glm::vec3(model[2]);

            if (glm::length2(c0) > 0) model[0] = glm::vec4(glm::normalize(c0), 0.0f);
            if (glm::length2(c1) > 0) model[1] = glm::vec4(glm::normalize(c1), 0.0f);
            if (glm::length2(c2) > 0) model[2] = glm::vec4(glm::normalize(c2), 0.0f);

            glm::vec3 newDir = ExtractDirectionFromMatrix(model);
            if (glm::length2(newDir) > 0.0f)
            {
                if (cfg.LockToViewAxis)
                {
                    const glm::vec3 camFwd = -glm::vec3(glm::inverse(view)[2]);
                    const glm::vec3 axis   = glm::normalize(camFwd);
                    newDir = glm::normalize(newDir - axis * glm::dot(newDir, axis));
                }

                light.Direction = glm::normalize(newDir);
                changed = true;
            }
        }

        if (cfg.DrawBillboard)
        {
            const glm::mat4 VP = proj * view;

            auto worldToScreen = [&](const glm::vec3& p) -> ImVec2
            {
                glm::vec4 clip = VP * glm::vec4(p, 1.0f);
                const float iw = (clip.w == 0.0f) ? SceneEditorLayer::EPSILON : clip.w;
                const glm::vec3 ndc = glm::vec3(clip) / iw;
                ImVec2 out{};
                out.x = rect.min.x + (ndc.x * 0.5f + 0.5f) * rect.width();
                out.y = rect.min.y + (-ndc.y * 0.5f + 0.5f) * rect.height();
                return out;
            };

            const glm::vec3 iconPos   = pos;
            const glm::vec3 iconAhead = pos + glm::normalize(-light.Direction) * cfg.IconLength;

            dl->AddCircleFilled(worldToScreen(iconPos), 4.0f * cfg.IconScale, cfg.IconColor);
            dl->AddLine(worldToScreen(iconPos), worldToScreen(iconAhead), cfg.IconColor, 2.0f * cfg.IconScale);

            if (cfg.DrawRays)
            {
                const glm::vec3 fwd = glm::normalize(-light.Direction);
                glm::vec3 t = glm::normalize(glm::cross(fwd, glm::vec3(0,1,0)));
                if (glm::length2(t) < 1e-5f) t = glm::vec3(1,0,0);
                const glm::vec3 b = glm::normalize(glm::cross(fwd, t));

                for (int i = 0; i < cfg.RayCount; ++i)
                {
                    const float a = (glm::two_pi<float>() / cfg.RayCount) * i;
                    const glm::vec3 dir = glm::normalize(t * std::cos(a) + b * std::sin(a));
                    const glm::vec3 a0 = iconPos + dir * (0.2f * cfg.IconScale);
                    const glm::vec3 a1 = iconPos + dir * (0.2f + cfg.RayLength) * cfg.IconScale;
                    dl->AddLine(worldToScreen(a0), worldToScreen(a1), cfg.IconColor, 1.0f);
                }
            }
        }

        ImGuizmo::PopID();
        return changed;
    }

    /**
     * @brief Render the scene viewport.
     * @details This function renders the scene viewport. It draws the scene using the
     *          current camera and frame buffer. It also renders the gizmos and
     *          handles the input for the gizmos.
     * @param context The scene context.
     */
    void SceneEditorLayer::RenderViewport(SceneContext& context)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        ImGui::Begin("Scene Viewport");
        {
            context.View->ViewportFocusedOrHovered = ImGui::IsWindowFocused() || 
                ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

            const ImVec2 vpAvail = ImGui::GetContentRegionAvail();
            if (FrameTextureID tex = context.View->FrameTexturePtr; tex != 0)
                ImGui::Image((ImTextureID)tex, vpAvail, ImVec2(0, 1), ImVec2(1, 0));
            else
                ImGui::Dummy(vpAvail);

            const ViewportRect rect = ComputeViewportRect();

            const ImVec2 mouse      = ImGui::GetMousePos();
            ImDrawList* windowDL    = ImGui::GetWindowDrawList();

            if(!context.Simulation->InSimulation)
            {
                const Camera3D& camera      = context.View->Camera;
                const glm::mat4& view       = camera.View;
                const glm::mat4& projection = camera.Projection;

                bool canPickEntites = false;
                canPickEntites |= ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                canPickEntites |= ImGui::IsWindowFocused();
                canPickEntites &= ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                canPickEntites |= !ImGuizmo::IsUsing();

                if(canPickEntites && context.View->ViewportFocusedOrHovered)
                {
                    glm::vec2 local = { mouse.x - rect.min.x, mouse.y - rect.min.y };
                    local.y = vpAvail.y - local.y;
                    
                    const auto& fbSpecs = context.View->FrameSpecification;
                    const glm::vec2 fbSize = { (float)fbSpecs.Width, (float)fbSpecs.Height };
                    const glm::vec2 mouseMapFB = { local.x * (fbSize.x / vpAvail.x), local.y * (fbSize.y / vpAvail.y) };
                    const RayWS ray = BuildMouseRayFromFB(mouseMapFB, fbSize, view, projection);

                    static constexpr float MAX_DISTANCE = 5000.0f;
                    const glm::vec3 P0 = ray.Origin;
                    const glm::vec3 P1 = ray.Origin + ray.Direction * MAX_DISTANCE;

                    RayHitResults result{};
                    const bool hit = RaycastFirstHit(context.Physics->World, P0, P1, result);
                    if (hit) 
                    {
                        m_Scene->SelectedEntity(result.Entity);
                    }
                }

                static GizmoState gizmo;
                gizmo.HandleHotkeys();
                ImGuizmo::SetDrawlist(windowDL);
                ImGuizmo::SetRect(rect.min.x, rect.min.y, rect.width(), rect.height());
                ImGuizmo::SetGizmoSizeClipSpace(0.18f);
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::AllowAxisFlip(false);

                bool gizmoConsumedInput = false;
                float snapTriplet[3] = {0,0,0};
                gizmo.FillSnapTriplet(snapTriplet);
                const float* snapPtr = (snapTriplet[0] != 0 || snapTriplet[1] != 0 
                    || snapTriplet[2] != 0) ? snapTriplet : nullptr;

                if (context.Entities->SelectedEntity != entt::null)
                {
                    const bool hasTransform  = context.Entities->Registry.any_of<TransformComponent>(context.Entities->SelectedEntity);
                    TagComponent* tag        = context.Entities->Registry.try_get<TagComponent>(context.Entities->SelectedEntity);
                    const bool isActive      = tag ? tag->IsActive : false;

                    if (canPickEntites && ImGui::IsMouseClicked(ImGuiPopupFlags_MouseButtonMiddle))
                    {
                        ImGui::OpenPopup("ViewportContextMenu");
                    }

                    if (ImGui::BeginPopupContextWindow("ViewportContextMenu", ImGuiPopupFlags_MouseButtonMiddle))
                    {
                        ImGui::Indent(5.0f);
                        if(ImGui::MenuItem("Delete"))
                        {
                            m_Scene->DestroyEntity(context.Entities->SelectedEntity, true);
                            ImGui::CloseCurrentPopup();
                        }

                        if(ImGui::MenuItem("Duplicate"))
                        {
                            m_Scene->DuplicateEntity(context.Entities->SelectedEntity);
                            ImGui::CloseCurrentPopup();
                        }

                        if(tag->IsActive)
                        {
                            ImGui::Separator();
                            if(ImGui::MenuItem("Hide"))
                            {
                                tag->IsActive = false;
                                ImGui::CloseCurrentPopup();
                            }
                        }

                        ImGui::Separator();

                        if (std::find(m_SimulationWatchList.begin(), m_SimulationWatchList.end(), context.Entities->SelectedEntity) == m_SimulationWatchList.end())
                        {
                            if(ImGui::MenuItem("Add To Watchlist"))
                            {
                                m_SimulationWatchList.push_back(context.Entities->SelectedEntity);
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        else
                        {
                            if(ImGui::MenuItem("Remove From Watchlist"))
                            {
                                m_SimulationWatchList.erase(std::remove(m_SimulationWatchList.begin(), m_SimulationWatchList.end(), context.Entities->SelectedEntity), m_SimulationWatchList.end());
                                ImGui::CloseCurrentPopup();
                            }
                        }

                        ImGui::Separator();

                        if(ImGui::MenuItem("Open In Material Editor"))
                        {
                            if (context.Entities->Registry.try_get<MaterialComponent>(context.Entities->SelectedEntity))
                            {
                                openMaterialEditors[context.Entities->SelectedEntity] = true;
                            }
                        }

                        ImGui::Unindent(5.0f);
                        ImGui::EndPopup();
                    }

                    for (auto it = openMaterialEditors.begin(); it != openMaterialEditors.end();)
                    {
                        entt::entity entity = it->first;
                        bool& isOpen = it->second;

                        auto* material = context.Entities->Registry.try_get<MaterialComponent>(entity);
                        if (!material)
                        {
                            it = openMaterialEditors.erase(it);
                            continue;
                        }
                        
                        std::string windowName = "Material Editor##" + std::to_string((uint32_t)entity);
                        ImGui::SetNextWindowSize(ImVec2(600.0f, 400.0f), ImGuiCond_FirstUseEver);
                        
                        if (ImGui::Begin(windowName.c_str(), &isOpen, ImGuiWindowFlags_NoDocking))
                        {
                            if (ImGui::BeginChild("##inspector-area", ImVec2(0.0f, 0.0f)))
                            {
                                if (material->MaterialPointer)
                                    DrawMaterialUI(material->MaterialPointer);
                                else
                                    ImGui::TextDisabled("No material assigned");
                                
                                ImGui::EndChild();
                            }
                            ImGui::End();
                        }
                        
                        if (!isOpen)
                        {
                            it = openMaterialEditors.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }

                    if (isActive && hasTransform)
                    {
                        ImGuizmo::PushID(1);
                        auto& TRS = context.Entities->Registry.get<TransformComponent>(context.Entities->SelectedEntity);

                        glm::vec3 T = TRS.Translation;
                        glm::vec3 S = TRS.Scale;

                        glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(TRS.Rotation));
                        auto wrap180 = [](float a)
                        {
                            a = std::fmod(a + 180.0f, 360.0f);
                            if (a < 0) a += 360.0f;
                            return a - 180.0f;
                        };

                        eulerDeg.x = wrap180(eulerDeg.x);
                        eulerDeg.y = wrap180(eulerDeg.y);
                        eulerDeg.z = wrap180(eulerDeg.z);

                        glm::mat4 transform{1.0f};
                        ImGuizmo::RecomposeMatrixFromComponents(&T.x, &eulerDeg.x, &S.x, glm::value_ptr(transform));

                        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), gizmo.Operation, gizmo.Mode, glm::value_ptr(transform), nullptr, snapPtr))
                        {
                            gizmoConsumedInput = true;

                            float Td[3], RdDeg[3], Sd[3];
                            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform), Td, RdDeg, Sd);

                            TRS.Translation = { Td[0], Td[1], Td[2] };
                            TRS.Scale       = { Sd[0], Sd[1], Sd[2] };

                            const glm::vec3 RdRad = glm::radians(glm::vec3(RdDeg[0], RdDeg[1], RdDeg[2]));
                            const glm::quat q     = glm::normalize(glm::quat(RdRad));
                            if (glm::any(glm::epsilonNotEqual(q, TRS.Rotation, 1e-6f))) TRS.Rotation = q;
                        }

                        ImGuizmo::PopID();
                    }
                }

                if (m_PhysicsAnalysis.SelectedEntity != entt::null && context.Entities->Registry.valid(m_PhysicsAnalysis.SelectedEntity))
                {
                    auto* transform = context.Entities->Registry.try_get<TransformComponent>(m_PhysicsAnalysis.SelectedEntity);
                    auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(m_PhysicsAnalysis.SelectedEntity);
                    
                    if (transform && rb)
                    {
                        // Draw force vectors
                        if (m_PhysicsAnalysis.ShowForceAnalysisPanel && 
                            m_PhysicsAnalysis.ForceAnalysis.ShowForceVectors)
                        {
                            for (const auto& force : m_PhysicsAnalysis.ForceAnalysis.Forces)
                            {
                                if (force.IsActive)
                                {
                                    DrawForceVector(transform->Translation, force.Force, 
                                                force.Color, m_PhysicsAnalysis.ForceAnalysis.VectorScale);
                                }
                            }
                            
                            // Draw net force
                            if (m_PhysicsAnalysis.ForceAnalysis.ShowNetForce)
                            {
                                DrawForceVector(transform->Translation, 
                                            m_PhysicsAnalysis.ForceAnalysis.NetForce,
                                            IM_COL32(255, 255, 0, 255), 
                                            m_PhysicsAnalysis.ForceAnalysis.VectorScale * 1.5f);
                            }
                        }
                        
                        // Draw momentum vector
                        if (m_PhysicsAnalysis.ShowMomentumPanel && 
                            m_PhysicsAnalysis.Momentum.ShowMomentumVector)
                        {
                            DrawMomentumVector(transform->Translation, 
                                            m_PhysicsAnalysis.Momentum.LinearMomentum,
                                            IM_COL32(200, 100, 255, 255),
                                            m_PhysicsAnalysis.Momentum.VectorScale);
                        }
                        
                        // Draw acceleration vector
                        if (m_PhysicsAnalysis.ShowAccelerationPanel && 
                            m_PhysicsAnalysis.Acceleration.ShowVector)
                        {
                            glm::vec3 accelVector = m_PhysicsAnalysis.Acceleration.CurrentAcceleration * 
                                                m_PhysicsAnalysis.Acceleration.VectorScale;
                            DrawForceVector(transform->Translation, accelVector,
                                        m_PhysicsAnalysis.Acceleration.VectorColor,
                                        1.0f);
                        }
                        
                        // Draw trajectory path
                        if (m_PhysicsAnalysis.ShowTrajectoryPanel && 
                            m_PhysicsAnalysis.Trajectory.ShowPredictionPath &&
                            !m_PhysicsAnalysis.Trajectory.PredictedPath.empty())
                        {
                            DrawTrajectoryPath(m_PhysicsAnalysis.Trajectory.PredictedPath,
                                            m_PhysicsAnalysis.Trajectory.PathColor);
                        }
                    }
                }
                
                // Draw collision points
                if (m_PhysicsAnalysis.ShowCollisionPanel && 
                    m_PhysicsAnalysis.Collisions.ShowCollisionPoints)
                {
                    for (const auto& collision : m_PhysicsAnalysis.Collisions.RecentCollisions)
                    {
                        DrawCollisionPoint(collision.CollisionPoint, 0.2f, IM_COL32(255, 100, 100, 200));
                    }
                }

                const bool clutchHide = ImGui::IsKeyDown(ImGuiKey_4);
                static LightGizmoConfig lightCfg;
                lightCfg.Enabled = context.Physics->SunLight.ShowGuizmo && !clutchHide;

                if (!gizmoConsumedInput && lightCfg.Enabled)
                {
                    context.Physics->SunLight.ShowGuizmo = true;
                    (void)DrawDirectionalLight(context.Physics->SunLight, camera, rect, windowDL, lightCfg);
                }
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }


    void RenderEducationalInfoBox(const char* icon, const char* title, const char* text, float width = 400.0f)
    {
        // Calculate sizes
        ImVec2 titleSize = ImGui::CalcTextSize(title);
        ImVec2 textSize = ImGui::CalcTextSize(text, nullptr, false, width - 30.0f);
        
        float iconSize = 20.0f;
        float titleHeight = std::max(iconSize, titleSize.y);
        float totalHeight = titleHeight + 10.0f + textSize.y + 20.0f;
        
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.25f, 0.35f, 0.9f));
        if (ImGui::BeginChild("##EduBox", ImVec2(width, totalHeight), true))
        {
            // Icon and title
            ImGui::SetCursorPosY(10.0f);
            ImGui::SetCursorPosX(10.0f);
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
            ImGui::Text("%s", icon);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", title);
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Body text
            ImGui::SetCursorPosX(10.0f);
            ImGui::PushTextWrapPos(width - 10.0f);
            ImGui::TextWrapped("%s", text);
            ImGui::PopTextWrapPos();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }


    /**
     * @brief Renders a window for analyzing force vectors acting on an object.
     *
     * This window allows the user to visualize and analyze force vectors acting on an object.
     * Features include showing force vectors, net force, and components.
     * The user can select an object in the simulation watchlist to analyze its data.
     * The analysis is split into two sections: individual forces and net force.
     * The individual forces section shows the active forces and their magnitude.
     * The net force section shows the vector sum of all forces acting on the object.
     * A third section allows the user to compare the two types of motion.
     *
     * @param[in] context The current scene context.
     */
    void SceneEditorLayer::RenderForceAnalysisPanel(SceneContext& context)
    {
        if (!m_PhysicsAnalysis.ShowForceAnalysisPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Force Analysis", &m_PhysicsAnalysis.ShowForceAnalysisPanel, 
                     ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Force Vector Analysis");
        ImGui::Separator();
        ImGui::Spacing();
        
        auto& analysis = m_PhysicsAnalysis.ForceAnalysis;
        
        ImGui::Text("Visualization");
        ImGui::Checkbox("Show Force Vectors", &analysis.ShowForceVectors);
        ImGui::SameLine(); 
        ShowPhysicsTooltip("Force Vectors", "Display arrows representing the magnitude and direction of forces acting on the object");
        
        ImGui::Checkbox("Show Net Force", &analysis.ShowNetForce);
        ImGui::SameLine(); 
        ShowPhysicsTooltip("Net Force", "The vector sum of all forces acting on the object (resultant force)");
        
        ImGui::Checkbox("Show Components", &analysis.ShowComponents);
        ImGui::SameLine(); 
        ShowPhysicsTooltip("Components", "Break down forces into X, Y, and Z components");
        
        ImGui::SliderFloat("Vector Scale", &analysis.VectorScale, 0.1f, 5.0f, "%.2fx");
        ImGui::SliderFloat("Arrow Size", &analysis.ArrowHeadSize, 0.05f, 0.5f);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Net Force");
        ImGui::Text("Magnitude: %.2f N", analysis.NetForceMagnitude);
        
        if (analysis.NetForceMagnitude > EPSILON)
        {
            ImGui::Text("Direction: X: %.2f, Y: %.2f, Z: %.2f", 
                       analysis.NetForce.x, analysis.NetForce.y, analysis.NetForce.z);
            
            glm::vec3 unitDir = glm::normalize(analysis.NetForce);
            ImGui::Text("Unit Vector: (%.3f, %.3f, %.3f)", 
                       unitDir.x, unitDir.y, unitDir.z);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                              "Object is in equilibrium (ΣF = 0)");
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        

        ImGui::Text("Active Forces (%zu)", analysis.Forces.size());    
        if (ImGui::BeginChild("ForcesList", ImVec2(400, 250), true))
        {
            for (size_t i = 0; i < analysis.Forces.size(); ++i)
            {
                auto& force = analysis.Forces[i];
                
                ImGui::PushID(static_cast<int>(i));
                
                // Force name with color indicator
                ImGui::ColorButton("##color", ImGui::ColorConvertU32ToFloat4(force.Color), 
                                  ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20));
                ImGui::SameLine();
                
                ImGui::Checkbox(force.Name.c_str(), &force.IsActive);
                
                if (force.IsActive)
                {
                    ImGui::Indent(30);
                    ImGui::Text("Magnitude: %.2f N", force.GetMagnitude());
                    ImGui::Text("Force: (%.2f, %.2f, %.2f) N", 
                               force.Force.x, force.Force.y, force.Force.z);
                    
                    if (analysis.ShowComponents && force.GetMagnitude() > EPSILON)
                    {
                        ImGui::Text("Components:");
                        ImGui::BulletText("Fx: %.2f N", force.Force.x);
                        ImGui::BulletText("Fy: %.2f N", force.Force.y);
                        ImGui::BulletText("Fz: %.2f N", force.Force.z);
                    }
                    ImGui::Unindent(30);
                }
                
                ImGui::PopID();
                
                if (i < analysis.Forces.size() - 1)
                    ImGui::Separator();
            }
        }
        ImGui::EndChild();
        
        ImGui::Spacing();

        RenderEducationalInfoBox(nullptr, "Force Info", "Forces are vector quantities that cause acceleration. "
                          "The net force determines how an object moves according to Newton's Second Law (F=ma).");
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
    
    /**
     * @brief Renders a window for tracking the energy of objects in a simulation.
     *
     * This window allows the user to analyze the energy data of objects in a simulation.
     * Features include tracking kinetic, potential, and total energy over time, displaying
     * this data in a graph, and calculating the energy conservation of the simulation.
     * The user can choose to display the kinetic energy, potential energy, or total energy of the
     * objects in the simulation.
     *
     * @param[in] context The current scene context.
     */
    void SceneEditorLayer::RenderEnergyTrackingPanel(SceneContext& context)
    {
        if (!m_PhysicsAnalysis.ShowEnergyPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Energy Analysis", &m_PhysicsAnalysis.ShowEnergyPanel);
        
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Energy Tracking");
        ImGui::Separator();
        ImGui::Spacing();
        
        auto& energy = m_PhysicsAnalysis.Energy;
        
        // Enable/Disable Tracking
        ImGui::Checkbox("Enable Tracking", &energy.TrackingEnabled);
        ImGui::SameLine(); ShowPhysicsTooltip("Energy Tracking", 
            "Record kinetic and potential energy over time to analyze energy conservation");
        
        ImGui::Spacing();
        
        // Current Energy Values - Large Display
        ImGui::BeginGroup();
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.5f, 1.0f));
            ImGui::Text("Kinetic Energy");
            ImGui::PopStyleColor();
            ImGui::SameLine(200);
            ImGui::Text("%.2f J", energy.CurrentKE);
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));
            ImGui::Text("Potential Energy");
            ImGui::PopStyleColor();
            ImGui::SameLine(200);
            ImGui::Text("%.2f J", energy.CurrentPE);
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
            ImGui::Text("Total Energy");
            ImGui::PopStyleColor();
            ImGui::SameLine(200);
            ImGui::Text("%.2f J", energy.CurrentTotal);
        }
        ImGui::EndGroup();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Conservation Analysis
        float conservation = energy.GetEnergyConservation();
        ImGui::Text("Energy Conservation: ");
        ImGui::SameLine();
        
        ImVec4 conservationColor;
        if (conservation > 95.0f)
            conservationColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // Green - excellent
        else if (conservation > 85.0f)
            conservationColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow - good
        else
            conservationColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red - poor
        
        ImGui::TextColored(conservationColor, "%.1f%%", conservation);
        
        if (energy.InitialTotal > EPSILON)
        {
            ImGui::Text("Initial Total: %.2f J", energy.InitialTotal);
            ImGui::Text("Energy Lost: %.2f J", energy.EnergyLoss);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Energy Graph
        ImGui::Text("Energy Over Time");
        
        ImGui::Checkbox("Show KE", &energy.ShowKE);
        ImGui::SameLine();
        ImGui::Checkbox("Show PE", &energy.ShowPE);
        ImGui::SameLine();
        ImGui::Checkbox("Show Total", &energy.ShowTotal);
        
        if (!energy.TimeStamps.empty() && energy.TimeStamps.size() == energy.TotalEnergyHistory.size())
        {
            // Convert deque to vector for plotting
            std::vector<ImVec2> keData, peData, totalData;
            
            for (size_t i = 0; i < energy.TimeStamps.size(); ++i)
            {
                float time = energy.TimeStamps[i];
                if (energy.ShowKE && i < energy.KineticEnergyHistory.size())
                    keData.push_back(ImVec2(time, energy.KineticEnergyHistory[i]));
                if (energy.ShowPE && i < energy.PotentialEnergyHistory.size())
                    peData.push_back(ImVec2(time, energy.PotentialEnergyHistory[i]));
                if (energy.ShowTotal && i < energy.TotalEnergyHistory.size())
                    totalData.push_back(ImVec2(time, energy.TotalEnergyHistory[i]));
            }
            
            // Find max energy for scaling
            float maxEnergy = 1.0f;
            if (!totalData.empty())
            {
                for (const auto& point : totalData)
                    maxEnergy = std::max(maxEnergy, point.y);
            }
            
            ImVec2 graphSize(ImGui::GetContentRegionAvail().x, 200);
            
            if (ImGui::BeginChild("EnergyGraph", graphSize, true))
            {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 graphMin = ImGui::GetCursorScreenPos();
                ImVec2 graphMax = ImVec2(graphMin.x + graphSize.x - 20, graphMin.y + graphSize.y - 20);
                
                // Draw grid
                drawList->AddRectFilled(graphMin, graphMax, IM_COL32(20, 20, 25, 255));
                
                // Helper to convert data to screen space
                auto toScreen = [&](const ImVec2& dataPoint) -> ImVec2
                {
                    float timeRange = energy.TimeStamps.empty() ? 1.0f : 
                        (energy.TimeStamps.back() - energy.TimeStamps.front());
                    if (timeRange < EPSILON) timeRange = 1.0f;
                    
                    float x = graphMin.x + ((dataPoint.x - energy.TimeStamps.front()) / timeRange) * 
                             (graphMax.x - graphMin.x);
                    float y = graphMax.y - (dataPoint.y / maxEnergy) * (graphMax.y - graphMin.y);
                    return ImVec2(x, y);
                };
                
                // Plot lines
                if (energy.ShowKE && keData.size() > 1)
                {
                    for (size_t i = 0; i < keData.size() - 1; ++i)
                    {
                        ImVec2 p1 = toScreen(keData[i]);
                        ImVec2 p2 = toScreen(keData[i + 1]);
                        drawList->AddLine(p1, p2, IM_COL32(50, 255, 100, 255), 2.0f);
                    }
                }
                
                if (energy.ShowPE && peData.size() > 1)
                {
                    for (size_t i = 0; i < peData.size() - 1; ++i)
                    {
                        ImVec2 p1 = toScreen(peData[i]);
                        ImVec2 p2 = toScreen(peData[i + 1]);
                        drawList->AddLine(p1, p2, IM_COL32(100, 150, 255, 255), 2.0f);
                    }
                }
                
                if (energy.ShowTotal && totalData.size() > 1)
                {
                    for (size_t i = 0; i < totalData.size() - 1; ++i)
                    {
                        ImVec2 p1 = toScreen(totalData[i]);
                        ImVec2 p2 = toScreen(totalData[i + 1]);
                        drawList->AddLine(p1, p2, IM_COL32(255, 200, 50, 255), 2.0f);
                    }
                }
                
                // Draw border
                drawList->AddRect(graphMin, graphMax, IM_COL32(100, 100, 100, 255), 0, 0, 1.5f);
                
                // Labels
                ImGui::SetCursorScreenPos(ImVec2(graphMin.x + 5, graphMin.y + 5));
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%.1f J", maxEnergy);
                
                ImGui::SetCursorScreenPos(ImVec2(graphMin.x + 5, graphMax.y - 15));
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "0 J");
            }
            ImGui::EndChild();
            
            // Legend
            ImGui::Spacing();
            if (energy.ShowKE)
            {
                ImGui::ColorButton("##ke", ImVec4(0.2f, 1.0f, 0.4f, 1.0f), 
                                  ImGuiColorEditFlags_NoTooltip, ImVec2(15, 15));
                ImGui::SameLine();
                ImGui::Text("Kinetic Energy");
                ImGui::SameLine(200);
            }
            if (energy.ShowPE)
            {
                ImGui::ColorButton("##pe", ImVec4(0.4f, 0.6f, 1.0f, 1.0f), 
                                  ImGuiColorEditFlags_NoTooltip, ImVec2(15, 15));
                ImGui::SameLine();
                ImGui::Text("Potential Energy");
                ImGui::SameLine(200);
            }
            if (energy.ShowTotal)
            {
                ImGui::ColorButton("##total", ImVec4(1.0f, 0.8f, 0.2f, 1.0f), 
                                  ImGuiColorEditFlags_NoTooltip, ImVec2(15, 15));
                ImGui::SameLine();
                ImGui::Text("Total Energy");
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                              "Start simulation to track energy data");
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Settings
        ImGui::Text("Settings");
        ImGui::SliderFloat("Gravity", &energy.GravityMagnitude, 0.0f, 20.0f, "%.2f m/s²");
        
        ImGui::Spacing();
        
        // Action buttons
        if (ImGui::Button("Reset", ImVec2(120, 0)))
        {
            energy.Reset();
        }
        ImGui::Spacing();
        
        // Educational Info
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.25f, 0.35f, 0.9f));
        if (ImGui::BeginChild("EnergyInfo", ImVec2(-1, 100), true))
        {
            RenderPhysicsEquation("KE = ½mv²", "Kinetic Energy");
            RenderPhysicsEquation("PE = mgh", "Gravitational Potential Energy");
            ImGui::Spacing();
            ImGui::TextWrapped("Law of Conservation of Energy: In a closed system, "
                              "total energy remains constant. Energy can transform between "
                              "kinetic and potential forms.");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
    
    
    /**
     * @brief Renders a window for momentum analysis of physics data.
     *
     * This window allows the user to analyze momentum data with statistical tools.
     * Features include mean, median, standard deviation, min/max value detection,
     * peak detection for oscillations, numerical integration (area under curve),
     * sample statistics and distributions.
     *
     * The user can select an object in the simulation watchlist to analyze its data.
     * The analysis is split into two sections: linear motion and rotational motion.
     * The linear motion section shows the statistical analysis of the object's linear velocity.
     * The rotational motion section shows the statistical analysis of the object's angular velocity.
     * A third section allows the user to compare the two types of motion.
     *
     * @param[in] context The current scene context.
     */
    void SceneEditorLayer::RenderMomentumPanel(SceneContext& context)
    {
        if (!m_PhysicsAnalysis.ShowMomentumPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Momentum Analysis", &m_PhysicsAnalysis.ShowMomentumPanel);
        
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "Momentum & Impulse");
        ImGui::Separator();
        ImGui::Spacing();
        
        auto& momentum = m_PhysicsAnalysis.Momentum;
        
        // Current Momentum Display
        ImGui::Text("Linear Momentum");
        ImGui::Text("Magnitude: %.2f kg⋅m/s", momentum.LinearMagnitude);
        ImGui::Text("Direction: (%.2f, %.2f, %.2f)", 
                   momentum.LinearMomentum.x, momentum.LinearMomentum.y, momentum.LinearMomentum.z);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Conservation Analysis
        if (glm::length(momentum.InitialLinearMomentum) > EPSILON)
        {
            float conservation = momentum.GetConservationPercentage();
            ImGui::Text("Momentum Conservation: ");
            ImGui::SameLine();
            
            ImVec4 color = conservation > 95.0f ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : 
                          conservation > 85.0f ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : 
                          ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            
            ImGui::TextColored(color, "%.1f%%", conservation);
            
            ImGui::Text("Initial: %.2f kg⋅m/s", glm::length(momentum.InitialLinearMomentum));
            ImGui::Text("Current: %.2f kg⋅m/s", momentum.LinearMagnitude);
            
            float change = momentum.LinearMagnitude - glm::length(momentum.InitialLinearMomentum);
            ImGui::Text("Change: %.2f kg⋅m/s", change);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Visualization Options
        ImGui::Text("Visualization");
        ImGui::Checkbox("Show Momentum Vector", &momentum.ShowMomentumVector);
        ImGui::SameLine(); ShowPhysicsTooltip("Momentum Vector", 
            "Display the momentum vector from the object's center of mass");
        
        ImGui::Checkbox("Track Conservation", &momentum.TrackConservation);
        ImGui::SameLine(); ShowPhysicsTooltip("Conservation Tracking", 
            "Monitor how well momentum is conserved during collisions");
        
        ImGui::SliderFloat("Vector Scale", &momentum.VectorScale, 0.1f, 3.0f);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Momentum History Graph
        if (!momentum.MomentumHistory.empty() && !momentum.TimeStamps.empty())
        {
            ImGui::Text("Momentum Magnitude Over Time");
            
            std::vector<ImVec2> graphData;
            for (size_t i = 0; i < momentum.MomentumHistory.size() && i < momentum.TimeStamps.size(); ++i)
            {
                float mag = glm::length(momentum.MomentumHistory[i]);
                graphData.push_back(ImVec2(momentum.TimeStamps[i], mag));
            }
            
            if (graphData.size() > 1)
            {
                ImVec2 graphSize(ImGui::GetContentRegionAvail().x, 150);
                if (ImGui::BeginChild("MomentumGraph", graphSize, true))
                {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    ImVec2 graphMin = ImGui::GetCursorScreenPos();
                    ImVec2 graphMax = ImVec2(graphMin.x + graphSize.x - 20, graphMin.y + graphSize.y - 20);
                    
                    drawList->AddRectFilled(graphMin, graphMax, IM_COL32(20, 20, 25, 255));
                    
                    float maxMomentum = 1.0f;
                    for (const auto& point : graphData)
                        maxMomentum = std::max(maxMomentum, point.y);
                    
                    float timeRange = graphData.back().x - graphData.front().x;
                    if (timeRange < EPSILON) timeRange = 1.0f;
                    
                    for (size_t i = 0; i < graphData.size() - 1; ++i)
                    {
                        float x1 = graphMin.x + ((graphData[i].x - graphData.front().x) / timeRange) * 
                                  (graphMax.x - graphMin.x);
                        float y1 = graphMax.y - (graphData[i].y / maxMomentum) * (graphMax.y - graphMin.y);
                        
                        float x2 = graphMin.x + ((graphData[i+1].x - graphData.front().x) / timeRange) * 
                                  (graphMax.x - graphMin.x);
                        float y2 = graphMax.y - (graphData[i+1].y / maxMomentum) * (graphMax.y - graphMin.y);
                        
                        drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), 
                                        IM_COL32(200, 100, 255, 255), 2.0f);
                    }
                    
                    drawList->AddRect(graphMin, graphMax, IM_COL32(100, 100, 100, 255), 0, 0, 1.5f);
                }
                ImGui::EndChild();
            }
        }
        
        ImGui::Spacing();
        
        // Action Buttons
        if (ImGui::Button("Reset", ImVec2(120, 0)))
        {
            momentum.Reset();
        }
        
        ImGui::Spacing();
        
        // Educational Info
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.25f, 0.35f, 0.9f));
        if (ImGui::BeginChild("MomentumInfo", ImVec2(-1, 120), true))
        {
            RenderPhysicsEquation("p = mv", "Linear Momentum");
            RenderPhysicsEquation("Δp = FΔt", "Impulse-Momentum Theorem");
            ImGui::Spacing();
            ImGui::TextWrapped("Law of Conservation of Momentum: In a closed system with "
                              "no external forces, the total momentum remains constant. "
                              "This is especially evident in collisions.");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
    
    // ==================== COLLISION ANALYSIS PANEL ====================
    
    void SceneEditorLayer::RenderCollisionAnalysisPanel(SceneContext& context)
    {
        if (!m_PhysicsAnalysis.ShowCollisionPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Collision Analysis", &m_PhysicsAnalysis.ShowCollisionPanel);
        
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "Collision Analysis");
        ImGui::Separator();
        ImGui::Spacing();
        
        auto& collisions = m_PhysicsAnalysis.Collisions;
        
        // Enable Analysis
        ImGui::Checkbox("Enable Analysis", &collisions.EnableAnalysis);
        ImGui::SameLine(); ShowPhysicsTooltip("Collision Analysis", 
            "Record and analyze collision events including velocity changes and energy transfer");
        
        ImGui::Spacing();
        
        // Visualization Options
        ImGui::Checkbox("Show Collision Points", &collisions.ShowCollisionPoints);
        ImGui::Checkbox("Show Impulse Vectors", &collisions.ShowImpulseVectors);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Recent Collisions List
        ImGui::Text("Recent Collisions (%zu)", collisions.RecentCollisions.size());
        
        if (collisions.RecentCollisions.empty())
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                              "No collisions recorded yet");
        }
        else
        {
            if (ImGui::BeginChild("CollisionsList", ImVec2(-1, 300), true))
            {
                for (int i = static_cast<int>(collisions.RecentCollisions.size()) - 1; i >= 0; --i)
                {
                    const auto& event = collisions.RecentCollisions[i];
                    
                    ImGui::PushID(i);
                    
                    // Collision header with type indicator
                    ImVec4 typeColor;
                    switch (event.Type)
                    {
                        case CollisionEvent::CollisionType::Elastic:
                            typeColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
                            break;
                        case CollisionEvent::CollisionType::Inelastic:
                            typeColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                            break;
                        case CollisionEvent::CollisionType::PartiallyElastic:
                            typeColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
                            break;
                        default:
                            typeColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                    }
                    
                    ImGui::TextColored(typeColor, "%s Collision", event.GetTypeName());
                    ImGui::Text("Time: %.2f s", event.TimeStamp);
                    
                    if (ImGui::TreeNode("Details"))
                    {
                        ImGui::BulletText("Coefficient of Restitution: %.3f", event.CoefficientOfRestitution);
                        ImGui::BulletText("Relative Velocity: %.2f m/s", event.RelativeVelocity);
                        ImGui::BulletText("Impulse: %.2f N⋅s", event.ImpulseMagnitude);
                        
                        ImGui::Spacing();
                        ImGui::Text("Velocities Before:");
                        ImGui::Indent();
                        ImGui::Text("Object A: (%.2f, %.2f, %.2f) m/s", 
                                   event.VelocityABefore.x, event.VelocityABefore.y, event.VelocityABefore.z);
                        ImGui::Text("Object B: (%.2f, %.2f, %.2f) m/s", 
                                   event.VelocityBBefore.x, event.VelocityBBefore.y, event.VelocityBBefore.z);
                        ImGui::Unindent();
                        
                        ImGui::Spacing();
                        ImGui::Text("Velocities After:");
                        ImGui::Indent();
                        ImGui::Text("Object A: (%.2f, %.2f, %.2f) m/s", 
                                   event.VelocityAAfter.x, event.VelocityAAfter.y, event.VelocityAAfter.z);
                        ImGui::Text("Object B: (%.2f, %.2f, %.2f) m/s", 
                                   event.VelocityBAfter.x, event.VelocityBAfter.y, event.VelocityBAfter.z);
                        ImGui::Unindent();
                        
                        ImGui::Spacing();
                        ImGui::Text("Energy Analysis:");
                        ImGui::Indent();
                        ImGui::Text("KE Before: %.2f J", event.KEBefore);
                        ImGui::Text("KE After: %.2f J", event.KEAfter);
                        ImGui::Text("Energy Lost: %.2f J (%.1f%%)", 
                                   event.EnergyLoss, 
                                   event.KEBefore > EPSILON ? (event.EnergyLoss / event.KEBefore * 100.0f) : 0.0f);
                        ImGui::Unindent();
                        
                        ImGui::TreePop();
                    }
                    
                    ImGui::PopID();
                    
                    if (i > 0)
                        ImGui::Separator();
                }
            }
            ImGui::EndChild();
        }
        
        ImGui::Spacing();
        
        // Action Buttons
        if (ImGui::Button("Clear History", ImVec2(150, 0)))
        {
            collisions.Clear();
        }
        
        ImGui::Spacing();
        
        // Educational Info
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.25f, 0.35f, 0.9f));
        if (ImGui::BeginChild("CollisionInfo", ImVec2(-1, 130), true))
        {
            ImGui::TextWrapped("Collision Types:");
            ImGui::BulletText("Elastic (e ≈ 1): Kinetic energy conserved");
            ImGui::BulletText("Inelastic (e ≈ 0): Maximum energy lost, objects stick");
            ImGui::BulletText("Partially Elastic (0 < e < 1): Some energy lost");
            ImGui::Spacing();
            RenderPhysicsEquation("e = (v₂ - v₁) / (u₁ - u₂)", "Coefficient of Restitution");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
    
    // ==================== NEWTON'S LAWS PANEL ====================
    
    void SceneEditorLayer::RenderNewtonsLawsPanel(SceneContext& context)
    {
        if (!m_PhysicsAnalysis.ShowNewtonsLawsPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Newton's Laws Interactive", &m_PhysicsAnalysis.ShowNewtonsLawsPanel);
        
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Newton's Laws of Motion");
        ImGui::Separator();
        ImGui::Spacing();
        
        auto& laws = m_PhysicsAnalysis.NewtonsLaws;
        
        // Law Selector Tabs
        const char* lawNames[] = { "First Law", "Second Law", "Third Law" };
        ImGui::Text("Select Law:");
        for (int i = 0; i < 3; ++i)
        {
            if (i > 0) ImGui::SameLine();
            
            bool isSelected = (laws.CurrentLaw == i);
            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
            
            if (ImGui::Button(lawNames[i], ImVec2(150, 30)))
                laws.CurrentLaw = i;
            
            if (isSelected)
                ImGui::PopStyleColor();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Display selected law
        switch (laws.CurrentLaw)
        {
            case 0: // First Law - Inertia
            {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Newton's First Law: Law of Inertia");
                ImGui::Spacing();
                
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.15f, 0.2f, 1.0f));
                if (ImGui::BeginChild("FirstLawStatement", ImVec2(-1, 80), true))
                {
                    ImGui::TextWrapped("An object at rest stays at rest, and an object in motion "
                                      "stays in motion with the same velocity, unless acted upon "
                                      "by a net external force.");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                ImGui::Text("Demonstration Controls:");
                ImGui::Checkbox("Show Inertia Line", &laws.FirstLaw.ShowInertiaLine);
                ImGui::SameLine(); ShowPhysicsTooltip("Inertia Line", 
                    "Shows the path the object would follow without external forces");
                
                ImGui::Checkbox("Highlight Balanced Forces", &laws.FirstLaw.HighlightBalancedForces);
                ImGui::SameLine(); ShowPhysicsTooltip("Balanced Forces", 
                    "When forces are balanced (ΣF = 0), object maintains constant velocity");
                
                ImGui::SliderFloat("Friction", &laws.FirstLaw.FrictionCoefficient, 0.0f, 1.0f);
                
                ImGui::Spacing();
                ImGui::Text("Key Concepts:");
                ImGui::BulletText("Inertia: Resistance to changes in motion");
                ImGui::BulletText("Velocity remains constant when ΣF = 0");
                ImGui::BulletText("Friction acts to oppose motion");
                
                break;
            }
            
            case 1: // Second Law - F=ma
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Newton's Second Law: F = ma");
                ImGui::Spacing();
                
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.15f, 0.2f, 1.0f));
                if (ImGui::BeginChild("SecondLawStatement", ImVec2(-1, 60), true))
                {
                    ImGui::TextWrapped("The acceleration of an object is directly proportional to "
                                      "the net force acting on it and inversely proportional to its mass.");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                RenderPhysicsEquation("F = ma", "Force equals mass times acceleration");
                RenderPhysicsEquation("a = F/m", "Acceleration equals force divided by mass");
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Text("Interactive Controls:");
                ImGui::Checkbox("Show Force Equation", &laws.SecondLaw.ShowForceEquation);
                ImGui::Checkbox("Show Acceleration Vector", &laws.SecondLaw.ShowAccelerationVector);
                ImGui::Checkbox("Enable Mass Slider", &laws.SecondLaw.EnableMassSlider);
                
                if (laws.SecondLaw.EnableMassSlider)
                {
                    ImGui::SliderFloat("Target Mass", &laws.SecondLaw.TargetMass, 0.1f, 10.0f, "%.2f kg");
                    ImGui::Text("Change mass to see how it affects acceleration!");
                }
                
                ImGui::Spacing();
                ImGui::Text("Applied Force:");
                ImGui::DragFloat3("Force (N)", &laws.SecondLaw.AppliedForce.x, 0.1f, -100.0f, 100.0f);
                
                if (m_PhysicsAnalysis.SelectedEntity != entt::null && 
                    context.Entities->Registry.valid(m_PhysicsAnalysis.SelectedEntity))
                {
                    if (auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(m_PhysicsAnalysis.SelectedEntity))
                    {
                        float mass = rb->PhysicsBody->getMass();
                        if (laws.SecondLaw.EnableMassSlider)
                            mass = laws.SecondLaw.TargetMass;
                        
                        glm::vec3 acceleration = laws.SecondLaw.AppliedForce / mass;
                        float accelMag = glm::length(acceleration);
                        
                        ImGui::Spacing();
                        ImGui::Text("Results:");
                        ImGui::Text("Mass: %.2f kg", mass);
                        ImGui::Text("Acceleration: %.2f m/s²", accelMag);
                        ImGui::Text("Direction: (%.2f, %.2f, %.2f)", 
                                   acceleration.x, acceleration.y, acceleration.z);
                    }
                }
                
                ImGui::Spacing();
                ImGui::Text("Key Concepts:");
                ImGui::BulletText("Greater force → greater acceleration");
                ImGui::BulletText("Greater mass → less acceleration (same force)");
                ImGui::BulletText("Acceleration is in direction of net force");
                
                break;
            }
            
            case 2: // Third Law - Action-Reaction
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.8f, 1.0f), "Newton's Third Law: Action-Reaction");
                ImGui::Spacing();
                
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.15f, 0.2f, 1.0f));
                if (ImGui::BeginChild("ThirdLawStatement", ImVec2(-1, 60), true))
                {
                    ImGui::TextWrapped("For every action, there is an equal and opposite reaction. "
                                      "Forces always occur in pairs.");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                ImGui::Text("Visualization:");
                ImGui::Checkbox("Show Reaction Forces", &laws.ThirdLaw.ShowReactionForces);
                ImGui::SameLine(); ShowPhysicsTooltip("Reaction Forces", 
                    "Display force pairs acting on different objects");
                
                ImGui::Checkbox("Highlight Force Pairs", &laws.ThirdLaw.HighlightPairs);
                ImGui::SameLine(); ShowPhysicsTooltip("Force Pairs", 
                    "Highlight corresponding action-reaction force pairs");
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Text("Select two objects to see their interaction forces:");
                
                // Entity selectors (simplified - you'd implement proper entity picking)
                ImGui::Text("Object A: %s", laws.ThirdLaw.EntityA != entt::null ? "Selected" : "None");
                ImGui::Text("Object B: %s", laws.ThirdLaw.EntityB != entt::null ? "Selected" : "None");
                
                if (laws.ThirdLaw.EntityA != entt::null && laws.ThirdLaw.EntityB != entt::null)
                {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), 
                                      "Force pair detected! Watch the visualization.");
                }
                
                ImGui::Spacing();
                ImGui::Text("Key Concepts:");
                ImGui::BulletText("Forces always occur in pairs");
                ImGui::BulletText("Action and reaction are equal in magnitude");
                ImGui::BulletText("Action and reaction are opposite in direction");
                ImGui::BulletText("Forces act on different objects");
                
                ImGui::Spacing();
                ImGui::Text("Examples:");
                ImGui::BulletText("Rocket thrust: Gas pushed down, rocket pushed up");
                ImGui::BulletText("Walking: Foot pushes ground back, ground pushes foot forward");
                ImGui::BulletText("Collision: Both objects experience equal and opposite forces");
                
                break;
            }
        }
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
    
    // ==================== ACCELERATION PANEL ====================
    
    void SceneEditorLayer::RenderAccelerationPanel(SceneContext& context)
    {
        if (!m_PhysicsAnalysis.ShowAccelerationPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Acceleration Analysis", &m_PhysicsAnalysis.ShowAccelerationPanel);
        
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Acceleration Tracking");
        ImGui::Separator();
        ImGui::Spacing();
        
        auto& accel = m_PhysicsAnalysis.Acceleration;
        
        // Current Acceleration
        ImGui::Text("Current Acceleration");
        ImGui::Text("Magnitude: %.2f m/s²", accel.AccelerationMagnitude);
        ImGui::Text("Direction: (%.2f, %.2f, %.2f)", 
                   accel.CurrentAcceleration.x, accel.CurrentAcceleration.y, accel.CurrentAcceleration.z);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Visualization
        ImGui::Text("Visualization");
        ImGui::Checkbox("Show Acceleration Vector", &accel.ShowVector);
        ImGui::SliderFloat("Vector Scale", &accel.VectorScale, 0.1f, 5.0f);
        ImGui::ColorEdit4("Vector Color", (float*)&accel.VectorColor, 
                         ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Acceleration Graph
        if (!accel.AccelerationHistory.empty() && !accel.TimeStamps.empty())
        {
            ImGui::Text("Acceleration Magnitude Over Time");
            
            std::vector<ImVec2> graphData;
            for (size_t i = 0; i < accel.AccelerationHistory.size() && i < accel.TimeStamps.size(); ++i)
            {
                float mag = glm::length(accel.AccelerationHistory[i]);
                graphData.push_back(ImVec2(accel.TimeStamps[i], mag));
            }
            
            if (graphData.size() > 1)
            {
                ImVec2 graphSize(ImGui::GetContentRegionAvail().x, 150);
                if (ImGui::BeginChild("AccelGraph", graphSize, true))
                {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    ImVec2 graphMin = ImGui::GetCursorScreenPos();
                    ImVec2 graphMax = ImVec2(graphMin.x + graphSize.x - 20, graphMin.y + graphSize.y - 20);
                    
                    drawList->AddRectFilled(graphMin, graphMax, IM_COL32(20, 20, 25, 255));
                    
                    float maxAccel = 1.0f;
                    for (const auto& point : graphData)
                        maxAccel = std::max(maxAccel, point.y);
                    
                    float timeRange = graphData.back().x - graphData.front().x;
                    if (timeRange < EPSILON) timeRange = 1.0f;
                    
                    for (size_t i = 0; i < graphData.size() - 1; ++i)
                    {
                        float x1 = graphMin.x + ((graphData[i].x - graphData.front().x) / timeRange) * 
                                  (graphMax.x - graphMin.x);
                        float y1 = graphMax.y - (graphData[i].y / maxAccel) * (graphMax.y - graphMin.y);
                        
                        float x2 = graphMin.x + ((graphData[i+1].x - graphData.front().x) / timeRange) * 
                                  (graphMax.x - graphMin.x);
                        float y2 = graphMax.y - (graphData[i+1].y / maxAccel) * (graphMax.y - graphMin.y);
                        
                        drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), 
                                        IM_COL32(255, 200, 50, 255), 2.0f);
                    }
                    
                    drawList->AddRect(graphMin, graphMax, IM_COL32(100, 100, 100, 255), 0, 0, 1.5f);
                }
                ImGui::EndChild();
            }
        }
        
        ImGui::Spacing();
        
        // Action Button
        if (ImGui::Button(ICON_MD_REFRESH " Reset", ImVec2(120, 0)))
        {
            accel.Reset();
        }
        
        ImGui::Spacing();
        
        // Educational Info
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.25f, 0.35f, 0.9f));
        if (ImGui::BeginChild("AccelInfo", ImVec2(-1, 100), true))
        {
            RenderPhysicsEquation("a = Δv/Δt", "Acceleration is change in velocity over time");
            RenderPhysicsEquation("a = F/m", "Acceleration from Newton's Second Law");
            ImGui::Spacing();
            ImGui::TextWrapped("Acceleration is a vector quantity showing how quickly "
                              "velocity changes. It has both magnitude and direction.");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
    
    // ==================== TRAJECTORY PREDICTION PANEL ====================
    
    void SceneEditorLayer::RenderTrajectoryPanel(SceneContext& context)
    {
        if (!m_PhysicsAnalysis.ShowTrajectoryPanel) return;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Trajectory Prediction", &m_PhysicsAnalysis.ShowTrajectoryPanel);
        
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), ICON_MD_SHOW_CHART " Trajectory Prediction");
        ImGui::Separator();
        ImGui::Spacing();
        
        auto& traj = m_PhysicsAnalysis.Trajectory;
        
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
            if (m_PhysicsAnalysis.SelectedEntity != entt::null)
                UpdateTrajectoryPrediction(m_PhysicsAnalysis.SelectedEntity);
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
    
    // ==================== HELPER METHODS ====================
    
    void SceneEditorLayer::ShowPhysicsTooltip(const char* concepts, const char* explanation)
    {
        ImGui::TextDisabled(ICON_MD_HELP);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", concepts);
            ImGui::Separator();
            ImGui::TextUnformatted(explanation);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
    
    void SceneEditorLayer::RenderPhysicsEquation(const char* equation, const char* description)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.25f, 0.3f, 1.0f));
        if (ImGui::BeginChild(equation, ImVec2(-1, 50), true))
        {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "%s", equation);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", description);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    
    // ==================== PHYSICS UPDATE METHODS ====================
    
    void SceneEditorLayer::UpdateForceAnalysis(entt::entity entity, float deltaTime)
    {
        if (!m_Scene) return;

        auto& context = m_Scene->GetContext();
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(entity);
        if (!rb) return;
        
        auto& analysis = m_PhysicsAnalysis.ForceAnalysis;
        analysis.Forces.clear();
        
        // Add gravity force
        ForceVector gravity;
        float mass = rb->PhysicsBody->getMass();
        gravity.Name = "Gravity";
        gravity.Force = glm::vec3(0.0f, -mass * 9.81f, 0.0f);
        gravity.Color = IM_COL32(100, 200, 255, 255);
        analysis.Forces.push_back(gravity);
        
        // Add applied forces from physics body
        // (You would extract actual forces from your physics engine here)
        
        // Update net force
        analysis.UpdateNetForce();
    }
    
    void SceneEditorLayer::UpdateEnergyTracking(entt::entity entity, float time)
    {
        if (!m_Scene) return;
        if (!m_PhysicsAnalysis.Energy.TrackingEnabled) return;
        
        auto& context = m_Scene->GetContext();
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(entity);
        auto* transform = context.Entities->Registry.try_get<TransformComponent>(entity);
        if (!rb || !transform) return;

        auto mass = rb->PhysicsBody->getMass();
        auto linearVelocity = ToVec3(rb->PhysicsBody->getLinearVelocity());
        
        // Calculate kinetic energy: KE = 0.5 * m * v²
        float velocityMag = glm::length(linearVelocity);
        float ke = 0.5f * mass * velocityMag * velocityMag;
        
        // Calculate potential energy: PE = m * g * h
        float pe = mass * m_PhysicsAnalysis.Energy.GravityMagnitude * transform->Translation.y;
        
        m_PhysicsAnalysis.Energy.AddSample(ke, pe, time);
    }
    
    void SceneEditorLayer::UpdateMomentumTracking(entt::entity entity, float time)
    {
        if (!m_Scene) return;
        
        auto& context = m_Scene->GetContext();
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(entity);
        if (!rb) return;

        auto mass = rb->PhysicsBody->getMass();
        auto linearVelocity = ToVec3(rb->PhysicsBody->getLinearVelocity());
        
        // Calculate momentum: p = m * v
        glm::vec3 momentum = mass * linearVelocity;
        m_PhysicsAnalysis.Momentum.Update(momentum, time);
    }
    
    void SceneEditorLayer::UpdateAcceleration(entt::entity entity, float deltaTime)
    {
        if (!m_Scene) return;
        
        auto& context = m_Scene->GetContext();
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(entity);
        if (!rb) return;

        auto linearVelocity = ToVec3(rb->PhysicsBody->getLinearVelocity());
        m_PhysicsAnalysis.Acceleration.Update(linearVelocity, deltaTime);
    }
    
    void SceneEditorLayer::UpdateTrajectoryPrediction(entt::entity entity)
    {
        if (!m_Scene) return;
        if (!m_PhysicsAnalysis.Trajectory.EnablePrediction) return;
        
        auto& context = m_Scene->GetContext();
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(entity);
        auto* transform = context.Entities->Registry.try_get<TransformComponent>(entity);
        if (!rb || !transform) return;
        
        // Use gravity as acceleration
        glm::vec3 acceleration(0.0f, -9.81f, 0.0f);
        auto mass = rb->PhysicsBody->getMass();
        auto linearVelocity = ToVec3(rb->PhysicsBody->getLinearVelocity());
        
        m_PhysicsAnalysis.Trajectory.PredictTrajectory(
            transform->Translation,
            linearVelocity,
            acceleration,
            mass
        );
    }

    // ==================== HELPER STRUCTURES ====================
    
    struct ScreenProjection
    {
        ImVec2 ScreenPos;
        bool IsVisible;
        float Depth; // Z-depth for sorting
    };
    
    // ==================== PROJECTION HELPER ====================
    
    ScreenProjection ProjectWorldToScreen(const glm::vec3& worldPos, 
                                          const glm::mat4& view, 
                                          const glm::mat4& projection,
                                          const ImVec2& viewportMin,
                                          const ImVec2& viewportSize)
    {
        ScreenProjection result;
        result.IsVisible = false;
        result.Depth = 0.0f;
        
        // Transform to clip space
        glm::vec4 clipPos = projection * view * glm::vec4(worldPos, 1.0f);
        
        // Check if behind camera
        if (clipPos.w <= 0.0f)
        {
            return result;
        }
        
        // Perspective divide
        glm::vec3 ndcPos = glm::vec3(clipPos) / clipPos.w;
        
        // Check if outside NDC box
        if (ndcPos.x < -1.0f || ndcPos.x > 1.0f ||
            ndcPos.y < -1.0f || ndcPos.y > 1.0f ||
            ndcPos.z < -1.0f || ndcPos.z > 1.0f)
        {
            return result;
        }
        
        // Convert to screen space
        result.ScreenPos.x = viewportMin.x + (ndcPos.x * 0.5f + 0.5f) * viewportSize.x;
        result.ScreenPos.y = viewportMin.y + (1.0f - (ndcPos.y * 0.5f + 0.5f)) * viewportSize.y;
        result.Depth = ndcPos.z;
        result.IsVisible = true;
        
        return result;
    }
    
    // ==================== ARROW DRAWING HELPER ====================
    
    void DrawArrow2D(ImDrawList* drawList,
                    const ImVec2& start,
                    const ImVec2& end,
                    ImU32 color,
                    float thickness = 2.0f,
                    float arrowHeadSize = 10.0f)
    {
        if (!drawList) return;
        
        // Draw main line
        drawList->AddLine(start, end, color, thickness);
        
        // Calculate arrow direction
        ImVec2 direction = ImVec2(end.x - start.x, end.y - start.y);
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        
        if (length < 0.001f) return; // Too short to draw arrow
        
        // Normalize direction
        direction.x /= length;
        direction.y /= length;
        
        // Perpendicular vector
        ImVec2 perpendicular(-direction.y, direction.x);
        
        // Arrow head points
        float headLength = arrowHeadSize;
        float headWidth = arrowHeadSize * 0.6f;
        
        ImVec2 arrowBase = ImVec2(
            end.x - direction.x * headLength,
            end.y - direction.y * headLength
        );
        
        ImVec2 arrowLeft = ImVec2(
            arrowBase.x + perpendicular.x * headWidth,
            arrowBase.y + perpendicular.y * headWidth
        );
        
        ImVec2 arrowRight = ImVec2(
            arrowBase.x - perpendicular.x * headWidth,
            arrowBase.y - perpendicular.y * headWidth
        );
        
        // Draw arrow head as filled triangle
        drawList->AddTriangleFilled(end, arrowLeft, arrowRight, color);
        
        // Optional: Add outline for better visibility
        drawList->AddTriangle(end, arrowLeft, arrowRight, color, thickness * 0.5f);
    }
    
    // ==================== GET CAMERA MATRICES ====================
    
    struct CameraMatrices
    {
        glm::mat4 View;
        glm::mat4 Projection;
        ImVec2 ViewportMin;
        ImVec2 ViewportSize;
        bool IsValid;
    };
    
    CameraMatrices GetCurrentCameraMatrices(SceneContext& context)
    {
        CameraMatrices matrices;
        matrices.IsValid = false;
        
        // Get current ImGui viewport
        ImVec2 viewportMin = ImGui::GetCursorScreenPos();
        ImVec2 viewportMax = ImVec2(
            viewportMin.x + ImGui::GetContentRegionAvail().x,
            viewportMin.y + ImGui::GetContentRegionAvail().y
        );
        
        matrices.ViewportMin = viewportMin;
        matrices.ViewportSize = ImVec2(
            viewportMax.x - viewportMin.x,
            viewportMax.y - viewportMin.y
        );
        
        matrices.View = context.View->Camera.GetView();
        matrices.Projection = context.View->Camera.GetProjection();
        matrices.IsValid = true;
        
        return matrices;
    }
    
    // ==================== FORCE VECTOR VISUALIZATION ====================
    
    void SceneEditorLayer::DrawForceVector(const glm::vec3& origin, 
                                          const glm::vec3& force,
                                          const ImU32& color, 
                                          float scale)
    {
        if (!m_Scene) return;
        
        auto& context = m_Scene->GetContext();
        auto matrices = GetCurrentCameraMatrices(context);
        
        if (!matrices.IsValid) return;
        
        // Calculate endpoint of force vector
        glm::vec3 endpoint = origin + (force * scale);
        
        // Project to screen space
        auto startProj = ProjectWorldToScreen(origin, matrices.View, matrices.Projection,
                                             matrices.ViewportMin, matrices.ViewportSize);
        auto endProj = ProjectWorldToScreen(endpoint, matrices.View, matrices.Projection,
                                           matrices.ViewportMin, matrices.ViewportSize);
        
        if (!startProj.IsVisible || !endProj.IsVisible) return;
        
        // Get ImGui draw list
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (!drawList) return;
        
        // Calculate arrow size based on screen distance
        float screenDistance = std::sqrt(
            (endProj.ScreenPos.x - startProj.ScreenPos.x) * (endProj.ScreenPos.x - startProj.ScreenPos.x) +
            (endProj.ScreenPos.y - startProj.ScreenPos.y) * (endProj.ScreenPos.y - startProj.ScreenPos.y)
        );
        
        float arrowHeadSize = std::min(15.0f, screenDistance * 0.2f);
        float thickness = 3.0f;
        
        // Draw the arrow
        DrawArrow2D(drawList, startProj.ScreenPos, endProj.ScreenPos, 
                   color, thickness, arrowHeadSize);
        
        // Draw magnitude label
        float magnitude = glm::length(force);
        if (magnitude > EPSILON)
        {
            ImVec2 labelPos = ImVec2(
                endProj.ScreenPos.x + 5.0f,
                endProj.ScreenPos.y - 10.0f
            );
            
            // Background for text
            char labelText[64];
            snprintf(labelText, sizeof(labelText), "%.2f N", magnitude);
            
            ImVec2 textSize = ImGui::CalcTextSize(labelText);
            ImVec2 bgMin = ImVec2(labelPos.x - 2, labelPos.y - 2);
            ImVec2 bgMax = ImVec2(labelPos.x + textSize.x + 2, labelPos.y + textSize.y + 2);
            
            // Semi-transparent background
            drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 180), 3.0f);
            drawList->AddText(labelPos, color, labelText);
        }
        
        // Draw origin point
        auto originProj = ProjectWorldToScreen(origin, matrices.View, matrices.Projection,
                                              matrices.ViewportMin, matrices.ViewportSize);
        if (originProj.IsVisible)
        {
            drawList->AddCircleFilled(originProj.ScreenPos, 4.0f, color);
            drawList->AddCircle(originProj.ScreenPos, 4.0f, IM_COL32(255, 255, 255, 200), 12, 1.5f);
        }
    }
    
    // ==================== MOMENTUM VECTOR VISUALIZATION ====================
    
    void SceneEditorLayer::DrawMomentumVector(const glm::vec3& position, 
                                             const glm::vec3& momentum,
                                             const ImU32& color, 
                                             float scale)
    {
        if (!m_Scene) return;
        
        auto& context = m_Scene->GetContext();
        auto matrices = GetCurrentCameraMatrices(context);
        
        if (!matrices.IsValid) return;
        
        // Calculate endpoint
        glm::vec3 endpoint = position + (momentum * scale);
        
        // Project to screen
        auto startProj = ProjectWorldToScreen(position, matrices.View, matrices.Projection,
                                             matrices.ViewportMin, matrices.ViewportSize);
        auto endProj = ProjectWorldToScreen(endpoint, matrices.View, matrices.Projection,
                                           matrices.ViewportMin, matrices.ViewportSize);
        
        if (!startProj.IsVisible || !endProj.IsVisible) return;
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (!drawList) return;
        
        // Calculate sizes
        float screenDistance = std::sqrt(
            (endProj.ScreenPos.x - startProj.ScreenPos.x) * (endProj.ScreenPos.x - startProj.ScreenPos.x) +
            (endProj.ScreenPos.y - startProj.ScreenPos.y) * (endProj.ScreenPos.y - startProj.ScreenPos.y)
        );
        
        float arrowHeadSize = std::min(15.0f, screenDistance * 0.2f);
        float thickness = 4.0f; // Slightly thicker for momentum
        
        // Draw with double-line style for momentum (makes it distinctive)
        ImVec2 dir = ImVec2(
            endProj.ScreenPos.x - startProj.ScreenPos.x,
            endProj.ScreenPos.y - startProj.ScreenPos.y
        );
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > EPSILON)
        {
            dir.x /= len;
            dir.y /= len;
            
            ImVec2 perp(-dir.y, dir.x);
            float offset = 2.0f;
            
            // Draw two parallel lines
            ImVec2 start1 = ImVec2(startProj.ScreenPos.x + perp.x * offset,
                                  startProj.ScreenPos.y + perp.y * offset);
            ImVec2 end1 = ImVec2(endProj.ScreenPos.x + perp.x * offset,
                                endProj.ScreenPos.y + perp.y * offset);
            
            ImVec2 start2 = ImVec2(startProj.ScreenPos.x - perp.x * offset,
                                  startProj.ScreenPos.y - perp.y * offset);
            ImVec2 end2 = ImVec2(endProj.ScreenPos.x - perp.x * offset,
                                endProj.ScreenPos.y - perp.y * offset);
            
            drawList->AddLine(start1, end1, color, thickness - 1.0f);
            drawList->AddLine(start2, end2, color, thickness - 1.0f);
        }
        
        // Draw arrow head
        DrawArrow2D(drawList, startProj.ScreenPos, endProj.ScreenPos, 
                   color, thickness, arrowHeadSize);
        
        // Draw magnitude label
        float magnitude = glm::length(momentum);
        if (magnitude > EPSILON)
        {
            ImVec2 labelPos = ImVec2(
                endProj.ScreenPos.x + 5.0f,
                endProj.ScreenPos.y - 10.0f
            );
            
            char labelText[64];
            snprintf(labelText, sizeof(labelText), "%.2f kg·m/s", magnitude);
            
            ImVec2 textSize = ImGui::CalcTextSize(labelText);
            ImVec2 bgMin = ImVec2(labelPos.x - 2, labelPos.y - 2);
            ImVec2 bgMax = ImVec2(labelPos.x + textSize.x + 2, labelPos.y + textSize.y + 2);
            
            drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 180), 3.0f);
            drawList->AddText(labelPos, color, labelText);
        }
    }
    
    // ==================== TRAJECTORY PATH VISUALIZATION ====================
    
    void SceneEditorLayer::DrawTrajectoryPath(const std::vector<glm::vec3>& path, 
                                             const ImU32& color)
    {
        if (path.size() < 2 || !m_Scene) return;
        
        auto& context = m_Scene->GetContext();
        auto matrices = GetCurrentCameraMatrices(context);
        
        if (!matrices.IsValid) return;
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (!drawList) return;
        
        // Project all points
        std::vector<ScreenProjection> screenPoints;
        screenPoints.reserve(path.size());
        
        for (const auto& worldPos : path)
        {
            screenPoints.push_back(
                ProjectWorldToScreen(worldPos, matrices.View, matrices.Projection,
                                   matrices.ViewportMin, matrices.ViewportSize)
            );
        }
        
        // Draw line segments between visible points
        for (size_t i = 0; i < screenPoints.size() - 1; ++i)
        {
            if (screenPoints[i].IsVisible && screenPoints[i + 1].IsVisible)
            {
                // Fade color based on position in trajectory (future = more transparent)
                float alpha = 1.0f - (static_cast<float>(i) / screenPoints.size()) * 0.5f;
                ImU32 fadedColor = (color & 0x00FFFFFF) | (static_cast<ImU32>(alpha * 255) << 24);
                
                drawList->AddLine(
                    screenPoints[i].ScreenPos,
                    screenPoints[i + 1].ScreenPos,
                    fadedColor,
                    2.5f
                );
            }
        }
        
        // Draw points along trajectory
        for (size_t i = 0; i < screenPoints.size(); i += 5) // Every 5th point
        {
            if (screenPoints[i].IsVisible)
            {
                float alpha = 1.0f - (static_cast<float>(i) / screenPoints.size()) * 0.5f;
                ImU32 fadedColor = (color & 0x00FFFFFF) | (static_cast<ImU32>(alpha * 255) << 24);
                
                drawList->AddCircleFilled(screenPoints[i].ScreenPos, 3.0f, fadedColor);
            }
        }
        
        // Highlight start and end points
        if (screenPoints.front().IsVisible)
        {
            drawList->AddCircleFilled(screenPoints.front().ScreenPos, 5.0f, color);
            drawList->AddCircle(screenPoints.front().ScreenPos, 6.0f, 
                              IM_COL32(255, 255, 255, 255), 12, 2.0f);
            
            // "Start" label
            ImVec2 labelPos = ImVec2(
                screenPoints.front().ScreenPos.x + 10.0f,
                screenPoints.front().ScreenPos.y - 5.0f
            );
            
            ImVec2 textSize = ImGui::CalcTextSize("Start");
            ImVec2 bgMin = ImVec2(labelPos.x - 2, labelPos.y - 2);
            ImVec2 bgMax = ImVec2(labelPos.x + textSize.x + 2, labelPos.y + textSize.y + 2);
            
            drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 180), 3.0f);
            drawList->AddText(labelPos, IM_COL32(255, 255, 255, 255), "Start");
        }
        
        if (screenPoints.back().IsVisible)
        {
            // Make end point blink
            float time = static_cast<float>(ImGui::GetTime());
            float pulse = (std::sin(time * 3.0f) * 0.5f + 0.5f);
            ImU32 pulseColor = (color & 0x00FFFFFF) | (static_cast<ImU32>((128 + pulse * 127)) << 24);
            
            drawList->AddCircleFilled(screenPoints.back().ScreenPos, 5.0f, pulseColor);
            drawList->AddCircle(screenPoints.back().ScreenPos, 6.0f, 
                              IM_COL32(255, 255, 255, 200), 12, 2.0f);
            
            // "End" label
            ImVec2 labelPos = ImVec2(
                screenPoints.back().ScreenPos.x + 10.0f,
                screenPoints.back().ScreenPos.y - 5.0f
            );
            
            ImVec2 textSize = ImGui::CalcTextSize("End");
            ImVec2 bgMin = ImVec2(labelPos.x - 2, labelPos.y - 2);
            ImVec2 bgMax = ImVec2(labelPos.x + textSize.x + 2, labelPos.y + textSize.y + 2);
            
            drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 180), 3.0f);
            drawList->AddText(labelPos, IM_COL32(255, 255, 255, 255), "End");
        }
    }
    
    // ==================== COLLISION POINT VISUALIZATION ====================
    
    void SceneEditorLayer::DrawCollisionPoint(const glm::vec3& point, 
                                             float radius, 
                                             const ImU32& color)
    {
        if (!m_Scene) return;
        
        auto& context = m_Scene->GetContext();
        auto matrices = GetCurrentCameraMatrices(context);
        
        if (!matrices.IsValid) return;
        
        // Project collision point
        auto projection = ProjectWorldToScreen(point, matrices.View, matrices.Projection,
                                              matrices.ViewportMin, matrices.ViewportSize);
        
        if (!projection.IsVisible) return;
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (!drawList) return;
        
        // Pulsing effect
        float time = static_cast<float>(ImGui::GetTime());
        float pulse = std::sin(time * 5.0f) * 0.5f + 0.5f;
        float visualRadius = 8.0f + pulse * 4.0f;
        
        // Draw expanding rings (impact effect)
        int numRings = 3;
        for (int i = 0; i < numRings; ++i)
        {
            float ringPhase = std::fmod(time * 2.0f + i * 0.3f, 1.0f);
            float ringRadius = visualRadius * (1.0f + ringPhase * 2.0f);
            float ringAlpha = (1.0f - ringPhase) * 0.6f;
            
            ImU32 ringColor = (color & 0x00FFFFFF) | (static_cast<ImU32>(ringAlpha * 255) << 24);
            drawList->AddCircle(projection.ScreenPos, ringRadius, ringColor, 24, 2.0f);
        }
        
        // Draw main collision point
        drawList->AddCircleFilled(projection.ScreenPos, visualRadius, color);
        
        // Draw cross at center
        float crossSize = 6.0f;
        drawList->AddLine(
            ImVec2(projection.ScreenPos.x - crossSize, projection.ScreenPos.y),
            ImVec2(projection.ScreenPos.x + crossSize, projection.ScreenPos.y),
            IM_COL32(255, 255, 255, 255), 2.0f
        );
        drawList->AddLine(
            ImVec2(projection.ScreenPos.x, projection.ScreenPos.y - crossSize),
            ImVec2(projection.ScreenPos.x, projection.ScreenPos.y + crossSize),
            IM_COL32(255, 255, 255, 255), 2.0f
        );
        
        // Outer glow
        drawList->AddCircle(projection.ScreenPos, visualRadius + 2.0f, 
                          IM_COL32(255, 255, 255, 150), 24, 1.5f);
        
        // "IMPACT" label
        ImVec2 labelPos = ImVec2(
            projection.ScreenPos.x + visualRadius + 5.0f,
            projection.ScreenPos.y - 10.0f
        );
        
        ImVec2 textSize = ImGui::CalcTextSize("IMPACT");
        ImVec2 bgMin = ImVec2(labelPos.x - 2, labelPos.y - 2);
        ImVec2 bgMax = ImVec2(labelPos.x + textSize.x + 2, labelPos.y + textSize.y + 2);
        
        drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 200), 3.0f);
        drawList->AddRect(bgMin, bgMax, color, 3.0f, 0, 1.5f);
        drawList->AddText(labelPos, IM_COL32(255, 255, 255, 255), "IMPACT");
    }

    void SceneEditorLayer::DetectAndAnalyzeCollisions(SceneContext& context)
    {
        if (!m_PhysicsAnalysis.Collisions.EnableAnalysis) return;
        
        // This would integrate with your physics engine's collision detection
        // For now, this is a placeholder showing the structure
        
        // Example: When a collision is detected in your physics system:
        // CollisionEvent event;
        // event.EntityA = entityA;
        // event.EntityB = entityB;
        // event.CollisionPoint = contactPoint;
        // event.CollisionNormal = normal;
        // ... fill in other data ...
        // m_PhysicsAnalysis.Collisions.RecordCollision(event);
    }

}