#include "CorePCH.hpp"

#include "ProjectSceneViewPanel.hpp"
#include "SceneEditorLayer.hpp"

namespace Motion
{
    void SceneViewPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::Begin("Project Scenes");

        // ── Top row: Search box + Add button (same line)
        static char s_SearchBuf[128] = {};
        {
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 38.0f); // leave room for the button
            ImGui::InputTextWithHint("##SearchScenes",
                ICON_MD_SEARCH " Search scenes...",
                s_SearchBuf, sizeof(s_SearchBuf));
            ImGui::PopItemWidth();

            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_ADD "##AddScene"))
            {
                ImGui::OpenPopup("New Scene");
            }
            ImGui::Separator();
        }

        // ── Create New Scene modal
        if (ImGui::BeginPopupModal("New Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char s_NewSceneName[128] = {};
            static bool s_SetActive = true;
            static bool s_Init = true;
            static std::string s_Error;

            if (ImGui::IsWindowAppearing() || s_Init)
            {
                s_Init = false;
                s_Error.clear();
                static int s_Counter = 1;
                std::snprintf(s_NewSceneName, sizeof(s_NewSceneName), "New Scene %d", s_Counter++);
                s_SetActive = true;
                ImGui::SetKeyboardFocusHere();
            }

            ImGui::TextUnformatted("Scene name:");
            ImGui::SetNextItemWidth(320.0f);
            bool enterPressed = ImGui::InputText("##scene_name", s_NewSceneName, sizeof(s_NewSceneName),
                ImGuiInputTextFlags_EnterReturnsTrue);

            ImGui::Checkbox("Set active after creating", &s_SetActive);

            // Validation helpers
            auto isBlank =
                [](const char* s)
                {
                    for (const char* p = s; *p; ++p) if (!std::isspace((unsigned char)*p)) return false;
                    return true;

                };

            auto nameExists = [&](const std::string& n)
                {
                    for (const std::shared_ptr<Scene>& sc : *context.EditorLayerInstance)
                        if (sc->GetSpecification().Name == n) return true;

                    return false;

                };

            if (!s_Error.empty())
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1, 0.35f, 0.35f, 1), "%s", s_Error.c_str());
            }

            ImGui::Separator();

            auto tryCreate = [&]() {
                std::string name = s_NewSceneName;
                if (isBlank(name.c_str()))
                {
                    s_Error = "Name cannot be empty.";
                    return false;
                }
                if (nameExists(name))
                {
                    s_Error = "A scene with this name already exists.";
                    return false;
                }
                context.EditorLayerInstance->AddNewScene(name, s_SetActive);
                s_Error.clear();
                s_Init = true;
                ImGui::CloseCurrentPopup();
                return true;
                };

            bool createClicked = ImGui::Button("Create", ImVec2(100, 0));
            if (createClicked || enterPressed)
                tryCreate();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0)))
            {
                s_Error.clear();
                s_Init = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // ── Tree of scenes (icon on the root)
        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (ImGui::TreeNodeEx((void*)context.UILayerInstance, nodeFlags, "%s  %s", ICON_MD_COLLECTIONS, "Project Scenes"))
        {
            // case-insensitive substring matcher
            auto ci_contains = [](std::string hay, std::string needle)
                {
                    std::transform(hay.begin(), hay.end(), hay.begin(),
                        [](unsigned char c) { return (char)std::tolower(c); });
                    std::transform(needle.begin(), needle.end(), needle.begin(),
                        [](unsigned char c) { return (char)std::tolower(c); });
                    return needle.empty() || (hay.find(needle) != std::string::npos);
                };

            for (auto& scene : *context.EditorLayerInstance)
            {
                ImGui::PushID(scene.get());

                const bool isActive = scene->IsActive();

                // Scene row label with icon + optional active star
                std::string label = std::format("{}  {}[{:X}]{}",
                    ICON_MD_DASHBOARD,                     // scene icon
                    scene->GetName(),
                    scene->GetID(),
                    isActive ? std::string("  ") + ICON_MD_STAR : "");

                // Filter using our local static buffer
                if (!ci_contains(label, std::string(s_SearchBuf)))
                {
                    ImGui::PopID();
                    continue;
                }

                ImGuiTreeNodeFlags hdrFlags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Framed;

                bool open = ImGui::CollapsingHeader(label.c_str(), hdrFlags);

                // Left click / activate -> set active
                if (ImGui::IsItemClicked() ||
                    ImGui::IsItemActivated() ||
                    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) ||
                    ImGui::IsItemFocused())
                {
                    scene->SelectEntityIf();
                    context.ActiveScene = scene;                        // view-side context
                    context.EditorLayerInstance->SetActiveScene(scene); // editor state
                }

                // Right-click context menu
                if (ImGui::BeginPopupContextItem("SceneCtx"))
                {
                    if (ImGui::MenuItem(ICON_MD_EDIT "  Rename..."))
                    {
                        ImGui::CloseCurrentPopup();
                        ImGui::OpenPopup("RenameScenePopup");
                    }
                    if (ImGui::MenuItem(ICON_MD_DELETE "  Delete..."))
                    {
                        ImGui::CloseCurrentPopup();
                        ImGui::OpenPopup("DeleteScenePopup");
                    }
                    ImGui::EndPopup();
                }

                // Rename modal (unique per scene thanks to PushID)
                if (ImGui::BeginPopupModal("RenameScenePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    static char s_RenameBuf[128] = {};
                    if (ImGui::IsWindowAppearing())
                    {
                        memset(s_RenameBuf, 0, sizeof(s_RenameBuf));
                        const std::string& n = scene->GetSpecification().Name;
                        strncpy(s_RenameBuf, n.c_str(), sizeof(s_RenameBuf) - 1);
                        ImGui::SetKeyboardFocusHere();
                    }

                    ImGui::TextUnformatted("New scene name:");
                    ImGui::InputText("##rename", s_RenameBuf, sizeof(s_RenameBuf));

                    ImGui::Separator();
                    if (ImGui::Button("OK", { 80,0 }))
                    {
                        scene->GetSpecification().Name = std::string(s_RenameBuf);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", { 80,0 }))
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                // Delete confirmation modal
                if (ImGui::BeginPopupModal("DeleteScenePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::TextWrapped("%s  Delete scene \"%s\"?\nThis cannot be undone.",
                        ICON_MD_WARNING, scene->GetName().c_str());
                    ImGui::Separator();

                    if (ImGui::Button("Delete", { 80,0 }))
                    {
                        auto id = scene->GetID();
                        ImGui::CloseCurrentPopup();
                        context.EditorLayerInstance->DeleteScene(id);
                        ImGui::EndPopup(); // avoid touching 'scene' after deletion
                        ImGui::PopID();
                        break; // container mutated; restart next frame
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", { 80,0 }))
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                // Optional: inner Entities list (icon on header + bullets with tags)
                if (open)
                {
                    std::string entitiesHeader = std::string(ICON_MD_LIST) + "  Entities";
                    if (ImGui::CollapsingHeader(entitiesHeader.c_str(), hdrFlags))
                    {
                        for (const auto& entity : *scene)
                        {
                            if (entity->HasComponent<TagComponent>())
                            {
                                const std::string& tag = entity->GetComponent<TagComponent>().Tag;
                                ImGui::BulletText("%s  %s", ICON_MD_LABEL, tag.c_str());
                            }
                            else
                            {
                                ImGui::BulletText("%s  %s", ICON_MD_LABEL_OFF, "Unnamed Entity");
                            }
                        }
                    }
                }

                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        ImGui::End();
    }
}