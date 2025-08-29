#include "CorePCH.hpp"
#include "MaterialEditorPanel.hpp"

namespace Motion
{
    static void MirrorToBase(std::shared_ptr<BaseMaterial>& base, TextureType t, const std::shared_ptr<ITexture>& tex)
    {
        if (!base)  return;
        if (tex)    base->Textures[t] = tex;
        else        base->Textures.erase(t);
    }

    void MaterialEditorPanel::RenderUI(ScenePanelContext& ctx)
    {
        ImGui::Begin(ICON_MD_IMAGE " Material Editor");

        if (!ctx.ActiveScene) return;

        auto selected = ctx.ActiveScene->GetSelectedEntity();
        if (!selected || selected == EntityFactory::EMPTYENTITY)
        {
            ImGui::TextDisabled(ICON_MD_INFO " No entity selected.");
            ImGui::End();
            return;
        }

        if (!selected->HasComponent<StaticMeshComponent>())
        {
            ImGui::TextDisabled(ICON_MD_INFO " Selected entity has no StaticMeshComponent.");
            ImGui::End();
            return;
        }

        auto& smc = selected->GetComponent<StaticMeshComponent>();
        if (!smc.Model)
        {
            ImGui::TextDisabled(ICON_MD_INFO " Entity has no model.");
            ImGui::End();
            return;
        }

        std::shared_ptr<Material> mat = nullptr;
        if (m_SelectedMesh >= 0 && m_SelectedMesh < smc.Model->GetMeshesCount())
        {
            auto& model = *smc.Model;
            auto mesh = model[m_SelectedMesh];
            if (mesh && !mesh->Materials)
            {
                auto& AM = AssetManager::GetInstance();
                auto base = AM.Get<BaseMaterial>("MetalBaseMaterial");
                auto& factory = MaterialBuilder::GetInstance();
                mesh->Materials = factory.Create(base);
            }

            mat = mesh->Materials;
        }

        if (ImGui::BeginTable("##toolbar-content", 1))
        {
            ImGui::TableNextRow(ImGuiTableRowFlags_None, 36.0f);
            ImGui::TableNextColumn();
            {
                if (ImGui::BeginTable("##toolbar", 1))
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    if (mat)
                        Toolbar(mat);
                }

                ImGui::EndTable();
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            if (ImGui::BeginTable("##content-splited", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody))
            {
                ImGui::TableSetupColumn("##side-bar", ImGuiTableColumnFlags_WidthFixed, 260.0f);
                ImGui::TableSetupColumn("##inpector-panel", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                {
                    if (ImGui::BeginChild("##side-bar-list", ImVec2(0.0, 0.0), true))
                    {
                        auto& model = *smc.Model;
                        if (model.GetMeshesCount() > 0)
                        {
                            for (std::uint32_t i = 0; i < model.GetMeshesCount(); ++i)
                            {
                                bool isSelected = (m_SelectedMesh == i);
                                if (ImGui::Selectable(fmt::format("[{}] - {}", model[i]->Index, model[i]->Name).c_str(), isSelected))
                                {
                                    m_SelectedMesh = i;
                                }
                            }
                        }
                    }

                    ImGui::EndChild();
                }

                ImGui::TableSetColumnIndex(1);
                {
                    if (ImGui::BeginChild("##inspector-area", ImVec2(0.0, 0.0), true))
                    {
                        if (mat)
                            DrawMaterialUI(ctx, mat);
                        else
                            ImGui::TextDisabled(ICON_MD_INFO " Select a mesh from the list");
                    }

                    ImGui::EndChild();
                }

                ImGui::EndTable();
            }

            ImGui::EndTable();
        }

        ImGui::End();;
    }

    void MaterialEditorPanel::Toolbar(std::shared_ptr<Material>& mat)
    {
        UI::ToolbarBegin("##mat_toolbar");

        if (UI::ToolbarButton("##load_yaml", ICON_MD_FOLDER_OPEN, "Load BaseMaterial from YAML"))
        {
            std::filesystem::path yaml = DialogBoxes::OpenFileDialog();
            if (!yaml.empty())
            {
                BaseMaterial::Import(yaml);
                try
                {
                    YAML::Node root = YAML::LoadFile(std::filesystem::absolute(yaml).string());
                    auto name = root["Material"]["Name"].as<std::string>();
                    auto& AM = AssetManager::GetInstance();
                    if (auto base = AM.Get<BaseMaterial>(name))
                    {
                        auto& factory = MaterialBuilder::GetInstance();
                        mat = factory.Create(base);
                    }
                }
                catch (const std::exception& e)
                {
                    MOTION_CORE_ERROR("Failed to import base material: {}", e.what());
                }
            }
        }

        if (UI::ToolbarButton("##save_yaml", ICON_MD_SAVE_AS, "Save BaseMaterial as YAML"))
        {
            if (mat && mat->GetBaseMaterial())
            {
                std::filesystem::path out = DialogBoxes::SaveFileDialog();
                if (!out.empty())
                {
                    if (out.extension() != ".yml" && out.extension() != ".yaml")
                        out.replace_extension(".yaml");

                    mat->GetBaseMaterial()->SerializeYAML(out);
                }
            }
        }

        UI::ToolbarEnd();
    }

    void MaterialEditorPanel::DrawMaterialUI(ScenePanelContext&, std::shared_ptr<Material>& mat)
    {
        if (!mat) return;

        if (ImGui::TreeNodeEx("Material Properties", ImGuiTreeNodeFlags_Framed))
        {
            DrawAttributes(mat);
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Textures", ImGuiTreeNodeFlags_Framed))
        {
            DrawTexturesSlots(mat);
            ImGui::TreePop();
        }
    }

    void MaterialEditorPanel::DrawAttributes(std::shared_ptr<Material>& mat)
    {
        if(mat->HasTexture<CorePBR>())
        {
            auto& C = mat->GetTexture<CorePBR>();

            if(UI::BeginPropertyGrid("##core-pbr"))
            {
                UI::ColorEdit4("Base Color", C.BaseColorFactor);
                UI::SliderFloat("Metallic Factor", &C.MetallicFactor, 0.0f, 1.0f, "%.3f");
                UI::SliderFloat("Roughness Factor", &C.RoughnessFactor, 0.0f, 1.0f, "%.3f");
                UI::SliderFloat("Normal Scaling", &C.NormalScale, 0.0f, 1.0f, "%.3f");
                UI::SliderFloat("Occlusion Strength", &C.OcclusionStrength, 0.0f, 1.0f, "%.3f");
                UI::ColorEdit3("Emissive Factor", C.EmissiveFactor);
                UI::SliderFloat("Emissive Strength", &C.EmissiveStrength, 0.0f, 1.0f, "%.3f");
                UI::SliderFloat("Opacity Factor", &C.OpacityFactor, 0.0f, 1.0f, "%.3f");

                UI::EndPropertyGrid();
            }

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
        }
    }

    void MaterialEditorPanel::DrawTexturesSlots(std::shared_ptr<Material>& mat)
    {
        if(mat->HasTexture<CorePBR>())
        {
            auto& C = mat->GetTexture<CorePBR>();

            struct TextureEntry { const char* Label; std::shared_ptr<ITexture>& Tex; TextureType Type; };
            std::vector<TextureEntry> textures = 
            {
                {"Base Color",  C.BaseColorTexture, TextureType::BaseColorTexture},
                {"Metallic",    C.MetallicTexture,  TextureType::MetallicTexture},
                {"Roughness",   C.RoughnessTexture, TextureType::RoughnessTexture},
                {"Normal",      C.NormalTexture,    TextureType::NormalTexture},
                {"Occlusion",   C.OcclusionTexture, TextureType::AmbientOcclusionTexture},
                {"Emissive",    C.EmissiveTexture,  TextureType::EmissiveTexture},
            };

            const int columns = 4;                     // number of cards per row
            const float cardSpacing = 3.0f;            // space between cards

            ImGui::BeginTable("##core-pbr", columns, ImGuiTableFlags_NoBordersInBody);

            for(size_t i = 0; i < textures.size(); i++)
            {
                if(i % columns == 0)
                    ImGui::TableNextRow();

                ImGui::TableNextColumn();
                UI::TextureSlot(textures[i].Label, textures[i].Tex, textures[i].Type, nullptr, false);
            }

            ImGui::EndTable();
        }
    }
}