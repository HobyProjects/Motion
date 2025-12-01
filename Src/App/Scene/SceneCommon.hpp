#pragma once 

#include <limits>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <numeric>
#include <cmath>
#include <deque>

#include <reactphysics3d/reactphysics3d.h>
#include <entt/entt.hpp>

#include "UUID.hpp"
#include "Buffers.hpp"
#include "Camera3D.hpp"
#include "Components.hpp"

#define EPSILON 1e-6f

namespace Motion
{
    class Scene;

    struct SceneEntities
    {
        entt::entity SelectedEntity{entt::null};
        std::vector<entt::entity> EntryPoints{};
        entt::registry Registry{};
    };

    struct SceneOperations
    {
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
            std::string Name{ "MyScene" };
            std::shared_ptr<Scene> Result{ nullptr }; 
            std::filesystem::path FilePath{ DialogBoxes::GetSystemFolder(SystemFolder::Documents) };
            bool ShowDialog{ false };

            void Reset()
            {
                Name = "MyScene";
                FilePath = DialogBoxes::GetSystemFolder(SystemFolder::Documents);
                ShowDialog = false;
                Result = nullptr;
            }
        };

        struct SceneLoadRequest
        {
            std::shared_ptr<Scene> Result{ nullptr };
            std::filesystem::path FilePath{};
            bool ShowDialog{ false };

            void Reset()
            {
                FilePath.clear();
                ShowDialog = false;
                Result = nullptr;
            }
        };

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

        void ResetOperations()
        {
            if (SceneCreationOp.State == AsyncOperationState::InProgress) SceneCreationOp.Reset();
            if (SceneLoadOp.State == AsyncOperationState::InProgress) SceneLoadOp.Reset();
            if (EntityImportOp.State == AsyncOperationState::InProgress) EntityImportOp.Reset();
        }

        SceneCreationRequest SceneCreationRequest{};
        AsyncOperation<std::shared_ptr<Scene>> SceneCreationOp{};
        
        SceneLoadRequest SceneLoadRequest{};
        AsyncOperation<std::shared_ptr<Scene>> SceneLoadOp{};
    
