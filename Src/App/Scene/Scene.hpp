#pragma once

#include "MainCamera.hpp"
#include "SceneRenderer.hpp"
#include "Entity.hpp"
#include "Components.hpp"

namespace Motion::App
{
    class Scene
    {
        public:
            Scene();
            ~Scene();

            void OnUpdate(Motion::Core::WindowHandle handle, Motion::Core::Timer deltaTime);
            void OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e);
            void OnUIRenders(Motion::Core::WindowHandle handle);

        private:
            void RenderScene();

        private:
            std::shared_ptr<MainCamera> m_MainCamera{nullptr};
            std::vector<std::shared_ptr<Motion::Core::Entity>> m_Entities{};
    };
}