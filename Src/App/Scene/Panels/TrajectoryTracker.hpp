#pragma once

#include "ScenePanel.hpp"
#include "SceneCommon.hpp"
#include "SceneUtils.hpp"

namespace Motion
{
    class TrajectoryTracker : public IPanel
    {
    public:
        TrajectoryTracker() = default;
        virtual ~TrajectoryTracker() = default;

        void OnRender(Scene* scene) override;

    private:
        void UpdateTrajectoryPrediction(SceneContext& context, entt::entity entity);
    };
}