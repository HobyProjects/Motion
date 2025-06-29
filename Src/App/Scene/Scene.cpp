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
        RenderScene();
    }

    void Scene::OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e)
    {

    }

    void Scene::OnUIRenders(Motion::Core::WindowHandle handle)
    {

    }

    void Scene::OnViewportSizeChanges(float width, float height)
    {
        m_MainCamera->SetAspectRatio(width, height);
    }

    void Scene::RenderScene()
    {
        /*SceneRenderer::BeginScene(m_MainCamera->GetCameraMatrix());

        for (auto& entity : m_Entities)
        {
            if(entity->HasComponent<Motion::Core::MeshComponent>() && entity->HasComponent<Motion::Core::TransformComponent>())
            {
                auto& mesh = entity->GetComponent<Motion::Core::MeshComponent>();
                auto& transform = entity->GetComponent<Motion::Core::TransformComponent>();
                SceneRenderer::SubmitModel(mesh.Object, transform.GetTransform());
            }
        }

        SceneRenderer::EndScene();
        SceneRenderer::Flush(); */
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