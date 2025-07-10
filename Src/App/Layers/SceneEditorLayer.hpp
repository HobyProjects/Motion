#pragma once

#include "Layer.hpp"
#include "Timer.hpp"

#include "ImguiLayer.hpp"
#include "Scene.hpp"

namespace Motion::App
{
    class SceneEditorLayer : public Motion::Core::Layer
    {
        public:
            SceneEditorLayer(Motion::Core::WindowHandle, const std::shared_ptr<Motion::App::ImGuiLayer>& imguiLayer);
            virtual ~SceneEditorLayer() = default;

            virtual void OnAttach() override;
            virtual void OnDetach() override;
            virtual void OnUpdate(Motion::Core::WindowHandle handle, Motion::Core::Timer deltaTime) override;
            virtual void OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e);
            virtual void OnUIRender(Motion::Core::WindowHandle handle) override;

        private:
            void DrawDockspace();

        private:
            std::shared_ptr<Motion::Core::IFrameBuffer> m_Framebuffer{ nullptr };
            std::shared_ptr<Motion::App::Scene> m_Scene{ nullptr };
            SceneViewport m_Viewport{};

            float m_ViewportWidth{ 1280.0f }, m_ViewportHeight{ 720.0f };
    };
}