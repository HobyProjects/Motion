#pragma once

#include <glm/glm.hpp>

#include "MainCamera.hpp"
#include "Model.hpp"

namespace Motion::App
{
    class SceneRenderer
    {
        private:
            SceneRenderer() = default;
            ~SceneRenderer() = default;

            SceneRenderer(const SceneRenderer&) = delete;
            SceneRenderer& operator=(const SceneRenderer&) = delete;
            SceneRenderer(SceneRenderer&&) = delete;
            SceneRenderer& operator=(SceneRenderer&&) = delete;

        public:
            static void BeginScene(const std::shared_ptr<MainCamera>& camera);
            static void SubmitModel(const std::shared_ptr<Motion::Core::Model>& model, const glm::mat4& transform);
            static void EndScene();
            static void Flush();
    };
}