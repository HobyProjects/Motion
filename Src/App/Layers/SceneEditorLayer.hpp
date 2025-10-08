#pragma once

#include "Layer.hpp"
#include "Timer.hpp"

#include "ImguiLayer.hpp"
#include "Scene.hpp"
#include "ScenePanel.hpp"

namespace Motion
{
    class SceneEditorLayer : public Layer
    {
        public:
            SceneEditorLayer() : Layer("EditorLayer") {};
            virtual ~SceneEditorLayer() override = default;

            virtual void OnAttach() override;
            virtual void OnDetach() override;

            virtual void OnUpdate(WindowHandle handle, Timer deltaTime) override;
            virtual void OnEvent(WindowHandle handle, IEvent& e) override;
            virtual void OnUIRender(WindowHandle handle) override;

            void LoadScene(const std::filesystem::path& path);
            void UnloadScene();

        private:
            void BuildDockspace();

        private:
            glm::vec2               m_CurrentViewportSize{ 1280.0f, 720.0f };
            std::shared_ptr<Scene>  m_Scene{ nullptr };     
    };
}