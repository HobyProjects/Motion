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
    void SceneEditorLayer::OnUIRender([[maybe_unused]] WindowHandle handle)
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
        auto& style = ImGui::GetStyle();

        ImVec4 ACCENT           = UserInterface::ThemeManager::GetAccentColor();
        ImVec4 OVERLAY_DIM      = ImVec4(0.0f, 0.0f, 0.0f, 0.50f);
        ImVec4 CARD_BG          = style.Colors[ImGuiCol_WindowBg];
        ImVec4 TEXT_PRIMARY     = style.Colors[ImGuiCol_Text];
        ImVec4 TEXT_SECONDARY   = style.Colors[ImGuiCol_TextDisabled];
        ImVec4 BORDER           = style.Colors[ImGuiCol_Border];
        
        const ImVec4 SUCCESS        = ImVec4(0.06f, 0.54f, 0.24f, 1.0f);
        const ImVec4 WARNING        = ImVec4(1.0f, 0.73f, 0.0f, 1.0f);
        const ImVec4 ERROR_COLOR    = ImVec4(0.91f, 0.07f, 0.14f, 1.0f);
        
        auto MixColors = [](const ImVec4& a, const ImVec4& b, float t) -> ImVec4 
        {
            return ImVec4(
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t,
                a.w + (b.w - a.w) * t
            );
        };

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

        if (m_SceneCreationRequest.ShowDialog)
        {
            ImGui::OpenPopup("Create New Scene");
            m_SceneCreationRequest.ShowDialog = false;
        }

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
                                ImGuiWindowFlags_NoSavedSettings;
    
        if (ImGui::BeginPopupModal("Create New Scene", nullptr, flags))
        {
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::InvisibleButton("##overlay_blocker", viewport->WorkSize);
            
            std::vector<std::string> errors;
            std::vector<std::string> warnings;
            
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(50, 45));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
            
            ImGui::PushStyleColor(ImGuiCol_WindowBg, CARD_BG);
            ImGui::PushStyleColor(ImGuiCol_Border, BORDER);

            static bool useMaxHeight = false;
            const float maxHeight = viewport->WorkSize.y * 0.85f;
            const ImVec2 cardSize = (useMaxHeight ? ImVec2(800, maxHeight) : ImVec2(800, 0));

            if (ImGui::BeginChild("LoadingContent", cardSize, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar))
            {
                ImVec2 childSize = ImGui::GetWindowSize();
                if (childSize.y > maxHeight) { useMaxHeight = true; }
                
                // Header
                HeadingConfig headerConfig;
                headerConfig.Color = ACCENT;
                headerConfig.Separator = false;
                Heading("Create a New Physics Scene", HeadingLevel::H2, headerConfig);

                LabelConfig descConfig;
                descConfig.Color = TEXT_SECONDARY;
                descConfig.Wrapped = true;
                LabelSimple("Set up a new scene to start building and simulating your physics experiments.", descConfig);
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Scene Name Input
                std::string name = m_SceneCreationRequest.Name;
                trim(name);
                bool nameHasError = name.empty() || has_invalid_win_chars(name);
                
                TextBoxConfig nameConfig;
                nameConfig.Layout.LabelWidthRatio = 0.25f;
                
                // Custom styling for error state
                if (nameHasError)
                {
                    ImVec4 errorBg = MixColors(ERROR_COLOR, CARD_BG, 0.90f);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, errorBg);
                    ImGui::PushStyleColor(ImGuiCol_Border, MixColors(ERROR_COLOR, BORDER, 0.30f));
                }

                std::string nameStr(m_SceneCreationRequest.Name);
                TextBoxWithHint("Scene Name", nameStr, "Enter scene name...", nameConfig);
#ifdef MOTION_PLATFORM_WINDOWS
                strncpy_s(m_SceneCreationRequest.Name, sizeof(m_SceneCreationRequest.Name), nameStr.c_str(), _TRUNCATE);
#else
                strncpy(m_SceneCreationRequest.Name, nameStr.c_str(), sizeof(m_SceneCreationRequest.Name) - 1);
                m_SceneCreationRequest.Name[sizeof(m_SceneCreationRequest.Name) - 1] = '\0';
