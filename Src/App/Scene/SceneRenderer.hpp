#pragma once

#include <glm/glm.hpp>

#include "Entity.hpp"
#include "Model.hpp"
#include "SceneEnviroment.hpp"
#include "Environment.hpp"

namespace Motion
{
    class Scene; // forward declaration

    struct SceneDrawCommand
    {
        // Sort/group key — e.g., per-view or per-camera id
        UUID   SortKey{ 0 };

        // Draw payload
        PhysicalBasedMaterialInstance* PBR_MatPtr{ nullptr };
        Mesh* MeshPtr{ nullptr };
        IEnvironment* EnvironmentPtr{ nullptr };

        // Transforms & camera
        glm::mat4 ModelMatrix{ 1.0f };
        glm::mat4 ViewMatrix{ 1.0f };
        glm::mat4 ProjectionMatrix{ 1.0f };
        glm::mat3 NormalMatrix{ 1.0f };
        glm::vec3 CameraPosition{ 0.0f, 0.0f, 0.0f };

        // Single directional light (“Sun”) pushed per view
        glm::vec3 SunDirection{ 0.0f, -1.0f, 0.0f };
        glm::vec3 SunColor{ 1.0f, 1.0f, 1.0f };
        float     SunIntensity{ 0.0f };

        // Pipeline hint (kept for compatibility)
        ShadingMethod ShadingMethod{ ShadingMethod::PhysicalBased };

        // Sort by view, then material, environment, mesh to reduce state changes
        bool operator<(const SceneDrawCommand& other) const
        {
            return std::tie(SortKey, PBR_MatPtr, EnvironmentPtr, MeshPtr)
                < std::tie(other.SortKey, other.PBR_MatPtr, other.EnvironmentPtr, other.MeshPtr);
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
        static void RenderSkyboxPass(Scene* scene) noexcept;

    private:
        static Scene* s_CurrentScene;
    };
}