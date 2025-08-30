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
        
        if (ImGui::TreeNodeEx((void*)1, envFlags, "%s  %s", ICON_MD_WB_SUNNY " Environment\0"))
        {
            const ImGuiTreeNodeFlags secFlags =
                ImGuiTreeNodeFlags_FramePadding |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_Framed;

            if (ImGui::CollapsingHeader(ICON_MD_LIGHTBULB " Light", secFlags))
            {
                if (UI::BeginPropertyGrid("##sun-properties"))
                {
                    UI::DragFloat3(ICON_MD_DIRECTIONS " Direction", env.Sun.Direction);
                    UI::ColorEdit3(ICON_MD_PALETTE " Color", env.Sun.Color);
                    UI::DragFloat(ICON_MD_TUNGSTEN " Intensity", &env.Sun.Intensity, 0.01f, 0.0f, 50.0f);

                    UI::EndPropertyGrid();
                }
            }

            if (ImGui::CollapsingHeader(ICON_MD_STYLE " Sky", secFlags))
            {
                if(UI::BeginPropertyGrid("##skybox-properties"))
                {
                    auto& skyBox = ibl->GetSpecification();
                    UI::SliderFloat(ICON_MD_BRIGHTNESS_6 " Intensity", &skyBox.Intensity, 0.0f, 5.0f);
                    UI::SliderFloat(ICON_MD_LIGHTBULB " Gamma", &skyBox.Gamma, 1.8f, 2.4f);
                    UI::SliderFloat(ICON_MD_EXPOSURE " Exposure", &skyBox.Exposure, -5.0f, 5.0f);
                    UI::SliderFloat(ICON_MD_EXPOSURE " Level", &skyBox.MaxMipLevel, -1.0f, 0.0f);
                    UI::ComboBox(ICON_MD_FILTER_1 " Tone", { "None", "Reinhard", "ACES" }, skyBox.Tonemap, [&](std::int32_t index, const std::string& selected) { skyBox.Tonemap = index; });

                    UI::EndPropertyGrid();
                }
            }


            ImGui::TreePop();
        }

        ImGui::End();
    };
}