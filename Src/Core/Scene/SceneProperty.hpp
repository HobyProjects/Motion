#pragma once

#include "ScenePanel.hpp"
#include "Material.hpp"

namespace Motion
{
    class ScenePropertyPanel final : public IScenePanel
    {
        public:
            ScenePropertyPanel(const std::string& name);
            ~ScenePropertyPanel() = default;

            std::string GetTitle() const override { return m_Title; }
            PanelCategory GetCategory() const override { return PanelCategory::Property; }
            void RenderUI(ScenePanelContext& context) override;

        private:
            // ===== public UI pieces kept (same signatures) =====
            void DrawMaterialUI(ScenePanelContext& ctx, std::shared_ptr<Material>& mat);
            void DrawAttributes(std::shared_ptr<Material>& mat);
            void DrawTexturesSlots(std::shared_ptr<Material>& mat);

            void RenderNodeEntities(ScenePanelContext& c, entt::registry& r, entt::entity root);
            void RenderTagAndModel(entt::registry& r, entt::entity e);
            void RenderTransform(entt::registry& r, entt::entity e);
            void RenderPhysics(ScenePanelContext& c, entt::registry& r, entt::entity e);
            void RenderMaterialEditor(ScenePanelContext& c, entt::registry& r, entt::entity e);

            // ===== new helpers to simplify RenderUI =====
            void RenderToolbarAndSearch(ScenePanelContext& ctx);
            void RenderImportPopup(ScenePanelContext& ctx);
            void RenderEntityHierarchy(ScenePanelContext& ctx);
            void RenderEnvironmentSettings(ScenePanelContext& ctx);

            // ===== small UI helpers to decompose complex sections =====
            void DrawRigidBodyUI(RigidBodyComponent& rb);
            void DrawColliderUI(ColliderComponent& cc);

            // ===== resources =====
            void LoadDefaultBaseMaterials();

        private:
            std::string m_Title{ "ProjectScenes" };
            std::vector<std::shared_ptr<BaseMaterial>> m_BaseMaterial;
            int m_SelectedMesh = -1;

            // Moved off a static to a member so panel instances are well-behaved
            std::future<std::shared_ptr<Motion::ImportedResults>> m_ImportFuture;

            // search buffer as a member instead of function-static
            char m_SearchBuf[128] = {};
    };
}
