#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <imgui/imgui.h>

#include "Scene.hpp"

namespace Motion
{
    class IPanel
    {
        public:
            IPanel() = default;
            virtual ~IPanel() = default;

            virtual void OnCreate(Scene*){};
            virtual void OnUpdate(Scene*, float dt){};
            virtual void OnRender(Scene*){};

        protected:
            void ShowPhysicsTooltip(const char* concepts, const char* explanation)
            {
                ImGui::TextDisabled(ICON_MD_HELP);
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", concepts);
                    ImGui::Separator();
                    ImGui::TextUnformatted(explanation);
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }

            void RenderPhysicsEquation(const char* equation, const char* description)
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.25f, 0.3f, 1.0f));
                if (ImGui::BeginChild(equation, ImVec2(-1, 50), true))
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "%s", equation);
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", description);
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
    
    };

    class ScenePanel
    {
        public:
            ScenePanel() = default;
            ~ScenePanel() = default;

            template<typename T, typename... Args>
            requires std::is_base_of_v<IPanel, T>
            void Register(Args&&... args) 
            { 
                m_Panels.emplace_back(std::make_unique<T>(std::forward<Args>(args)...)); 
            }

            void OnCreate(Scene* scene) { for (auto& panel : m_Panels) panel->OnCreate(scene); }
            void OnUpdate(Scene* scene, float dt) { for (auto& panel : m_Panels) panel->OnUpdate(scene, dt); }
            void OnRender(Scene* scene) { for (auto& panel : m_Panels) panel->OnRender(scene); }

        private:
            std::vector<std::unique_ptr<IPanel>> m_Panels;
    };
}