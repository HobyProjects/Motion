#pragma once

#include "Layer.hpp"
#include "Timer.hpp"
#include "DialogBoxes.hpp"
#include "Scene.hpp"

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
            
            float IntegralValue = 0.0f;
            
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

        // ==================== NEW NEWTONIAN PHYSICS FEATURES ====================
        
        // Force Analysis System
        struct ForceVector
        {
            glm::vec3 Force{0.0f};
            glm::vec3 ApplicationPoint{0.0f};
            std::string Name{"Unknown Force"};
            ImU32 Color{IM_COL32(255, 100, 100, 255)};
            bool IsActive{true};
            
            float GetMagnitude() const { return glm::length(Force); }
            glm::vec3 GetDirection() const 
            { 
                float mag = GetMagnitude();
                return mag > EPSILON ? Force / mag : glm::vec3(0.0f);
            }
        };
        
        struct ForceAnalysisData
        {
            std::vector<ForceVector> Forces;
            glm::vec3 NetForce{0.0f};
            glm::vec3 NetTorque{0.0f};
            float NetForceMagnitude{0.0f};
            
            bool ShowForceVectors{true};
            bool ShowNetForce{true};
            bool ShowComponents{false};
            float VectorScale{1.0f};
            float ArrowHeadSize{0.15f};
            
            void UpdateNetForce()
            {
                NetForce = glm::vec3(0.0f);
                for (const auto& force : Forces)
                {
                    if (force.IsActive)
                        NetForce += force.Force;
                }
                NetForceMagnitude = glm::length(NetForce);
            }
            
            void Clear()
            {
                Forces.clear();
                NetForce = glm::vec3(0.0f);
                NetTorque = glm::vec3(0.0f);
                NetForceMagnitude = 0.0f;
            }
        };
        
        // Energy Tracking System
        struct EnergyTracker
        {
            static constexpr size_t MAX_HISTORY = 500;
            
            std::deque<float> KineticEnergyHistory;
            std::deque<float> PotentialEnergyHistory;
            std::deque<float> TotalEnergyHistory;
            std::deque<float> TimeStamps;
            
            float CurrentKE{0.0f};
            float CurrentPE{0.0f};
            float CurrentTotal{0.0f};
            float InitialTotal{0.0f};
            float EnergyLoss{0.0f};
            
            bool TrackingEnabled{false};
            bool ShowKE{true};
            bool ShowPE{true};
            bool ShowTotal{true};
            float GravityMagnitude{9.81f};
            
            void AddSample(float ke, float pe, float time)
            {
                CurrentKE = ke;
                CurrentPE = pe;
                CurrentTotal = ke + pe;
                
                if (InitialTotal < EPSILON && CurrentTotal > EPSILON)
                    InitialTotal = CurrentTotal;
                
                EnergyLoss = InitialTotal - CurrentTotal;
                
                KineticEnergyHistory.push_back(ke);
                PotentialEnergyHistory.push_back(pe);
                TotalEnergyHistory.push_back(CurrentTotal);
                TimeStamps.push_back(time);
                
                if (KineticEnergyHistory.size() > MAX_HISTORY)
                {
                    KineticEnergyHistory.pop_front();
                    PotentialEnergyHistory.pop_front();
                    TotalEnergyHistory.pop_front();
                    TimeStamps.pop_front();
                }
            }
            
            float GetEnergyConservation() const
            {
                if (InitialTotal < EPSILON) return 100.0f;
                return (CurrentTotal / InitialTotal) * 100.0f;
            }
            
            void Reset()
            {
                KineticEnergyHistory.clear();
                PotentialEnergyHistory.clear();
                TotalEnergyHistory.clear();
                TimeStamps.clear();
                CurrentKE = CurrentPE = CurrentTotal = InitialTotal = EnergyLoss = 0.0f;
            }
        };
        
        // Momentum & Impulse System
        struct MomentumTracker
        {
            glm::vec3 LinearMomentum{0.0f};
            glm::vec3 AngularMomentum{0.0f};
            float LinearMagnitude{0.0f};
            float AngularMagnitude{0.0f};
            
            glm::vec3 InitialLinearMomentum{0.0f};
            glm::vec3 InitialAngularMomentum{0.0f};
            
            std::deque<glm::vec3> MomentumHistory;
            std::deque<float> TimeStamps;
            static constexpr size_t MAX_HISTORY = 300;
            
            bool ShowMomentumVector{true};
            bool TrackConservation{true};
            float VectorScale{0.5f};
            
            void Update(const glm::vec3& momentum, float time)
            {
                LinearMomentum = momentum;
                LinearMagnitude = glm::length(momentum);
                
                if (glm::length(InitialLinearMomentum) < EPSILON && LinearMagnitude > EPSILON)
                    InitialLinearMomentum = momentum;
                
                MomentumHistory.push_back(momentum);
                TimeStamps.push_back(time);
                
                if (MomentumHistory.size() > MAX_HISTORY)
                {
                    MomentumHistory.pop_front();
                    TimeStamps.pop_front();
                }
            }
            
            float GetConservationPercentage() const
            {
                float initialMag = glm::length(InitialLinearMomentum);
                if (initialMag < EPSILON) return 100.0f;
                return (LinearMagnitude / initialMag) * 100.0f;
            }
            
            void Reset()
            {
                LinearMomentum = glm::vec3(0.0f);
                AngularMomentum = glm::vec3(0.0f);
                InitialLinearMomentum = glm::vec3(0.0f);
                InitialAngularMomentum = glm::vec3(0.0f);
                LinearMagnitude = AngularMagnitude = 0.0f;
                MomentumHistory.clear();
                TimeStamps.clear();
            }
        };
        
        // Collision Analysis System
        struct CollisionEvent
        {
            entt::entity EntityA{entt::null};
            entt::entity EntityB{entt::null};
            glm::vec3 CollisionPoint{0.0f};
            glm::vec3 CollisionNormal{0.0f};
            float RelativeVelocity{0.0f};
            float CoefficientOfRestitution{0.0f};
            float ImpulseMagnitude{0.0f};
            float TimeStamp{0.0f};
            
            glm::vec3 VelocityABefore{0.0f};
            glm::vec3 VelocityBBefore{0.0f};
            glm::vec3 VelocityAAfter{0.0f};
            glm::vec3 VelocityBAfter{0.0f};
            
            float KEBefore{0.0f};
            float KEAfter{0.0f};
            float EnergyLoss{0.0f};
            
            enum class CollisionType
            {
                Elastic,
                Inelastic,
                PartiallyElastic,
                Unknown
            } Type{CollisionType::Unknown};
            
            void ClassifyCollision()
            {
                if (std::abs(CoefficientOfRestitution - 1.0f) < 0.05f)
                    Type = CollisionType::Elastic;
                else if (CoefficientOfRestitution < 0.1f)
                    Type = CollisionType::Inelastic;
                else
                    Type = CollisionType::PartiallyElastic;
            }
            
            const char* GetTypeName() const
            {
                switch (Type)
                {
                    case CollisionType::Elastic: return "Elastic";
                    case CollisionType::Inelastic: return "Inelastic";
                    case CollisionType::PartiallyElastic: return "Partially Elastic";
                    default: return "Unknown";
                }
            }
        };
        
        struct CollisionAnalyzer
        {
            std::vector<CollisionEvent> RecentCollisions;
            static constexpr size_t MAX_COLLISIONS = 50;
            
            bool EnableAnalysis{false};
            bool ShowCollisionPoints{true};
            bool ShowImpulseVectors{true};
            bool PlayCollisionSound{false};
            
            void RecordCollision(const CollisionEvent& event)
            {
                CollisionEvent evt = event;
                evt.ClassifyCollision();
                
                RecentCollisions.push_back(evt);
                
                if (RecentCollisions.size() > MAX_COLLISIONS)
                    RecentCollisions.erase(RecentCollisions.begin());
            }
            
            void Clear()
            {
                RecentCollisions.clear();
            }
            
            CollisionEvent* GetMostRecent()
            {
                return RecentCollisions.empty() ? nullptr : &RecentCollisions.back();
            }
        };
        
        // Acceleration Tracker
        struct AccelerationTracker
        {
            glm::vec3 CurrentAcceleration{0.0f};
            glm::vec3 PreviousVelocity{0.0f};
            float AccelerationMagnitude{0.0f};
            
            std::deque<glm::vec3> AccelerationHistory;
            std::deque<float> TimeStamps;
            static constexpr size_t MAX_HISTORY = 300;
            
            bool ShowVector{true};
            float VectorScale{1.0f};
            ImU32 VectorColor{IM_COL32(255, 200, 0, 255)};
            
            void Update(const glm::vec3& velocity, float deltaTime)
            {
                if (deltaTime > EPSILON)
                {
                    CurrentAcceleration = (velocity - PreviousVelocity) / deltaTime;
                    AccelerationMagnitude = glm::length(CurrentAcceleration);
                }
                
                PreviousVelocity = velocity;
                
                AccelerationHistory.push_back(CurrentAcceleration);
                TimeStamps.push_back(TimeStamps.empty() ? 0.0f : TimeStamps.back() + deltaTime);
                
                if (AccelerationHistory.size() > MAX_HISTORY)
                {
                    AccelerationHistory.pop_front();
                    TimeStamps.pop_front();
                }
            }
            
            void Reset()
            {
                CurrentAcceleration = glm::vec3(0.0f);
                PreviousVelocity = glm::vec3(0.0f);
                AccelerationMagnitude = 0.0f;
                AccelerationHistory.clear();
                TimeStamps.clear();
            }
        };
        
        // Newton's Laws Interactive Panel
        struct NewtonsLawsDemo
        {
            int CurrentLaw{0}; // 0 = First, 1 = Second, 2 = Third
            
            // First Law (Inertia) Demo
            struct FirstLawDemo
            {
                bool ShowInertiaLine{true};
                bool HighlightBalancedForces{true};
                float FrictionCoefficient{0.1f};
            } FirstLaw;
            
            // Second Law (F=ma) Demo  
            struct SecondLawDemo
            {
                bool ShowForceEquation{true};
                bool ShowAccelerationVector{true};
                bool EnableMassSlider{false};
                float TargetMass{1.0f};
                glm::vec3 AppliedForce{0.0f};
            } SecondLaw;
            
            // Third Law (Action-Reaction) Demo
            struct ThirdLawDemo
            {
                bool ShowReactionForces{true};
                bool HighlightPairs{true};
                entt::entity EntityA{entt::null};
                entt::entity EntityB{entt::null};
            } ThirdLaw;
        };
        
        // Trajectory Prediction System
        struct TrajectoryPredictor
        {
            std::vector<glm::vec3> PredictedPath;
            bool EnablePrediction{false};
            bool ShowPredictionPath{true};
            int PredictionSteps{50};
            float TimeStep{0.1f};
            ImU32 PathColor{IM_COL32(100, 255, 100, 150)};
            
            void PredictTrajectory(const glm::vec3& position, const glm::vec3& velocity, 
                                  const glm::vec3& acceleration, float mass)
            {
                PredictedPath.clear();
                PredictedPath.reserve(PredictionSteps);
                
                glm::vec3 pos = position;
                glm::vec3 vel = velocity;
                
                for (int i = 0; i < PredictionSteps; ++i)
                {
                    PredictedPath.push_back(pos);
                    vel += acceleration * TimeStep;
                    pos += vel * TimeStep;
                }
            }
            
            void Clear()
            {
                PredictedPath.clear();
            }
        };
        
        // Physics Analysis State
        struct PhysicsAnalysisState
        {
            entt::entity SelectedEntity{entt::null};
            
            ForceAnalysisData ForceAnalysis;
            EnergyTracker Energy;
            MomentumTracker Momentum;
            CollisionAnalyzer Collisions;
            AccelerationTracker Acceleration;
            NewtonsLawsDemo NewtonsLaws;
            TrajectoryPredictor Trajectory;
            
            bool ShowForceAnalysisPanel{false};
            bool ShowEnergyPanel{false};
            bool ShowMomentumPanel{false};
            bool ShowCollisionPanel{false};
            bool ShowNewtonsLawsPanel{false};
            bool ShowAccelerationPanel{false};
            bool ShowTrajectoryPanel{false};
            
            void ResetAll()
            {
                ForceAnalysis.Clear();
                Energy.Reset();
                Momentum.Reset();
                Collisions.Clear();
                Acceleration.Reset();
                Trajectory.Clear();
            }
        };
        
        PhysicsAnalysisState m_PhysicsAnalysis;

    private:
        // Original methods
        void RenderStatisticalAnalysisPanel(SceneContext& context);
        void RenderStatisticsSection(const char* title, const StatisticalData& stats, const ImVec4& color);
        void RenderComparisonSection(const StatisticalData& linear, const StatisticalData& angular);
        
        // New Newtonian Physics UI Methods
        void RenderForceAnalysisPanel(SceneContext& context);
        void RenderEnergyTrackingPanel(SceneContext& context);
        void RenderMomentumPanel(SceneContext& context);
        void RenderCollisionAnalysisPanel(SceneContext& context);
        void RenderNewtonsLawsPanel(SceneContext& context);
        void RenderAccelerationPanel(SceneContext& context);
        void RenderTrajectoryPanel(SceneContext& context);
        
        // Physics calculation helpers
        void UpdateForceAnalysis(entt::entity entity, float deltaTime);
        void UpdateEnergyTracking(entt::entity entity, float time);
        void UpdateMomentumTracking(entt::entity entity, float time);
        void DetectAndAnalyzeCollisions(SceneContext& context);
        void UpdateAcceleration(entt::entity entity, float deltaTime);
        void UpdateTrajectoryPrediction(entt::entity entity);
        
        // Visualization helpers
        void DrawForceVector(const glm::vec3& origin, const glm::vec3& force, const ImU32& color, float scale);
        void DrawMomentumVector(const glm::vec3& position, const glm::vec3& momentum, const ImU32& color, float scale);
        void DrawTrajectoryPath(const std::vector<glm::vec3>& path, const ImU32& color);
        void DrawCollisionPoint(const glm::vec3& point, float radius, const ImU32& color);
        
        // Educational helpers
        void ShowPhysicsTooltip(const char* concepts, const char* explanation);
        void RenderPhysicsEquation(const char* equation, const char* description);
    };
}