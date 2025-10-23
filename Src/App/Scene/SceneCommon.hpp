#pragma once 

#include <limits>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <filesystem>
#include <chrono>

#include <reactphysics3d/reactphysics3d.h>
#include <entt/entt.hpp>

#include "UUID.hpp"
#include "Buffers.hpp"
#include "Camera3D.hpp"

namespace Motion
{
    class Scene;

    struct SceneEntities
    {
        entt::entity SelectedEntity{entt::null};
        std::vector<entt::entity> EntryPoints{};
        entt::registry Registry{};
    };

    struct SceneSpecification
    {
        UUID ID{UniqueIdentity::GetUniqueID()};
        std::string Name{"Unamed"};
        std::filesystem::path SavedPath{};
    };

    struct SceneViewport
    { 
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

    struct ScenePhysicsWorld
    {
        static constexpr float  FIXED_STEPS_PERFRAME    = 8;
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
    };

    struct SceneContext
    {
        Scene* MyScene{nullptr};
        SceneEntities* Entities{nullptr};
        SceneSpecification* Specification{nullptr};
        SceneViewport* View{nullptr};
        ScenePhysicsWorld* Physics{nullptr};
        SceneSimulation* Simulation{nullptr};
    };
}