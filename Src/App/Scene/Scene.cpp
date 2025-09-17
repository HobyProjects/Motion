#include "CorePCH.hpp"
#include "Scene.hpp"

namespace Motion
{
    Scene::Scene(const SceneSpecification& spec)
    {
        m_Specification = spec;
        m_Camera = SceneCamera(spec.Viewport.Size.x, spec.Viewport.Size.y, false);
    }

    void Scene::OnUpdate(WindowHandle handle, Timer deltaTime) noexcept
    {
        m_Camera.OnUpdate(handle, deltaTime);

        if (m_SimState == SimulationState::Running)
        {
            auto& KX = KinetiX::GetInstance();
            KX.Step(deltaTime);
        }
    }

    void Scene::OnEvent(WindowHandle handle, IEvent& e) noexcept
    {
        m_Camera.OnEvents(handle, e);
    }

    void Scene::OnViewportSizeChanges(const glm::vec2& size) noexcept
    {
        m_Camera.SetAspectRatio(size.x, size.y);
    }

    void Scene::SelectEntityIf()
    {
        if (m_SelectedEntity != EntityFactory::EMPTYENTITY && !m_Entities.empty())
            m_SelectedEntity = m_Entities.front();
        else
            m_SelectedEntity = EntityFactory::EMPTYENTITY;
    }

    void Scene::RemoveEntity(const std::shared_ptr<Entity>& entity)
    {
        if (!entity) return;

        auto it = std::find(m_Entities.begin(), m_Entities.end(), entity);
        if (it == m_Entities.end())
            return;

        const bool wasSelected = (m_SelectedEntity == *it);
        auto& EF = EntityFactory::GetInstance();
        EF.DestroyEntity(*it);       
        m_Entities.erase(it);       

        if (wasSelected)
        {
            if (!m_Entities.empty())
                m_SelectedEntity = m_Entities.front();
            else
                m_SelectedEntity = EntityFactory::EMPTYENTITY;
        }
    }

    std::shared_ptr<Entity> Scene::PickEntity(const glm::vec2& mousePos, const glm::vec2& viewportSize)
    {
        return nullptr;
    }

    void Scene::GotoSimulation(SimulationState state)
    {
        switch(state)
        {
            case SimulationState::Running:
                m_InSimulation = true;
                m_SimState     = state;
                break;

            case SimulationState::Paused:
                m_InSimulation = false;   // don't tick while paused
                m_SimState     = state;
                m_PhysicsAcc   = 0.0;     // optional: freeze accumulation when pausing
                break;

            case SimulationState::Stop:
                m_InSimulation = false;
                m_SimState     = state;
                m_PhysicsAcc   = 0.0;     // ensure a clean restart
                break;
        }
    }

}