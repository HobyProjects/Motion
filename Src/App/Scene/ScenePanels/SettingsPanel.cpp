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
        IEnvironment* ibl = env.EnvironmentInstance ? env.EnvironmentInstance.get() : nullptr; 

        ImGuiTreeNodeFlags envFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        
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
                    DragFloat(ICON_MD_TUNGSTEN " Intensity", &env.Sun.Intensity, 0.01f, 0.0f, 50.0f);
                    ToggleSwitch("Cast Shadow", env.Sun.CastShadow);
                    ToggleSwitch("Show Light Direction", env.Sun.ShowLightDirectionGuizmo);
                    EndPropertyGrid();
                }
            }

            if (ImGui::CollapsingHeader(ICON_MD_STYLE " Sky", secFlags))
            {
                if(BeginPropertyGrid("##skybox-properties"))
                {
                    auto& skyBox = ibl->GetSpecification();
                    SliderFloat(ICON_MD_BRIGHTNESS_6 " Intensity", &skyBox.Intensity, 0.0f, 5.0f);
                    SliderFloat(ICON_MD_LIGHTBULB " Gamma", &skyBox.Gamma, 1.8f, 2.4f);
                    SliderFloat(ICON_MD_EXPOSURE " Exposure", &skyBox.Exposure, -5.0f, 5.0f);
                    SliderFloat(ICON_MD_SETTINGS_BRIGHTNESS " Level", &skyBox.MaxMipLevel, -1.0f, 0.0f);
                    ComboBox(ICON_MD_FILTER_1 " Tone", { "None", "Reinhard", "ACES" }, skyBox.Tonemap, [&](std::int32_t index, const std::string& selected) { skyBox.Tonemap = index; });
                    EndPropertyGrid();
                }
            }


            ImGui::TreePop();
        }

        ImGui::End();
    };
}