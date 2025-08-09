#pragma once

#include "ScenePanel.hpp"

namespace Motion
{
    class SceneViewportPanel final : public IScenePanel
    {
    public:
        SceneViewportPanel() = default;
        ~SceneViewportPanel() = default;

        virtual std::string GetTitle() const override { return m_Title; }
        virtual PanelCategory GetCategory() const override { return PanelCategory::ScenePanel; }
        virtual void RenderUI(ScenePanelContext& context) override;

    private:
        std::string m_Title{ "SceneViewport" };
    };
}