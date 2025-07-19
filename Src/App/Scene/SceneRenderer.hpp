#pragma once

#include <glm/glm.hpp>

#include "Entity.hpp"
#include "MainCamera.hpp"
#include "Model.hpp"
#include "SceneEnviroment.hpp"

namespace Motion::App
{
    class Scene; // forward declaration

    struct SceneDrawCommand
    {
        Motion::Core::UUID SortKey{ 0 };
        Motion::Core::UUID MaterialID{ 0 };
        Motion::Core::UUID MeshID{ 0 };

        glm::mat4 TransformMatrix{ 1.0f };
        glm::mat4 ViewProjectionMatrix{ 1.0f };

        bool operator<(const SceneDrawCommand& other) const
        {
            return std::tie(SortKey, MaterialID, MeshID) <
                std::tie(other.SortKey, other.MaterialID, other.MeshID);
        }
    };

    class SceneRenderer
    {
    public:
        SceneRenderer() = default;
        ~SceneRenderer() = default;

        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer& operator=(const SceneRenderer&) = delete;
        SceneRenderer(SceneRenderer&&) = delete;
        SceneRenderer& operator=(SceneRenderer&&) = delete;

    public:
        static SceneRenderer& GetInstance() noexcept
        {
            static SceneRenderer instance;
            return instance;
        }

    public:
        void Submit(const std::shared_ptr<Motion::Core::Entity>& entity, const glm::mat4& viewProjectionMatrix);
        void Flush();

    private:
        std::vector<SceneDrawCommand> m_DrawCommands;
    };


}