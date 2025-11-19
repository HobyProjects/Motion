#include "CorePCH.hpp"
#include "SimulationEntity.hpp"

namespace Motion
{
    void SimulationEntity::OnRender(Scene* scene)
    {
        if (!scene) return;

        auto& context = scene->GetContext();
        ImGui::Begin("Simulation Watch List", nullptr);

        if (context.Simulation->SimulatedEntities.empty())
        {
            ImGui::TextDisabled("No entities in watch list");
        }
        else
        {
            ImGui::BeginChild("EntityList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            for (size_t i = 0; i < context.Simulation->SimulatedEntities.size(); ++i)
            {
                auto entity = context.Simulation->SimulatedEntities[i];
                bool isSelected = context.Simulation->SelectedEntity == entity;

                std::string entityLabel;
                if (context.Entities->Registry.try_get<TagComponent>(entity))
                {
                    auto& tag = context.Entities->Registry.get<TagComponent>(entity);
                    entityLabel = tag.Tag + " (ID: " + std::to_string(static_cast<uint32_t>(entity)) + ")";
                }
                else
                {
                    entityLabel = "Entity " + std::to_string(static_cast<uint32_t>(entity));
                }

                // Selectable item
                if (ImGui::Selectable(entityLabel.c_str(), isSelected))
                {
                    if (isSelected)
                    {
                        context.Simulation->SelectedEntity = entt::null;
                    }
                    else
                    {
                        context.Simulation->SelectedEntity = entity;
                    }
                }

                // Right-click context menu
                if (ImGui::BeginPopupContextItem(("EntityContext_" + std::to_string(i)).c_str()))
                {
                    if (ImGui::MenuItem("Remove from Simulation"))
                    {
                        context.Simulation->RemoveSimulatedEntity(entity);
                        if (context.Simulation->SelectedEntity == entity)
                        {
                            context.Simulation->SelectedEntity = entt::null;
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }

            ImGui::EndChild();
        }

        // Clear all button
        if (!context.Simulation->SimulatedEntities.empty())
        {
            ImGui::Separator();
            if (ImGui::Button("Clear All"))
            {
                context.Simulation->ClearSimulatedEntities();
                context.Simulation->SelectedEntity = entt::null;
            }
        }

        ImGui::End();
    }
}