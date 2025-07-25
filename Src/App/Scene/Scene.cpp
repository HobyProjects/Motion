#include "CorePCH.hpp"
#include "Scene.hpp"

namespace Motion
{
    Scene::Scene(SceneHandle handle, const std::string& name, const glm::vec2& viewportSize)
    {
        m_SceneID = handle;
        m_Name = name;
        m_SceneCamera = std::make_unique<SceneCamera>(viewportSize.x, viewportSize.y, false);
    }

    Scene::~Scene()
    {

    }

    void Scene::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        m_SceneCamera->OnUpdate(handle, deltaTime);

        if (m_SimulationStarted)
        {
            UpdatePhysicsComponents(deltaTime);
        }
    }

    void Scene::OnEvent(WindowHandle handle, IEvent& e)
    {
        m_SceneCamera->OnEvents(handle, e);
    }

    void Scene::OnUIRenders(WindowHandle handle)
    {
        RenderEntities(handle);
    }

    void Scene::OnViewportSizeChanges(float width, float height)
    {
        m_SceneCamera->SetAspectRatio(width, height);
    }

    void Scene::StartSimulation()
    {
        auto& physicsAttri = m_Environment.Physics.GetSettings();
        physicsAttri.IsEnabled = true;
        m_Environment.StepModeEnabled = false;
        m_SimulationStarted = true;

    }

    void Scene::StopSimulation()
    {
        auto& physicsAttri = m_Environment.Physics.GetSettings();
        physicsAttri.IsEnabled = false;
        m_Environment.StepModeEnabled = false;
        m_SimulationStarted = false;
    }

    void Scene::ManualSimulation()
    {
        if (m_Environment.SimMode == SimulationMode::ManualStep)
        {
            auto& physicsAttri = m_Environment.Physics.GetSettings();
            physicsAttri.IsEnabled = true;
            m_Environment.StepModeEnabled = true;
            m_SimulationStarted = true;
        }
    }

    void Scene::RenderEntities(WindowHandle handle)
    {
        ImGui::Begin("Scene Entities");

        if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (m_SimulationStarted) ImGui::BeginDisabled();
            if (ImGui::MenuItem("Import StaticMesh"))
            {
                auto& windowManager = WindowManager::GetInstance();
                std::weak_ptr<IWindow> window = windowManager.GetWindow(handle);
                if (!window.expired())
                {
                    auto windowPtr = window.lock();
                    std::wstring filter = L"StaticMesh Files\0*.fbx;*.obj;*.gltf;*.glb;*.dae;*.stl;*.ply;\0\0";
                    std::filesystem::path filePath(DialogBoxes::OpenFileDialog(filter, L"Import Static Mesh").value_or(""));
                    if (!filePath.empty())
                    {
                        std::string fileName = filePath.filename().stem().string();
                        std::shared_ptr<StaticMesh> staticMesh = Importer::ImportModel(fileName, filePath);
                        if (staticMesh)
                        {
                            auto& entityFactory = EntityFactory::GetInstance();
                            std::shared_ptr<Entity> entity = entityFactory.CreateEntity(filePath.filename().string());
                            entity->AddComponent<TransformComponent>();
                            entity->AddComponent<MeshComponent>(fileName, staticMesh);
                            m_Entities.push_back(entity);
                            m_SelectedEntity = entity;
                        }
                        else
                        {
                            MOTION_ERROR("Failed to load static Mesh from file: {0}", filePath.string());
                        }
                    }
                }
            }

            ImGui::EndPopup();
            if (m_SimulationStarted) ImGui::EndDisabled();
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
        {
            m_SelectedEntity = EntityFactory::EMPTYENTITY;
        }

        for (uint32_t i = 0; i < m_Entities.size(); i++)
        {
            if (m_SimulationStarted) ImGui::BeginDisabled();
            std::shared_ptr<Entity> entity = m_Entities[i];
            auto& tag = entity->GetComponent<TagComponent>();
            ImGuiTreeNodeFlags flags = ((m_SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
            flags |= ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

            bool Opend = ImGui::TreeNodeEx((void*)tag.ID, flags, tag.Tag.c_str());
            if (ImGui::IsItemClicked())
            {
                m_SelectedEntity = entity;
            }

            if (Opend)
            {
                ImGui::TreePop();
            }
            if (m_SimulationStarted) ImGui::EndDisabled();
        }

        ImGui::Begin("Properties");
        if (m_SelectedEntity && m_SelectedEntity != EntityFactory::EMPTYENTITY)
        {
            RenderComponents(handle, m_SelectedEntity);
        }
        ImGui::End();

        ImGui::End();
    }

    template<typename T, typename UIFunc>
    static void DrawComponentControls(const std::string& name, const std::shared_ptr<Entity>& entity, UIFunc uiFunc, bool enabled = true)
    {
        static const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        if (entity->HasComponent<T>())
        {
            auto& component = entity->GetComponent<T>();
            bool open = ImGui::TreeNodeEx((void*)component.ID, treeNodeFlags, name.c_str());

            if (open)
            {
                if (!enabled) ImGui::BeginDisabled();
                uiFunc(component);
                ImGui::TreePop();
                if (!enabled) ImGui::EndDisabled();
            }
        }
    }

    void Scene::RenderComponents(WindowHandle handle, const std::shared_ptr<Entity>& entity)
    {
        if (entity->HasComponent<TagComponent>())
        {
            auto& tag = entity->GetComponent<TagComponent>();

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy_s(buffer, sizeof(buffer), tag.Tag.c_str());

            if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
            {
                tag.Tag = std::string(buffer);
            }
        }

        DrawComponentControls<TransformComponent>("Transform", entity,
            [](auto& component)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10.0f, 0.0f });

                CustomUIControl::DragControllerVec3("Translation", component.Translation, 0.0f);
                CustomUIControl::DragControllerVec3("Rotation", component.Rotation, 0.0f);
                CustomUIControl::DragControllerVec3("Scale", component.Scale, 1.0f);

                ImGui::PopStyleVar();

            }, !m_SimulationStarted
        );


        //[TODO]: Other components can be added here
    }

    void Scene::UpdatePhysicsComponents(Timer deltaTime)
    {
        if (m_SimulationStarted)
        {
            auto& physicsAttri = m_Environment.Physics.GetSettings();
            if (!physicsAttri.IsEnabled)
                return;

            switch (m_Environment.SimMode)
            {
            case SimulationMode::Realtime:
            {
                for (auto& entity : m_Entities)
                    m_Environment.Physics.Update(entity, deltaTime);

                break;
            }
            case SimulationMode::ManualStep:
            {
                for (auto& entity : m_Entities)
                    m_Environment.Physics.Update(entity, physicsAttri.FixedTimeStep);

                break;
            }
            }
        }
    }

    void SceneViewport::Update(const FrameBufferSpecification& spec)
    {
        FrameSpec = spec;
        Size = { (float)spec.Width, (float)spec.Height };
    }

    void SceneViewport::Update(const glm::vec2& size)
    {
        Size = size;
        FrameSpec.Width = (uint32_t)size.x;
        FrameSpec.Height = (uint32_t)size.y;
    }

    bool SceneViewport::SizeHasChanged(float width, float height)
    {
        return Size.x != width || Size.y != height || FrameSpec.Width != width || FrameSpec.Height != height;
    }
}