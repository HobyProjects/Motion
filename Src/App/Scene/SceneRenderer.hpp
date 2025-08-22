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
    struct SceneDrawCommand
    {
        UUID            SortKey{ 0 };

        Material*           MaterialPointer{ nullptr };
        Mesh*               MeshPointer{ nullptr };
        IEnvironment*       EnvironmentPointer{ nullptr };

        ShaderFeatureMask   ShaderMask{ GLSL_SHADER_EXT_NONE };
        DrawFlags           Flags{ DrawFlags::DepthTest | DrawFlags::CullFace };

        glm::mat4 ModelMatrix{ 1.0f };
        glm::mat4 ViewMatrix{ 1.0f };
        glm::mat4 ProjectionMatrix{ 1.0f };
        glm::mat3 NormalMatrix{ 1.0f };
        glm::vec3 CameraPosition{ 0.0f, 0.0f, 0.0f };

        glm::vec3 SunDirection{ 0.0f, -1.0f, 0.0f };
        glm::vec3 SunColor{ 1.0f, 1.0f, 1.0f };
        float     SunIntensity{ 0.0f };

        bool operator<(const SceneDrawCommand& other) const
        {
            return std::tie(SortKey, MaterialPointer, EnvironmentPointer, MeshPointer)
                < std::tie(other.SortKey, other.MaterialPointer, other.EnvironmentPointer, other.MeshPointer);
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