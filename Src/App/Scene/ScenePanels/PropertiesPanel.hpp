#pragma once

#include "ScenePanel.hpp"
#include "Material.hpp"

namespace Motion
{
    class SceneEntityPropertiesPanel final : public IScenePanel
    {
    public:
        SceneEntityPropertiesPanel();
        ~SceneEntityPropertiesPanel() = default;

        virtual std::string GetTitle() const override { return m_Title; }
        virtual PanelCategory GetCategory() const override { return PanelCategory::PropertiesPanel; }
        virtual void RenderUI(ScenePanelContext& context) override;

    private:
        void DrawMaterialUI(ScenePanelContext& ctx, std::shared_ptr<Material>& mat);
        void DrawAttributes(std::shared_ptr<Material>& mat);
        void DrawTexturesSlots(std::shared_ptr<Material>& mat);

    private:
        std::vector<std::shared_ptr<BaseMaterial>> m_BaseMaterial;
        std::string m_Title{ "SceneEntityProperties" };
        int m_SelectedMesh = -1;
    };

}