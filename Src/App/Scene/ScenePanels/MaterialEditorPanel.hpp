#pragma once

#include "ScenePanel.hpp"

namespace Motion
{
    class MaterialEditorPanel final : public IScenePanel
    {
    public:
        MaterialEditorPanel() = default;
        ~MaterialEditorPanel() override = default;

        std::string GetTitle() const override { return m_Title; }
        PanelCategory GetCategory() const override { return PanelCategory::InspectorPanel; }
        void RenderUI(ScenePanelContext& context) override;

    private:
        std::string m_Title{ "MaterialEditor" };
        std::shared_ptr<Material> m_SelectedMaterial{ nullptr };
    };
}