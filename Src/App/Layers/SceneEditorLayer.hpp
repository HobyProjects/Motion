#pragma once

#include "Layer.hpp"
#include "Timer.hpp"

#include "ImguiLayer.hpp"
#include "Scene.hpp"
#include "Environment.hpp"

namespace Motion
{
    class SceneViewPanel final : public IScenePanel
    {
    public:
        SceneViewPanel() = default;
        ~SceneViewPanel() = default;

        virtual std::string GetTitle() const override { return m_Title; }
        virtual PanelCategory GetCategory() const override { return PanelCategory::InspectorPanel; }
        virtual void RenderUI(ScenePanelContext& context) override;

    private:
        std::string m_Title{ "Project Scenes" };
    };

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
        void BuildDockspace();

    private:
        glm::vec2 m_CurrentViewportSize{ 1280.0f, 720.0f };
        std::shared_ptr<IFrameBuffer> m_Framebuffer{ nullptr };
        std::shared_ptr<IEnvironment> m_Environment{ nullptr };

        //SCENE
        SceneViewport m_Viewport{};
        std::shared_ptr<Scene> m_ActiveScene{ nullptr };
        std::vector<std::shared_ptr<Scene>> m_Scenes{};
        std::unordered_map<std::shared_ptr<Scene>, FrameTextureID> m_SceneTextures{};

        // Panel Management
        std::shared_ptr<ScenePanelManager> m_Panels{ nullptr };
    };
}