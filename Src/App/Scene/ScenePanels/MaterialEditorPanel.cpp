#include "CorePCH.hpp"
#include "MaterialEditorPanel.hpp"
#include <numeric>

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
        ImGui::Begin("Material Editor");

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
        auto baseMaterial = mat->GetBaseMaterial();
        if (!baseMaterial)
        {
            MOTION_ASSERT(false, "Material has no BaseMaterial!");
            return;
        }

        if (mat->HasTexture<CoreTextures>())
        {
            auto& core = mat->GetTexture<CoreTextures>();
            if (UI::CollapsibleSection("Core Texture Parameters"))
            {
                if (UI::BeginPropertyGrid("##core-pram"))
                {
                    if (core.AlbedoTexture)
                        UI::ColorEdit3("Base Color", core.BaseColor);
                    else
                        UI::ColorEdit3("Base Color", baseMaterial->BaseColor);

                    if (core.MetallicTexture)
                        UI::SliderFloat("Metallic Factor", &core.MetallicFactor, 0.0f, 1.0f);
                    else
                        UI::SliderFloat("Metallic Factor", &baseMaterial->MetallicFactor, 0.0f, 1.0f);

                    if (core.RoughnessTexture)
                        UI::SliderFloat("Roughness Factor", &core.RoughnessFactor, 0.0f, 1.0f);
                    else
                        UI::SliderFloat("Roughness Factor", &baseMaterial->RoughnessFactor, 0.0f, 1.0f);

                    if (core.OpacityTexture)
                        UI::SliderFloat("Opacity Factor", &core.Opacity, 0.0f, 1.0f);
                    else
                        UI::SliderFloat("Opacity Factor", &baseMaterial->OpacityFactor, 0.0f, 1.0f);

                    UI::EndPropertyGrid();
                }
            }
        }

        if (mat->HasTexture<ExtendedTextures>())
        {
            auto& ext = mat->GetTexture<ExtendedTextures>();
            if (UI::CollapsibleSection("Extended Texture Parameters"))
            {
                if (UI::BeginPropertyGrid("##ext-pram"))
                {
                    UI::SliderFloat("Clearcoat Factor", &ext.ClearcoatFactor, 0.0f, 1.0f);
                    UI::SliderFloat("Clearcoat Roughness Factor", &ext.ClearcoatRoughnessFactor, 0.0f, 1.0f);
                    UI::SliderFloat("Specular Level", &ext.SpecularLevel, 0.0f, 1.0f);
                    UI::ColorEdit3("Specular Color", ext.SpecularColor);

                    UI::EndPropertyGrid();
                }
            }
        }

        if (mat->HasTexture<SheenFabricTextures>())
        {
            auto& sheen = mat->GetTexture<SheenFabricTextures>();
            if (UI::CollapsibleSection("Sheen & Fabric Parameters"))
            {
                if (UI::BeginPropertyGrid("##sheen-pram"))
                {
                    UI::ColorEdit3("Sheen Color", sheen.SheenColor);
                    UI::SliderFloat("Sheen Roughness Factor", &sheen.SheenRoughness, 0.0f, 1.0f);

                    UI::EndPropertyGrid();
                }
            }
        }

        if (mat->HasTexture<TransmissionSubsurfaceTextures>())
        {
            auto& trans = mat->GetTexture<TransmissionSubsurfaceTextures>();
            if (UI::CollapsibleSection("Transmission & Subsurface Parameters"))
            {
                if (UI::BeginPropertyGrid("##trans-pram"))
                {
                    UI::SliderFloat("Transmission", &trans.Transmission, 0.0f, 1.0f);
                    UI::SliderFloat("Thickness", &trans.Thickness, 0.0f, 1.0f);
                    UI::ColorEdit3("Attenuation Color", trans.AttenuationColor);
                    UI::SliderFloat("Attenuation Distance", &trans.AttenuationDistance, 0.0f, 1.0f);

                    UI::EndPropertyGrid();
                }
            }
        }

    }

    void MaterialEditorPanel::DrawTexturesSlots(std::shared_ptr<Material>& mat)
    {
        if (mat->HasTexture<CoreTextures>())
        {
            auto& core = mat->GetTexture<CoreTextures>();
            if (UI::CollapsibleSection("Core Textures"))
            {
                UI::Grid grid("##core-textures-grid", 4, ImVec2(100, 100));
                grid.cell([&] { UI::TextureSlot("Base Color", core.AlbedoTexture, TextureType::BaseColorTexture);})
                    .cell([&] { UI::TextureSlot("Metallic", core.MetallicTexture, TextureType::MetallicTexture);})
                    .cell([&] { UI::TextureSlot("Roughness", core.RoughnessTexture, TextureType::RoughnessTexture);})
                    .cell([&] { UI::TextureSlot("Normal Map", core.NormalMapTexture, TextureType::NormalTexture);})

                    .newline()

                    .cell([&] { UI::TextureSlot("Ambient Occlusion", core.AmbientOcclusionTexture, TextureType::AmbientOcclusionTexture);})
                    .cell([&] { UI::TextureSlot("Displacement", core.DisplacementTexture, TextureType::DisplacementTexture);})
                    .cell([&] { UI::TextureSlot("Emissive", core.EmissiveTexture, TextureType::EmissiveTexture);})
                    .cell([&] { UI::TextureSlot("Opacity", core.OpacityTexture, TextureType::OpacityTexture);});
            }
        }

        if (mat->HasTexture<ExtendedTextures>())
        {
            auto& ext = mat->GetTexture<ExtendedTextures>();
            if (UI::CollapsibleSection("Extended Textures"))
            {
                UI::Grid grid("##extended-textures-grid", 4, ImVec2(100, 100));
                grid.cell([&] { UI::TextureSlot("Clearcoat Texture", ext.ClearcoatTexture, TextureType::ClearcoatTexture); })
                    .cell([&] { UI::TextureSlot("Clearcoat Roughness Texture", ext.ClearcoatRoughnessTexture, TextureType::ClearcoatRoughnessTexture); })
                    .cell([&] { UI::TextureSlot("Specular Texture", ext.SpecularTexture, TextureType::SpecularTexture); })
                    .cell([&] { UI::TextureSlot("Specular Color Texture", ext.SpecularColorTexture, TextureType::SpecularColorTexture); });
            }
        }

        if (mat->HasTexture<PackedTextures>())
        {
            auto& packed = mat->GetTexture<PackedTextures>();
            if (UI::CollapsibleSection("Packed Textures"))
            {
                UI::Grid grid("##packed-textures-grid", 1, ImVec2(100, 100));
                grid.cell([&] { UI::TextureSlot("ORM Texture", packed.ORMTexture, TextureType::ORMTexture); });
            }
        }

        if (mat->HasTexture<SheenFabricTextures>())
        {
            auto& sheen = mat->GetTexture<SheenFabricTextures>();
            if (UI::CollapsibleSection("Sheen & Fabric Textures"))
            {
                UI::Grid grid("##sheen-textures-grid", 2, ImVec2(100, 100));
                grid.cell([&] { UI::TextureSlot("Sheen Texture", sheen.SheenTexture, TextureType::SheenTexture); });
                grid.cell([&] { UI::TextureSlot("Sheen Roughness Texture", sheen.SheenRoughnessTexture, TextureType::SheenRoughnessTexture); });
            }
        }

        if (mat->HasTexture<TransmissionSubsurfaceTextures>())
        {
            auto& trans = mat->GetTexture<TransmissionSubsurfaceTextures>();
            if (UI::CollapsibleSection("Transmission & Subsurface Textures"))
            {
                UI::Grid grid("##transmission-textures-grid", 2, ImVec2(100, 100));
                grid.cell([&] { UI::TextureSlot("Transmission Texture", trans.TransmissionTexture, TextureType::TransmissionTexture); });
                grid.cell([&] { UI::TextureSlot("Thickness Texture", trans.ThicknessTexture, TextureType::ThicknessTexture); });
            }
        }
    }
}