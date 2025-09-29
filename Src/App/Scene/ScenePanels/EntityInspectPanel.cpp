#include "CorePCH.hpp"
#include "EntityInspectPanel.hpp"

namespace Motion
{
    void SceneEntityInspectPanel::RenderUI(ScenePanelContext& ctx)
    {
        ImGui::Begin(ICON_MD_LIST " Entities");

        if (ImGui::BeginPopupContextWindow("##import-mesh-context", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem(ICON_MD_FILE_UPLOAD "  Import Model"))
            {
                if (auto path = DialogBoxes::OpenFileDialog(); !path.empty())
                {
                    auto entity = Importer::ImportModelAsync(path, false, "default");
                    if(entity) ctx.ActiveScene->EmplaceEntity(entity);
                }
            }
            
            ImGui::EndPopup();
        }

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed;
        static std::shared_ptr<Entity> selectedEntity = nullptr;
        for (auto& ent : *ctx.ActiveScene) 
        {
            std::string name = ent->Get<TagComponent>().Tag;
            ImGui::PushID(ent.get());
            if(ImGui::CollapsingHeader(name.c_str(), flags))
            {
                if (ImGui::IsItemClicked())
                {
                    selectedEntity = ent;
                    ctx.ActiveScene->SelectedEntity(selectedEntity);
                }

                std::shared_ptr<Entity> current = ent;
                std::shared_ptr<Entity> next = nullptr;
                while(current)
                {
                    if(current->Has<NodeComponent>())
                    {
                        next = current->Get<NodeComponent>().EnTTNext;
                        if(current->Get<NodeComponent>().IsRoot)
                        {
                            current = next;
                            continue;
                        }
                    }
                    
                    std::string currentEntityName = current->Get<TagComponent>().Tag;
                    ImGui::PushID(current.get());
                    ImGui::Indent();
                    if(ImGui::CollapsingHeader(currentEntityName.c_str()))
                    {
                        if (ImGui::IsItemClicked()) 
                        {
                            selectedEntity = current;
                            ctx.ActiveScene->SelectedEntity(selectedEntity);
                        }

                        ImGui::Indent();
                        ImGui::BulletText(ICON_FA_CUBE" Transform Component");
                        ImGui::BulletText(ICON_FA_CUBES" Mesh Component");
                        ImGui::BulletText(ICON_MD_IMAGE" Material Component");
                        ImGui::BulletText(ICON_MD_3D_ROTATION" RigidBody Component");
                        ImGui::BulletText(ICON_FA_BOX" Collider Component");
                        ImGui::Unindent();
                    }
                    ImGui::Unindent();
                    ImGui::PopID();

                    current = next;
                }
            }
            ImGui::PopID();
        }

        ImGui::End();
    }
}