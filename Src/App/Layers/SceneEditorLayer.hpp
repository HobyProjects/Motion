#pragma once

#include "Layer.hpp"
#include "Timer.hpp"
#include "DialogBoxes.hpp"
#include "Scene.hpp"
#include "ScenePropertyPanels.hpp"

#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <deque>

namespace Motion
{
    class SceneEditorLayer : public Layer
    {
    public:
        SceneEditorLayer() : Layer("EditorLayer") {}
        virtual ~SceneEditorLayer() override = default;

        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnUpdate(WindowHandle handle, Timer deltaTime) override;
        virtual void OnEvent(WindowHandle handle, IEvent& e) override;
        virtual void OnUIRender(WindowHandle handle) override;

    private:
        std::unique_ptr<ScenePanel> m_ScenePanel;

        glm::vec2               m_CurrentViewportSize{ 1280.0f, 720.0f };
        std::shared_ptr<Scene>  m_Scene{ nullptr };
        std::string             m_SceneName{};
        std::filesystem::path   m_ScenePath{};

        char m_SearchBuf[1024] = {};
        bool m_ShowAboutBox{false};

        enum class AsyncOperationState
        {
            Idle,
            InProgress,
            Completed,
            Failed
        };

        template<typename T>
        struct AsyncOperation
        {
            std::future<T> Future{};
            AsyncOperationState State{ AsyncOperationState::Idle };
            std::chrono::steady_clock::time_point StartTime{};
            std::string ErrorMessage{};

            void Start(std::future<T>&& future)
            {
                Future = std::move(future);
                State = AsyncOperationState::InProgress;
                StartTime = std::chrono::steady_clock::now();
                ErrorMessage.clear();
            }

            bool IsReady() const
            {
                return State == AsyncOperationState::InProgress &&
                       Future.valid() &&
                       Future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }

            T GetResult()
            {
                if (Future.valid())
                {
                    try
                    {
                        T result = Future.get();
                        State = AsyncOperationState::Completed;
                        return result;
                    }
                    catch (const std::exception& e)
                    {
                        State = AsyncOperationState::Failed;
                        ErrorMessage = e.what();
                        throw;
                    }
                }
                throw std::runtime_error("Future is not valid");
            }

            void Reset()
            {
                Future = std::future<T>{};
                State = AsyncOperationState::Idle;
                ErrorMessage.clear();
            }

            float GetElapsedSeconds() const
            {
                if (State != AsyncOperationState::InProgress)
                    return 0.0f;
                
                auto now = std::chrono::steady_clock::now();
                return std::chrono::duration<float>(now - StartTime).count();
            }
        };

        struct SceneCreationRequest
        {
            char Name[256] = "MyScene";  
            std::filesystem::path FilePath{ DialogBoxes::GetSystemFolder(SystemFolder::Documents) };
            bool ShowDialog{ false };

            void Reset()
            {
                strcpy_s(Name, sizeof(Name), "MyScene");
                FilePath = DialogBoxes::GetSystemFolder(SystemFolder::Documents);
                ShowDialog = false;
            }
        };

        SceneCreationRequest m_SceneCreationRequest{};
        AsyncOperation<std::shared_ptr<Scene>> m_SceneCreationOp{};

        struct SceneLoadRequest
        {
            std::filesystem::path FilePath{};
            bool ShowDialog{ false };

            void Reset()
            {
                FilePath.clear();
                ShowDialog = false;
            }
        };

        SceneLoadRequest m_SceneLoadRequest{};
        AsyncOperation<std::shared_ptr<Scene>> m_SceneLoadOp{};
 
        struct EntityImportRequest
        {
            std::filesystem::path FilePath{};
            bool ShowDialog{ false };
            bool ShouldExport{ false };

            void Reset()
            {
                FilePath.clear();
                ShowDialog = false;
                ShouldExport = false;
            }
        };

        EntityImportRequest m_EntityImportRequest{};
        AsyncOperation<std::shared_ptr<ImportedResults>> m_EntityImportOp{};

    private:
        void BuildDockspace();
        void DrawMenuBar();
        
        void HandleSceneCreation();
        void HandleSceneLoading();
        void HandleEntityImport();
        
        void RenderScene();
        void RenderLoadingOverlay();
        void RenderErrorModal();

        void RenderToolbarAndSearch();
        void RenderEntityHierarchy(SceneContext& context);
        void RenderTagAndModel(SceneContext& context, entt::entity e);
        void RenderNodeEntities(SceneContext& context, entt::entity root);
        void RenderAbout();
    
        bool RequestEntityImport(bool showDialog, bool shouldExport, std::filesystem::path path = {});
    };
}