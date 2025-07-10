#pragma once

#include <glm/glm.hpp>

#include "Entity.hpp"
#include "MainCamera.hpp"
#include "Model.hpp"
#include "SceneEnviroment.hpp"

namespace Motion::App
{
    class Scene; // forward declaration

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
            static void BeginScene(Scene* currentScene, const glm::mat4& cameraMatrix);
            static void SubmitModel(const std::shared_ptr<Motion::Core::Model>& model, const glm::mat4& transform);
            static void EndScene();
            static void Flush();
    };
}