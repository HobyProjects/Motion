#pragma once

#include "ScenePanel.hpp"

namespace Motion
{
    class SceneEntityInspectPanel final : public IScenePanel
    {
        public:
            SceneEntityInspectPanel() = default;
            ~SceneEntityInspectPanel() = default;

            virtual std::string GetTitle() const override { return m_Title; }
            virtual PanelCategory GetCategory() const override { return PanelCategory::PropertiesPanel; }
            virtual void RenderUI(ScenePanelContext& context) override;

        private:
            std::string m_Title{ "SceneEntities" };
    };
}