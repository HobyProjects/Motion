#include "CorePCH.hpp"
#include "SceneRenderer.hpp"

namespace Motion::App
{
    struct DrawCalls
    {
        std::shared_ptr<Motion::Core::Model> model;
        glm::mat4 transform;
    };

    static std::weak_ptr<MainCamera> s_CurrentCamera;
    static std::vector<DrawCalls> s_DrawCalls;


    void SceneRenderer::BeginScene(const std::shared_ptr<MainCamera>& camera)
    {
        s_CurrentCamera = camera;
        s_DrawCalls.clear();
    }

    void SceneRenderer::SubmitModel(const std::shared_ptr<Motion::Core::Model>& model, const glm::mat4& transform)
    {
        s_DrawCalls.push_back({ model, transform });
    }

    void SceneRenderer::EndScene()
    {
        // nothing here yet, but could do sorting, culling, etc.
    }

    void SceneRenderer::Flush()
    {
        if(!s_CurrentCamera.expired())
        {
            auto camera = s_CurrentCamera.lock();
            for(const auto& drawCall : s_DrawCalls)
            {
                drawCall.model->Render(drawCall.transform, camera->GetCameraMatrix());
            }

            s_DrawCalls.clear();
        }
    }

}