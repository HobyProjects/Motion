#pragma once

#include <glm/glm.hpp>

namespace Motion
{
    class Scene; 

    class SceneRenderer
    {
        private:
            SceneRenderer()     = default;
            ~SceneRenderer()    = default;

            SceneRenderer(const SceneRenderer&)             = delete;
            SceneRenderer(SceneRenderer&&)                  = delete;
            SceneRenderer& operator=(const SceneRenderer&)  = delete;
            SceneRenderer& operator=(const SceneRenderer&&) = delete;

        public:
            static void BeginScene() noexcept;
            static void Submit(Scene* scene) noexcept;
            static void EndScene() noexcept;
    };
}