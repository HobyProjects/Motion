#pragma once

#include <memory>
#include <string>
#include <vector>

#include <reactphysics3d/reactphysics3d.h>

namespace Motion
{
    class Scene;

    struct ScenePanelContext
    {
        FrameTextureID FrameTexture{0};
        Scene* ScenePointer{ nullptr };
        entt::registry* SceneRegistry{nullptr};

        rp3d::PhysicsWorld* PhysicsWorld{ nullptr };
        rp3d::PhysicsCommon* PhysicsCommon{ nullptr };
        rp3d::PhysicsWorld::WorldSettings* WorldSettings{nullptr};
    };

    enum class PanelCategory
    {
        Property,
        Viewport
    };

    class IScenePanel
    {
        public:
            virtual ~IScenePanel() = default;

            virtual std::string GetTitle() const = 0;
            virtual PanelCategory GetCategory() const = 0;
            virtual void RenderUI(ScenePanelContext& context) = 0;
    };

    class ScenePanelManager
    {
        public:
            ScenePanelManager() = default;
            ~ScenePanelManager() = default;

            template<class T, class... Args>
            T* Emplace(Args&&... args)
            {
                auto p = std::make_unique<T>(std::forward<Args>(args)...);
                T* raw = p.get();
                m_Panels.emplace_back(std::move(p));
                return raw;
            }

            std::vector<std::unique_ptr<IScenePanel>>::const_iterator begin() const { return m_Panels.begin(); }
            std::vector<std::unique_ptr<IScenePanel>>::const_iterator end() const { return m_Panels.end(); }

        private:
            std::vector<std::unique_ptr<IScenePanel>> m_Panels;
    };

}