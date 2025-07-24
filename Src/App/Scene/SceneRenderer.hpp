#pragma once

#include <glm/glm.hpp>

#include "Entity.hpp"
#include "Model.hpp"
#include "SceneEnviroment.hpp"

namespace Motion
{
    class Scene; // forward declaration

    struct SceneDrawCommand
    {
        UUID SortKey{ 0 };
        UUID MaterialID{ 0 };
        UUID MeshID{ 0 };

        glm::mat4 Model{ 1.0f };
        glm::mat4 MVP{ 1.0f };

        glm::vec3 CameraPosition{ 0.0f, 0.0f, 0.0f };
        glm::vec3 LightPosition{ 0.0f, 0.0f, 0.0f };
        glm::vec3 LightColor{ 1.0f, 1.0f, 1.0f };
        float LightIntensity{ 1.0f };

        TextureID EnvironmentTexture{ 0 };

        SceneDrawCommand() = default;
        ~SceneDrawCommand() = default;

        bool operator<(const SceneDrawCommand& other) const
        {
            return std::tie(SortKey, MaterialID, MeshID) < std::tie(other.SortKey, other.MaterialID, other.MeshID);
        }
    };

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
    };


}