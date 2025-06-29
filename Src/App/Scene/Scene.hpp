#pragma once

#include "MainCamera.hpp"
#include "SceneRenderer.hpp"
#include "Entity.hpp"
#include "Components.hpp"
#include "Buffers.hpp"

namespace Motion::App
{
    struct Viewport
    {
        Motion::Core::FrameBufferSpecification FrameSpec{};
        glm::vec2 Size{ 0.0f, 0.0f };

        bool Focused{ false };
        bool Hovered{ false };

        void Update(const Motion::Core::FrameBufferSpecification& spec);
        void Update(const glm::vec2& size);
        bool SizeHasChanged(float width, float height);
        void Clear();

        Viewport() = default;
        ~Viewport() = default;
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

        private:
            void RenderScene();
            void RenderEntities();
            void RenderComponents(const std::shared_ptr<Motion::Core::Entity>& entity);

        private:
            std::shared_ptr<MainCamera> m_MainCamera{nullptr};
            std::vector<std::shared_ptr<Motion::Core::Entity>> m_Entities{};
            std::shared_ptr<Motion::Core::Entity> m_SelectedEntity{ Motion::Core::EntityBuilder::ENULL };
    };
}