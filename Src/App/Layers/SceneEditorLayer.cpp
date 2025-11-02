#include "CorePCH.hpp"
#include "SceneUtils.hpp"
#include "SceneSerializer.hpp"
#include "SceneEditorLayer.hpp"

namespace Motion
{
    // ========================================================================
    // LIFECYCLE
    // ========================================================================
    
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
    }

    void SceneEditorLayer::OnDetach()
    {
        // Clean up any pending async operations
        if (m_SceneCreationOp.State == AsyncOperationState::InProgress)
        {
            m_SceneCreationOp.Reset();
        }
        
        if (m_SceneLoadOp.State == AsyncOperationState::InProgress)
        {
            m_SceneLoadOp.Reset();
        }
        
        if (m_EntityImportOp.State == AsyncOperationState::InProgress)
        {
            m_EntityImportOp.Reset();
        }
    }

    void SceneEditorLayer::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        if (!m_Scene) return;

        m_Scene->OnUpdate(handle, deltaTime);
        m_Scene->Submit();
    }

    void SceneEditorLayer::OnEvent(WindowHandle handle, IEvent& e)
    {
        if (m_Scene) m_Scene->OnEvent(handle, e);
    }

    void SceneEditorLayer::OnUIRender(WindowHandle handle)
    {
        BuildDockspace();
        
        // Handle async operations (non-blocking checks every frame)
        HandleSceneCreation();
        HandleSceneLoading();
        HandleEntityImport();

        // Render scene if loaded
        if (m_Scene) 
            RenderScene();

        // Show loading overlay for any in-progress operations
        if (m_SceneCreationOp.State == AsyncOperationState::InProgress ||
            m_SceneLoadOp.State == AsyncOperationState::InProgress ||
            m_EntityImportOp.State == AsyncOperationState::InProgress)
        {
            RenderLoadingOverlay();
        }

        // Show error modal if any operation failed
        if (m_SceneCreationOp.State == AsyncOperationState::Failed ||
            m_SceneLoadOp.State == AsyncOperationState::Failed ||
            m_EntityImportOp.State == AsyncOperationState::Failed)
        {
            RenderErrorModal();
        }
    }

    // ========================================================================
    // ASYNC SCENE CREATION (IMPROVED)
    // ========================================================================
    
    void SceneEditorLayer::HandleSceneCreation()
    {
        // Show dialog if requested
        if (m_SceneCreationRequest.ShowDialog)
        {
            ImGui::OpenPopup("Create New Scene");
            m_SceneCreationRequest.ShowDialog = false;
        }

        // Render creation dialog with validation
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal("Create New Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::vector<std::string> errors;

            // Helper functions
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

            ImGui::Text("Enter scene details:");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Scene name input
            ImGui::Text("Scene Name:");
            ImGui::SetNextItemWidth(360.0f);
            ImGui::InputText("##scenename", m_SceneCreationRequest.Name, sizeof(m_SceneCreationRequest.Name));
            
            ImGui::Spacing();

            // Location picker
            ImGui::Text("Save Location:");
            std::string pathStr = m_SceneCreationRequest.FilePath.string();
            ImGui::SetNextItemWidth(300.0f);
            if (ImGui::InputText("##path", &pathStr, ImGuiInputTextFlags_ReadOnly))
            {
                m_SceneCreationRequest.FilePath = std::filesystem::path(pathStr);
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Browse..."))
            {
                DialogBoxes::InitializeCOM();
                if (auto folder = DialogBoxes::SelectFolderDialog(); !folder.empty())
                {
                    m_SceneCreationRequest.FilePath = folder;
                }
                DialogBoxes::UninitializeCOM();
            }

            ImGui::Spacing();

            // Validation
            std::string name = m_SceneCreationRequest.Name;
            trim(name);

            if (name.empty())
                errors.emplace_back("Name cannot be empty.");
            else 
            {
                if (has_invalid_win_chars(name))
                    errors.emplace_back("Name contains invalid characters (< > : \" / \\ | ? *).");

#ifdef MOTION_PLATFORM_WINDOWS
                if (!name.empty() && (name.back() == ' ' || name.back() == '.'))
                    errors.emplace_back("Name cannot end with a space or period on Windows.");
#endif
            }

            if (m_SceneCreationRequest.FilePath.empty())
                errors.emplace_back("File path cannot be empty.");
            else if (!std::filesystem::exists(m_SceneCreationRequest.FilePath))
                errors.emplace_back("Parent directory does not exist.");

            // Show errors
            if (!errors.empty())
            {
                ImGui::Separator();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                for (const auto& e : errors)
                    ImGui::TextWrapped("%s", e.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Buttons
            bool canCreate = errors.empty();
            
            if (!canCreate) ImGui::BeginDisabled();
            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                // Submit scene creation to LOADER thread
                std::string sceneName = name;
                auto scenePath = m_SceneCreationRequest.FilePath / sceneName;
                
                m_SceneCreationOp.Start(LOADER::Submit([sceneName, scenePath]() -> std::shared_ptr<Scene>
                {
                    // Create specification
                    SceneSpecification spec;
                    spec.ID = UniqueIdentity::GetUniqueID();
                    spec.Name = sceneName;
                    spec.SavedPath = scenePath;

                    // Create scene on background thread
                    auto scene = std::make_shared<Scene>(spec);
                    
                    if (!scene)
                        throw std::runtime_error("Failed to create scene object");

                    // Create directories
                    std::filesystem::create_directories(scenePath);
                    std::filesystem::create_directory(scenePath / "Assets");
                    std::filesystem::create_directory(scenePath / ".motion_temp");
                    
                    return scene;
                }));

                m_ScenePath = scenePath;
                m_SceneCreationRequest.Reset();
                ImGui::CloseCurrentPopup();
            }
            if (!canCreate) ImGui::EndDisabled();
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_SceneCreationRequest.Reset();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // Check if creation completed (non-blocking check)
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
                    
                    MOTION_CORE_INFO("Scene created successfully: {}", m_ScenePath.string());
                }
                else
                {
                    throw std::runtime_error("Scene creation returned null");
                }
            }
            catch (const std::exception& e)
            {
                MOTION_CORE_ERROR("Failed to create scene: {}", e.what());
            }
        }
    }

    // ========================================================================
    // ASYNC SCENE LOADING (IMPROVED)
    // ========================================================================
    
    void SceneEditorLayer::HandleSceneLoading()
    {
        // Show file dialog if requested
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
                
                // Validate file exists
                if (!std::filesystem::exists(path))
                {
                    DialogBoxes::UninitializeCOM();
                    MOTION_CORE_ERROR("Scene file does not exist: {}", path.string());
                    m_SceneLoadRequest.ShowDialog = false;
                    return;
                }
                
                // Submit scene loading to LOADER thread
                m_SceneLoadOp.Start(LOADER::Submit([path]() -> std::shared_ptr<Scene>
                {
                    // Load scene from disk on background thread
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

        // Check if loading completed (non-blocking check)
        if (m_SceneLoadOp.IsReady())
        {
            try
            {
                m_Scene = m_SceneLoadOp.GetResult();
                
                if (m_Scene)
                {
                    // Load ImGui layout
                    ImGuiIO& io = ImGui::GetIO();
                    io.IniFilename = nullptr;
                    std::string layoutFile = std::format("{}/mes-config.ini", m_SceneLoadRequest.FilePath.string());
                    ImGui::LoadIniSettingsFromDisk(layoutFile.c_str());
                    
                    MOTION_CORE_INFO("Scene loaded successfully: {}", m_SceneLoadRequest.FilePath.string());
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

    // ========================================================================
    // ASYNC ENTITY IMPORT (IMPROVED)
    // ========================================================================
    
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
            options.Filters = { {L"Mesh Files", L"*.fbx;*.obj;*.gltf;*.glb"} };
            
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

        // Validate file exists
        if (!std::filesystem::exists(m_EntityImportRequest.FilePath))
        {
            MOTION_CORE_ERROR("Import file does not exist: {}", m_EntityImportRequest.FilePath.string());
            return false;
        }

        // Submit import to LOADER thread
        ImportSettings settings{};
        settings.FilePath = m_EntityImportRequest.FilePath;
        settings.ShouldExport = m_EntityImportRequest.ShouldExport;
        settings.ExportPath = m_ScenePath;

        m_EntityImportOp.Start(LOADER::Submit([settings]() -> std::shared_ptr<ImportedResults>
        {
            // Import on background thread
            return Importer::ImportEntity(settings);
        }));

        return true;
    }

    void SceneEditorLayer::HandleEntityImport()
    {
        // Check if import completed (non-blocking check)
        if (m_EntityImportOp.IsReady())
        {
            try
            {
                auto results = m_EntityImportOp.GetResult();
                
                if (results && m_Scene)
                {
                    // Add imported entities to scene (on main thread)
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

    // ========================================================================
    // UI RENDERING - LOADING & ERROR STATES
    // ========================================================================
    
    void SceneEditorLayer::RenderLoadingOverlay()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;
        
        if (ImGui::Begin("LoadingOverlay", nullptr, flags))
        {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40, 40));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.98f, 0.98f, 0.95f));
            
            if (ImGui::BeginChild("LoadingContent", ImVec2(400, 200), true, 
                                 ImGuiWindowFlags_NoScrollbar))
            {
                // Animated loading spinner
                const float time = ImGui::GetTime();
                const float radius = 30.0f;
                const ImVec2 pos = ImGui::GetCursorScreenPos();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                
                const int num_segments = 30;
                const float angle_offset = time * 8.0f;
                
                for (int i = 0; i < num_segments; i++)
                {
                    const float a = ((float)i / (float)num_segments) * 2.0f * 3.14159f + angle_offset;
                    const float alpha = 1.0f - ((float)i / (float)num_segments);
                    const ImU32 segment_col = ImGui::GetColorU32(ImVec4(0.13f, 0.59f, 0.95f, alpha));
                    
                    draw_list->AddCircleFilled(
                        ImVec2(pos.x + 200 + cosf(a) * radius, pos.y + 60 + sinf(a) * radius),
                        3.0f, segment_col
                    );
                }
                
                ImGui::Dummy(ImVec2(0, 100));
                
                // Loading text
                const char* loadingText = "Loading...";
                float elapsedTime = 0.0f;
                
                if (m_SceneCreationOp.State == AsyncOperationState::InProgress)
                {
                    loadingText = "Creating Scene...";
                    elapsedTime = m_SceneCreationOp.GetElapsedSeconds();
                }
                else if (m_SceneLoadOp.State == AsyncOperationState::InProgress)
                {
                    loadingText = "Loading Scene...";
                    elapsedTime = m_SceneLoadOp.GetElapsedSeconds();
                }
                else if (m_EntityImportOp.State == AsyncOperationState::InProgress)
                {
                    loadingText = "Importing Model...";
                    elapsedTime = m_EntityImportOp.GetElapsedSeconds();
                }
                
                ImGui::SetCursorPosX((400 - ImGui::CalcTextSize(loadingText).x) * 0.5f);
                ImGui::Text("%s", loadingText);
                
                char timeBuffer[32];
                snprintf(timeBuffer, sizeof(timeBuffer), "%.1f seconds", elapsedTime);
                ImGui::SetCursorPosX((400 - ImGui::CalcTextSize(timeBuffer).x) * 0.5f);
                ImGui::TextDisabled("%s", timeBuffer);
            }
            ImGui::EndChild();
            
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
        }
        ImGui::End();
        
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    }

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
                // Reset failed operations
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

    // ========================================================================
    // MENU BAR (IMPROVED)
    // ========================================================================
    
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
                    // Fire-and-forget save to LOADER thread
                    auto scene = m_Scene.get();
                    auto path = m_ScenePath;
                    auto sceneName = m_Scene->GetContext().Specification->Name;
                    
                    LOADER::Submit([scene, path, sceneName]()
                    {
                        std::string filename = std::format("{}.mes", sceneName);
                        SceneSerializer::Serialize(scene, path / filename);
                    });
                }
            }
            ImGui::EndDisabled();

            ImGui::Separator();
            
            if (ImGui::MenuItem("  Quit ", "Alt+F4"))
            {
                // Request application quit
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

        // === SIMULATION CONTROLS (from original) ===
        const float pad_x = style.ItemSpacing.x;
        const float content_min_x = ImGui::GetWindowContentRegionMin().x;
        const float content_max_x = ImGui::GetWindowContentRegionMax().x;
        const float bar_w = content_max_x - content_min_x;
        const float cur_x = ImGui::GetCursorPosX();

        const char* kPlay  = "Play";
        const char* kPause = "Pause";
        const char* kStop  = "Stop";

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
            // TODO: sim->Start() or sim->Resume()
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(canPlay ? "Play / Resume" : "Play (disabled)");
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!canPause);
        if (ImGui::Button(kPause, btnSz) && sim && canPause)
        {
            // TODO: sim->Pause()
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(canPause ? "Pause" : "Pause (disabled)");
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!canStop);
        if (ImGui::Button(kStop, btnSz) && sim && canStop)
        {
            // TODO: sim->Stop()
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

    // ========================================================================
    // DOCKSPACE
    // ========================================================================
    
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
                ImGui::DockBuilderDockWindow("Scene Viewport",   dock_main_id);
                ImGui::DockBuilderDockWindow("Scene Properties", dock_right_id);
                ImGui::DockBuilderFinish(dockspace_id);
            }
        }

        ImGui::End();
    }

    // ========================================================================
    // SCENE RENDERING
    // ========================================================================
    
    void SceneEditorLayer::RenderScene()
    {
        if(!m_Scene) return;

        auto& context = m_Scene->GetContext();
        ImGui::Begin("Scene Properties");
        {
            RenderToolbarAndSearch();
            RenderEntityHierarchy(context);
            RenderEnvironmentSettings(context);
        }
        ImGui::End();

        RenderViewport(context);
    }

    // ========================================================================
    // MATERIAL UI
    // ========================================================================
    
    void SceneEditorLayer::DrawMaterialUI(std::shared_ptr<Material>& mat)
    {
        if (!mat) return;

        if (ImGui::TreeNodeEx("Material Properties", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawAttributes(mat);
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Textures", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawTexturesSlots(mat);
            ImGui::TreePop();
        }
    }

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
                    if (auto it = std::ranges::find_if(m_BaseMaterial, [&](const auto& m){ return m->Name == selectedName; });
                        it != m_BaseMaterial.end())
                    {
                        mat->SetBaseMaterial(*it);
                    }
                });

            EndPropertyGrid();
        }

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

        if (mat->Has<CoreMaterialComponents>())
        {
            auto& C = mat->Get<CoreMaterialComponents>();
            if (BeginPropertyGrid("##core-pbr"))
            {
                ColorEdit4("Base Color",            C.BaseColorFactor);
                SliderFloat("Metallic Factor",      &C.MetallicFactor, 0.0f, 1.0f, "%.3f");
                SliderFloat("Roughness Factor",     &C.RoughnessFactor, 0.0f, 1.0f, "%.3f");
                SliderFloat("Normal Scaling",       &C.NormalScale,     0.0f, 1.0f, "%.3f");
                SliderFloat("Occlusion Strength",   &C.OcclusionStrength, 0.0f, 1.0f, "%.3f");
                ColorEdit3("Emissive Factor",       C.EmissiveFactor);
                SliderFloat("Emissive Strength",    &C.EmissiveStrength, 0.0f, 1.0f, "%.3f");
                SliderFloat("Opacity Factor",       &C.OpacityFactor, 0.0f, 1.0f, "%.3f");
                EndPropertyGrid();
            }
        }
        else
        {
            ImGui::TextDisabled("No materials assigned");
        }
    }

    void SceneEditorLayer::DrawTexturesSlots(std::shared_ptr<Material>& mat)
    {
        if (mat->Has<CoreMaterialComponents>())
        {
            auto& C = mat->Get<CoreMaterialComponents>();

            struct Row { const char* Label; std::shared_ptr<ITexture>& Tex; TextureType Type; };
            std::vector<Row> textures =
            {
                {"Base Color",  C.BaseColorTexture, TextureType::BaseColorTexture},
                {"Metallic",    C.MetallicTexture,  TextureType::MetallicTexture},
                {"Roughness",   C.RoughnessTexture, TextureType::RoughnessTexture},
                {"Normal",      C.NormalTexture,    TextureType::NormalTexture},
                {"Occlusion",   C.OcclusionTexture, TextureType::AmbientOcclusionTexture},
                {"Emissive",    C.EmissiveTexture,  TextureType::EmissiveTexture},
            };

            const int columns = 4;
            ImGui::BeginTable("##core-pbr", columns, ImGuiTableFlags_NoBordersInBody);
            for (size_t i = 0; i < textures.size(); ++i)
            {
                if (i % columns == 0) ImGui::TableNextRow();
                ImGui::TableNextColumn();
                TextureSlot(textures[i].Label, textures[i].Tex, textures[i].Type);
            }
            ImGui::EndTable();
        }
        else
        {
            ImGui::TextDisabled("No textures assigned");
        }
    }

    // ========================================================================
    // ENTITY RENDERING
    // ========================================================================
    
    void SceneEditorLayer::RenderNodeEntities(SceneContext& context, entt::entity root)
    {
        m_Scene->ForEachNodeEntity(root, [&](entt::entity e)
        {
            if (e == root) return;
            const TagComponent* tagOpt = context.Entities->Registry.try_get<TagComponent>(e);
            const char* label = tagOpt ? tagOpt->Tag.c_str() : "Unnamed";

            ImGui::PushID(static_cast<std::int32_t>(entt::to_integral(e)));

            const ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_SpanAvailWidth |ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

            const bool open = ImGui::TreeNodeEx("##node", flags, "%s %s", label, 
                (e == context.Entities->SelectedEntity) ? "*" : "");
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) m_Scene->SelectedEntity(e);

            if (open)
            {
                RenderTagAndModel(context, e);

                if (tagOpt && tagOpt->IsActive)
                {
                    RenderTransform(context, e);
                    RenderPhysics(context, e);
                    RenderMaterialEditor(context, e);
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

    void SceneEditorLayer::RenderTagAndModel(SceneContext& context, entt::entity e)
    {
        if (!context.Entities->Registry.any_of<TagComponent>(e)) return;

        auto& tag = context.Entities->Registry.get<TagComponent>(e);

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
    }

    void SceneEditorLayer::RenderTransform(SceneContext& context, entt::entity e)
    {
        if (auto* tr = context.Entities->Registry.try_get<TransformComponent>(e))
        {
            BeginPropertyGrid("##transform-grid");

            glm::vec3 t = tr->Translation;
            if (DragFloat3("Position (m)", t, 0.1f))
                tr->Translation = t;

            glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(tr->Rotation));
            if (DragFloat3("Rotation (deg)", eulerDeg, 1.0f))
                tr->Rotation = glm::normalize(glm::quat(glm::radians(eulerDeg)));

            glm::vec3 s = tr->Scale;
            if (DragFloat3("Scale (m)", s, 0.1f))
                tr->Scale = s;

            EndPropertyGrid();
        }
    }

    void SceneEditorLayer::RenderPhysics(SceneContext& context, entt::entity e)
    {
        auto* cc = context.Entities->Registry.try_get<ColliderComponent>(e);
        auto* rb = context.Entities->Registry.try_get<RigidBodyComponent>(e);
        if (!cc || !rb) return;

        BeginPropertyGrid("##physics-grid");

        DrawRigidBodyUI(*rb);
        DrawColliderUI(*cc);

        EndPropertyGrid();
    }

    void SceneEditorLayer::RenderMaterialEditor(SceneContext & context, entt::entity e)
    {
        if (auto* material = context.Entities->Registry.try_get<MaterialComponent>(e))
        {
            static bool isEditorOpen = false;
            if (ImGui::Button("Material Editor"))
                isEditorOpen = !isEditorOpen;

            if (isEditorOpen)
            {
                ImGui::SetNextWindowSize(ImVec2(600.0f, 400.0f), ImGuiCond_FirstUseEver);
                std::string w = std::string("Material Editor##") + std::to_string((uintptr_t)material->ID);
                if (ImGui::Begin(w.c_str(), &isEditorOpen, ImGuiWindowFlags_NoDocking))
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
            }
        }
    }

    void SceneEditorLayer::DrawRigidBodyUI(RigidBodyComponent & rb)
    {
        std::int32_t interaction = (rb.PhysicsBody->getType() == rp3d::BodyType::DYNAMIC) ? 1 : 0;
        ComboBox("Interaction", { "Static", "Dynamic" }, interaction,
            [&](std::int32_t idx, const std::string&)
            {
                if (idx == 0) { rb.Type = BodyType::Static;  rb.PhysicsBody->setType(rp3d::BodyType::STATIC); }
                if (idx == 1) { rb.Type = BodyType::Dynamic; rb.PhysicsBody->setType(rp3d::BodyType::DYNAMIC); }
            });

        auto* body = rb.PhysicsBody;

        float mass = static_cast<float>(body->getMass());
        if (DragFloat("Compute Mass", &mass, 0.001f, 0.0f, 1e10f)) body->setMass(mass);

        float linDamp = static_cast<float>(body->getLinearDamping());
        if (DragFloat("Linear Damping", &linDamp, 0.01f, 0.0f, 1.0f)) body->setLinearDamping(linDamp);

        float angDamp = static_cast<float>(body->getAngularDamping());
        if (DragFloat("Angular Damping", &angDamp, 0.01f, 0.0f, 1.0f)) body->setAngularDamping(angDamp);
    }

    void SceneEditorLayer::DrawColliderUI(ColliderComponent & cc)
    {
        float bounce = cc.Restitution;
        if (DragFloat("Bounce", &bounce, 0.001f, 0.0f, 1.0f))
        {
            cc.Collider->getMaterial().setBounciness(bounce);
            cc.Restitution = bounce;
        }

        float friction = cc.Friction;
        if (DragFloat("Friction", &friction, 0.001f, 0.0f, 1.0f))
        {
            cc.Collider->getMaterial().setFrictionCoefficient(friction);
            cc.Friction = friction;
        }

        float density = cc.MassDensity;
        if (DragFloat("Density", &density, 0.01f, 0.0f, FLT_MAX))
        {
            cc.Collider->getMaterial().setMassDensity(density);
            cc.MassDensity = density;
        }
    }

    void SceneEditorLayer::RenderToolbarAndSearch()
    {
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 50.0f);
        ImGui::InputTextWithHint("##SearchScenes", "Search scenes...", m_SearchBuf, sizeof(m_SearchBuf));
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("Import##Button", ImVec2(50.0f, 0.0f)))
        {
            RequestEntityImport(true, true);
        }

        ImGui::Separator();
    }

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

    void SceneEditorLayer::RenderEnvironmentSettings(SceneContext & context)
    {
        const ImGuiTreeNodeFlags flags =
              ImGuiTreeNodeFlags_Framed
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_AllowItemOverlap
            | ImGuiTreeNodeFlags_FramePadding
            | ImGuiTreeNodeFlags_DefaultOpen;

        if (ImGui::TreeNodeEx("##environment", flags, "Environment"))
        {
            ImGui::Indent();

            if (ImGui::CollapsingHeader("Light", flags))
            {
                auto& light = context.Physics->SunLight;
                BeginPropertyGrid("##sun-properties");

                DragFloat3("Direction", light.Direction);
                ColorEdit3("Color",     light.Color);
                DragFloat("Intensity",  &light.Intensity, 0.01f, 0.0f, 50.0f);
                ToggleSwitch("Show Direction", light.ShowGuizmo);

                EndPropertyGrid();
            }

            if (ImGui::CollapsingHeader("Physics World", flags))
            {
                auto& world = context.Physics->Settings;
                BeginPropertyGrid("##world-properties");

                std::string worldName = world.worldName.empty() ? "New World" : world.worldName;
                TextBox("World Name", worldName);

                glm::vec3 gravity = ToVec3(world.gravity);
                if (DragFloat3("Gravity", gravity, 0.01f, -50.0f, 50.0f))
                    world.gravity = ToVec3(gravity);

                float defaultRestitution = world.defaultBounciness;
                if (DragFloat("Default Restitution", &defaultRestitution, 0.01f, 0.0f, 1.0f))
                    world.defaultBounciness = defaultRestitution;

                float defaultFriction = world.defaultFrictionCoefficient;
                if (DragFloat("Default Friction", &defaultFriction, 0.01f, 0.0f, 1.0f))
                    world.defaultFrictionCoefficient = defaultFriction;

                ToggleSwitch("Allow Sleeping", world.isSleepingEnabled);

                EndPropertyGrid();

                if (ImGui::CollapsingHeader("Advanced Settings", flags))
                {
                    GridSpec spec;
                    spec.twoColumns = true;
                    spec.labelWidth = 300.0f;

                    BeginPropertyGrid("##world-advanced-properties", spec);

                    float velIters = static_cast<float>(world.defaultVelocitySolverNbIterations);
                    if (DragFloat("Velocity Solver Iterations", &velIters, 1.0f, 1.0f, 100.0f))
                        world.defaultVelocitySolverNbIterations = static_cast<uint32_t>(velIters);

                    float posIters = static_cast<float>(world.defaultPositionSolverNbIterations);
                    if (DragFloat("Position Solver Iterations", &posIters, 1.0f, 1.0f, 100.0f))
                        world.defaultPositionSolverNbIterations = static_cast<uint32_t>(posIters);

                    float rvThresh = world.restitutionVelocityThreshold;
                    if (DragFloat("Restitution Velocity Threshold", &rvThresh, 0.01f, 0.0f, 10.0f))
                        world.restitutionVelocityThreshold = rvThresh;

                    float sleepLin = world.defaultSleepLinearVelocity;
                    if (DragFloat("Sleep Linear Velocity", &sleepLin, 0.01f, 0.0f, 10.0f))
                        world.defaultSleepLinearVelocity = sleepLin;

                    float sleepAng = world.defaultSleepAngularVelocity;
                    if (DragFloat("Sleep Angular Velocity", &sleepAng, 0.01f, 0.0f, 10.0f))
                        world.defaultSleepAngularVelocity = sleepAng;

                    float cosAngle = world.cosAngleSimilarContactManifold;
                    if (DragFloat("Angle Similar Contact Manifold", &cosAngle, 0.01f, 0.0f, 1.0f))
                        world.cosAngleSimilarContactManifold = std::clamp(cosAngle, 0.0f, 1.0f);

                    float tBeforeSleep = world.defaultTimeBeforeSleep;
                    if (DragFloat("Time Before Sleep", &tBeforeSleep, 0.01f, 0.0f, 10.0f))
                        world.defaultTimeBeforeSleep = tBeforeSleep;

                    EndPropertyGrid();
                }
            }

            ImGui::Unindent();
            ImGui::TreePop();
        }
    }

    static SceneEditorLayer::ViewportRect ComputeViewportRect()
    {
        const ImVec2 winPos = ImGui::GetWindowPos();
        const ImVec2 crMin  = ImGui::GetWindowContentRegionMin();
        const ImVec2 crMax  = ImGui::GetWindowContentRegionMax();
        return { { winPos.x + crMin.x, winPos.y + crMin.y }, { winPos.x + crMax.x, winPos.y + crMax.y } };
    }

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

    static glm::vec3 ExtractDirectionFromMatrix(const glm::mat4& M)
    {
        const glm::vec3 fwd = glm::normalize(glm::vec3(M[2]));
        return -fwd;
    }
    
    static glm::vec3 ChooseDummyPosition(const Camera3D& cam, float distance = 6.0f)
    {
        const glm::mat4 invView = glm::inverse(cam.View);
        const glm::vec3 camFwd  = glm::normalize(glm::vec3(invView[2]) * -1.0f);
        return cam.Position + camFwd * distance;
    }


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
                    if (hit) m_Scene->SelectedEntity(result.Entity);
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
}