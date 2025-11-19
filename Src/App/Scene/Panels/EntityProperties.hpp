#pragma once

#include "ScenePanel.hpp"
#include "SceneCommon.hpp"

namespace Motion
{
    class EntityProperties : public IPanel
    {
    public:
        EntityProperties() = default;
        virtual ~EntityProperties() = default;

        void OnRender(Scene* scene) override;

    private:
        void RenderTransform(TransformComponent* tc);
        void RenderRigidBody(RigidBodyComponent* rb);
        void RenderCollider(ColliderComponent* cc);
    };
}