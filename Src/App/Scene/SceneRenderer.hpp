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
        UUID SortKey{ 0 };
        PhysicalBasedMaterialInstance* PBR_MatPtr{ 0 };
        StandardMaterialInstance* STD_MatPtr{ 0 };
        Mesh* MeshPtr{ 0 };

        TextureID IrradianceTexture{ 0 };
        TextureID PrefilteredTexture{ 0 };
        TextureID BRDFLUTTexture{ 0 };

        glm::mat4 ModelMatrix{ 1.0f };
        glm::mat4 ViewMatrix{ 1.0f };
        glm::mat4 ProjectionMatrix{ 1.0f };
        glm::mat3 NormalMatrix{ 1.0f };

        glm::vec3 CameraPosition{ 0.0f, 0.0f, 0.0f };
        glm::vec3* LightPosition{ nullptr };
        glm::vec3* LightColor{ nullptr };
        float* LightIntensity{ nullptr };

        ShadingMethod ShadingMethod{ ShadingMethod::Standard };

        SceneDrawCommand() = default;
        ~SceneDrawCommand() = default;

        bool operator<(const SceneDrawCommand& other) const
        {
            return std::tie(SortKey, PBR_MatPtr, STD_MatPtr, MeshPtr) < std::tie(other.SortKey, other.PBR_MatPtr, other.STD_MatPtr, other.MeshPtr);
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
        static void Submit(const std::shared_ptr<Scene>& scene, const std::shared_ptr<IEnvironment>& environment) noexcept;
        static void EndScene() noexcept;
    };
}