#pragma once

#include "ScenePanel.hpp"
#include "SceneCommon.hpp"
#include "SceneUtils.hpp"

namespace Motion
{
    class EnergyTracker : public IPanel
    {
    public:
        EnergyTracker() = default;
        virtual ~EnergyTracker() = default;

        void OnUpdate(Scene* scene, float dt) override;
        void OnRender(Scene* scene) override;
    };
}