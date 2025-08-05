#pragma once

#include "Entity.hpp"
#include "Components.hpp"
#include "Buffers.hpp"

#include "SceneCamera.hpp"
#include "SceneRenderer.hpp"
#include "SceneEnviroment.hpp"

namespace Motion
{
    using SceneHandle = UUID;

    struct SceneViewport
    {
        FrameBufferSpecification FrameSpec{};
        glm::vec2 Size{ 0.0f, 0.0f };

        bool Focused{ false };
        bool Hovered{ false };

        void Update(const FrameBufferSpecification& spec);
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

        void OnUpdate(WindowHandle handle, Timer deltaTime);
        void OnEvent(WindowHandle handle, IEvent& e);
        void OnUIRenders(WindowHandle handle);
        void OnViewportSizeChanges(float width, float height);
        void SetName(const std::string& name) { m_Name = name; }
        void SetActive(bool active) { m_IsActive = active; }
        void SetSelectedEntity(const std::shared_ptr<Entity>& entity) { m_SelectedEntity = entity; }

        std::shared_ptr<Entity> PickEntity(const glm::vec2& mousePos, const glm::vec2& viewportSize);
        glm::mat4 GetViewProjectionMatrix() const { return m_SceneCamera.Camera.MVP; }
        glm::mat4 GetViewMatrix() const { return m_SceneCamera.Camera.View; }
        glm::mat4 GetProjectionMatrix() const { return m_SceneCamera.Camera.Projection; }
        glm::vec3 GetCameraPosition() const { return m_SceneCamera.Camera.Position; }
        Camera3D& GetSceneCamera() { return m_SceneCamera.Camera; }
        SceneEnvironment& GetEnvironment() { return m_Environment; }


        SceneHandle GetSceneID() const { return m_SceneID; }
        std::string GetSceneName() const { return m_Name; }
        bool IsActive() const { return m_IsActive; }

    private:
        void RenderEntities(WindowHandle handle);
        void RenderComponents(WindowHandle handle, const std::shared_ptr<Entity>& entity);

    private:
        SceneHandle m_SceneID{ 0 };
        std::string m_Name{ "Untitled Scene" };
        bool m_IsActive{ false };

        std::vector<std::shared_ptr<Entity>> m_Entities{};
        std::shared_ptr<Entity> m_SelectedEntity{ EntityFactory::EMPTYENTITY };

        SceneCamera m_SceneCamera;
        SceneEnvironment m_Environment{};

        friend class SceneRenderer;
    };
}