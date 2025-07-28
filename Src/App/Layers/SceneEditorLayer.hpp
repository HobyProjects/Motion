#pragma once

#include "Layer.hpp"
#include "Timer.hpp"

#include "ImguiLayer.hpp"
#include "Scene.hpp"
#include "Environment.hpp"

namespace Motion
{
    class SceneEditorLayer : public Layer
    {
    public:
        SceneEditorLayer(WindowHandle, const std::shared_ptr<ImGuiLayer>& imguiLayer);
        virtual ~SceneEditorLayer() = default;

        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnUpdate(WindowHandle handle, Timer deltaTime) override;
        virtual void OnEvent(WindowHandle handle, IEvent& e);
        virtual void OnUIRender(WindowHandle handle) override;

    private:
        void DrawDockspace();

    private:
        float m_ViewportWidth{ 1280.0f }, m_ViewportHeight{ 720.0f };
        std::shared_ptr<IFrameBuffer> m_Framebuffer{ nullptr };
        std::shared_ptr<IEnvironment> m_Environment{ nullptr };

        //SCENE
        SceneViewport m_Viewport{};
        std::shared_ptr<Scene> m_ActiveScene{ nullptr };
        std::vector<std::shared_ptr<Scene>> m_Scenes{};
        std::unordered_map<std::shared_ptr<Scene>, FrameTextureID> m_SceneTextures{};
    };
}