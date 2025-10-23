#pragma once

#include "Event.hpp"
#include "Buffers.hpp"
#include "Components.hpp"
#include "ModelImporter.hpp"

#include "SceneCommon.hpp"

namespace Motion
{
    class SceneSerializer;

    class Scene
    {
        public:
            Scene(const SceneSpecification& spec, const glm::vec2& viewport = glm::vec2(1280.0f, 720.0f));
            ~Scene();

            void OnUpdate(WindowHandle handle, Timer deltaTime);
            void OnEvent(WindowHandle handle, IEvent& e);
            
            void Submit();
            void SetApectRatio(const glm::vec2& size);

            void SelectedEntity(const entt::entity& entt);
            void EmplaceEntity(const entt::entity& entity);
            void RemoveEntity(const entt::entity& entity);

            void ForEachActiveEntity(const std::function<void(entt::entity)>& fn);
            void ForEachEntity(const std::function<void(entt::entity)>& fn);
            void ForEachRootEntity(const std::function<void(entt::entity)>& fn);
            void ForEachNodeEntity(const entt::entity root, const std::function<void(entt::entity)>& fn);
            
            void ApplyPhysics(float deltaTime);
            void RefreshPhysicBodies();
            
            [[nodiscard]] const bool IsRootEntity(entt::entity entity) const;
            [[nodiscard]] const bool IsNodeEntity(entt::entity entity) const;
            [[nodiscard]] SceneContext& GetContext() { return m_Context; }


        private:
            bool OnMouseCursorPosChange(WindowHandle handle, EventMouseCursorMove& e);
            bool OnMouseWheelScrollEvent(WindowHandle handle, EventMouseWheelScroll& e);

        private:
            SceneEntities       m_Entities{};
            SceneViewport       m_Viewport{};
            ScenePhysicsWorld   m_Physics{};
            SceneSimulation     m_Simulation{};
            SceneContext        m_Context{};
            SceneSpecification  m_Specification{};
            
            friend class SceneSerializer;
    };
}
