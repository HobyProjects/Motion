#pragma once

#include "ScenePanel.hpp"

namespace Motion
{
    class SceneViewPanel final : public IScenePanel
    {
    public:
        SceneViewPanel() = default;
        ~SceneViewPanel() = default;

        virtual std::string GetTitle() const override { return m_Title; }
        virtual PanelCategory GetCategory() const override { return PanelCategory::InspectorPanel; }
        virtual void RenderUI(ScenePanelContext& context) override;

    private:
        std::string m_Title{ "ProjectScenes" };
    };
}