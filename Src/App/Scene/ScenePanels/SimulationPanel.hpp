#pragma once

#include "ScenePanel.hpp"

namespace Motion
{
    class SimulationPanel final : public IScenePanel
    {
    public:
        SimulationPanel() = default;
        ~SimulationPanel() = default;

        virtual std::string GetTitle() const override { return m_Title; }
        virtual PanelCategory GetCategory() const override { return PanelCategory::ScenePanel; }
        virtual void RenderUI(ScenePanelContext& context) override;

    private:
        std::string m_Title{ "SceneViewport" };
    };
}