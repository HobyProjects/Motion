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

        glm::mat4 TransformMatrix{ 1.0f };
        glm::mat4 ViewProjectionMatrix{ 1.0f };

        Scene* ScenePtr{ nullptr };

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
        /**
         * @brief Returns the singleton instance of SceneRenderer.
         *
         * This method ensures that only one instance of SceneRenderer exists throughout the application.
         * It initializes the instance if it does not already exist.
         *
         * @return Reference to the singleton SceneRenderer instance.
         */
        [[nodiscard]] static SceneRenderer& GetInstance() noexcept
        {
            static SceneRenderer instance;
            return instance;
        }

    public:
        void BeginScene() noexcept;
        void Submit(Scene* scene) noexcept;
        void EndScene(TextureID skyBoxTextureID) noexcept;
        void Flush() noexcept;

        [[nodiscard]] std::uint32_t GetDrawCount() const noexcept { return m_DrawCount; }

    private:
        std::vector<SceneDrawCommand> m_DrawCommands;
        std::uint32_t m_DrawCount{ 0 };
    };


}