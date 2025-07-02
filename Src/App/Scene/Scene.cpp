#include "CorePCH.hpp"
#include "Scene.hpp"

namespace Motion::App
{
    Scene::Scene(const glm::vec2& viewportSize)
    {
        m_MainCamera = std::make_shared<MainCamera>(viewportSize.x, viewportSize.y, false);
    }

    Scene::~Scene()
    {

    }

    void Scene::OnUpdate(Motion::Core::WindowHandle handle, Motion::Core::Timer deltaTime)
    {
        RenderScene(handle);
    }

    void Scene::OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e)
    {
        m_MainCamera->OnEvents(handle, e);
    }

    void Scene::OnUIRenders(Motion::Core::WindowHandle handle)
    {
        RenderEntities(handle);
    }

    void Scene::OnViewportSizeChanges(float width, float height)
    {
        m_MainCamera->SetAspectRatio(width, height);
    }

    void Scene::RenderScene(Motion::Core::WindowHandle handle)
    {
        SceneRenderer::BeginScene(m_MainCamera->GetCameraMatrix());

        for (auto& entity : m_Entities)
        {
            if(entity->HasComponent<Motion::Core::MeshComponent>() && entity->HasComponent<Motion::Core::TransformComponent>())
            {
                auto& mesh = entity->GetComponent<Motion::Core::MeshComponent>();
                auto& transform = entity->GetComponent<Motion::Core::TransformComponent>();
                SceneRenderer::SubmitModel(mesh.Mesh, transform.GetTransform());
            }
        }

        SceneRenderer::EndScene();
        SceneRenderer::Flush();
    }

    void Scene::RenderEntities(Motion::Core::WindowHandle handle)
    {
        ImGui::Begin("Scene Entities");

        if( ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems) )
		{
			if( ImGui::MenuItem("Import Model") )
			{
                std::weak_ptr<Motion::Core::IWindow> window = Motion::Core::WindowManager::Get(handle);
                if( !window.expired() )
                {
                    auto windowPtr = window.lock();
                    std::filesystem::path filePath = Motion::Core::DialogBoxes::OpenFileDialog(
                        windowPtr->GetNativeWindow(), "Import Model",
                        "Model Files (*.fbx;*.obj;*.gltf;*.glb)\0*.fbx;*.obj;*.gltf;*.glb\0All Files (*.*)\0*.*\0");

                    if( !filePath.empty() )
                    {
                        std::shared_ptr<Motion::Core::Model> model = Motion::Core::AssetManager::LoadModel(filePath.filename().string(), filePath);
                        if( model )
                        {
                            std::shared_ptr<Motion::Core::Entity> entity = Motion::Core::EntityBuilder::CreateEntity(filePath.filename().string());
                            entity->AddComponent<Motion::Core::TransformComponent>();
                            entity->AddComponent<Motion::Core::MeshComponent>(filePath.filename().string(), model);
                            m_Entities.push_back(entity);
                            m_SelectedEntity = entity;
                        }
                        else
                        {
                            MOTION_ERROR("Failed to load model from file: {}", filePath.string());
                        }
                    }
                }
			}

			ImGui::EndPopup();
		}

		if( ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() )
		{
			m_SelectedEntity = Motion::Core::EntityBuilder::ENULL;
		}

        for( uint32_t i = 0; i < m_Entities.size(); i++ )
		{
			std::shared_ptr<Motion::Core::Entity> entity = m_Entities[i];
			auto& tag = entity->GetComponent<Motion::Core::TagComponent>();
			ImGuiTreeNodeFlags flags = ( ( m_SelectedEntity == entity ) ? ImGuiTreeNodeFlags_Selected : 0 ) | ImGuiTreeNodeFlags_OpenOnArrow;
			flags |= ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

			bool Opend = ImGui::TreeNodeEx((void*)tag.ID, flags, tag.Tag.c_str());
			if( ImGui::IsItemClicked() )
			{
				m_SelectedEntity = entity;
			}

			if( Opend )
			{
				ImGui::TreePop();
			}
		}

        ImGui::Begin("Properties");
		if( m_SelectedEntity && m_SelectedEntity != Motion::Core::EntityBuilder::ENULL )
		{
			RenderComponents(handle, m_SelectedEntity);
		}
		ImGui::End();

        ImGui::End();
    }

    template<typename T, typename UIFunc>
    static void DrawComponentControls(const std::string& name, const std::shared_ptr<Motion::Core::Entity>& entity, UIFunc uiFunc)
    {
        static const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        if( entity->HasComponent<T>() )
        {
            auto& component = entity->GetComponent<T>();
            bool open = ImGui::TreeNodeEx(( void* )component.ID, treeNodeFlags, name.c_str());

            if( open )
            {
                uiFunc(component);
                ImGui::TreePop();
            }
        }
    }

    void Scene::RenderComponents(Motion::Core::WindowHandle handle, const std::shared_ptr<Motion::Core::Entity>& entity)
    {
        if( entity->HasComponent<Motion::Core::TagComponent>() )
		{
			auto& tag = entity->GetComponent<Motion::Core::TagComponent>();

			char buffer [256];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, sizeof(buffer), tag.Tag.c_str());

			if( ImGui::InputText("Tag", buffer, sizeof(buffer)) )
			{
				tag.Tag = std::string(buffer);
			}
		}

        DrawComponentControls<Motion::Core::TransformComponent>("Transform", entity, [](auto& component)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10.0f, 0.0f });

			Motion::Core::UI::CustomControl::DragControllerVec3("Translation", component.Translation, 0.0f);
			Motion::Core::UI::CustomControl::DragControllerVec3("Rotation", component.Rotation, 0.0f);
			Motion::Core::UI::CustomControl::DragControllerVec3("Scale", component.Scale, 1.0f);

			ImGui::PopStyleVar();
		});


        //[TODO]: Other components can be added here
    }

    void Viewport::Update(const Motion::Core::FrameBufferSpecification & spec)
    {
        FrameSpec = spec;
        Size = { (float)spec.Width, (float)spec.Height };
    }

    void Viewport::Update(const glm::vec2 & size)
    {
        Size = size;
        FrameSpec.Width = (uint32_t)size.x;
        FrameSpec.Height = (uint32_t)size.y;
    }

    bool Viewport::SizeHasChanged(float width, float height)
    {
        return Size.x != width || Size.y != height || FrameSpec.Width != width || FrameSpec.Height != height;
    }
}