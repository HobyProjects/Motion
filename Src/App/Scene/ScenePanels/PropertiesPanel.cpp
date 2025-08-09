#include "CorePCH.hpp"
#include "PropertiesPanel.hpp"

namespace Motion
{
    template<typename T, typename UIFunc>
    static void DrawComponentControls(const std::string& name, const std::shared_ptr<Entity>& entity, UIFunc uiFunc, bool enabled = true)
    {
        static const ImGuiTreeNodeFlags treeNodeFlags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        if (entity->HasComponent<T>())
        {
            auto& component = entity->GetComponent<T>();
            if (ImGui::TreeNodeEx((void*)component.ID, treeNodeFlags, name.c_str()))
            {
                if (!enabled) ImGui::BeginDisabled();

                uiFunc(component);

                if (!enabled) ImGui::EndDisabled();
                ImGui::TreePop();
            }
        }
    }

    void SceneEntityPropertiesPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::Begin("Properties");

        // Safer selected-entity resolution
        std::shared_ptr<Entity> selectedEntity = context.ActiveScene ? context.ActiveScene->GetSelectedEntity() : nullptr;
        const bool hasSelection = selectedEntity && selectedEntity != EntityFactory::EMPTYENTITY;

        if (!hasSelection)
        {
            ImGui::TextDisabled("%s  No entity selected", ICON_MD_INFO);
            ImGui::End();
            return;
        }

        // ─────────────────────────────────────────────────────────────
        // Tag / Name
        // ─────────────────────────────────────────────────────────────
        if (selectedEntity->HasComponent<TagComponent>())
        {
            auto& tag = selectedEntity->GetComponent<TagComponent>();
            // Label with an icon
            CustomUIControl::TextBox(std::string(ICON_MD_LABEL "  Tag").c_str(), tag.Tag, false, 256, 150.0f);
        }

        // ─────────────────────────────────────────────────────────────
        // Transform (icon on the header)
        // ─────────────────────────────────────────────────────────────
        DrawComponentControls<TransformComponent>(std::string(ICON_MD_OPEN_WITH "  Transform").c_str(), selectedEntity,
            [](TransformComponent& component)
            {
                CustomUIControl::DrawFloat3(std::string(ICON_MD_NEAR_ME "  Translation").c_str(), component.Translation, 0.0f);
                CustomUIControl::DrawQuatEuler(std::string(ICON_MD_ROTATE_90_DEGREES_CW "  Rotation").c_str(), component.Rotation, 0.0f);
                CustomUIControl::DrawFloat3(std::string(ICON_MD_ZOOM_OUT_MAP "  Scale").c_str(), component.Scale, 10.0f);
            }
        );

        ImGui::End();
    }
}