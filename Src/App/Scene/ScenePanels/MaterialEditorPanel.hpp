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
        void RenderUI(ScenePanelContext& ctx) override;

    private:
        std::string m_Title = "Material Editor";
        int m_SelectedMesh = -1;

        void DrawMaterialUI(ScenePanelContext& ctx, std::shared_ptr<Material>& mat);
        void DrawAttributes(std::shared_ptr<Material>& mat);
        void DrawTexturesSlots(std::shared_ptr<Material>& mat);
        void Toolbar(std::shared_ptr<Material>& mat);
    };
}