#include "CorePCH.hpp"
#include "Scene.hpp"

namespace Motion::App
{
    Scene::Scene()
    {

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

    void Scene::RenderScene()
    {
        SceneRenderer::BeginScene(m_MainCamera->GetCameraMatrix());

        for (auto& entity : m_Entities)
        {

        }

        SceneRenderer::EndScene();
        SceneRenderer::Flush();
    }
}