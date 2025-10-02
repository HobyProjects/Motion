#pragma once

#include "Entity.hpp"
#include "Components.hpp"
#include "Buffers.hpp"

#include "SceneRenderer.hpp"
#include "SceneEnviroment.hpp"
#include "ImguiLayer.hpp"

namespace Motion
{
    enum class SimulationState { Running, Stop, Paused };
    
    struct SceneViewport
    {
        FrameBufferSpecification FrameSpec{};
        glm::vec2 Size{ 0.0f, 0.0f };

        glm::vec2 MIN{0.0f};
        glm::vec2 MAX{0.0F};

        bool Focused{ false };
        bool Hovered{ false };

        SceneViewport() = default;
        ~SceneViewport() = default;
    };

    struct SceneSpecification
    {
        UUID SceneHandle{};
        std::string Name{};
        bool IsActive{ false };
        SceneViewport Viewport{};


        SceneSpecification() = default;
        ~SceneSpecification() = default;
    };

    class Scene
    {
        public:
            Scene() = default;
            Scene(const SceneSpecification& spec);
            ~Scene();

            void OnUpdate(WindowHandle handle, Timer deltaTime) noexcept;
            void OnEvent(WindowHandle handle, IEvent& e) noexcept;
            void OnViewportSizeChanges(const glm::vec2& size) noexcept;

            void SelectEntityIf();
            void SelectedEntity(const std::shared_ptr<Entity>& entity) { m_SelectedEntity = entity; }
            void EmplaceEntity(const std::shared_ptr<Entity>& entity) { m_Entities.emplace_back(std::move(entity)); }
            void RemoveEntity(const std::shared_ptr<Entity>& entity);

            [[nodiscard]] std::shared_ptr<Entity> GetSelectedEntity() const { return m_SelectedEntity; }
            [[nodiscard]] std::shared_ptr<Entity> PickEntity(const glm::vec2& mousePos, const glm::vec2& viewportSize);
            
            [[nodiscard]] Camera3D& GetCamera() { return m_Camera; }
            [[nodiscard]] glm::mat4 GetCameraProjection() const { return m_Camera.Projection; }
            [[nodiscard]] glm::mat4 GetCameraView() const { return m_Camera.View; }
            [[nodiscard]] glm::vec3 GetCameraPosition() const { return m_Camera.Position; }

            [[nodiscard]] std::vector<std::shared_ptr<Entity>>::iterator begin() { return m_Entities.begin(); }
            [[nodiscard]] std::vector<std::shared_ptr<Entity>>::iterator end() { return m_Entities.end(); }
            [[nodiscard]] std::vector<std::shared_ptr<Entity>>::const_iterator begin() const { return m_Entities.begin(); }
            [[nodiscard]] std::vector<std::shared_ptr<Entity>>::const_iterator end() const { return m_Entities.end(); }

            [[nodiscard]] SceneSpecification& GetSpecification() { return m_Specification; }
            [[nodiscard]] SceneEnvironment& GetEnvironment() { return m_Environment; }
            [[nodiscard]] std::string GetName() const { return m_Specification.Name; }
            [[nodiscard]] UUID GetID() const { return m_Specification.SceneHandle; }
            [[nodiscard]] bool IsActive() const { return m_Specification.IsActive; }

            void Activate(bool active) { m_Specification.IsActive = active; }
            const std::vector<std::shared_ptr<Entity>>& GetEntities() { return m_Entities; }
            
            bool InSimulationMode() const { return m_InSimulation; }
            SimulationState GetSimualtionState() { return m_SimState; }
            void GotoSimulation(SimulationState state);

        private:
            bool OnMouseCursorPosChange(WindowHandle handle, EventMouseCursorMove& e);
            bool OnMouseWheelScrollEvent(WindowHandle handle, EventMouseWheelScroll& e);

        private:
            std::vector<std::shared_ptr<Entity>>    m_Entities{};
            std::shared_ptr<Entity>                 m_SelectedEntity{ Entity::Empty() };

            SceneSpecification  m_Specification;
            Camera3D            m_Camera;
            SceneEnvironment    m_Environment;


            SimulationState     m_SimState{SimulationState::Stop};
            bool                m_InSimulation{false};

            float m_MouseX  = 0.0f, m_MouseY    = 0.0f;
            float m_Yaw     = -90.0f, m_Pitch   = 0.0f;

            friend class SceneRenderer;
    };
}