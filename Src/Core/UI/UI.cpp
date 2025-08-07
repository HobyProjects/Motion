#include "CorePCH.hpp"
#include "UI.hpp"

namespace Motion
{
    /**
     * @brief Initializes the IMGUI library and sets up the rendering backend.
     *
     * @param[in] windowHandle The window handle to associate the IMGUI context with.
     *
     * @details This function will first check if the window handle is valid and if the window exists.
     * If the window exists, it will create the IMGUI context and associate it with the window.
     * Then it will initialize the IMGUI backend according to the specified API and renderer.
     * Finally, it will set up the color scheme and viewport settings according to the renderer and API.
     *
     * @warning If the window handle is invalid or the window does not exist, this function will assert.
     * @warning If the specified API or renderer is not supported, this function will assert.
     * @warning If the IMGUI context fails to initialize, this function will assert.
     */
    void UserInterfaceInitializer::Init(WindowHandle windowHandle) noexcept
    {
        auto& windowManager = WindowManager::GetInstance();
        std::weak_ptr<IWindow> window = windowManager.GetWindow(windowHandle);
        if (!window.expired())
        {
            auto windowPtr = window.lock();
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

            auto& coreAPI = CoreAPI::GetInstance();
            switch (coreAPI.API())
            {
            case PlatformBaseAPIs::GLFW:
            {
                switch (Renderer::GetAPI())
                {
                case RenderingAPI::OpenGL:
                {
                    ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)windowPtr->GetNativeWindow(), true);
                    ImGui_ImplOpenGL3_Init("#version 460 core");
                    break;
                }
                case RenderingAPI::Vulkan:
                {
                    MOTION_ASSERT(false, "Vulkan is not implemented yet!");
                    break;
                }
                case RenderingAPI::DirectX:
                {
                    MOTION_ASSERT(false, "DirectX is not implemented yet!");
                    break;
                }
                default:
                {
                    MOTION_ASSERT(false, "Unknown rendering API!");
                    break;
                }
                }

                break;
            }
            case PlatformBaseAPIs::Win32:
            {
                MOTION_ASSERT(false, "Win32 is not implemented yet!");
                break;
            }
            default:
            {
                MOTION_ASSERT(false, "Unknown base API!");
                break;
            }
            }

