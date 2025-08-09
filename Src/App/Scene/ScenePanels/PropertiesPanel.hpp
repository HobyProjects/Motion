#pragma once

#include "ScenePanel.hpp"

namespace Motion
{
    class SceneEntityPropertiesPanel final : public IScenePanel
    {
    public:
        SceneEntityPropertiesPanel() = default;
        ~SceneEntityPropertiesPanel() = default;

        virtual std::string GetTitle() const override { return m_Title; }
        virtual PanelCategory GetCategory() const override { return PanelCategory::PropertiesPanel; }
        virtual void RenderUI(ScenePanelContext& context) override;

    private:
        std::string m_Title{ "SceneEntityProperties" };
    };

}