#pragma once

#include "Entity.hpp"
#include "Components.hpp"
#include "Buffers.hpp"

#include "SceneCamera.hpp"
#include "SceneRenderer.hpp"
#include "SceneEnviroment.hpp"

namespace Motion::App
{
    using SceneHandle = Motion::Core::UUID;

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
        Scene(SceneHandle handle, const std::string& name, const glm::vec2& viewportSize);
        ~Scene();

        void OnUpdate(Motion::Core::WindowHandle handle, Motion::Core::Timer deltaTime);
        void OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e);
        void OnUIRenders(Motion::Core::WindowHandle handle);
        void OnViewportSizeChanges(float width, float height);
        void SetName(const std::string& name) { m_Name = name; }
        void SetActive(bool active) { m_IsActive = active; }

        SceneHandle GetSceneID() const { return m_SceneID; }
        std::string GetSceneName() const { return m_Name; }
        bool IsActive() const { return m_IsActive; }



        /* SIMULATION */
        void StartSimulation();
        void StopSimulation();
        void ManualSimulation();
        void SetSimulationMode(SimulationMode mode) { m_Environment.SimMode = mode; }

        SimulationMode GetSimulationMode() { return m_Environment.SimMode; }

    private:
        /* IMGUI RENDERINGS */
        void RenderEntities(Motion::Core::WindowHandle handle);
        void RenderComponents(Motion::Core::WindowHandle handle, const std::shared_ptr<Motion::Core::Entity>& entity);

        /* SIMULATION */
        void UpdatePhysicsComponents(Motion::Core::Timer deltaTime);

    private:
        SceneHandle m_SceneID{ 0 };
        std::string m_Name{ "Untitled Scene" };
        bool m_IsActive{ false };

        std::unique_ptr<SceneCamera> m_SceneCamera{ nullptr };
        std::vector<std::shared_ptr<Motion::Core::Entity>> m_Entities{};
        std::shared_ptr<Motion::Core::Entity> m_SelectedEntity{ Motion::Core::EntityFactory::EMPTYENTITY };

        /* SCENE ENVIRONMENT */
        SceneEnvironment m_Environment{};
        bool m_SimulationStarted{ false };

        friend class SceneRenderer;
    };
}