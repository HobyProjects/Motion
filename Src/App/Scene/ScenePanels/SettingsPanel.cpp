#include "CorePCH.hpp"
#include "SettingsPanel.hpp"

namespace Motion
{
    void SceneSettingsPanel::RenderUI(ScenePanelContext& ctx)
    {
        ImGui::Begin("Scene Settings");

        if (!ctx.ActiveScene)
        {
            ImGui::TextDisabled("%s  No active scene", ICON_MD_INFO);
            ImGui::End();
            return;
        }

        auto& env = ctx.ActiveScene->GetEnvironment();          // SceneEnvironment
        IEnvironment* ibl = env.Env ? env.Env.get() : nullptr;  // IBL backend (may be null)

        // ───────────────────────────────── Environment Lighting (Sun) ─────────────────────────────────
        if (ImGui::TreeNodeEx((void*)1,
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_AllowItemOverlap |
            ImGuiTreeNodeFlags_FramePadding,
            "%s  %s", ICON_MD_WB_SUNNY, "Environment Lighting"))
        {
            const ImGuiTreeNodeFlags secFlags =
                ImGuiTreeNodeFlags_FramePadding |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_Framed;

            if (ImGui::CollapsingHeader(std::string(ICON_MD_LIGHTBULB "  Sun").c_str(), secFlags))
            {
                // Sun direction (unit vector recommended)
                CustomUIControl::DrawFloat3(std::string(ICON_MD_NEAR_ME "  Direction").c_str(),
                    env.Sun.Direction, 0.0f);
                // Normalize to avoid surprises
                if (glm::length2(env.Sun.Direction) > 0.0f)
                    env.Sun.Direction = glm::normalize(env.Sun.Direction);

                // Sun color
                CustomUIControl::ColorEdit3(std::string(ICON_MD_PALETTE "  Color").c_str(),
                    env.Sun.Color);

                // Sun intensity
                CustomUIControl::DrawFloat(std::string(ICON_MD_TUNGSTEN "  Intensity").c_str(),
                    env.Sun.Intensity, 0.0f, 50.0f, 0.01f);
            }

            ImGui::TreePop();
        }

        // ───────────────────────────────── Image-Based Lighting (IBL) ────────────────────────────────
        if (ImGui::TreeNodeEx((void*)2,
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_AllowItemOverlap |
            ImGuiTreeNodeFlags_FramePadding,
            "%s  %s", ICON_MD_HDR_ENHANCED_SELECT, "Image-Based Lighting"))
        {
            if (!ibl)
            {
                ImGui::TextDisabled("%s  No environment loaded (HDR).", ICON_MD_INFO);
                if (ImGui::Button(ICON_MD_ADD_PHOTO_ALTERNATE "  Load HDR…"))
                {
                    std::string path = Motion::DialogBoxes::OpenFileDialog();
                    if (!path.empty())
                    {
                        // Keep current IBL intensity and rotation if any
                        Motion::EnvIntensity keep = ibl ? ibl->GetIntensity() : Motion::EnvIntensity{ 1.0f, 1.0f };
                        float rotY = ibl ? ibl->GetSkyboxRotationY() : 0.0f;

                        auto newEnv = Motion::IEnvironment::Create(path);
                        if (newEnv)
                        {
                            newEnv->SetIntensity(keep);
                            newEnv->SetSkyboxRotationY(rotY);
                            env.Env = std::move(newEnv);
                        }
                        else
                        {
                            MOTION_CORE_ERROR("Failed to create environment from: {}", path);
                        }
                    }
                }
            }
            else
            {
                auto intens = ibl->GetIntensity();

                // Diffuse & Specular IBL intensity
                CustomUIControl::DrawFloat(std::string(ICON_MD_TUNE "  Diffuse Intensity").c_str(),
                    intens.Diffuse, 0.0f, 4.0f, 0.01f);
                CustomUIControl::DrawFloat(std::string(ICON_MD_TUNE "  Specular Intensity").c_str(),
                    intens.Specular, 0.0f, 4.0f, 0.01f);
                ibl->SetIntensity(intens);

                // Skybox rotation (Y)
                float rotY = ibl->GetSkyboxRotationY();
                if (CustomUIControl::DrawFloat(std::string(ICON_MD_ROTATE_90_DEGREES_CW "  Skybox Y Rotation").c_str(),
                    rotY, -glm::pi<float>(), glm::pi<float>(), 0.005f))
                {
                    ibl->SetSkyboxRotationY(rotY);
                }

                // Optional: debug IDs (collapsed)
                if (ImGui::CollapsingHeader(std::string(ICON_MD_DEVELOPER_MODE "  Debug").c_str()))
                {
                    ImGui::TextDisabled("Irradiance ID:   %u", (unsigned)ibl->GetIrradianceTexture());
                    ImGui::TextDisabled("Prefiltered ID:  %u", (unsigned)ibl->GetPrefilteredTexture());
                    ImGui::TextDisabled("BRDF LUT ID:     %u", (unsigned)ibl->GetBRDFLUTTexture());
                    ImGui::TextDisabled("Env Cube ID:     %u", (unsigned)ibl->GetEnvironmentCubeTexture());
                }
            }

            ImGui::TreePop();
        }

        // ───────────────────────────────── Global Appearance ─────────────────────────────────────────
        if (ImGui::TreeNodeEx((void*)3,
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_AllowItemOverlap |
            ImGuiTreeNodeFlags_FramePadding,
            "%s  %s", ICON_MD_STYLE, "Appearance"))
        {
            CustomUIControl::DrawFloat(std::string(ICON_MD_EXPOSURE "  Exposure").c_str(),
                env.Exposure, 0.0f, 8.0f, 0.01f);

            CustomUIControl::DrawFloat(std::string(ICON_MD_CONTRAST "  Gamma").c_str(),
                env.Gamma, 1.0f, 3.0f, 0.01f);

            CustomUIControl::ColorEdit3(std::string(ICON_MD_COLOR_LENS "  Ambient Tint").c_str(),
                env.AmbientTint);

            ImGui::TreePop();
        }

        // ───────────────────────────────── Fog (optional) ────────────────────────────────────────────
        if (ImGui::TreeNodeEx((void*)4,
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_AllowItemOverlap |
            ImGuiTreeNodeFlags_FramePadding,
            "%s  %s", ICON_MD_CLOUD, "Fog"))
        {
            ImGui::Checkbox(std::string(ICON_MD_TOGGLE_ON "  Enabled").c_str(), &env.FogSettings.Enabled);
            CustomUIControl::DrawFloat(std::string(ICON_MD_BLUR_ON "  Density").c_str(),
                env.FogSettings.Density, 0.0f, 1.0f, 0.001f);
            CustomUIControl::ColorEdit3(std::string(ICON_MD_INVERT_COLORS "  Color").c_str(),
                env.FogSettings.Color);

            ImGui::TreePop();
        }

        ImGui::End();
    };
}