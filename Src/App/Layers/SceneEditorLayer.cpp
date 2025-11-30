#include "CorePCH.hpp"

#include "SceneUtils.hpp"
#include "SceneSerializer.hpp"
#include "SceneEditorLayer.hpp"

#include "MotionVersion.hpp"

namespace Motion
{
    /**
     * @brief Called when the layer is attached to the application.
     * @details This method is used to load the base materials of the scene.
     * @note It is not recommended to load assets in this method, but rather in the OnUpdate method.
     * @see OnUpdate
     */
    void SceneEditorLayer::OnAttach()
    {
        std::memset(m_SearchBuf, 0, sizeof(m_SearchBuf));
        m_ScenePanel = std::make_unique<ScenePanel>();

        m_ScenePanel->Register<SceneEnvironmentSettings>();
        m_ScenePanel->Register<SceneView>();
        m_ScenePanel->Register<SimulationEntity>();
        m_ScenePanel->Register<EntityProperties>();
        m_ScenePanel->Register<MaterialEditor>();

        m_ScenePanel->Register<AccelerationTracker>();
        m_ScenePanel->Register<EnergyTracker>();
        m_ScenePanel->Register<MomentumTracker>();
        m_ScenePanel->Register<TrajectoryTracker>();
        m_ScenePanel->Register<ForceAnalysis>();

        if(m_Scene) m_ScenePanel->OnCreate(m_Scene.get());
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

        m_Scene->OnUpdate(handle, deltaTime);
        m_ScenePanel->OnUpdate(m_Scene.get(), deltaTime.GetDeltaTimeSeconds());
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
                        m_ScenePanel->OnCreate(m_Scene.get());
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

                    m_ScenePanel->OnCreate(m_Scene.get()); 
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

                        CreateRigidBody(context.PhysicsWorld->World, &context.Entities->Registry, e);

                        std::vector<glm::vec3> verts;
                        verts.reserve(mesh.Vertices.size());
                        std::transform(mesh.Vertices.begin(), mesh.Vertices.end(), std::back_inserter(verts),
                                      [](const Vertex& v) { return v.Position; });

                        CreateConvexCollider(&context.PhysicsWorld->Properties, &context.Entities->Registry, e, verts);
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

            if (ImGui::BeginMenu("  Themes"))
            {
                if (ImGui::MenuItem("Dark", nullptr, m_CurrentTheme == 0))
                {
                    UserInterface::ThemeManager::ApplyDarkTheme();
                    m_CurrentTheme = 0;
                }
                
                if (ImGui::MenuItem("Light", nullptr, m_CurrentTheme == 1))
                {
                    UserInterface::ThemeManager::ApplyLightTheme();
                    m_CurrentTheme = 1;
                }
                
                if (ImGui::MenuItem("Classic", nullptr, m_CurrentTheme == 2))
                {
                    UserInterface::ThemeManager::ApplyClassicTheme();
                    m_CurrentTheme = 2;
                }
                
                if (ImGui::MenuItem("Material Design", nullptr, m_CurrentTheme == 3))
                {
                    auto scheme = UserInterface::ThemeManager::GetMaterialDesignScheme();
                    UserInterface::ThemeManager::UseColorScheme(scheme);
                    m_CurrentTheme = 3;
                }

                if (ImGui::MenuItem("Neumorphic", nullptr, m_CurrentTheme == 4))
                {
                    auto scheme = UserInterface::ThemeManager::GetNeumorphicScheme();
                    UserInterface::ThemeManager::UseColorScheme(scheme);
                    m_CurrentTheme = 4;
                }


                
                ImGui::Separator();
                
                if (ImGui::BeginMenu("Accent Color"))
                {
                    if (ImGui::MenuItem("Blue"))
                        UserInterface::ThemeManager::SetAccentColor(ImVec4(0.13f, 0.59f, 0.95f, 1.0f));
                    
                    if (ImGui::MenuItem("Red"))
                        UserInterface::ThemeManager::SetAccentColor(ImVec4(0.96f, 0.26f, 0.21f, 1.0f));
                    
                    if (ImGui::MenuItem("Green"))
                        UserInterface::ThemeManager::SetAccentColor(ImVec4(0.30f, 0.69f, 0.31f, 1.0f));
                    
                    if (ImGui::MenuItem("Purple"))
                        UserInterface::ThemeManager::SetAccentColor(ImVec4(0.61f, 0.15f, 0.69f, 1.0f));
                    
                    if (ImGui::MenuItem("Orange"))
                        UserInterface::ThemeManager::SetAccentColor(ImVec4(1.00f, 0.60f, 0.00f, 1.0f));
                    
                    ImGui::EndMenu();
                }
                
                ImGui::EndMenu();
            }

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

        if(ImGui::BeginMenu("View"))
        {
            auto& context = m_Scene->GetContext();

            if(ImGui::MenuItem("Entity Hierarchy", nullptr, &context.Panels->ShowEntityHierarchy, m_Scene != nullptr));
            if(ImGui::MenuItem("Entity Properties", nullptr, &context.Panels->ShowEntityComponents, m_Scene != nullptr));
            if(ImGui::MenuItem("Material Editor", nullptr, &context.Panels->ShowEntityMaterials, m_Scene != nullptr));
            if(ImGui::MenuItem("Simulation WatchList", nullptr, &context.Panels->ShowEntitySimulated, m_Scene != nullptr));
            if(ImGui::MenuItem("Scene Environment", nullptr, &context.Panels->ShowEnvironmentSettings, m_Scene != nullptr));

            ImGui::Separator();

            if(ImGui::MenuItem("Force Analyser", nullptr, &context.Panels->ShowForceAnalysisPanel, m_Scene != nullptr));
            if(ImGui::MenuItem("Energy Analyser", nullptr, &context.Panels->ShowEnergyPanel, m_Scene != nullptr));
            if(ImGui::MenuItem("Momentum Analyser", nullptr, &context.Panels->ShowMomentumPanel, m_Scene != nullptr));
            if(ImGui::MenuItem("Acceleration Analyser", nullptr, &context.Panels->ShowAccelerationPanel, m_Scene != nullptr));
            if(ImGui::MenuItem("Trajectory Analyser", nullptr, &context.Panels->ShowTrajectoryPanel, m_Scene != nullptr));

            ImGui::EndMenu();
        }
        ImGui::EndDisabled();

        if(ImGui::BeginMenu("About"))
        {
            if(ImGui::MenuItem("About Motion Engine", nullptr))
            {
                m_ShowAboutBox = !m_ShowAboutBox;
            };

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
        RenderAbout();
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

                ImGuiID dock_right_id   = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.30f, nullptr, &dock_main_id);
                ImGuiID dock_rbottom_id = ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.30f, nullptr, &dock_right_id);
                
