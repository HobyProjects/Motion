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

        auto& env = ctx.ActiveScene->GetEnvironment();          // SceneEnvironment
        IEnvironment* ibl = env.EnvironmentInstance ? env.EnvironmentInstance.get() : nullptr;  // IBL backend (may be null)

        // ───────────────────────────────── Environment Lighting (Sun) ─────────────────────────────────
        ImGuiTreeNodeFlags envFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (ImGui::TreeNodeEx((void*)1, envFlags, "%s  %s", ICON_MD_WB_SUNNY " Environment"))
        {
            const ImGuiTreeNodeFlags secFlags =
                ImGuiTreeNodeFlags_FramePadding |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_Framed;

            if (ImGui::CollapsingHeader(ICON_MD_LIGHTBULB " Sun", secFlags))
            {
                if (UI::BeginPropertyGrid("##sun-properties"))
                {
                    UI::DragFloat3(ICON_MD_DIRECTIONS "Direction", env.Sun.Direction);
                    UI::ColorEdit3(ICON_MD_PALETTE "Color", env.Sun.Color);
                    UI::DragFloat(ICON_MD_TUNGSTEN "Intensity", &env.Sun.Intensity, 0.01f, 0.0f, 50.0f);

                    UI::EndPropertyGrid();
                }
            }

            ImGui::TreePop();
        }

        // ───────────────────────────────── Image-Based Lighting (IBL) ────────────────────────────────
        ImGuiTreeNodeFlags iblFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (ImGui::TreeNodeEx((void*)2, iblFlags, "%s  %s", ICON_MD_HDR_ENHANCED_SELECT, "Image Based Lighting"))
        {
            if (UI::BeginPropertyGrid("ibl-properties"))
            {
                if (!ibl)
                {
                    ImGui::TextDisabled("%s  No environment loaded (HDR).", ICON_MD_INFO);
                    if (ImGui::Button(ICON_MD_ADD_PHOTO_ALTERNATE "  Load HDR…"))
                    {

                    }
                }
                else
                {
                    
                }

                UI::EndPropertyGrid();
            }

            ImGui::TreePop();
        }

        // ───────────────────────────────── Global Appearance ─────────────────────────────────────────
        ImGuiTreeNodeFlags appearanceFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        if (ImGui::TreeNodeEx((void*)3, appearanceFlags, "%s  %s", ICON_MD_STYLE, "Appearance"))
        {
            if (UI::BeginPropertyGrid("appearance-grid"))
            {
                UI::DragFloat(ICON_MD_EXPOSURE " Exposure", &env.Exposure, 0.0f, 8.0f, 0.01f);
                UI::DragFloat(ICON_MD_CONTRAST " Gamma", &env.Gamma, 1.0f, 3.0f, 0.01f);
                UI::ColorEdit3(ICON_MD_INVERT_COLORS " Ambient Tint", env.AmbientTint);
                UI::EndPropertyGrid();
            }

            ImGui::TreePop();
        }

        // ───────────────────────────────── Fog (optional) ────────────────────────────────────────────

        ImGuiTreeNodeFlags fogFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (ImGui::TreeNodeEx((void*)4, fogFlags, "%s  %s", ICON_MD_CLOUD, "Fog"))
        {
            if (UI::BeginPropertyGrid("fog-grid"))
            {
                UI::ToggleSwitch("Enabled", env.FogSettings.Enabled);
                UI::DragFloat(ICON_MD_BLUR_ON " Density", &env.FogSettings.Density, 0.0f, 1.0f, 0.001f);
                UI::ColorEdit3(ICON_MD_INVERT_COLORS " Color", env.FogSettings.Color);
                UI::EndPropertyGrid();
            }

            ImGui::TreePop();
        }

        ImGui::End();
    };
}