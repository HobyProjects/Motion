#pragma once

#include "ScenePanel.hpp"
#include "SceneCommon.hpp"
#include "SceneUtils.hpp"

namespace Motion
{
    class AccelerationTracker : public IPanel
    {
    public:
        AccelerationTracker() = default;
        virtual ~AccelerationTracker() = default;

        void OnUpdate(Scene* scene, float dt) override;
        void OnRender(Scene* scene) override;
    };
}