#include "CorePCH.hpp"
#include "SceneRenderer.hpp"

namespace Motion::App
{
    struct DrawCalls
    {
        std::shared_ptr<Motion::Core::Model> Model{nullptr};
        glm::mat4 Transform;
    };

    static glm::mat4 s_CurrentCameraMatrix;
    static std::vector<DrawCalls> s_DrawCalls;


    void SceneRenderer::BeginScene(const glm::mat4& cameraMatrix)
    {
        s_CurrentCameraMatrix = cameraMatrix;
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
        for(const auto& drawCall : s_DrawCalls)
        {
            drawCall.Model->Render(drawCall.Transform, s_CurrentCameraMatrix);
        }

        s_DrawCalls.clear();
    }

}