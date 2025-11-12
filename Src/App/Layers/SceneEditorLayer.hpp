#pragma once

#include "Layer.hpp"
#include "Timer.hpp"
#include "DialogBoxes.hpp"
#include "Scene.hpp"

#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>

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
        std::unique_ptr<ToastManager> m_ToastManager;
        glm::vec2               m_CurrentViewportSize{ 1280.0f, 720.0f };
        std::shared_ptr<Scene>  m_Scene{ nullptr };
        std::string             m_SceneName{};
        std::filesystem::path   m_ScenePath{};

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

        void RenderViewport(SceneContext& context);
        void RenderNodeEntities(SceneContext& context, entt::entity root);
        void RenderTagAndModel(SceneContext& context, entt::entity e);
        void RenderTransform(SceneContext& context, entt::entity e);
        void RenderPhysics(SceneContext& context, entt::entity e);
        
        void RenderToolbarAndSearch();
        void RenderEntityHierarchy(SceneContext& context);
        void RenderEnvironmentSettings(SceneContext& context);
        void RenderSimulationWatchList(SceneContext& context);

        void DrawRigidBodyUI(RigidBodyComponent& rb);
        void DrawColliderUI(ColliderComponent& cc);
        void DrawMaterialUI(std::shared_ptr<Material>& mat);
        void DrawAttributes(std::shared_ptr<Material>& mat);
        void DrawTexturesSlots(std::shared_ptr<Material>& mat);
        void DrawLivePhysicsData(RigidBodyComponent& rb);

        bool RequestEntityImport(bool showDialog, bool shouldExport, std::filesystem::path path = {});

        std::vector<std::shared_ptr<BaseMaterial>> m_BaseMaterial{};
        std::vector<entt::entity> m_SimulationWatchList{};
        std::shared_ptr<IPlotExporter> m_PlotExporter{ nullptr };
        char m_SearchBuf[1024] = {};
        
        inline static std::map<entt::entity, bool> openMaterialEditors;

    public:
        static constexpr float EPSILON = 1e-6f;

        struct GizmoState
        {
            ImGuizmo::OPERATION Operation = ImGuizmo::TRANSLATE;
            ImGuizmo::MODE      Mode      = ImGuizmo::LOCAL;

            float TranslateSnap = 0.0f;
            float RotateSnapDeg = 0.0f;
            float ScaleSnap     = 0.0f;

            ImGuiKey KeyTranslate = ImGuiKey_1;
            ImGuiKey KeyRotate    = ImGuiKey_2;
            ImGuiKey KeyScale     = ImGuiKey_3;
            ImGuiKey KeyToggleMode= ImGuiKey_5;

            ImGuiKey KeySnap      = ImGuiKey_LeftCtrl;
            ImGuiKey KeyFineSnap  = ImGuiKey_LeftShift;

            float TranslateSnapCoarse = 0.1f;
            float RotateSnapCoarseDeg = 5.0f;
            float ScaleSnapCoarse     = 0.05f;

            float TranslateSnapFine   = 0.01f;
            float RotateSnapFineDeg   = 1.0f;
            float ScaleSnapFine       = 0.01f;

            void FillSnapTriplet(float outSnap[3]) const
            {
                const bool coarse = ImGui::IsKeyDown(KeySnap) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
                const bool fine   = ImGui::IsKeyDown(KeyFineSnap) || ImGui::IsKeyDown(ImGuiKey_RightShift);

                float sT = 0.f, sR = 0.f, sS = 0.f;
                if (coarse)      { sT = TranslateSnapCoarse; sR = RotateSnapCoarseDeg; sS = ScaleSnapCoarse; }
                else if (fine)   { sT = TranslateSnapFine;   sR = RotateSnapFineDeg;   sS = ScaleSnapFine;   }
                else             { sT = TranslateSnap;       sR = RotateSnapDeg;       sS = ScaleSnap;       }

                switch (Operation)
                {
                    case ImGuizmo::TRANSLATE: outSnap[0] = outSnap[1] = outSnap[2] = sT; break;
                    case ImGuizmo::ROTATE:    outSnap[0] = outSnap[1] = outSnap[2] = sR; break;
                    case ImGuizmo::SCALE:     outSnap[0] = outSnap[1] = outSnap[2] = sS; break;
                    default:                  outSnap[0] = outSnap[1] = outSnap[2] = 0.f; break;
                }
            }

            void HandleHotkeys()
            {
                if (!ImGui::IsWindowFocused()) return;
                if (ImGui::IsKeyPressed(KeyTranslate)) Operation = ImGuizmo::TRANSLATE;
                if (ImGui::IsKeyPressed(KeyRotate))    Operation = ImGuizmo::ROTATE;
                if (ImGui::IsKeyPressed(KeyScale))     Operation = ImGuizmo::SCALE;
                if (ImGui::IsKeyPressed(KeyToggleMode))
                    Mode = (Mode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            }
        };

        struct LightGizmoConfig
        {
            int   GizmoId         = 2;
            bool  Enabled         = true;
            float CameraDistance  = 6.0f;
            float IconScale       = 1.0f;
            float IconLength      = 1.5f;
            ImU32 IconColor       = IM_COL32(255, 255, 0, 255);
            bool  DrawBillboard   = true;
            bool  LockToViewAxis  = false;
            bool  AllowAxisFlip   = false;
            float GizmoSizeClip   = 0.18f;
            bool  UseLocalSpace   = true;

            bool  DrawRays        = true;
            float RayLength       = 0.75f;
            int   RayCount        = 6;
        };

        struct ViewportRect
        {
            ImVec2 min{}, max{};
            float width()  const { return max.x - min.x; }
            float height() const { return max.y - min.y; }
        };

        struct RayWS 
        { 
            glm::vec3 Origin; 
            glm::vec3 Direction; 
        };
    
    public:
        struct StatisticalData
        {
            float Mean = 0.0f;
            float Median = 0.0f;
            float StdDev = 0.0f;
            float Min = 0.0f;
            float Max = 0.0f;
            float Range = 0.0f;
            int SampleCount = 0;
            
            std::vector<float> Peaks;
            std::vector<float> PeakTimes;
            
            float IntegralValue = 0.0f; // Area under curve
            
            void Calculate(std::vector<ImVec2>& data)
            {
                if (data.empty())
                {
                    Reset();
                    return;
                }
                
                SampleCount = (int)data.size();
                
                std::vector<float> values;
                values.reserve(data.size());
                for (const auto& point : data)
                    values.push_back(point.y);
                
                float sum = std::accumulate(values.begin(), values.end(), 0.0f);
                Mean = sum / values.size();
                
                std::vector<float> sortedValues = values;
                std::sort(sortedValues.begin(), sortedValues.end());
                if (sortedValues.size() % 2 == 0)
                    Median = (sortedValues[sortedValues.size()/2 - 1] + sortedValues[sortedValues.size()/2]) / 2.0f;
                else
                    Median = sortedValues[sortedValues.size()/2];
                
                Min = *std::min_element(values.begin(), values.end());
                Max = *std::max_element(values.begin(), values.end());
                Range = Max - Min;
                
                float variance = 0.0f;
                for (float val : values)
                    variance += (val - Mean) * (val - Mean);
                variance /= values.size();
                StdDev = std::sqrt(variance);
                
                DetectPeaks(data);
                CalculateIntegral(data);
            }
            
            void DetectPeaks(std::vector<ImVec2>& data, float threshold = 0.1f)
            {
                Peaks.clear();
                PeakTimes.clear();
                
                if (data.size() < 3) return;
                
                for (size_t i = 1; i < data.size() - 1; i++)
                {
                    float prev = data[i-1].y;
                    float curr = data[i].y;
                    float next = data[i+1].y;
                    
                    if (curr > prev && curr > next && curr > (Mean + threshold * Range))
                    {
                        Peaks.push_back(curr);
                        PeakTimes.push_back(data[i].x);
                    }
                }
            }
            
            void CalculateIntegral(std::vector<ImVec2>& data)
            {
                IntegralValue = 0.0f;
                
                if (data.size() < 2) return;
                for (size_t i = 0; i < data.size() - 1; i++)
                {
                    float dt = data[i+1].x - data[i].x;
                    float avgHeight = (data[i].y + data[i+1].y) / 2.0f;
                    IntegralValue += avgHeight * dt;
                }
            }
            
            void Reset()
            {
                Mean = Median = StdDev = Min = Max = Range = IntegralValue = 0.0f;
                SampleCount = 0;
                Peaks.clear();
                PeakTimes.clear();
            }
        };

    private:
        void RenderStatisticalAnalysisPanel(SceneContext& context);
        void RenderStatisticsSection(const char* title, const StatisticalData& stats, const ImVec4& color);
        void RenderComparisonSection(const StatisticalData& linear, const StatisticalData& angular);
    
    };
}