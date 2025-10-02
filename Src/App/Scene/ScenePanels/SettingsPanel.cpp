#include "CorePCH.hpp"
#include "SettingsPanel.hpp"

namespace Motion
{
    void SceneSettingsPanel::RenderUI(ScenePanelContext& ctx)
    {
        ImGui::Begin(ICON_MD_ENERGY_SAVINGS_LEAF " Scene Settings");

        if (!ctx.ActiveScene)
        {
            ImGui::TextDisabled("%s  No active scene", ICON_MD_INFO);
            ImGui::End();
            return;
        }

        auto& env = ctx.ActiveScene->GetEnvironment(); 
        ImGuiTreeNodeFlags envFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_DefaultOpen;
        
        if (ImGui::TreeNodeEx((void*)1, envFlags, ICON_MD_WB_SUNNY " Environment"))
        {
            const ImGuiTreeNodeFlags secFlags =
                ImGuiTreeNodeFlags_FramePadding |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_Framed;

            if (ImGui::CollapsingHeader(ICON_MD_LIGHTBULB " Light", secFlags))
            {
                if (BeginPropertyGrid("##sun-properties"))
                {
                    DragFloat3(ICON_MD_DIRECTIONS " Direction", env.Sun.Direction);
                    ColorEdit3(ICON_MD_PALETTE " Color", env.Sun.Color);
                    DragFloat(ICON_MD_TUNGSTEN " Intensity", &env.Sun.Intensity, 0.01f, 0.0f, 50.0f);;
                    ToggleSwitch("Show Light Direction", env.Sun.ShowLightDirectionGuizmo);
                    EndPropertyGrid();
                }
            }

            ImGui::TreePop();
        }

        ImGui::End();
    };
}