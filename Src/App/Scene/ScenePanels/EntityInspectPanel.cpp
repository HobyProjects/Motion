#include "CorePCH.hpp"
#include "EntityInspectPanel.hpp"

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

    void SceneEntityInspectPanel::RenderUI(ScenePanelContext& ctx)
    {
        ImGui::Begin(ICON_MD_DATA_OBJECT " Scene Entities");

        if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem(ICON_MD_FILE_UPLOAD "  Import StaticMesh"))
            {
                if (auto path = DialogBoxes::OpenFileDialog(); !path.empty())
                {
                    if (auto mesh = Importer::ImportModel(path))
                    {
                        auto& fac   = EntityFactory::GetInstance();
                        auto e      = fac.CreateEntity(mesh->GetName());

                        e->AddComponent<StaticMeshComponent>(mesh->GetName(), mesh);
                        auto& transform = e->AddComponent<TransformComponent>();

                        Bounds b{ .Min = mesh->GetMinBounds(), .Max = mesh->GetMaxBounds() };
                        FitTransformToWorldBox(transform, b, FitMode::NonUniformToBox, glm::vec3(1.8f), glm::vec3(5.0f, 0.0f, -2.0f), true, true, 0.0f);

                        ctx.ActiveScene->EmplaceEntity(e);
                    }
                }
            }
            ImGui::EndPopup();
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
            ctx.ActiveScene->SelectedEntity(EntityFactory::EMPTYENTITY);

        for (auto& ent : *ctx.ActiveScene)
        {
            auto& tag = ent->GetComponent<TagComponent>();

            const char* iconEntity = ICON_MD_LABEL_OUTLINE;    
            if (ent->HasComponent<StaticMeshComponent>()) iconEntity = ICON_MD_VIEW_IN_AR;

            // Tree row flags
            ImGuiTreeNodeFlags flags =
                ((ctx.ActiveScene->GetSelectedEntity() == ent) ? ImGuiTreeNodeFlags_Selected : 0) |
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                ImGuiTreeNodeFlags_SpanFullWidth |
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_FramePadding;

            ImGui::PushID((void*)ent.get());
            std::string label = std::format("{}  {}", iconEntity, tag.Tag);
            bool open = ImGui::TreeNodeEx("##node", flags, "%s", label.c_str());

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
                ctx.ActiveScene->SelectedEntity(ent);

            if (open)
            {
                if (ent->HasComponent<StaticMeshComponent>())
                {
                    auto& model = ent->GetComponent<StaticMeshComponent>().Model;
                    if (model)
                    {
                        ImGuiTreeNodeFlags secFlags =
                            ImGuiTreeNodeFlags_SpanAvailWidth |
                            ImGuiTreeNodeFlags_Framed |
                            ImGuiTreeNodeFlags_FramePadding;

                        if (ImGui::CollapsingHeader(std::string(ICON_MD_INFO "  Mesh Details").c_str(), secFlags))
                        {
                            std::string meshCount = std::to_string(model->GetMeshesCount());
                            std::string minBounds = glm::to_string(model->GetMinBounds());
                            std::string maxBounds = glm::to_string(model->GetMaxBounds());
                            std::string filePath = model->GetSource();

                            if (UI::BeginPropertyGrid("##entity-details"))
                            {
                                UI::TextBox("Mesh Count", meshCount, true);
                                UI::TextBox("Min Bounds", minBounds, true);
                                UI::TextBox("Max Bounds", maxBounds, true);
                                UI::TextBox("File Path", filePath, true);

                                UI::EndPropertyGrid();
                            }
                        }
                    }
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::End();
    }
}