            MOTION_CORE_INFO("IMGUI initialized successfully. IMGUI VERSION: {0}", IMGUI_VERSION);
            return;
        }

        MOTION_ASSERT(false, "Failed to initialize IMGUI");
        return;
    }

    /**
     * @brief Shuts down the IMGUI library and destroys the context.
     *
     * @details This function will first shut down the IMGUI backend according to the specified API and renderer.
     * Then it will destroy the IMGUI context.
     *
     * @warning If the specified API or renderer is not supported, this function will assert.
     * @warning If the IMGUI context fails to destroy, this function will assert.
     */
    void UserInterfaceInitializer::Quit() noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:      ImGui_ImplOpenGL3_Shutdown(); break;
        case RenderingAPI::Vulkan:      MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break;
        case RenderingAPI::DirectX:     MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
        default:                        MOTION_ASSERT(false, "Unknown rendering API!"); break;
        };

        auto& coreAPI = CoreAPI::GetInstance();
        switch (coreAPI.API())
        {
        case PlatformBaseAPIs::GLFW:        ImGui_ImplGlfw_Shutdown(); break;
        case PlatformBaseAPIs::Win32:       MOTION_ASSERT(false, "Win32 is not implemented yet!"); break;
        default:                    MOTION_ASSERT(false, "Unknown base API!"); break;
        };

        ImGui::DestroyContext();
    }

    /**
     * @brief Changes the IMGUI colors to a dark theme.
     *
     * @details This function sets the colors of the IMGUI context to a dark theme. This is done by
     * modifying the colors of the ImGuiStyle structure. The colors are set to a dark theme, which is
     * a dark blue-gray color scheme.
     */
    void UserInterfaceInitializer::UseColorDark() noexcept
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // Modern, flat rounding and spacing
        style.WindowRounding = 6.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;

        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.TabBorderSize = 0.0f;

        style.WindowPadding = ImVec2(10, 10);
        style.FramePadding = ImVec2(8, 4);
        style.ItemSpacing = ImVec2(8, 6);

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.28f, 0.60f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.20f, 0.23f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.29f, 0.35f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.24f, 0.28f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.21f, 0.22f, 0.25f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.29f, 0.32f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.34f, 0.35f, 0.39f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.59f, 0.98f, 1.00f); // Accent color
        colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.29f, 0.35f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.28f, 0.59f, 0.98f, 1.00f); // Accent
        colors[ImGuiCol_Button] = ImVec4(0.18f, 0.20f, 0.23f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.29f, 0.35f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.59f, 0.98f, 1.00f); // Accent
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.28f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.21f, 0.23f, 0.29f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.28f, 0.59f, 0.98f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.28f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.59f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.28f, 0.59f, 0.98f, 0.78f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.28f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.28f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.28f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.28f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.59f, 0.98f, 0.28f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.20f, 0.22f, 0.27f, 0.05f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.28f, 0.59f, 0.98f, 0.28f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.28f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.28f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.28f, 0.59f, 0.98f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.12f, 0.13f, 0.15f, 0.50f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.12f, 0.13f, 0.15f, 0.70f);

    }

    /**
     * @brief Changes the IMGUI colors to a light theme.
     *
     * @details This function sets the colors of the IMGUI context to a light theme. It modifies the
     * colors of the ImGuiStyle structure to a light color scheme and adjusts style properties such as
     * alpha and frame rounding. It also includes additional settings for when viewports are enabled.
     */
    void UserInterfaceInitializer::UseColorLight() noexcept
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = 6.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;

        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.TabBorderSize = 0.0f;

        style.WindowPadding = ImVec2(10, 10);
        style.FramePadding = ImVec2(8, 4);
        style.ItemSpacing = ImVec2(8, 6);

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(0.10f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.57f, 0.60f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.96f, 0.97f, 0.99f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.98f, 0.98f, 0.99f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.97f, 0.98f, 1.00f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.85f, 0.87f, 0.90f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.93f, 0.95f, 0.98f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.88f, 0.92f, 0.97f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.81f, 0.86f, 0.95f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.81f, 0.85f, 0.95f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.96f, 0.97f, 0.99f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.95f, 0.96f, 0.99f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.97f, 0.97f, 0.99f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.81f, 0.86f, 0.94f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.72f, 0.80f, 0.91f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.67f, 0.76f, 0.89f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.21f, 0.47f, 0.87f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.72f, 0.80f, 0.91f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.21f, 0.47f, 0.87f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.92f, 0.95f, 0.98f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.81f, 0.86f, 0.94f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.21f, 0.47f, 0.87f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.87f, 0.92f, 0.97f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.72f, 0.80f, 0.91f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.21f, 0.47f, 0.87f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.80f, 0.85f, 0.93f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.21f, 0.47f, 0.87f, 0.65f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.21f, 0.47f, 0.87f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.72f, 0.80f, 0.91f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.21f, 0.47f, 0.87f, 0.65f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.21f, 0.47f, 0.87f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.72f, 0.80f, 0.91f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.81f, 0.86f, 0.94f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.96f, 0.97f, 0.99f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.32f, 0.35f, 0.38f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.21f, 0.47f, 0.87f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.21f, 0.47f, 0.87f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.21f, 0.47f, 0.87f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.94f, 0.96f, 0.99f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.80f, 0.85f, 0.93f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.87f, 0.92f, 0.97f, 0.35f);
        colors[ImGuiCol_TableRowBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.93f, 0.95f, 0.98f, 0.45f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.21f, 0.47f, 0.87f, 0.25f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.21f, 0.47f, 0.87f, 0.95f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.21f, 0.47f, 0.87f, 0.95f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.21f, 0.47f, 0.87f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.96f, 0.97f, 0.99f, 0.40f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.96f, 0.97f, 0.99f, 0.70f);
    }

    static std::int32_t GetPID(const char* label) noexcept
    {
        return static_cast<std::int32_t>(std::hash<std::string>{}(label));
    }

    bool CustomUIControl::DrawFloat3(const char* label, glm::vec3& values, float resetValue, float columnWidth)
    {
        bool changed = false;
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();

        ImGui::PushID(std::format("{}##{}", label, GetPID(label) + static_cast<std::int32_t>(values.x + values.y + values.z)).c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label);
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

        float lineHeight = ImGui::GetFrameHeight();
        ImVec2 buttonSize = { lineHeight + 2.0f, lineHeight };

        // --- X ---
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.2f, 0.3f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.3f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.85f, 0.2f, 0.3f, 1.0f));
        if (ImGui::Button("X", buttonSize)) { values.x = resetValue; changed = true; }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        changed |= ImGui::DragFloat("##X", &values.x, 0.1f, -FLT_MAX, FLT_MAX, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // --- Y ---
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        if (ImGui::Button("Y", buttonSize)) { values.y = resetValue; changed = true; }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        changed |= ImGui::DragFloat("##Y", &values.y, 0.1f, -FLT_MAX, FLT_MAX, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // --- Z ---
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.85f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.4f, 0.85f, 1.0f));
        if (ImGui::Button("Z", buttonSize)) { values.z = resetValue; changed = true; }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        changed |= ImGui::DragFloat("##Z", &values.z, 0.1f, -FLT_MAX, FLT_MAX, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();
        ImGui::Columns(1);

        ImGui::PopID();
        return changed;
    }

    bool CustomUIControl::DrawFloat(const char* label, float& value, float minValue, float maxValue, float speed, float columnWidth)
    {
        bool changed = false;
        ImGui::PushID(std::format("{}##{}", label, GetPID(label) + static_cast<std::int32_t>(value + minValue + maxValue)).c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label);
        ImGui::NextColumn();

        ImGui::PushItemWidth(-1);
        changed = ImGui::DragFloat("##value", &value, speed, minValue, maxValue, "%.3f");
        ImGui::PopItemWidth();

        ImGui::Columns(1);
        ImGui::PopID();
        return changed;
    }

    bool CustomUIControl::DrawColor3(const char* label, glm::vec3& color, float columnWidth)
    {
        bool changed = false;

        ImGui::PushID(std::format("{}##{}", label, GetPID(label) + static_cast<std::int32_t>(color.x + color.y + color.z)).c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();

        ImGui::BeginGroup();

        // Draw the color box
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::ColorEdit3("##color", glm::value_ptr(color), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::PopStyleVar();

        ImGui::SameLine(0.0f, 10.0f);

        // Calculate available width for 3 fields and gaps
        float totalWidth = ImGui::GetContentRegionAvail().x;
        float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float rgbWidth = (totalWidth - (itemSpacing * 2.0f)) / 3.0f;
        if (rgbWidth < 48.0f) rgbWidth = 48.0f; // minimum width for usability

        // Draw each channel as a field with proper width
        ImGui::PushItemWidth(rgbWidth);
        changed |= ImGui::DragFloat("##R", &color.x, 0.01f, 0.0f, 1.0f, "R:%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::PopItemWidth();

        ImGui::SameLine(0.0f, itemSpacing);

        ImGui::PushItemWidth(rgbWidth);
        changed |= ImGui::DragFloat("##G", &color.y, 0.01f, 0.0f, 1.0f, "G:%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::PopItemWidth();

        ImGui::SameLine(0.0f, itemSpacing);

        ImGui::PushItemWidth(rgbWidth);
        changed |= ImGui::DragFloat("##B", &color.z, 0.01f, 0.0f, 1.0f, "B:%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::PopItemWidth();

        ImGui::EndGroup();

        ImGui::Columns(1);
        ImGui::PopID();
        return changed;
    }

    bool CustomUIControl::TextBox(const char* label, std::string& textValue, bool isReadOnly, size_t maxLen, float columnWidth)
    {
        bool changed = false;
        ImGui::PushID(std::format("{}##{}", label, textValue.size() + GetPID(label)).c_str());

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label);
        ImGui::NextColumn();

        ImGui::BeginGroup();
        ImGui::SameLine(0.0f, 18.0f);

        // Text box
        char buffer[512] = {};
        strncpy(buffer, textValue.c_str(), std::min(maxLen, sizeof(buffer) - 1));
        if (ImGui::InputText("##TextBox", buffer, maxLen, isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0))
        {
            textValue = buffer;
            changed = true;
        }

        ImGui::EndGroup();
        ImGui::Columns(1);
        ImGui::PopID();

        return changed;
    }

    void CustomUIControl::TextureSlotCard(const std::string& slotLabel, std::shared_ptr<ITexture>& texture, std::function<void()> onLoad)
    {
        ImGui::PushID(std::format("{}##{}{}", slotLabel, texture ? texture->GetID() + slotLabel.size() : slotLabel.size(), GetPID(slotLabel.c_str())).c_str());
        ImGui::BeginGroup();
        const float cardWidth = 92.0f;
        const float cardHeight = 116.0f;
        const float imgSize = 56.0f;
        const float btnWidth = 54.0f;
        const float btnHeight = 22.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

        // Card background
        ImGui::Dummy(ImVec2(cardWidth, cardHeight));
        ImVec2 p_min = ImGui::GetItemRectMin();
        ImVec2 p_max = ImGui::GetItemRectMax();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImU32 bgCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0.18f, 0.19f, 0.22f, 1.0f));
        ImU32 borderCol = ImGui::GetColorU32(ImGuiCol_Border);
        draw->AddRectFilled(p_min, p_max, bgCol, 9.0f);
        draw->AddRect(p_min, p_max, borderCol, 9.0f, 0, 1.0f);

        // Thumbnail centered in card
        float imgX = p_min.x + (cardWidth - imgSize) * 0.5f;
        float imgY = p_min.y + 8.0f;
        ImGui::SetCursorScreenPos(ImVec2(imgX, imgY));
        if (texture)
            ImGui::Image(texture->GetID(), ImVec2(imgSize, imgSize));
        else
            ImGui::Dummy(ImVec2(imgSize, imgSize));

        // Label, centered under thumbnail, with ellipsis for long names
        float textY = imgY + imgSize + 4.0f;
        ImVec2 textSz = ImGui::CalcTextSize(slotLabel.c_str());
        float textX = p_min.x + (cardWidth - textSz.x) * 0.5f;
        ImGui::SetCursorScreenPos(ImVec2(textX, textY));
        float labelMaxWidth = cardWidth - 10.0f;
        if (textSz.x > labelMaxWidth) {
            // Shrink and ellipsis
            std::string clipped = slotLabel.substr(0, 10) + "...";
            ImGui::Text("%s", clipped.c_str());
        }
        else {
            ImGui::Text("%s", slotLabel.c_str());
        }

        // "Load" button centered at bottom
        float btnX = p_min.x + (cardWidth - btnWidth) * 0.5f;
        float btnY = p_max.y - btnHeight - 8.0f;
        ImGui::SetCursorScreenPos(ImVec2(btnX, btnY));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.50f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.29f, 0.65f, 1.00f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.36f, 0.65f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

        if (ImGui::Button(("Load##" + slotLabel).c_str(), ImVec2(btnWidth, btnHeight)))
            if (onLoad) onLoad();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::PopStyleVar(2);
        ImGui::EndGroup();
        ImGui::PopID();
    }

    void CustomUIControl::TextureSlotCard(const std::string& slotLabel, const std::shared_ptr<ITexture>& texture)
    {
        ImGui::PushID(std::format("{}##{}{}", slotLabel, texture ? texture->GetID() + slotLabel.size() : slotLabel.size(), GetPID(slotLabel.c_str())).c_str());
        ImGui::BeginGroup();
        const float cardWidth = 92.0f;
        const float cardHeight = 116.0f;
        const float imgSize = 56.0f;
        const float btnWidth = 54.0f;
        const float btnHeight = 22.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

        // Card background
        ImGui::Dummy(ImVec2(cardWidth, cardHeight));
        ImVec2 p_min = ImGui::GetItemRectMin();
        ImVec2 p_max = ImGui::GetItemRectMax();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImU32 bgCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0.18f, 0.19f, 0.22f, 1.0f));
        ImU32 borderCol = ImGui::GetColorU32(ImGuiCol_Border);
        draw->AddRectFilled(p_min, p_max, bgCol, 9.0f);
        draw->AddRect(p_min, p_max, borderCol, 9.0f, 0, 1.0f);

        // Thumbnail centered in card
        float imgX = p_min.x + (cardWidth - imgSize) * 0.5f;
        float imgY = p_min.y + 8.0f;
        ImGui::SetCursorScreenPos(ImVec2(imgX, imgY));
        if (texture)
            ImGui::Image(texture->GetID(), ImVec2(imgSize, imgSize));
        else
            ImGui::Dummy(ImVec2(imgSize, imgSize));

        // Label, centered under thumbnail, with ellipsis for long names
        float textY = imgY + imgSize + 4.0f;
        ImVec2 textSz = ImGui::CalcTextSize(slotLabel.c_str());
        float textX = p_min.x + (cardWidth - textSz.x) * 0.5f;
        ImGui::SetCursorScreenPos(ImVec2(textX, textY));
        float labelMaxWidth = cardWidth - 10.0f;
        if (textSz.x > labelMaxWidth) {
            // Shrink and ellipsis
            std::string clipped = slotLabel.substr(0, 10) + "...";
            ImGui::Text("%s", clipped.c_str());
        }
        else {
            ImGui::Text("%s", slotLabel.c_str());
        }

        ImGui::PopStyleVar(2);
        ImGui::EndGroup();
        ImGui::PopID();
    }

    bool CustomUIControl::ComboBox(const char* label, int& currentItem, const std::vector<std::string>& items, float labelWidth, float comboWidth)
    {
        bool changed = false;
        ImGui::PushID(std::format("{}##{}{}", label, currentItem + items.size(), GetPID(label)).c_str());

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, labelWidth);
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6)); // pill-shaped
        ImGui::PushItemWidth(comboWidth);

        std::vector<const char*> cstrItems;
        cstrItems.reserve(items.size());
        for (const auto& s : items) cstrItems.push_back(s.c_str());

        if (ImGui::Combo("##ModernCombo", &currentItem, cstrItems.data(), static_cast<int>(cstrItems.size())))
            changed = true;

        ImGui::PopItemWidth();
        ImGui::PopStyleVar(2);
        ImGui::Columns(1);
        ImGui::PopID();
        return changed;
    }

    bool Motion::CustomUIControl::DrawFloat3(const char* label, glm::quat& values, float resetValue, float columnWidth)
    {
        glm::vec3 euler = glm::degrees(glm::eulerAngles(values)); // Convert to degrees for user editing
        bool changed = DrawFloat3(label, euler, resetValue, columnWidth);
        values = glm::quat(glm::radians(euler)); // Convert back to quaternion
        return changed;
    }
}

