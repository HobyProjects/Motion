#pragma once

#include "ScenePanel.hpp"
#include "Material.hpp"

namespace Motion
{
    class SceneViewPanel final : public IScenePanel
    {
        public:
            SceneViewPanel();
            ~SceneViewPanel() = default;

            virtual std::string GetTitle() const override { return m_Title; }
            virtual PanelCategory GetCategory() const override { return PanelCategory::InspectorPanel; }
            virtual void RenderUI(ScenePanelContext& context) override;
            
        private:
            void DrawMaterialUI(ScenePanelContext& ctx, std::shared_ptr<Material>& mat);
            void DrawAttributes(std::shared_ptr<Material>& mat);
            void DrawTexturesSlots(std::shared_ptr<Material>& mat);

        private:
            std::string m_Title{ "ProjectScenes" };
            std::vector<std::shared_ptr<BaseMaterial>> m_BaseMaterial;
            int m_SelectedMesh = -1;
    };
}