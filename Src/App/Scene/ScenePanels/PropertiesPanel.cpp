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
        ImGui::Begin(ICON_MD_SETTINGS " Properties");

        // Safer selected-entity resolution
        std::shared_ptr<Entity> selectedEntity = context.ActiveScene ? context.ActiveScene->GetSelectedEntity() : nullptr;
        const bool hasSelection = selectedEntity && selectedEntity != EntityFactory::EMPTYENTITY;

        if (!hasSelection)
        {
            ImGui::TextDisabled("%s  No entity selected", ICON_MD_INFO);
            ImGui::End();
            return;
        }

        if (selectedEntity->HasComponent<TagComponent>())
        {
            UI::BeginPropertyGrid("##tag-grid");
            auto& tag = selectedEntity->GetComponent<TagComponent>();
            UI::TextBox(ICON_MD_LABEL" Name Tag", tag.Tag, false, 256);
            UI::EndPropertyGrid();
        }

        DrawComponentControls<TransformComponent>(std::string(ICON_MD_OPEN_WITH " Transform").c_str(), selectedEntity,
            [](TransformComponent& component)
            {
                UI::BeginPropertyGrid("##transform-grid");
                UI::DragFloat3(ICON_MD_DIRECTIONS " Translation", component.Translation, 0.0f, 0.1f);
                UI::DragFloat3(ICON_MD_ROTATE_90_DEGREES_CW " Rotation", component.Rotation, 0.1f, -glm::pi<float>(), glm::pi<float>());
                UI::DragFloat3(ICON_MD_ZOOM_OUT_MAP " Scale", component.Scale, 0.1f, 10.0f);
                UI::EndPropertyGrid();
            }
        );

        ImGui::End();
    }
}