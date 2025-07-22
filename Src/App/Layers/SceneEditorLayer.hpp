#pragma once

#include "Layer.hpp"
#include "Timer.hpp"

#include "ImguiLayer.hpp"
#include "Scene.hpp"
#include "SkyBox.hpp"

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
        SceneViewport m_Viewport{};
        std::shared_ptr<Scene> m_ActiveScene{ nullptr };
        std::vector<std::shared_ptr<Scene>> m_Scenes{};
        float m_ViewportWidth{ 1280.0f }, m_ViewportHeight{ 720.0f };

        std::shared_ptr<Motion::Core::IFrameBuffer> m_Framebuffer{ nullptr };
        std::unique_ptr<Motion::Core::PostProcessor> m_PostProcessor{ nullptr };
        std::unique_ptr<Motion::Core::SkyBox> m_SkyBox{ nullptr };

        std::unordered_map<std::shared_ptr<Scene>, Motion::Core::FrameTextureID> m_SceneTextures{};
    };
}