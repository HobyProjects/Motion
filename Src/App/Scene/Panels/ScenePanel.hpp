#pragma once

#include <string>
#include <memory>
#include <vector>
#include <imgui/imgui.h>

#include "Scene.hpp"

namespace Motion
{
    /**
     * @brief Base interface for all UI panels in Motion Engine
     * 
     * Provides lifecycle hooks and educational helper methods for rendering
     * physics concepts with tooltips and formatted equations.
     */
    class IPanel
    {
    public:
        IPanel() = default;
        virtual ~IPanel() = default;

        IPanel(const IPanel&) = delete;
        IPanel& operator=(const IPanel&) = delete;

        IPanel(IPanel&&) noexcept = default;
        IPanel& operator=(IPanel&&) noexcept = default;

        /**
         * @brief Called once when the panel is first created
         * @param scene Pointer to the active scene
         */
        virtual void OnCreate(Scene* scene) {}

        /**
         * @brief Called every frame to update panel state
         * @param scene Pointer to the active scene
         * @param dt Delta time in seconds since last frame
         */
        virtual void OnUpdate(Scene* scene, float dt) {}

        /**
         * @brief Called every frame to render the panel UI
         * @param scene Pointer to the active scene
         */
        virtual void OnRender(Scene* scene) {}

    protected:
        /**
         * @brief Displays a help icon with tooltip explaining physics concepts
         * @param concepts Brief title of the physics concept
         * @param explanation Detailed explanation text
         */
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

        /**
         * @brief Renders a physics equation in a styled container
         * @param equation The mathematical equation to display
         * @param description Brief description of what the equation represents
         */
        void RenderPhysicsEquation(const char* equation, const char* description)
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.25f, 0.3f, 1.0f));
            
            if (ImGui::BeginChild(equation, ImVec2(-1.0f, 50.0f), true))
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "%s", equation);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", description);
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    };

    /**
     * @brief Container that manages multiple IPanel instances
     * 
     * Handles registration and lifecycle management of UI panels,
     * dispatching OnCreate, OnUpdate, and OnRender calls to all registered panels.
     */
    class ScenePanel
    {
    public:
        ScenePanel() = default;
        ~ScenePanel() = default;

        ScenePanel(const ScenePanel&) = delete;
        ScenePanel& operator=(const ScenePanel&) = delete;

        ScenePanel(ScenePanel&&) noexcept = default;
        ScenePanel& operator=(ScenePanel&&) noexcept = default;

        /**
         * @brief Registers a new panel of type T
         * @tparam T Panel type (must inherit from IPanel)
         * @tparam Args Constructor argument types
         * @param args Arguments forwarded to T's constructor
         */
        template<typename T, typename... Args>
        requires std::is_base_of_v<IPanel, T>
        void Register(Args&&... args)
        {
            m_Panels.emplace_back(std::move(std::make_unique<T>(std::forward<Args>(args)...)));
        }

        /**
         * @brief Calls OnCreate for all registered panels
         * @param scene Pointer to the active scene
         */
        void OnCreate(Scene* scene)
        {
            for (auto& panel : m_Panels)
            {
                panel->OnCreate(scene);
            }
        }

        /**
         * @brief Calls OnUpdate for all registered panels
         * @param scene Pointer to the active scene
         * @param dt Delta time in seconds since last frame
         */
        void OnUpdate(Scene* scene, float dt)
        {
            for (auto& panel : m_Panels)
            {
                panel->OnUpdate(scene, dt);
            }
        }

        /**
         * @brief Calls OnRender for all registered panels
         * @param scene Pointer to the active scene
         */
        void OnRender(Scene* scene)
        {
            for (auto& panel : m_Panels)
            {
                panel->OnRender(scene);
            }
        }

        /**
         * @brief Returns the number of registered panels
         */
        size_t GetPanelCount() const { return m_Panels.size(); }

        /**
         * @brief Clears all registered panels
         */
        void Clear() { m_Panels.clear(); }

    private:
        std::vector<std::unique_ptr<IPanel>> m_Panels;
    };
}