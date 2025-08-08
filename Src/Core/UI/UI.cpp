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

        // ---------- Layout / Shape ----------
        style.WindowRounding = 6.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 6.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 6.0f;
        style.TabRounding = 6.0f;

        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;
        style.TabBorderSize = 0.0f;

        style.WindowPadding = ImVec2(12, 10);
        style.FramePadding = ImVec2(10, 6);
        style.CellPadding = ImVec2(8, 4);
        style.ItemSpacing = ImVec2(10, 8);
        style.ItemInnerSpacing = ImVec2(8, 6);

        style.IndentSpacing = 16.0f;
        style.ScrollbarSize = 14.0f;
        style.GrabMinSize = 10.0f;

        // A tiny bit more contrast in separators
        style.SeparatorTextBorderSize = 1.0f;
        style.SeparatorTextPadding = ImVec2(10, 4);

        // ---------- Palette ----------
        // Core “UE-ish” slate with teal accent
        const ImVec4 bg0 = ImVec4(0.07f, 0.08f, 0.09f, 1.00f); // darkest
        const ImVec4 bg1 = ImVec4(0.09f, 0.10f, 0.12f, 1.00f); // window
        const ImVec4 bg2 = ImVec4(0.12f, 0.13f, 0.16f, 1.00f); // panels/frames
        const ImVec4 bg3 = ImVec4(0.16f, 0.18f, 0.21f, 1.00f); // hover
        const ImVec4 bg4 = ImVec4(0.20f, 0.22f, 0.26f, 1.00f); // active/pressed
        const ImVec4 brd = ImVec4(0.23f, 0.26f, 0.31f, 0.90f); // borders

        // Accent (cool teal/cyan)
        const ImVec4 acc = ImVec4(0.10f, 0.68f, 0.90f, 1.00f);
        const ImVec4 accHover = ImVec4(0.16f, 0.78f, 0.98f, 1.00f);
        const ImVec4 accActive = ImVec4(0.10f, 0.60f, 0.86f, 1.00f);

        // Text
        const ImVec4 txt = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
        const ImVec4 txtDim = ImVec4(0.60f, 0.64f, 0.70f, 1.00f);
        const ImVec4 txtMuted = ImVec4(0.50f, 0.54f, 0.60f, 1.00f);
        const ImVec4 txtSelBg = ImVec4(acc.x, acc.y, acc.z, 0.35f);

        ImVec4* c = style.Colors;

        // Text
        c[ImGuiCol_Text] = txt;
        c[ImGuiCol_TextDisabled] = txtMuted;

        // Windows / panels
        c[ImGuiCol_WindowBg] = bg1;
        c[ImGuiCol_ChildBg] = bg0;
        c[ImGuiCol_PopupBg] = ImVec4(bg1.x, bg1.y, bg1.z, 0.98f);

        // Borders
        c[ImGuiCol_Border] = brd;
        c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        // Frames (inputs, sliders, combo, color)
        c[ImGuiCol_FrameBg] = bg2;
        c[ImGuiCol_FrameBgHovered] = bg3;
        c[ImGuiCol_FrameBgActive] = bg4;

        // Title bars
        c[ImGuiCol_TitleBg] = bg0;
        c[ImGuiCol_TitleBgActive] = bg2;
        c[ImGuiCol_TitleBgCollapsed] = ImVec4(bg0.x, bg0.y, bg0.z, 0.75f);

        // Menus / header (collapsing header, selectable headers)
        c[ImGuiCol_MenuBarBg] = bg0;
        c[ImGuiCol_Header] = ImVec4(bg2.x, bg2.y, bg2.z, 0.90f);
        c[ImGuiCol_HeaderHovered] = ImVec4(bg3.x, bg3.y, bg3.z, 1.00f);
        c[ImGuiCol_HeaderActive] = ImVec4(bg4.x, bg4.y, bg4.z, 1.00f);

        // Scrollbars
        c[ImGuiCol_ScrollbarBg] = ImVec4(bg0.x, bg0.y, bg0.z, 0.60f);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.26f, 0.29f, 0.33f, 1.00f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.32f, 0.36f, 0.41f, 1.00f);
        c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.44f, 0.49f, 1.00f);

        // Check / radio / sliders
        c[ImGuiCol_CheckMark] = accHover;
        c[ImGuiCol_SliderGrab] = ImVec4(0.33f, 0.56f, 0.70f, 1.00f);
        c[ImGuiCol_SliderGrabActive] = acc;

        // Buttons
        c[ImGuiCol_Button] = ImVec4(bg2.x, bg2.y, bg2.z, 1.00f);
        c[ImGuiCol_ButtonHovered] = ImVec4(bg3.x, bg3.y, bg3.z, 1.00f);
        c[ImGuiCol_ButtonActive] = acc;

        // Tabs
        c[ImGuiCol_Tab] = ImVec4(0.13f, 0.15f, 0.18f, 1.00f);
        c[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
        c[ImGuiCol_TabActive] = ImVec4(0.17f, 0.20f, 0.23f, 1.00f);
        c[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.12f, 0.14f, 1.00f);
        c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.16f, 0.19f, 1.00f);

        // Separators
        c[ImGuiCol_Separator] = ImVec4(brd.x, brd.y, brd.z, 0.70f);
        c[ImGuiCol_SeparatorHovered] = accHover;
        c[ImGuiCol_SeparatorActive] = accActive;

        // Resize grips
        c[ImGuiCol_ResizeGrip] = ImVec4(acc.x, acc.y, acc.z, 0.22f);
        c[ImGuiCol_ResizeGripHovered] = ImVec4(accHover.x, accHover.y, accHover.z, 0.78f);
        c[ImGuiCol_ResizeGripActive] = accActive;

        // Tables
        c[ImGuiCol_TableHeaderBg] = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);
        c[ImGuiCol_TableBorderStrong] = ImVec4(0.20f, 0.22f, 0.26f, 1.00f);
        c[ImGuiCol_TableBorderLight] = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
        c[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_TableRowBgAlt] = ImVec4(1, 1, 1, 0.03f);

        // Selection / drag & drop / nav
        c[ImGuiCol_TextSelectedBg] = txtSelBg;
        c[ImGuiCol_DragDropTarget] = acc;
        c[ImGuiCol_NavHighlight] = ImVec4(acc.x, acc.y, acc.z, 0.90f);
        c[ImGuiCol_NavWindowingHighlight] = ImVec4(acc.x, acc.y, acc.z, 0.35f);
        c[ImGuiCol_NavWindowingDimBg] = ImVec4(0.05f, 0.06f, 0.07f, 0.60f);

        // Modals
        c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.04f, 0.05f, 0.06f, 0.85f);

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

    bool CustomUIControl::DrawFloat3(const char* label, glm::vec3& v, float resetValue, float labelWidth, float speed, float minV, float maxV, const char* fmt)
    {
        bool changed = false;

        // Stable, unique scope: data address + widget type + label
        ImGui::PushID(&v);
        ImGui::PushID("DrawFloat3");
        ImGui::PushID(label);

        if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("fields", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            // Label (left)
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            // Fields (right)
            ImGui::TableSetColumnIndex(1);

            ImGuiStyle& style = ImGui::GetStyle();
            const float full = ImGui::GetContentRegionAvail().x;
            const float itemSpacing = style.ItemInnerSpacing.x;
            const float each = (full - itemSpacing * 2.0f) / 3.0f;

            ImVec2 btnSize(ImGui::GetFrameHeight() + 2.0f, ImGui::GetFrameHeight());

            auto Axis = [&](const char* axisLabel, float& axisValue, ImVec4 col, const char* idTag)
                {
                    ImGui::PushID(idTag);

                    // Reset button
                    ImGui::PushStyleColor(ImGuiCol_Button, col);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(col.x + 0.1f, col.y + 0.1f, col.z + 0.1f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
                    if (ImGui::Button(axisLabel, btnSize)) { axisValue = resetValue; changed = true; }
                    ImGui::PopStyleColor(3);

                    ImGui::SameLine(0, itemSpacing);

                    // Field width per item
                    ImGui::SetNextItemWidth(each - btnSize.x - itemSpacing);
                    changed |= ImGui::DragFloat("##val", &axisValue, speed, minV, maxV, fmt);

                    ImGui::PopID();
                };

            // X / Y / Z inline
            Axis("X", v.x, ImVec4(0.85f, 0.20f, 0.30f, 0.95f), "X");
            ImGui::SameLine(0, itemSpacing);
            Axis("Y", v.y, ImVec4(0.20f, 0.70f, 0.20f, 0.95f), "Y");
            ImGui::SameLine(0, itemSpacing);
            Axis("Z", v.z, ImVec4(0.20f, 0.40f, 0.85f, 0.95f), "Z");

            ImGui::EndTable();
        }

        ImGui::PopID(); // label
        ImGui::PopID(); // "DrawFloat3"
        ImGui::PopID(); // &v
        return changed;
    }

    bool CustomUIControl::DrawFloat(const char* label, float& value, float minValue, float maxValue, float speed, float labelWidth, const char* fmt)
    {
        bool changed = false;

        // Stable, unique scope
        ImGui::PushID(&value);
        ImGui::PushID("DrawFloat");
        ImGui::PushID(label);

        if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("field", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            // --- Label ---
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            // --- Field ---
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN); // Fill full width of column
            changed = ImGui::DragFloat("##value", &value, speed, minValue, maxValue, fmt);

            ImGui::EndTable();
        }

        ImGui::PopID(); // label
        ImGui::PopID(); // "DrawFloat"
        ImGui::PopID(); // &value
        return changed;
    }

    bool CustomUIControl::TextBox(const char* label, std::string& textValue, bool isReadOnly, size_t maxLen, float columnWidth)
    {
        bool changed = false;

        ImGui::PushID(label);
        ImGui::PushID("TextBox");
        ImGui::PushID(&textValue);

        // One-row, two-column table: fixed label, stretchy input
        if (ImGui::BeginTable("##row", 2,
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, columnWidth > 0 ? columnWidth : 120.0f);
            ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            // Label column
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            // Input column (auto width)
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN); // <-- fill remaining width of this column

            char buffer[512];
            const size_t cap = std::min(maxLen, sizeof(buffer) - 1);
            std::strncpy(buffer, textValue.c_str(), cap);
            buffer[cap] = '\0';

            ImGuiInputTextFlags flags = isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0;
            if (ImGui::InputText("##v", buffer, cap + 1, flags))
            {
                textValue.assign(buffer);
                changed = true;
            }

            ImGui::EndTable();
        }

        ImGui::PopID(); // label
        ImGui::PopID(); // "TextBox"
        ImGui::PopID(); // &textValue
        return changed;
    }

    bool CustomUIControl::ComboBox(const char* label, int& currentItem, const std::vector<std::string>& items, float labelWidth /*= 120.0f*/, float comboWidth /*= -1.0f*/)
    {
        bool changed = false;

        // Stable ID scope
        ImGui::PushID(&currentItem);
        ImGui::PushID("ComboBox");
        ImGui::PushID(label);

        // Clamp current index (and allow -1 for "no selection")
        if (items.empty()) currentItem = -1;
        if (currentItem >= (int)items.size()) currentItem = (int)items.size() - 1;

        const char* preview = (currentItem >= 0 && currentItem < (int)items.size())
            ? items[currentItem].c_str()
            : "Select…";

        if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("field", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            // Label
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            // Field
            ImGui::TableSetColumnIndex(1);

            // Width: fill the column unless a specific width is provided
            if (comboWidth > 0.0f) ImGui::SetNextItemWidth(comboWidth);
            else                   ImGui::SetNextItemWidth(-FLT_MIN);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6)); // pill-y

            if (ImGui::BeginCombo("##v", preview, ImGuiComboFlags_HeightLarge))
            {
                for (int i = 0; i < (int)items.size(); ++i)
                {
                    bool isSelected = (i == currentItem);
                    if (ImGui::Selectable(items[i].c_str(), isSelected))
                    {
                        currentItem = i;
                        changed = true;
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::PopStyleVar(2);
            ImGui::EndTable();
        }

        ImGui::PopID(); // label
        ImGui::PopID(); // "ComboBox"
        ImGui::PopID(); // &currentItem
        return changed;
    }

    bool CustomUIControl::DrawQuatEuler(const char* label, glm::quat& q, float resetDeg, float labelWidth, float speed, const char* fmt)
    {
        auto WrapDeg = [](float a) { while (a > 180.f) a -= 360.f; while (a < -180.f) a += 360.f; return a; };

        bool changed = false;
        q = glm::normalize(q);

        // Convert to UI degrees (XYZ from glm::eulerAngles)
        glm::vec3 deg = glm::degrees(glm::eulerAngles(q));
        deg.x = WrapDeg(deg.x); deg.y = WrapDeg(deg.y); deg.z = WrapDeg(deg.z);

        // Stable unique scope
        ImGui::PushID(&q);
        ImGui::PushID("QuatEuler");
        ImGui::PushID(label);

        if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("fields", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            // Label
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            // Fields
            ImGui::TableSetColumnIndex(1);
            ImGuiStyle& style = ImGui::GetStyle();
            const float avail = ImGui::GetContentRegionAvail().x;
            const float spacing = style.ItemInnerSpacing.x;
            const float each = (avail - spacing * 2.f) / 3.f;
            const float fh = ImGui::GetFrameHeight();
            ImVec2 btnSz(fh + 2.f, fh);

            auto Axis = [&](const char* axLbl, float& v, ImVec4 col, const char* idTag)
                {
                    ImGui::PushID(idTag);
                    ImGui::PushStyleColor(ImGuiCol_Button, col);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(col.x + 0.1f, col.y + 0.1f, col.z + 0.1f, 1.f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
                    if (ImGui::Button(axLbl, btnSz)) { v = resetDeg; changed = true; }
                    ImGui::PopStyleColor(3);

                    ImGui::SameLine(0, spacing);
                    ImGui::SetNextItemWidth(each - btnSz.x - spacing);
                    changed |= ImGui::DragFloat("##v", &v, speed, -180.f, 180.f, fmt);
                    ImGui::PopID();
                };

            Axis("X", deg.x, ImVec4(0.85f, 0.20f, 0.30f, 0.95f), "X");
            ImGui::SameLine(0, spacing);
            Axis("Y", deg.y, ImVec4(0.20f, 0.70f, 0.20f, 0.95f), "Y");
            ImGui::SameLine(0, spacing);
            Axis("Z", deg.z, ImVec4(0.20f, 0.40f, 0.85f, 0.95f), "Z");

            ImGui::EndTable();
        }

        ImGui::PopID(); // label
        ImGui::PopID(); // "QuatEuler"
        ImGui::PopID(); // &q

        if (changed)
        {
            // wrap + write back
            deg.x = WrapDeg(deg.x); deg.y = WrapDeg(deg.y); deg.z = WrapDeg(deg.z);
            glm::vec3 rad = glm::radians(deg);
            if (std::all_of(&rad.x, &rad.x + 3, [](float v) { return std::isfinite(v); }))
                q = glm::normalize(glm::quat(rad));
        }

        return changed;
    }

    bool CustomUIControl::ColorEdit3(const char* label, glm::vec3& color, float labelWidth, float pickerWidth)
    {
        bool changed = false;

        // Unique and stable IDs
        ImGui::PushID(&color);
        ImGui::PushID("ColorEdit3");
        ImGui::PushID(label);

        if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("field", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            // Label
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            // Field
            ImGui::TableSetColumnIndex(1);

            // Auto width unless explicitly set
            if (pickerWidth > 0.0f) ImGui::SetNextItemWidth(pickerWidth);
            else                    ImGui::SetNextItemWidth(-FLT_MIN);

            // Apply modern style
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

            glm::vec3 tempColor = color;
            if (ImGui::ColorEdit3("##Color", glm::value_ptr(tempColor),
                ImGuiColorEditFlags_NoInputs |
                ImGuiColorEditFlags_NoLabel |
                ImGuiColorEditFlags_DisplayRGB))
            {
                color = tempColor;
                changed = true;
            }

            ImGui::PopStyleVar(2);
            ImGui::EndTable();
        }

        ImGui::PopID(); // label
        ImGui::PopID(); // "ColorEdit3"
        ImGui::PopID(); // &color

        return changed;
    }

    static void AutoFlowNext(float nextWidth, float spacing = -1.0f)
    {
        if (spacing < 0.0f) spacing = ImGui::GetStyle().ItemSpacing.x;
        float avail = ImGui::GetContentRegionAvail().x;
        if (nextWidth + spacing <= avail) ImGui::SameLine(0.0f, spacing);
    }

    static void DrawTextureSlot(const char* labelLeft, std::shared_ptr<Motion::ITexture>& tex, bool readOnly, const std::function<void()>& onLoad)
    {
        ImGui::PushID(labelLeft);
        ImGui::PushID(tex.get());
        ImGui::PushID(&labelLeft);

        // Responsive sizing
        const float maxCardW = 520.0f;             // nice target width
        const float minCardW = 360.0f;             // don't shrink below this
        float avail = ImGui::GetContentRegionAvail().x;
        float cardW = std::clamp(avail, minCardW, maxCardW);

        // Flow to same row if it fits
        AutoFlowNext(cardW);

        const float cardH = 108.0f;
        const float thumbSize = 72.0f;
        const float btnSize = 28.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        ImGui::BeginChild("##card", ImVec2(cardW, cardH), true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // 3 columns: thumb | details | icon
        if (ImGui::BeginTable("##row", 3, ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("thumb", ImGuiTableColumnFlags_WidthFixed, thumbSize + 4.0f);
            ImGui::TableSetupColumn("details", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("icon", ImGuiTableColumnFlags_WidthFixed, btnSize + 6.0f);

            // --- Column 0: thumbnail ---
            ImGui::TableNextColumn();
            {
                ImVec2 p = ImGui::GetCursorPos();
                float yPad = (cardH - thumbSize) * 0.5f - ImGui::GetStyle().FramePadding.y;
                ImGui::SetCursorPosY(std::max(p.y + yPad, p.y));
                if (tex && tex->GetID() != 0)
                    ImGui::Image((ImTextureID)(uintptr_t)tex->GetID(), ImVec2(thumbSize, thumbSize));
                else
                    ImGui::Dummy(ImVec2(thumbSize, thumbSize));
            }

            // --- Column 1: details ---
            ImGui::TableNextColumn();
            {
                if (tex)
                {
                    auto& spec = tex->GetSpecification();
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetColumnWidth());
                    ImGui::TextUnformatted(spec.Name.c_str());
                    ImGui::Text("Size: %d x %d", spec.Width, spec.Height);
                    ImGui::Text("Type: %s", GetTextureTypeString(spec.Type).c_str());
                    ImGui::Text("Source: %s", spec.TextureFile.c_str());
                    ImGui::PopTextWrapPos();
                }
                else
                {
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetColumnWidth());
                    ImGui::TextUnformatted(labelLeft);
                    ImGui::Text("Size: %d x %d", 0, 0);
                    ImGui::Text("Type: %s", "Not Set");
                    ImGui::Text("Source: %s", "Unknown");
                    ImGui::PopTextWrapPos();
                }
            }

            ImGui::TableNextColumn();
            {
                float curY = ImGui::GetCursorPosY();
                float tgtY = curY + (cardH - btnSize) * 0.5f - ImGui::GetStyle().FramePadding.y;
                ImGui::SetCursorPosY(std::max(curY, tgtY));

                if (!readOnly && onLoad)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.50f, 0.95f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.29f, 0.65f, 1.00f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.36f, 0.65f, 1.0f));

                    if (ImGui::Button(ICON_MD_UPLOAD, ImVec2(btnSize, btnSize)))
                        onLoad();

                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Load / Replace");

                    ImGui::PopStyleColor(3);
                    ImGui::PopStyleVar();
                }
                else
                {
                    ImGui::Dummy(ImVec2(btnSize, btnSize));
                }
            }

            ImGui::EndTable();
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        ImGui::PopID();
        ImGui::PopID();
        ImGui::PopID();
    }

    void CustomUIControl::TextureSlotCard(const char* label, std::shared_ptr<ITexture>& texture, const std::function<void()>& onLoad)
    {
        DrawTextureSlot(label, texture, false, onLoad);
    }

    void CustomUIControl::TextureSlotCard(const char* label, std::shared_ptr<ITexture>& texture)
    {
        DrawTextureSlot(label, texture, true, nullptr);
    }
}

