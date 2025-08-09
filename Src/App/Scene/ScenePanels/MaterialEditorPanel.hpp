#pragma once

#include "ScenePanel.hpp"

namespace Motion
{

    class MaterialEditorPanel final : public IScenePanel
    {
    public:
        MaterialEditorPanel();
        ~MaterialEditorPanel() override = default;

        std::string GetTitle() const override { return m_Title; }
        PanelCategory GetCategory() const override { return PanelCategory::InspectorPanel; }
        void RenderUI(ScenePanelContext& context) override;

    private:
        struct ItemRef
        {
            std::shared_ptr<PhysicalBasedMaterialInstance> Instance{};
            std::weak_ptr<Entity> Owner{}; // optional owner entity
            int MeshIndex{ -1 };             // optional mesh slot on owner
            bool operator==(const ItemRef& o) const noexcept { return Instance == o.Instance && MeshIndex == o.MeshIndex; }
        };

        // helpers
        void CollectFromSelection(ScenePanelContext& ctx);
        void ApplyToAllMeshSlots(ScenePanelContext& ctx, const std::shared_ptr<PhysicalBasedMaterialInstance>& src);
        void DrawAttributes(PhysicalBasedMaterialInstance& mat);
        void DrawTextures(std::unordered_map<TextureType, std::shared_ptr<ITexture>>& map);

    private:
        // Offscreen preview
        std::shared_ptr<IFrameBuffer> m_PreviewFB{ nullptr };
        std::shared_ptr<StaticMesh> m_SphereMesh{ nullptr };
        FrameTextureID m_PreviewTex{ 0 };
        glm::ivec2 m_LastFBSize{ 0,0 };

        void EnsurePreviewFB(int w, int h);
        void RenderPreviewScene(ScenePanelContext& ctx, PhysicalBasedMaterialInstance* mat);
        void RenderSphere(PhysicalBasedMaterialInstance* mat, Scene* activeScene, const glm::ivec2& fbSize);

    private:
        std::string m_Title{ "Material Editor" };
        std::vector<ItemRef> m_Items{};
        int m_Selected{ -1 };
        char m_Filter[128] = {};
    };
}