#endif
                if (nameHasError) ImGui::PopStyleColor(2);
                
                ImGui::Spacing();

                // Save Location Input
                std::string pathStr = m_SceneCreationRequest.FilePath.string();
                bool pathHasError = m_SceneCreationRequest.FilePath.empty() || 
                                    !std::filesystem::exists(m_SceneCreationRequest.FilePath);
                
                if (pathHasError)
                {
                    ImVec4 errorBg = MixColors(ERROR_COLOR, CARD_BG, 0.90f);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, errorBg);
                    ImGui::PushStyleColor(ImGuiCol_Border, MixColors(ERROR_COLOR, BORDER, 0.30f));
                }
                
                TextBoxConfig pathConfig;
                pathConfig.ReadOnly = true;
                pathConfig.Layout.LabelWidthRatio = 0.25f;
                pathConfig.Layout.MaxLabelWidth = 150.0f;
                
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Save Location:");
                ImGui::SameLine(150.0f + 12.0f);
                
                ImGui::SetNextItemWidth(280.0f);
                std::string tempPath = pathStr;
                TextBoxWithHint("##pathInput", tempPath, "Click Browse to select a folder...", pathConfig);
                
                if (pathHasError)
                    ImGui::PopStyleColor(2);

                ImGui::SameLine();
            
                ButtonConfig browseConfig;
                browseConfig.Style = ButtonStyle::Primary;
                browseConfig.Size = ImVec2(120, 0);
                
                Button("Browse", browseConfig, [this]()
                {
                    DialogBoxes::InitializeCOM();
                    if (auto folder = DialogBoxes::SelectFolderDialog(L"Select where to save your scene"); !folder.empty())
                    {
                        m_SceneCreationRequest.FilePath = folder;
                    }
                    DialogBoxes::UninitializeCOM();
                });

                ImGui::Spacing();
                ImGui::Spacing();

                {
                    // Validation
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
                }

                // Path Preview (Success State)
                if (errors.empty())
                {
                    ImGui::Spacing();
                    auto fullPath = m_SceneCreationRequest.FilePath / name;
                    
                    LabelConfig pathLabelConfig;
                    pathLabelConfig.Color = SUCCESS;
                    pathLabelConfig.Bullet = true;
                    pathLabelConfig.Wrapped = true;

                    std::string creationPath = std::format("Scene will be created at: {}", fullPath.string());
                    LabelSimple(creationPath, pathLabelConfig);
                    ImGui::Spacing();
                }

                // Display Warnings
                if (!warnings.empty())
                {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    HeadingConfig warningHeaderConfig;
                    warningHeaderConfig.Color = WARNING;
                    warningHeaderConfig.Separator = false;
                    Heading("Warning!", HeadingLevel::H4, warningHeaderConfig);

                    ImGui::Spacing();

                    LabelConfig warnConfig;
                    warnConfig.Color = MixColors(WARNING, TEXT_PRIMARY, 0.20f);
                    warnConfig.Wrapped = true;
                    warnConfig.Bullet = true;

                    ImGui::Indent(10.0f);
                    for (const auto& warning : warnings)
                    {
                        LabelSimple(warning, warnConfig);
                    }
                    ImGui::Unindent(10.0f);
                    ImGui::Spacing();
                }

                // Display Errors
                if (!errors.empty())
                {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    HeadingConfig errorHeaderConfig;
                    errorHeaderConfig.Color = ERROR_COLOR;
                    errorHeaderConfig.Separator = false;
                    Heading("Error!", HeadingLevel::H4, errorHeaderConfig);
                    
                    ImGui::Spacing();
                    
                    LabelConfig errorDescConfig;
                    errorDescConfig.Color = TEXT_SECONDARY;
                    LabelSimple("Please fix the following issues:", errorDescConfig);

                    ImGui::Spacing();

                    LabelConfig errorConfig;
                    errorConfig.Color = MixColors(ERROR_COLOR, TEXT_PRIMARY, 0.30f);
                    errorConfig.Bullet = true;
                    
                    ImGui::Indent(10.0f);
                    for (const auto& error : errors)
                    {
                        LabelSimple(error, errorConfig);
                    }
                    ImGui::Unindent(10.0f);
                    ImGui::Spacing();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Action Buttons
                bool canCreate = errors.empty();
                
                // Center the buttons
                float buttonGroupWidth = 140.0f + 8.0f + 120.0f; // Create + spacing + Cancel
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - buttonGroupWidth) * 0.5f);
                
                ButtonConfig createConfig;
                createConfig.Style = ButtonStyle::Success;
                createConfig.Size = ImVec2(140, 35);
                createConfig.Disabled = !canCreate;
                createConfig.Tooltip = !canCreate ? "Please fix all errors before creating the scene" : nullptr;
                
                Button("Create Scene", createConfig, [this, &name]()
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
                });
                
                ImGui::SameLine();
                
                ButtonConfig cancelConfig;
                cancelConfig.Style = ButtonStyle::Secondary;
                cancelConfig.Size = ImVec2(120, 35);
                
                Button("Cancel", cancelConfig, [this]()
                {
                    m_SceneCreationRequest.Reset();
                    ImGui::CloseCurrentPopup();
                });

                ImGui::Spacing();
                ImGui::EndChild();
            }
            
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(3);
            ImGui::EndPopup();
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
        
        // Handle scene creation completion
        if (m_SceneCreationOp.IsReady())
        {
            try
            {
                m_Scene = m_SceneCreationOp.GetResult();
                
                if (m_Scene)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    io.IniFilename = nullptr;
                    std::string layoutFile = std::format("{}/mes-config.ini", m_ScenePath.string());
                    ImGui::SaveIniSettingsToDisk(layoutFile.c_str());

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
            HeadingConfig errorHeaderConfig;
            errorHeaderConfig.Color = ERROR_COLOR;
            errorHeaderConfig.Separator = false;
            Heading("Scene Creation Failed", HeadingLevel::H4, errorHeaderConfig);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            LabelSimple("An error occurred while creating the scene.");
            
            ImGui::Spacing();
            
            LabelConfig checkConfig;
            checkConfig.Color = TEXT_SECONDARY;
            checkConfig.Wrapped = true;
            LabelSimple("Please check the following:", checkConfig);
            
            ImGui::Spacing();
            
            checkConfig.Bullet = true;
            ImGui::Indent(10.0f);
            LabelSimple("You have write permissions for the selected folder", checkConfig);
            LabelSimple("There is enough disk space available", checkConfig);
            LabelSimple("The folder path is valid and accessible", checkConfig);
            LabelSimple("No other application is blocking the folder", checkConfig);
            ImGui::Unindent(10.0f);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 120) * 0.5f);
            
            ButtonConfig okConfig;
            okConfig.Style = ButtonStyle::Primary;
            okConfig.Size = ImVec2(120, 30);
            
            Button("OK", okConfig, []()
            {
                ImGui::CloseCurrentPopup();
            });
            
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
                        std::transform(mesh.Vertices.begin(), mesh.Vertices.end(), std::back_inserter(verts), [](const Vertex& v) { return v.Position; });
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
                    
                    MOTION_CORE_INFO("Entity imported successfully: {}",  m_EntityImportRequest.FilePath.string());
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
        // Get theme colors
        auto& style = ImGui::GetStyle();
        ImVec4 OVERLAY_DIM = ImVec4(0.0f, 0.0f, 0.0f, 0.50f);
        ImVec4 CARD_BG = style.Colors[ImGuiCol_WindowBg];
        ImVec4 TEXT_PRIMARY = style.Colors[ImGuiCol_Text];
        ImVec4 TEXT_SECONDARY = style.Colors[ImGuiCol_TextDisabled];
        ImVec4 BORDER = style.Colors[ImGuiCol_Border];
        ImVec4 ACCENT = UserInterface::ThemeManager::GetAccentColor();
        
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
            
            const ImVec2 cardSize = ImVec2(540, 540);
            
            if (ImGui::BeginChild("LoadingContent", cardSize, true, ImGuiWindowFlags_NoScrollbar))
            {
                const float time = static_cast<float>(ImGui::GetTime());
                
                // Determine operation details
                const char* loadingTitle = "Processing";
                const char* loadingDescription = "Please wait while the operation completes.";
                const char* operationIcon = ICON_MD_HOURGLASS_EMPTY;
                ImVec4 accentColor = ACCENT;
                float elapsedTime = 0.0f;
                
                if (m_SceneCreationOp.State == AsyncOperationState::InProgress)
                {
                    loadingTitle = "Creating Your Scene";
                    loadingDescription = "Setting up the scene structure, creating folders, and initializing physics simulation...";
                    operationIcon = ICON_MD_CREATE_NEW_FOLDER;
                    accentColor = ImVec4(0.06f, 0.54f, 0.24f, 1.0f);
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
                    accentColor = ImVec4(1.0f, 0.73f, 0.0f, 1.0f);
                    elapsedTime = m_EntityImportOp.GetElapsedSeconds();
                }
                
                ImGui::Spacing();
                ImGui::Spacing();
                
                // === BEAUTIFUL ANIMATED LOADING SPINNER ===
                const float spinnerSize = 90.0f;
                const float spinnerTopMargin = 70.0f;
                
                // Get the content region and calculate perfect center
                ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
                ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
                ImVec2 windowPos = ImGui::GetWindowPos();
                
                // Calculate the center of the content area
                ImVec2 spinnerCenter = ImVec2(
                    windowPos.x + contentMin.x + ((contentMax.x - contentMin.x) * 0.5f),
                    windowPos.y + contentMin.y + spinnerTopMargin
                );
                
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                
                // Draw Background Glow Effect
                const int glowLayers = 3;
                for (int layer = 0; layer < glowLayers; layer++)
                {
                    float glowRadius = spinnerSize * 0.5f + (layer * 8.0f);
                    float glowAlpha = 0.08f * (1.0f - (float)layer / glowLayers);
                    
                    ImVec4 glowColor = accentColor;
                    glowColor.w = glowAlpha;
                    
                    drawList->AddCircleFilled(
                        spinnerCenter,
                        glowRadius,
                        ImGui::GetColorU32(glowColor),
                        64
                    );
                }
                
                // Draw Outer Ring (Stationary)
                const float outerRadius = spinnerSize * 0.45f;
                const float ringThickness = 2.0f;
                ImVec4 ringColor = accentColor;
                ringColor.w = 0.15f;
                
                drawList->AddCircle(
                    spinnerCenter,
                    outerRadius,
                    ImGui::GetColorU32(ringColor),
                    64,
                    ringThickness
                );
                
                // Draw Multiple Rotating Arcs with Different Speeds
                const int numArcs = 3;
                const float arcRadii[] = { outerRadius * 0.85f, outerRadius * 0.65f, outerRadius * 0.45f };
                const float arcSpeeds[] = { 3.5f, -2.8f, 4.2f }; // Different speeds, some reverse
                const float arcLengths[] = { 0.25f, 0.35f, 0.20f }; // Different arc lengths
                const float arcThicknesses[] = { 3.5f, 3.0f, 2.5f };
                
                for (int arc = 0; arc < numArcs; arc++)
                {
                    const int segments = 48;
                    const float angleOffset = time * arcSpeeds[arc];
                    const float arcLength = arcLengths[arc];
                    const float radius = arcRadii[arc];
                    const float thickness = arcThicknesses[arc];
                    
                    for (int i = 0; i < segments; i++)
                    {
                        float t = (float)i / (float)segments;
                        if (t > arcLength) continue;
                        
                        const float aStart = (t * 2.0f * IM_PI) + angleOffset;
                        const float aEnd = ((t + 0.015f) * 2.0f * IM_PI) + angleOffset;
                        
                        // Smooth gradient from bright to dim
                        const float alpha = 0.3f + (0.7f * (1.0f - (t / arcLength)));
                        
                        ImVec4 segmentColor = accentColor;
                        segmentColor.w = alpha;
                        
                        drawList->PathArcTo(spinnerCenter, radius, aStart, aEnd, 6);
                        drawList->PathStroke(ImGui::GetColorU32(segmentColor), 0, thickness);
                    }
                }
                
                // Draw Pulsing Center Circle
                const float pulseFreq = 2.0f;
                const float pulseAmount = 0.15f;
                const float pulse = 1.0f + (pulseAmount * sinf(time * pulseFreq));
                const float centerRadius = outerRadius * 0.25f * pulse;
                
                ImVec4 centerColor = accentColor;
                centerColor.w = 0.2f;
                
                drawList->AddCircleFilled(
                    spinnerCenter,
                    centerRadius,
                    ImGui::GetColorU32(centerColor),
                    32
                );
                
                // Draw Icon in Center with Pulse
                ImFont* iconFont = UserInterface::FontManager::GetFont("MaterialIcons-48");
                if (iconFont)
                {
                    ImGui::PushFont(iconFont);
                    ImVec2 iconSize = ImGui::CalcTextSize(operationIcon);
                    
                    // Draw icon centered in the spinner
                    drawList->AddText(
                        iconFont,
                        iconFont->Scale,
                        ImVec2(
                            spinnerCenter.x - (iconSize.x * 0.5f),
                            spinnerCenter.y - (iconSize.y * 0.5f)
                        ),
                        ImGui::GetColorU32(accentColor),
                        operationIcon
                    );
                    
                    ImGui::PopFont();
                }
                
                // Set cursor position for content below spinner
                ImGui::SetCursorPosY(spinnerTopMargin + spinnerSize + 20.0f);
                
                ImGui::Spacing();
                ImGui::Spacing();
                
                // === CONTENT SECTION ===
                
                // Get content width for centering
                float contentWidth = contentMax.x - contentMin.x;
                
                // Title
                ImFont* titleFont = UserInterface::FontManager::GetFont("JetBrainsMono-Bold-H4");
                if (titleFont) ImGui::PushFont(titleFont);
                
                float titleWidth = ImGui::CalcTextSize(loadingTitle).x;
                ImGui::SetCursorPosX(contentMin.x + ((contentWidth - titleWidth) * 0.5f));
                
                ImGui::PushStyleColor(ImGuiCol_Text, TEXT_PRIMARY);
                ImGui::Text("%s", loadingTitle);
                ImGui::PopStyleColor();
                
                if (titleFont) ImGui::PopFont();
                
                ImGui::Spacing();
                
                // Description - centered and wrapped
                ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                
                // Calculate wrapped text size for centering
                float descWidth = contentWidth - 80.0f; // Some margin
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + descWidth);
                
                // Center the wrapped text block
                ImGui::SetCursorPosX(contentMin.x + 40.0f);
                ImGui::TextWrapped("%s", loadingDescription);
                
                ImGui::PopTextWrapPos();
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                ImGui::Spacing();
                
                // Separator
                ImGui::PushStyleColor(ImGuiCol_Separator, BORDER);
                ImGui::Separator();
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                
                // Animated Status Indicator
                int numDots = ((int)(time * 2.5f)) % 4;
                std::string statusText = "Processing";
                for (int i = 0; i < numDots; i++)
                    statusText += ".";
                
                float statusWidth = ImGui::CalcTextSize(statusText.c_str()).x;
                ImGui::SetCursorPosX(contentMin.x + ((contentWidth - statusWidth) * 0.5f));
                
                ImGui::PushStyleColor(ImGuiCol_Text, accentColor);
                ImGui::Text("%s", statusText.c_str());
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                
                // Elapsed Time
                std::string timeText;
                if (elapsedTime < 60.0f)
                {
                    char buffer[64];
                    snprintf(buffer, sizeof(buffer), "%.1f seconds elapsed", elapsedTime);
                    timeText = buffer;
                }
                else
                {
                    int minutes = (int)(elapsedTime / 60.0f);
                    int seconds = (int)(elapsedTime) % 60;
                    char buffer[64];
                    snprintf(buffer, sizeof(buffer), "%d:%02d elapsed", minutes, seconds);
                    timeText = buffer;
                }
                
                float timeWidth = ImGui::CalcTextSize(timeText.c_str()).x;
                ImGui::SetCursorPosX(contentMin.x + ((contentWidth - timeWidth) * 0.5f));
                
                ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                ImGui::TextDisabled("%s", timeText.c_str());
                ImGui::PopStyleColor();
                
                // Long Operation Hint
                if (elapsedTime > 5.0f)
                {
                    ImGui::Spacing();
                    
                    const char* hintText = "This is taking longer than usual...";
                    float hintWidth = ImGui::CalcTextSize(hintText).x;
                    ImGui::SetCursorPosX(contentMin.x + ((contentWidth - hintWidth) * 0.5f));
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, TEXT_SECONDARY);
                    ImGui::TextDisabled("%s", hintText);
                    ImGui::PopStyleColor();
                }
                
                ImGui::Spacing();
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
        if (!ImGui::BeginMenuBar()) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 5.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 5.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(10.0f, 4.0f));

        DrawFileMenu();
        DrawShapesMenu();
        DrawViewMenu();
        DrawAboutMenu();
        
        ImGui::PopStyleVar(4);

        DrawSimulationControls();

        ImGui::EndMenuBar();
        RenderAbout();
    }

    void SceneEditorLayer::DrawFileMenu()
    {
        if (!ImGui::BeginMenu("File"))
            return;

        if (ImGui::MenuItem("New Scene", nullptr))
        {
            m_SceneCreationRequest.ShowDialog = true;
        }

        if (ImGui::MenuItem("Open...", nullptr))
        {
            m_SceneLoadRequest.ShowDialog = true;
        }

        ImGui::Separator();
        
        ImGui::BeginDisabled(m_Scene == nullptr);
        if (ImGui::MenuItem("Save", nullptr))
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

        DrawThemeMenu();

        ImGui::Separator();
        
        if (ImGui::MenuItem("Quit", nullptr))
        {
            // Handle quit
        }

        ImGui::EndMenu();
    }

    void SceneEditorLayer::DrawThemeMenu()
    {
        if (!ImGui::BeginMenu("Themes"))
            return;

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
        
        DrawAccentColorMenu();
        
        ImGui::EndMenu();
    }

    void SceneEditorLayer::DrawAccentColorMenu()
    {
        if (!ImGui::BeginMenu("Accent Color"))
            return;

        struct AccentColor { const char* name; ImVec4 color; };
        const AccentColor colors[] = {
            { "Blue",   ImVec4(0.13f, 0.59f, 0.95f, 1.0f) },
            { "Red",    ImVec4(0.96f, 0.26f, 0.21f, 1.0f) },
            { "Green",  ImVec4(0.30f, 0.69f, 0.31f, 1.0f) },
            { "Purple", ImVec4(0.61f, 0.15f, 0.69f, 1.0f) },
            { "Orange", ImVec4(1.00f, 0.60f, 0.00f, 1.0f) }
        };

        for (const auto& accent : colors)
        {
            if (ImGui::MenuItem(accent.name))
            {
                UserInterface::ThemeManager::SetAccentColor(accent.color);
            }
        }
        
        ImGui::EndMenu();
    }

    void SceneEditorLayer::DrawShapesMenu()
    {
        ImGui::BeginDisabled(m_Scene == nullptr);
        
        if (!ImGui::BeginMenu("Shapes") || !m_Scene)
        {
            ImGui::EndDisabled();
            return;
        }

        struct Primitive { const char* name; const char* path; };
        const Primitive primitives[] = {
            { "Cube",     "Assets/Primitives/Cube.obj" },
            { "Cone",     "Assets/Primitives/Cone.obj" },
            { "Cylinder", "Assets/Primitives/Cylinder.obj" },
            { "Plane",    "Assets/Primitives/Plane.obj" },
            { "Sphere",   "Assets/Primitives/Sphere.obj" },
            { "Torus",    "Assets/Primitives/Torus.obj" }
        };

        for (const auto& prim : primitives)
        {
            if (ImGui::MenuItem(prim.name))
            {
                RequestEntityImport(false, true, prim.path);
            }
        }

        ImGui::EndMenu();
        ImGui::EndDisabled();
    }

    void SceneEditorLayer::DrawViewMenu()
    {
        ImGui::BeginDisabled(m_Scene == nullptr);
        
        if (!ImGui::BeginMenu("View") || !m_Scene)
        {
            ImGui::EndDisabled();
            return;
        }

        auto& context = m_Scene->GetContext();
        auto* panels = context.Panels;

        // Core Panels
        ImGui::MenuItem("Entity Hierarchy", nullptr, &panels->ShowEntityHierarchy);
        ImGui::MenuItem("Entity Properties", nullptr, &panels->ShowEntityComponents);
        ImGui::MenuItem("Material Editor", nullptr, &panels->ShowEntityMaterials);
        ImGui::MenuItem("Simulation WatchList", nullptr, &panels->ShowEntitySimulated);
        ImGui::MenuItem("Scene Environment", nullptr, &panels->ShowEnvironmentSettings);

        ImGui::Separator();

        // Analysis Panels
        ImGui::MenuItem("Force Analyser", nullptr, &panels->ShowForceAnalysisPanel);
        ImGui::MenuItem("Energy Analyser", nullptr, &panels->ShowEnergyPanel);
        ImGui::MenuItem("Momentum Analyser", nullptr, &panels->ShowMomentumPanel);
        ImGui::MenuItem("Acceleration Analyser", nullptr, &panels->ShowAccelerationPanel);
        ImGui::MenuItem("Trajectory Analyser", nullptr, &panels->ShowTrajectoryPanel);

        ImGui::EndMenu();

        ImGui::EndDisabled();
    }

    void SceneEditorLayer::DrawAboutMenu()
    {
        if (!ImGui::BeginMenu("About"))
            return;

        if (ImGui::MenuItem("About Motion Engine"))
        {
            m_ShowAboutBox = !m_ShowAboutBox;
        }

        ImGui::EndMenu();
    }

    void SceneEditorLayer::DrawSimulationControls()
    {
        // Get simulation state
        auto* sim = (m_Scene ? m_Scene->GetContext().Simulation : nullptr);
        SceneSimulation::SimulationState simState = sim ? sim->State : SceneSimulation::SimulationState::IDLE;

        // Determine button states
        bool canPlay  = sim && (simState == SceneSimulation::SimulationState::IDLE || 
                                simState == SceneSimulation::SimulationState::PAUSED);
        bool canPause = sim && (simState == SceneSimulation::SimulationState::RUNNING);
        bool canStop  = sim && (simState == SceneSimulation::SimulationState::RUNNING || 
                                simState == SceneSimulation::SimulationState::PAUSED);

        // Calculate centering
        ImGuiStyle& style = ImGui::GetStyle();
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        
        const ImVec2 buttonSize(72.0f, ImGui::GetFrameHeight());
        const float spacing = style.ItemSpacing.x;
        const float controlsWidth = (buttonSize.x * 3) + (spacing * 2) + 
                                    spacing * 1.5f + 
                                    ImGui::CalcTextSize("Simulation: RUNNING").x;
        
        const float offsetX = (contentWidth - controlsWidth) * 0.5f;
        if (offsetX > 0.0f)
        {
            ImGui::SameLine(0.0f, offsetX);
        }

        // Draw transport controls
        ImGui::PushID("SimulationControls");
        
        ImGui::BeginDisabled(!canPlay);
        if (ImGui::Button(ICON_MD_PLAY_ARROW, buttonSize))
        {
            SceneSerializer::SerializeRuntime(m_Scene.get(), m_ScenePath / ".motion_temp" / "scene_sim.mes");
            sim->State = SceneSimulation::SimulationState::RUNNING;
            sim->InSimulation = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("%s", canPlay ? "Play / Resume" : "Play (disabled)");
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!canPause);
        if (ImGui::Button(ICON_MD_PAUSE, buttonSize))
        {
            sim->State = SceneSimulation::SimulationState::PAUSED;
            sim->InSimulation = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("%s", canPause ? "Pause" : "Pause (disabled)");
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!canStop);
        if (ImGui::Button(ICON_MD_STOP, buttonSize))
        {
            sim->InSimulation = false;
            sim->State = SceneSimulation::SimulationState::IDLE;
            SceneSerializer::DeserializeRuntime(m_Scene.get(), m_ScenePath / ".motion_temp" / "scene_sim.mes");
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("%s", canStop ? "Stop" : "Stop (disabled)");
        }
        ImGui::EndDisabled();

        ImGui::PopID();

        // Draw status label
        ImGui::SameLine(0.0f, spacing * 1.5f);
        ImGui::TextUnformatted("Simulation:");
        ImGui::SameLine();

        DrawSimulationStatus(simState);
    }

    void SceneEditorLayer::DrawSimulationStatus(SceneSimulation::SimulationState state)
    {
        switch (state)
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
                ImGui::DockBuilderDockWindow("Simulation Watch List", dock_rbottom_id);

                ImGui::DockBuilderDockWindow("Acceleration Analysis", dock_bottom_id);
                ImGui::DockBuilderDockWindow("Energy Analysis", dock_bottom_id);
                ImGui::DockBuilderDockWindow("Force Analysis", dock_bottom_id);
                ImGui::DockBuilderDockWindow("Momentum Analysis", dock_bottom_id);
                ImGui::DockBuilderDockWindow("Trajectory Prediction", dock_bottom_id);

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