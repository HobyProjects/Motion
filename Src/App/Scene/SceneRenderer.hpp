#pragma once

#include <glm/glm.hpp>

#include "Entity.hpp"
#include "Model.hpp"
#include "SceneEnviroment.hpp"
#include "Environment.hpp"
#include "Shaders.hpp"

namespace Motion
{
    class Scene; 

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
        static void BeginScene() noexcept;
        static void Submit(Scene* scene) noexcept;
        static void EndScene() noexcept;
        static void RenderSkyboxPass(Scene* scene) noexcept;
    };
}