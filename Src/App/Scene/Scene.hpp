#pragma once

#include "Event.hpp"
#include "Buffers.hpp"
#include "Components.hpp"
#include "ModelImporter.hpp"
#include "PostProcessing.hpp"  

#include "SceneCommon.hpp"

namespace Motion
{
    class SceneSerializer;

    class Scene
    {
    public:
        Scene(const SceneSpecification& spec, const glm::vec2& viewport = glm::vec2(1280.0f, 720.0f));
        ~Scene();

        void OnUpdate(WindowHandle handle, Timer deltaTime);
        void OnEvent(WindowHandle handle, IEvent& e);
        
        void Submit();
        void SetApectRatio(const glm::vec2& size);

        void SelectedEntity(const entt::entity& entt);
        void EmplaceEntity(const entt::entity& entity);
        void DestroyEntity(const entt::entity& entity, bool deleteResources = false);
        bool DuplicateEntity(const entt::entity& entity);

        void ForEachActiveEntity(const std::function<void(entt::entity)>& fn);
        void ForEachEntity(const std::function<void(entt::entity)>& fn);
        void ForEachRootEntity(const std::function<void(entt::entity)>& fn);
        void ForEachNodeEntity(const entt::entity root, const std::function<void(entt::entity)>& fn);
        
        void ApplyPhysics(float deltaTime);
        void RefreshPhysicBodies();
        void RenderPostProcessingUI();
        
        [[nodiscard]] const bool IsRootEntity(entt::entity entity) const;
        [[nodiscard]] const bool IsNodeEntity(entt::entity entity) const;
        [[nodiscard]] entt::entity FindRootOf(entt::entity entity);
        [[nodiscard]] SceneContext& GetContext() { return m_Context; }

        // ADD THESE POST-PROCESSING METHODS
        [[nodiscard]] PostProcessStack& GetPostProcessStack() { return m_PostProcessStack; }
        void EnablePostProcessing(bool enable) { m_PostProcessingEnabled = enable; }
        [[nodiscard]] bool IsPostProcessingEnabled() const { return m_PostProcessingEnabled; }

    private:
        bool OnMouseCursorPosChange(WindowHandle handle, EventMouseCursorMove& e);
        bool OnMouseWheelScrollEvent(WindowHandle handle, EventMouseWheelScroll& e);

        // ADD THIS HELPER METHOD
        void InitializePostProcessing();

    private:
        SceneEntities       m_Entities{};
        SceneViewport       m_Viewport{};
        ScenePhysics        m_Physics{};
        ScenePhysicsWorld   m_PhysicsWorld{};
        SceneSimulation     m_Simulation{};
        ScenePanelsView     m_Panels{};
        SceneSpecification  m_Specification{};
        
        SceneContext        m_Context{};

        // ADD THESE POST-PROCESSING MEMBERS
        PostProcessStack                    m_PostProcessStack{};
        std::shared_ptr<IFrameBuffer>       m_HDRSceneBuffer{};      // HDR rendering target
        std::shared_ptr<IFrameBuffer>       m_FinalBuffer{};         // Final output after post-processing
        bool                                m_PostProcessingEnabled{true};
        
        friend class SceneSerializer;
    };
}