        EntityImportRequest EntityRequest{};
        AsyncOperation<std::shared_ptr<ImportedResults>> EntityImportOp{};
    };

    struct SceneSpecification
    {
        UUID ID{UniqueIdentity::GetUniqueID()};
        std::string Name{"Unamed"};
        std::filesystem::path SavedPath{};
    };

    struct SceneViewport
    {
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
        struct ViewportMouseControls
        {
            float MouseX{0.0f};
            float MouseY{0.0f};
            
            float Yaw{-90.0f};
            float Pitch{0.0f};
            
            bool OnFirstClick{false};
        };
        
        Camera3D Camera{};
        ViewportMouseControls MouseControls{};

        glm::vec2 ViewportSize{1280.0f, 720.0f};

        std::shared_ptr<IFrameBuffer> Framebuffer{};
        FrameBufferSpecification FrameSpecification{};
        FrameTextureID FrameTexturePtr{0};
        bool ViewportFocusedOrHovered{false};
    };

    struct ScenePhysics
    {
        static constexpr size_t MAX_HISTORY = 1000;
    
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
            // Force Data
            std::vector<ForceVector> Forces;
            glm::vec3 NetForce{0.0f};
            glm::vec3 NetTorque{0.0f};
            float NetForceMagnitude{0.0f};
            
            // Visualization Settings
            bool ShowForceVectors{true};
            bool ShowNetForce{true};
            bool ShowComponents{false};
            float VectorScale{1.0f};
            float ArrowHeadSize{0.15f};
            
            void Update()
            {
                NetForce = glm::vec3(0.0f);
                for (const auto& force : Forces)
                {
                    if (force.IsActive)
                        NetForce += force.Force;
                }
                NetForceMagnitude = glm::length(NetForce);
            }
            
            void AddForce(const ForceVector& force)
            {
                Forces.push_back(force);
                Update();
            }
            
            void RemoveForce(size_t index)
            {
                if (index < Forces.size())
                {
                    Forces.erase(Forces.begin() + index);
                    Update();
                }
            }
            
            void Reset()
            {
                Forces.clear();
                NetForce = glm::vec3(0.0f);
                NetTorque = glm::vec3(0.0f);
                NetForceMagnitude = 0.0f;
            }
        };
        
        struct EnergyTracker
        {
            // Current Energy State
            float CurrentKE{0.0f};
            float CurrentPE{0.0f};
            float CurrentTotal{0.0f};
            float InitialTotal{0.0f};
            float EnergyLoss{0.0f};
            
            // Energy History
            std::deque<float> KineticEnergyHistory;
            std::deque<float> PotentialEnergyHistory;
            std::deque<float> TotalEnergyHistory;
            std::deque<float> TimeStamps;
            
            // Configuration
            float GravityMagnitude{9.81f};
            bool TrackingEnabled{false};
            
            // Visualization Settings
            bool ShowKE{true};
            bool ShowPE{true};
            bool ShowTotal{true};
            
            void Update(float ke, float pe, float time)
            {
                CurrentKE = ke;
                CurrentPE = pe;
                CurrentTotal = ke + pe;
                
                if (InitialTotal < EPSILON && CurrentTotal > EPSILON)
                    InitialTotal = CurrentTotal;
                
                EnergyLoss = InitialTotal - CurrentTotal;
                
                if (TrackingEnabled)
                {
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
            }
            
            float GetConservationPercentage() const
            {
                if (InitialTotal < EPSILON) return 100.0f;
                return (CurrentTotal / InitialTotal) * 100.0f;
            }
            
            void ClearHistory()
            {
                KineticEnergyHistory.clear();
                PotentialEnergyHistory.clear();
                TotalEnergyHistory.clear();
                TimeStamps.clear();
            }
            
            void Reset()
            {
                CurrentKE = CurrentPE = CurrentTotal = InitialTotal = EnergyLoss = 0.0f;
                ClearHistory();
            }
        };

        struct MomentumTracker
        {
            // Linear Momentum State
            glm::vec3 LinearMomentum{0.0f};
            float LinearMagnitude{0.0f};
            glm::vec3 InitialLinearMomentum{0.0f};
            
            // Angular Momentum State
            glm::vec3 AngularMomentum{0.0f};
            float AngularMagnitude{0.0f};
            glm::vec3 InitialAngularMomentum{0.0f};
            
            // Motion History
            std::deque<ImVec2> LinearSpeedHistory;
            std::deque<ImVec2> AngularSpeedHistory;
            std::deque<ImVec2> MomentumMagnitudeHistory;
            float TimeAccumulator{0.0f};
            
            // Configuration
            bool TrackingEnabled{true};
            bool TrackConservation{true};
            float GraphTimeWindow{10.0f};
            
            // Visualization Settings
            bool ShowLinearVector{true};
            bool ShowAngularVector{false};
            float VectorScale{0.5f};
            
            void Update(const glm::vec3& linearMom, const glm::vec3& angularMom, 
                        float linearSpeed, float angularSpeed, float deltaTime)
            {
                // Update linear momentum
                LinearMomentum = linearMom;
                LinearMagnitude = glm::length(linearMom);
                
                if (glm::length(InitialLinearMomentum) < EPSILON && LinearMagnitude > EPSILON)
                    InitialLinearMomentum = linearMom;
                
                // Update angular momentum
                AngularMomentum = angularMom;
                AngularMagnitude = glm::length(angularMom);
                
                if (glm::length(InitialAngularMomentum) < EPSILON && AngularMagnitude > EPSILON)
                    InitialAngularMomentum = angularMom;
                
                // Update time
                TimeAccumulator += deltaTime;
                
                // Record history
                if (TrackingEnabled)
                {
                    LinearSpeedHistory.push_back(ImVec2(TimeAccumulator, linearSpeed));
                    AngularSpeedHistory.push_back(ImVec2(TimeAccumulator, angularSpeed));
                    MomentumMagnitudeHistory.push_back(ImVec2(TimeAccumulator, LinearMagnitude));
                    
                    if (LinearSpeedHistory.size() > MAX_HISTORY)
                    {
                        LinearSpeedHistory.pop_front();
                        AngularSpeedHistory.pop_front();
                        MomentumMagnitudeHistory.pop_front();
                    }
                }
            }
            
            float GetLinearConservationPercentage() const
            {
                float initialMag = glm::length(InitialLinearMomentum);
                if (initialMag < EPSILON) return 100.0f;
                return (LinearMagnitude / initialMag) * 100.0f;
            }
            
            float GetAngularConservationPercentage() const
            {
                float initialMag = glm::length(InitialAngularMomentum);
                if (initialMag < EPSILON) return 100.0f;
                return (AngularMagnitude / initialMag) * 100.0f;
            }
            
            void ClearHistory()
            {
                LinearSpeedHistory.clear();
                AngularSpeedHistory.clear();
                MomentumMagnitudeHistory.clear();
                TimeAccumulator = 0.0f;
            }
            
            void Reset()
            {
                LinearMomentum = glm::vec3(0.0f);
                AngularMomentum = glm::vec3(0.0f);
                InitialLinearMomentum = glm::vec3(0.0f);
                InitialAngularMomentum = glm::vec3(0.0f);
                LinearMagnitude = AngularMagnitude = 0.0f;
                ClearHistory();
            }
        };
        
        struct CollisionEvent
        {
            // Entity References
            entt::entity EntityA{entt::null};
            entt::entity EntityB{entt::null};
            
            // Collision Geometry
            glm::vec3 CollisionPoint{0.0f};
            glm::vec3 CollisionNormal{0.0f};
            
            // Physics Data
            float RelativeVelocity{0.0f};
            float CoefficientOfRestitution{0.0f};
            float ImpulseMagnitude{0.0f};
            float TimeStamp{0.0f};
            
            // Velocity Analysis
            glm::vec3 VelocityABefore{0.0f};
            glm::vec3 VelocityBBefore{0.0f};
            glm::vec3 VelocityAAfter{0.0f};
            glm::vec3 VelocityBAfter{0.0f};
            
            // Energy Analysis
            float KEBefore{0.0f};
            float KEAfter{0.0f};
            float EnergyLoss{0.0f};
            
            // Collision Classification
            enum class Type
            {
                Elastic,
                Inelastic,
                PartiallyElastic,
                Unknown
            } CollisionType{Type::Unknown};
            
            void Classify()
            {
                if (std::abs(CoefficientOfRestitution - 1.0f) < 0.05f)
                    CollisionType = Type::Elastic;
                else if (CoefficientOfRestitution < 0.1f)
                    CollisionType = Type::Inelastic;
                else
                    CollisionType = Type::PartiallyElastic;
            }
            
            const char* GetTypeName() const
            {
                switch (CollisionType)
                {
                    case Type::Elastic:           return "Elastic";
                    case Type::Inelastic:         return "Inelastic";
                    case Type::PartiallyElastic:  return "Partially Elastic";
                    default:                      return "Unknown";
                }
            }
        };
        
        struct CollisionAnalyzer
        {
            // Collision History
            std::vector<CollisionEvent> RecentCollisions;
            static constexpr size_t MAX_COLLISIONS = 50;
            
            // Configuration
            bool AnalysisEnabled{false};
            bool PlayCollisionSound{false};
            
            // Visualization Settings
            bool ShowCollisionPoints{true};
            bool ShowImpulseVectors{true};
            bool ShowVelocityVectors{false};
            
            void RecordCollision(const CollisionEvent& event)
            {
                CollisionEvent evt = event;
                evt.Classify();
                
                RecentCollisions.push_back(evt);
                
                if (RecentCollisions.size() > MAX_COLLISIONS)
                    RecentCollisions.erase(RecentCollisions.begin());
            }
            
            CollisionEvent* GetMostRecent()
            {
                return RecentCollisions.empty() ? nullptr : &RecentCollisions.back();
            }
            
            size_t GetCollisionCount() const
            {
                return RecentCollisions.size();
            }
            
            void Reset()
            {
                RecentCollisions.clear();
            }
        };
        

        struct AccelerationTracker
        {
            // Current Acceleration State
            glm::vec3 CurrentAcceleration{0.0f};
            float AccelerationMagnitude{0.0f};
            glm::vec3 PreviousVelocity{0.0f};
            
            // Acceleration History
            std::deque<glm::vec3> AccelerationHistory;
            std::deque<float> MagnitudeHistory;
            std::deque<float> TimeStamps;
            static constexpr size_t MAX_ACCEL_HISTORY = 500;
            
            // Configuration
            bool TrackingEnabled{true};
            
            // Visualization Settings
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
                
                if (TrackingEnabled)
                {
                    AccelerationHistory.push_back(CurrentAcceleration);
                    MagnitudeHistory.push_back(AccelerationMagnitude);
                    TimeStamps.push_back(TimeStamps.empty() ? 0.0f : TimeStamps.back() + deltaTime);
                    
                    if (AccelerationHistory.size() > MAX_ACCEL_HISTORY)
                    {
                        AccelerationHistory.pop_front();
                        MagnitudeHistory.pop_front();
                        TimeStamps.pop_front();
                    }
                }
            }
            
            glm::vec3 GetDirection() const
            {
                return AccelerationMagnitude > EPSILON ? CurrentAcceleration / AccelerationMagnitude : glm::vec3(0.0f);
            }
            
            void ClearHistory()
            {
                AccelerationHistory.clear();
                MagnitudeHistory.clear();
                TimeStamps.clear();
            }
            
            void Reset()
            {
                CurrentAcceleration = glm::vec3(0.0f);
                PreviousVelocity = glm::vec3(0.0f);
                AccelerationMagnitude = 0.0f;
                ClearHistory();
            }
        };
        
        struct TrajectoryPredictor
        {
            // Predicted Path
            std::vector<glm::vec3> PredictedPath;
            
            // Configuration
            bool PredictionEnabled{false};
            int PredictionSteps{50};
            float TimeStep{0.1f};
            
            // Visualization Settings
            bool ShowPredictionPath{true};
            ImU32 PathColor{IM_COL32(100, 255, 100, 150)};
            float PathWidth{2.0f};
            
            void PredictTrajectory(const glm::vec3& position, const glm::vec3& velocity, 
                                const glm::vec3& acceleration)
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
            
            size_t GetPathPointCount() const
            {
                return PredictedPath.size();
            }
            
            void Reset()
            {
                PredictedPath.clear();
            }
        };
        
        struct PhysicsAnalysisState
        {
            ForceAnalysisData ForceAnalysis;
            EnergyTracker Energy;
            MomentumTracker Momentum;
            CollisionAnalyzer Collisions;
            AccelerationTracker Acceleration;
            TrajectoryPredictor Trajectory;
            
            void ResetAll()
            {
                ForceAnalysis.Reset();
                Energy.Reset();
                Momentum.Reset();
                Collisions.Reset();
                Acceleration.Reset();
                Trajectory.Reset();
            }
        };

        static float GetSpeed(const glm::vec3& velocity)
        {
            return glm::length(velocity);
        }
        
        static glm::vec3 GetDirection(const glm::vec3& velocity)
        {
            float speed = GetSpeed(velocity);
            return speed > EPSILON ? velocity / speed : glm::vec3(0.0f);
        }
        
        static float RadPerSecToRPM(float radPerSec)
        {
            return radPerSec * (60.0f / (2.0f * glm::pi<float>()));
        }
        
        static float MsToKmh(float ms)
        {
            return ms * 3.6f;
        }
        
        static const char* GetBodyTypeDescription(Motion::BodyType type)
        {
            switch (type)
            {
                case Motion::BodyType::Static:  return "Static (immovable, like walls or ground)";
                case Motion::BodyType::Dynamic: return "Dynamic (moves and collides with forces)";
                default:                        return "Unknown";
            }
        }
        
        static void StatusIndicator(const char* label, bool active, const char* tooltip = nullptr)
        {
            ImVec4 color = active ? ImVec4(0.1f, 0.9f, 0.3f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
            ImGui::TextColored(color, "%s %s", active ? "●" : "○", label);
            
            if (tooltip && ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(tooltip);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
        

        PhysicsAnalysisState PhysicsAnalysis;
    };

    struct ScenePhysicsWorld
    {
        static constexpr float  FIXED_STEPS_PERFRAME    = 10.0f;
        static constexpr float  FIXED_STEPS             = 1.0f / 120.0f;
        static constexpr float  SI_GRAVITY              = 9.80665f;
        
        float ACCUMULATOR = 0.0f;

        rp3d::PhysicsCommon Properties{};
        rp3d::PhysicsWorld* World{nullptr};
        rp3d::PhysicsWorld::WorldSettings Settings{};
        
        struct WorldLighting
        {
            glm::vec3 Direction{-100.0f, -100.0f, -100.0f};
            glm::vec3 Color{1.0f, 1.0f, 1.0f};
            float Intensity{3.0f};

            bool ShowGuizmo{false};

            void SetFromPosition(const glm::vec3& position, const glm::vec3& target = glm::vec3(0.0f))
            {
                Direction = glm::normalize(target - position);
            }
        };

        WorldLighting SunLight{};
    };

    struct SceneSimulation
    {
        enum class SimulationState
        {
            IDLE, PAUSED, RUNNING
        };

        using clock     = std::chrono::steady_clock;
        using secondsf  = std::chrono::duration<float>;
    
        bool InSimulation{false};
        SimulationState State{SimulationState::IDLE};
        clock::time_point LastFrameTime{ clock::now() };

        entt::entity SelectedEntity{entt::null};
        std::vector<entt::entity> SimulatedEntities{};

        void ChangeState(SimulationState state) { State = state; }
        bool StateEquals(SimulationState state) { return State == state; }

        void AddSimulatedEntity(entt::entity entity) 
        { 
            SimulatedEntities.push_back(entity); 
        }

        void RemoveSimulatedEntity(entt::entity entity) 
        { 
            SimulatedEntities.erase(std::remove(SimulatedEntities.begin(), SimulatedEntities.end(), entity), SimulatedEntities.end()); 
        }

        void ClearSimulatedEntities() 
        { 
            SimulatedEntities.clear(); 
        }

        bool IsEntitySimulated(entt::entity entity) 
        { 
            return std::find(SimulatedEntities.begin(), SimulatedEntities.end(), entity) != SimulatedEntities.end(); 
        }

        float GetSimulationTime() 
        { 
            return std::chrono::duration_cast<secondsf>(clock::now() - LastFrameTime).count(); 
        }
    };

    struct ScenePanelsView
    {
        bool ShowEntityHierarchy{true};
        bool ShowEnvironmentSettings{true};
        bool ShowEntitySimulated{true};
        bool ShowEntityMaterials{true};
        bool ShowEntityComponents{true};

        bool ShowEnvironmentPanel{true};
        bool ShowForceAnalysisPanel{true};
        bool ShowEnergyPanel{true};
        bool ShowMomentumPanel{true};
        bool ShowAccelerationPanel{true};
        bool ShowTrajectoryPanel{true};

        bool ShowConsole{true};
    };

    struct SceneContext
    {
        Scene* MyScene{nullptr};
        SceneEntities* Entities{nullptr};
        SceneSpecification* Specification{nullptr};
        SceneViewport* View{nullptr};
        ScenePhysics* Physics{nullptr};
        ScenePhysicsWorld* PhysicsWorld{nullptr};
        SceneSimulation* Simulation{nullptr};
        ScenePanelsView* Panels{nullptr};
    };
}