                ImGuiID dock_left_id    = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.30f, nullptr, &dock_main_id);
                ImGuiID dock_lbottom_id = ImGui::DockBuilderSplitNode(dock_left_id, ImGuiDir_Down, 0.30f, nullptr, &dock_left_id);
                
                ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.30f, nullptr, &dock_main_id);

                ImGui::DockBuilderDockWindow("Scene Viewport",   dock_main_id);

                ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_right_id);
                ImGui::DockBuilderDockWindow("Entity Properties", dock_rbottom_id);
                ImGui::DockBuilderDockWindow("Environment Settings", dock_rbottom_id);

                ImGui::DockBuilderDockWindow("Simulation Watch List", dock_left_id);
                ImGui::DockBuilderDockWindow("Acceleration Analysis", dock_lbottom_id);
                ImGui::DockBuilderDockWindow("Energy Analysis", dock_lbottom_id);
                ImGui::DockBuilderDockWindow("Force Analysis", dock_lbottom_id);
                ImGui::DockBuilderDockWindow("Momentum Analysis", dock_lbottom_id);
                ImGui::DockBuilderDockWindow("Trajectory Prediction", dock_lbottom_id);

                ImGui::DockBuilderDockWindow("Material Editor", dock_bottom_id);
                ImGui::DockBuilderDockWindow("Console", dock_bottom_id);
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

        m_ScenePanel->OnRender(m_Scene.get());
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

        ImGui::PushID(entt::to_integral(e));
        
        TextBoxConfig nameConfig;
        nameConfig.ReadOnly = false;
        TextBox("Name Tag", tag.Tag, nameConfig);
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("The identifier name for this entity");

        ToggleSwitch("Is Active", &tag.IsActive, ToggleSwitchPresets::iOS());
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("Enable or disable this entity in the scene");
    
        ImGui::PopID();
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
     * Renders the entity hierarchy for the scene editor layer.
     * This includes rendering the entities in a tree view, with the ability to expand and collapse entities.
     * If an entity is inactive, a disabled text is rendered instead.
     * @param context The scene context.
     * @param root The root entity of the hierarchy.
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
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) m_Scene->SelectedEntity(e);
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

                ImGui::EndPopup();
            }

            if (open)
            {
                RenderTagAndModel(context, e);
                ImGui::TreePop();
            }
        
            ImGui::PopID();
        });
    }

    /**
     * Renders a professional about dialog with comprehensive version information.
     * Displays engine name, version, build details, and git information in a structured layout.
     */
    void SceneEditorLayer::RenderAbout()
    {
        if(!m_ShowAboutBox) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::SetNextWindowSize(ImVec2(500, 0), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("About Motion Engine", &m_ShowAboutBox, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
        {
            // Header - Engine name and version
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Use default font for consistency
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("MOTION ENGINE").x) * 0.5f);
            ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "MOTION ENGINE");
            ImGui::PopFont();

            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(Motion::VERSION).x) * 0.5f);
            ImGui::Text("Version %s", Motion::VERSION);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Description
            ImGui::TextWrapped("Physics Simulation for Educational Purposes");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Build Information Section
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Build Information");
            ImGui::Spacing();

            ImGui::Indent(10);
            ImGui::Text("Configuration:"); ImGui::SameLine(150); ImGui::Text("%s", Motion::BUILD_TYPE);
            ImGui::Text("Timestamp:"); ImGui::SameLine(150); ImGui::Text("%s", Motion::BUILD_TIMESTAMP);
            ImGui::Unindent(10);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Git Information Section (only if available)
            std::string gitHash = Motion::GIT_COMMIT_HASH;
            if (gitHash != "unknown" && !gitHash.empty())
            {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.4f, 1.0f), "Version Control");
                ImGui::Spacing();

                ImGui::Indent(10);
                ImGui::Text("Branch:"); ImGui::SameLine(150); ImGui::Text("%s", Motion::GIT_BRANCH);
                ImGui::Text("Commit:"); ImGui::SameLine(150); ImGui::Text("%s", Motion::GIT_COMMIT_HASH);

                // Show dirty status if applicable
                if (Motion::GIT_IS_DIRTY)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(modified)");
                }

                // Show tag if available
                std::string gitTag = Motion::GIT_TAG;
                if (!gitTag.empty() && gitTag != "unknown")
                {
                    ImGui::Text("Tag:"); ImGui::SameLine(150); ImGui::Text("%s", gitTag);
                }

                ImGui::Text("Total Commits:"); ImGui::SameLine(150); ImGui::Text("%s", Motion::GIT_COMMIT_COUNT);
                ImGui::Unindent(10);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }

            // Footer with close button
            float buttonWidth = 120.0f;
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonWidth) * 0.5f);
            if (ImGui::Button("Close", ImVec2(buttonWidth, 0)))
            {
                m_ShowAboutBox = false;
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

}