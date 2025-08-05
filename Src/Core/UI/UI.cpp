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

            std::filesystem::path fontsPath = std::filesystem::absolute(std::filesystem::path(".") / "Assets" / "Fonts" / "JetBrainsMono" / "JetBrainsMono-Regular.ttf");
            io.Fonts->AddFontFromFileTTF(fontsPath.string().c_str(), 16.0f);

            UseColorDark();
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
        // auto& colors = ImGui::GetStyle().Colors;
        // colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

        // // Headers
        // colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
        // colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
        // colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // // Buttons
        // colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
        // colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
        // colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // // Frame BG
        // colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
        // colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
        // colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // // Tabs
        // colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        // colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
        // colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
        // colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        // colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

        // // Title
        // colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        // colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        // colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Modern Dark Theme for Dear ImGui

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
        ImGui::StyleColorsLight();
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha = 1.0f;
        style.FrameRounding = 3.0f;
        style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 10.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
    }

    /**
     * @brief Custom control for dragging and reseting a glm::vec3.
     *
     * @details This function creates a custom control for dragging and reseting a glm::vec3. It is a table with 4 columns.
     * The first column is the label of the control, the second column is the reset button, the third column is the X axis, the fourth column is the Y axis, and the fifth column is the Z axis.
     * The reset button is colored according to the axis it is associated with and will reset the respective axis to the specified reset value when clicked.
     * The axis drag controls are also colored according to the axis they are associated with. They will update the respective axis of the glm::vec3 when dragged.
     * The drag controls are also width-limited and will not expand beyond the specified width.
     *
     * @param label The label of the custom control.
     * @param values The glm::vec3 to be edited.
     * @param resetValue The value to which the glm::vec3 should be reset when the reset button is clicked.
     */
    void CustomUIControl::DragControllerVec3(const char* label, glm::vec3& values, float resetValue)
    {
        ImGui::PushID(label);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 0.0f });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });
        ImGuiTabBarFlags flags = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_SizingFixedFit;
        ImGui::BeginTable("vec3Dragfloats", 4, flags);

        // Set up columns
        ImGui::TableSetupColumn("0", ImGuiTableColumnFlags_WidthFixed, 100.0f); // Fixed width column
        ImGui::TableSetupColumn("1", ImGuiTableColumnFlags_NoHeaderWidth);
        ImGui::TableSetupColumn("2", ImGuiTableColumnFlags_NoHeaderWidth);
        ImGui::TableSetupColumn("3", ImGuiTableColumnFlags_NoHeaderWidth);

        float buffer[3] = { values.x, values.y, values.z };
        const char* component[] = { "X", "Y", "Z" };

        ImGui::PushStyleVar(ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableNextColumn();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text(label);
        ImGui::PopStyleVar();

        for (uint32_t i = 0; i < 3; i++)
        {
            switch (i)
            {
            case 0:
            {
                ImGui::TableNextColumn();
                ImGui::TableSetColumnIndex(1);
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.1f, 0.15f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.2f, 0.2f,  1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.8f, 0.1f, 0.15f, 1.0f });
                if (ImGui::Button(component[0], { 25.0f, 28.0f }))
                {
                    values[0] = resetValue;
                }
                ImGui::PopStyleColor(3);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 5.0f });
                if (ImGui::DragFloat("##X", &buffer[0], 0.1f))
                {
                    values[0] = buffer[0];
                }
                ImGui::PopStyleVar();
                break;
            }
            case 1:
            {
                ImGui::TableNextColumn();
                ImGui::TableSetColumnIndex(2);
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.7f, 0.3f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.3f, 0.8f, 0.4f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.2f, 0.7f, 0.3f, 1.0f });
                if (ImGui::Button(component[1], { 25.0f, 28.0f }))
                {
                    values[1] = resetValue;
                }
                ImGui::PopStyleColor(3);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 5.0f });
                if (ImGui::DragFloat("##Y", &buffer[1], 0.1f))
                {
                    values[1] = buffer[1];
                }
                ImGui::PopStyleVar();
                break;
            }
            case 2:
            {
                ImGui::TableNextColumn();
                ImGui::TableSetColumnIndex(3);
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f,  0.25f, 0.8f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.2f,  0.35f, 0.2f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.15f, 0.25f, 0.8f, 1.0f });
                if (ImGui::Button(component[2], { 25.0f, 28.0f }))
                {
                    values[2] = resetValue;
                }

                ImGui::PopStyleColor(3);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 5.0f });
                if (ImGui::DragFloat("##Z", &buffer[2], 0.1f))
                {
                    values[2] = buffer[2];
                }
                ImGui::PopStyleVar();
                break;
            }
            default:
            {
                break;
            }
            };
        }

        ImGui::EndTable();
        ImGui::PopStyleVar(2);
        ImGui::PopID();
    }

    bool CustomUIControl::DrawFloat3(const char* label, glm::vec3& values, float resetValue, float columnWidth)
    {
        bool changed = false;
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();

        ImGui::PushID(label);

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
        ImGui::PushID(label);

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
        ImGui::PushID(label);

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();

        ImGui::PushItemWidth(36.0f);
        changed |= ImGui::ColorEdit3("##color", glm::value_ptr(color),
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
            ImGuiColorEditFlags_DisplayRGB);
        ImGui::PopItemWidth();

        ImGui::SameLine();

        ImGui::PushItemWidth(44.0f);
        changed |= ImGui::DragFloat("##R", &color.x, 0.01f, 0.0f, 1.0f, "R:%.2f");
        ImGui::SameLine(0, 2);
        changed |= ImGui::DragFloat("##G", &color.y, 0.01f, 0.0f, 1.0f, "G:%.2f");
        ImGui::SameLine(0, 2);
        changed |= ImGui::DragFloat("##B", &color.z, 0.01f, 0.0f, 1.0f, "B:%.2f");
        ImGui::PopItemWidth();

        ImGui::Columns(1);
        ImGui::PopID();
        return changed;
    }


    /**
     * @brief Displays a draggable UI control for editing a quaternion as Euler angles.
     *
     * This function presents a UI control that allows the user to manipulate a quaternion (`glm::quat`)
     * by editing its Euler angles in degrees. The quaternion is converted to Euler angles for display and editing,
     * and then converted back to a quaternion after user interaction.
     *
     * @param label The label to display for the UI control.
     * @param values Reference to the quaternion to be edited.
     * @param resetValue The value to reset the Euler angles to when requested.
     */
    void Motion::CustomUIControl::DrawFloat3(const char* label, glm::quat& values, float resetValue, float columnWidth)
    {
        glm::vec3 euler = glm::degrees(glm::eulerAngles(values)); // Convert to degrees for user editing
        DrawFloat3(label, euler, resetValue, columnWidth);
        values = glm::quat(glm::radians(euler)); // Convert back to quaternion
    }
}

