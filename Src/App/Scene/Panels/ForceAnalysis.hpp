#pragma once

#include "ScenePanel.hpp"
#include "SceneCommon.hpp"

namespace Motion
{
    class ForceAnalysis : public IPanel
    {
    public:
        ForceAnalysis() = default;
        virtual ~ForceAnalysis() = default;

        void OnUpdate(Scene* scene, float dt) override;
        void OnRender(Scene* scene) override;
    };
}