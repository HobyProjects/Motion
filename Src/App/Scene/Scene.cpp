#include "CorePCH.hpp"
#include "Scene.hpp"

namespace Motion
{
    Scene::Scene(const SceneSpecification& spec)
    {
        m_Specification             = spec;
        m_Camera.AspectRatio        = spec.Viewport.Size.x / spec.Viewport.Size.y;
        m_Camera.ViewportWidth      = spec.Viewport.Size.x;
        m_Camera.ViewportHeight     = spec.Viewport.Size.y;
        m_Camera.RotationEnabled    = false;
        m_Camera.Position           = glm::vec3(0.0f, 0.0f, 15.0f);
        m_Camera.TranslationSpeed   = 0.1;
    }

    Scene::~Scene() 
    {
        m_Entities.clear();
    }

    void Scene::OnUpdate(WindowHandle handle, Timer deltaTime) noexcept
    {
        if ((InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED))
        {
            glm::vec3 forward   = glm::normalize(m_Camera.Oriantaion);
            glm::vec3 right     = glm::normalize(glm::cross(forward, m_Camera.WorldUp));
    
            if (InputsHandler::GetKeyState(handle, KEY_W))              m_Camera.Position += forward * m_Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_S))              m_Camera.Position -= forward * m_Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_A))              m_Camera.Position -= right * m_Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_D))              m_Camera.Position += right * m_Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_LEFT_CONTROL))   m_Camera.Position.y -= m_Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_SPACE))          m_Camera.Position.y += m_Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
        }

        auto& KX = KinetiX::GetInstance();
        if (m_SimState == SimulationState::Running)
            KX.Step(m_Entities, deltaTime.GetDeltaTimeMilliseconds());
        
        KX.Refresh(m_Entities);  
        m_Camera.RefreshCameraMatrix();
    }

    void Scene::OnEvent(WindowHandle handle, IEvent& e) noexcept
    {
        EventHandler handler(handle, e);
        handler.Dispatch<EventMouseCursorMove>(EVENT_CALLBACK(OnMouseCursorPosChange));
        handler.Dispatch<EventMouseWheelScroll>(EVENT_CALLBACK(OnMouseWheelScrollEvent));
    }

    bool Scene::OnMouseCursorPosChange(WindowHandle handle, EventMouseCursorMove& e)
    {
        static bool firstMouseMovement = true;

        if (InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED)
        {
            float currentX = e.GetX();
            float currentY = e.GetY();

            if (firstMouseMovement)
            {
                m_MouseX = currentX;
                m_MouseY = currentY;
                firstMouseMovement = false;
                return false; // Prevent jump
            }

            float xOffset = currentX - m_MouseX;
            float yOffset = currentY - m_MouseY;

            m_MouseX = currentX;
            m_MouseY = currentY;

            xOffset *= m_Camera.Sensitivity;
            yOffset *= m_Camera.Sensitivity;

            m_Yaw += xOffset;
            m_Pitch -= yOffset;

            m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);

            glm::vec3 direction;
            direction.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
            direction.y = sin(glm::radians(m_Pitch));
            direction.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

            m_Camera.Oriantaion = glm::normalize(direction);
            m_Camera.RefreshCameraMatrix();
        }
        else
        {
            firstMouseMovement = true; // Reset when not holding RMB
        }

        return false;
    }

    bool Scene::OnMouseWheelScrollEvent(WindowHandle handle, EventMouseWheelScroll& e)
    {
        if (InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED)
        {
            m_Camera.PerspectiveFov -= (float)e.OffsetY();
            if (m_Camera.PerspectiveFov < 1.0f) m_Camera.PerspectiveFov = 1.0f;
            if (m_Camera.PerspectiveFov > 45.0f) m_Camera.PerspectiveFov = 45.0f;

            m_Camera.RefreshCameraMatrix();
        }

        return false;
    }

    void Scene::OnViewportSizeChanges(const glm::vec2& size) noexcept
    {
        m_Camera.SetAspectRatio(size.x, size.y);
    }

    void Scene::SelectEntityIf()
    {
        if (m_SelectedEntity != Entity::Empty() && !m_Entities.empty())
            m_SelectedEntity = m_Entities.front();
        else
            m_SelectedEntity = Entity::Empty();
    }

    void Scene::RemoveEntity(const std::shared_ptr<Entity>& entity)
    {
        if (!entity) return;

        auto it = std::find(m_Entities.begin(), m_Entities.end(), entity);
        if (it == m_Entities.end())
            return;

        const bool wasSelected = (m_SelectedEntity == *it);
        Entity::Destroy(*it);       
        m_Entities.erase(it);       

        if (wasSelected)
        {
            if (!m_Entities.empty())
                m_SelectedEntity = m_Entities.front();
            else
                m_SelectedEntity = Entity::Empty();
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
                m_InSimulation = false;  
                m_SimState     = state;  
                break;

            case SimulationState::Stop:
                m_InSimulation = false;
                m_SimState     = state;    
                break;
        }
    }

}