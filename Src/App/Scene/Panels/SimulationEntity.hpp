#pragma once

#include "ScenePanel.hpp"
#include "SceneCommon.hpp"

namespace Motion
{
    class SimulationEntity : public IPanel
    {
    public:
        SimulationEntity() = default;
        virtual ~SimulationEntity() = default;
        
        void OnRender(Scene* scene) override;
    };
}