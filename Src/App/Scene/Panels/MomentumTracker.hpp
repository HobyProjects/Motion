#pragma once

#include "ScenePanel.hpp"
#include "SceneCommon.hpp"
#include "SceneUtils.hpp"

namespace Motion
{
    class MomentumTracker : public IPanel
    {
    public:
        MomentumTracker() = default;
        virtual ~MomentumTracker() = default;

        void OnUpdate(Scene* scene, float dt) override;
        void OnRender(Scene* scene) override;
    };
}
