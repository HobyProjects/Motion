#pragma once

#include "MainCamera.hpp"
#include "SceneRenderer.hpp"
#include "Entity.hpp"
#include "Components.hpp"
#include "Buffers.hpp"
#include "SceneEnviroment.hpp"

namespace Motion::App
{
    struct SceneViewport
    {
        Motion::Core::FrameBufferSpecification FrameSpec{};
        glm::vec2 Size{ 0.0f, 0.0f };

        bool Focused{ false };
        bool Hovered{ false };

        void Update(const Motion::Core::FrameBufferSpecification& spec);
        void Update(const glm::vec2& size);
        bool SizeHasChanged(float width, float height);
        void Clear();

        SceneViewport() = default;
        ~SceneViewport() = default;
    };


    class Scene
    {
        public:
            Scene(const glm::vec2& viewportSize);
            ~Scene();

            void OnUpdate(Motion::Core::WindowHandle handle, Motion::Core::Timer deltaTime);
            void OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e);
            void OnUIRenders(Motion::Core::WindowHandle handle);
            void OnViewportSizeChanges(float width, float height);

            /* SIMULATION */
            void StartSimulation();
            void StopSimulation();
            void ManualSimulation();

            void SetSimulationMode(SimulationMode mode) { m_Enviroment.SimMode = mode; }
            SimulationMode GetSimulationMode() { return m_Enviroment.SimMode; }

        private:
            /* IMGUI RENDERINGS */
            void RenderScene(Motion::Core::WindowHandle handle);
            void RenderEntities(Motion::Core::WindowHandle handle);
            void RenderComponents(Motion::Core::WindowHandle handle, const std::shared_ptr<Motion::Core::Entity>& entity);

            /* SIMULATION */
            void UpdatePhysicsComponents(Motion::Core::Timer deltaTime);

        private:
            std::shared_ptr<MainCamera> m_MainCamera{nullptr};
            std::vector<std::shared_ptr<Motion::Core::Entity>> m_Entities{};
            std::shared_ptr<Motion::Core::Entity> m_SelectedEntity{ Motion::Core::EntityBuilder::ENULL };

            /* SECENE ENVIROMENT */
            SceneEnviroment m_Enviroment{};
            bool m_SimulationStarted{ false };

            friend class SceneRenderer;
    };
}