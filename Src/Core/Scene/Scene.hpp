#pragma once

#include <limits>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>

#include <reactphysics3d/reactphysics3d.h>
#include <entt/entt.hpp>

#include "Event.hpp"
#include "Buffers.hpp"
#include "Components.hpp"
#include "SceneEnviroment.hpp"
#include "ScenePanel.hpp"

namespace Motion
{
    class SceneSerializer;

    class Scene
    {
        public:
            Scene(UUID sceneID, const std::string& name, const glm::vec2& viewport = glm::vec2(1280.0f, 720.0f));
            ~Scene();

            void OnUpdate(WindowHandle handle, Timer deltaTime);
            void OnEvent(WindowHandle handle, IEvent& e);
            void OnUIRender(WindowHandle handle);
            
            void Submit();
            void SetApectRatio(const glm::vec2& size);

            void SelectedEntity(const entt::entity& entt);
            void EmplaceEntity(const entt::entity& entity) { m_RootEntities.emplace_back(std::move(entity)); }
            void RemoveEntity(const entt::entity& entity);

            void ForEachActiveEntity(const std::function<void(entt::registry&, entt::entity)>& fn);
            void ForEachEntity(const std::function<void(entt::registry&, entt::entity)>& fn);
            void ForEachRootEntity(const std::function<void(entt::registry&, entt::entity)>& fn);
            void ForEachNodeEntity(const entt::entity root, const std::function<void(entt::registry&, entt::entity)>& fn);

            [[nodiscard]] UUID GetID() const { return m_SceneID; };
            [[nodiscard]] std::string& GetName() { return m_SceneName; };
            [[nodiscard]] Camera3D& GetCamera() { return m_Camera; }
            [[nodiscard]] SceneEnvironment& GetEnvironment() { return m_Environment; }
            [[nodiscard]] std::uint32_t GetEntityCount() const { return m_RootEntities.size(); }
            [[nodiscard]] const bool IsRootEntity(entt::entity entity) const;
            [[nodiscard]] entt::entity& GetSelectedEntity() { return m_SelectedEntity; }
            [[nodiscard]] std::vector<entt::entity>& GetEntities() { return m_RootEntities; }
            [[nodiscard]] FrameBufferSpecification& GetSceneFrameSpecification() const { return m_Framebuffer->GetSpecification(); }


            [[nodiscard]] std::vector<entt::entity>::iterator begin() { return m_RootEntities.begin(); }
            [[nodiscard]] std::vector<entt::entity>::iterator end() { return m_RootEntities.end(); }
            [[nodiscard]] std::vector<entt::entity>::const_iterator begin() const { return m_RootEntities.begin(); }
            [[nodiscard]] std::vector<entt::entity>::const_iterator end() const { return m_RootEntities.end(); }

        private:
            bool OnMouseCursorPosChange(WindowHandle handle, EventMouseCursorMove& e);
            bool OnMouseWheelScrollEvent(WindowHandle handle, EventMouseWheelScroll& e);

        public:
            enum class Simulation
            {
                IDLE,
                RUNNING,
                PAUSE
            };

            void StartSimulation() { m_SimulationState = Simulation::RUNNING; m_InSimulationMode = true; }
            void PauseSimulation() { m_SimulationState = Simulation::PAUSE; m_InSimulationMode = true; }
            void StopSimulation()  { m_SimulationState = Simulation::IDLE; m_InSimulationMode = false; }
            
            void Steps(float deltaTime);
            void RefreshPhysicBodies();
            
            bool InSimulation() { return m_InSimulationMode; }
            Simulation GetSimulationState() { return m_SimulationState; }
            rp3d::PhysicsCommon& GetScenePhysicsCommons() { return m_PhyCommon; }
            rp3d::PhysicsWorld* GetScenePhysicsWorld() const { return m_PhyWorld; }
            rp3d::PhysicsWorld::WorldSettings& GetPhysicsWorldSettings() { return m_PhySettings; } 

        private:
            // Scene Entities Handling
            std::vector<entt::entity>   m_RootEntities{};
            entt::entity                m_SelectedEntity{ entt::null };
            entt::registry              m_SceneRegistry{};

            // Scene Common Properties
            Camera3D            m_Camera;
            SceneEnvironment    m_Environment;
            UUID                m_SceneID{};
            std::string         m_SceneName{};
            ScenePanelManager   m_PanelManager{};

            // Scene Framebuffer
            FrameTextureID      m_FrameTextureID{0};
            std::shared_ptr<IFrameBuffer> m_Framebuffer{ nullptr };

            float m_MouseX  = 0.0f,     m_MouseY    = 0.0f;
            float m_Yaw     = -90.0f,   m_Pitch     = 0.0f;

            // Simulation Controlers
            Simulation  m_SimulationState{Simulation::IDLE};
            bool        m_InSimulationMode{false};

            // Scene Physics
            rp3d::PhysicsCommon                m_PhyCommon;
            rp3d::PhysicsWorld*                m_PhyWorld{nullptr};
            rp3d::PhysicsWorld::WorldSettings  m_PhySettings{};

            const float     FIXED_STEPS_PERFRAME    = 8;
            const float     FIXED_STEPS             = 1.0f / 120.0f;
            float           ACCUMULATOR             = 0.0f;
            static constexpr float SI_GRAVITY      = 9.80665f;

        private:
            friend class SceneSerializer;

    };
}
