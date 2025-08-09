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

        auto rgba =
            [](int r, int g, int b, float a = 1.0f) -> ImVec4
            {
                return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
            };

        auto lerp =
            [](const ImVec4& a, const ImVec4& b, float t) -> ImVec4
            {
                return ImVec4(a.x + (b.x - a.x) * t,
                    a.y + (b.y - a.y) * t,
                    a.z + (b.z - a.z) * t,
                    a.w + (b.w - a.w) * t);
            };

        // ---- Layout / Shape (nice but not bubbly) ----
        style.AntiAliasedFill = true;
        style.AntiAliasedLines = true;
        style.AntiAliasedLinesUseTex = true;

        style.WindowPadding = ImVec2(12, 10);
        style.FramePadding = ImVec2(10, 6);
        style.CellPadding = ImVec2(8, 4);
        style.ItemSpacing = ImVec2(10, 8);
        style.ItemInnerSpacing = ImVec2(8, 6);

        style.IndentSpacing = 16.0f;
        style.ScrollbarSize = 14.0f;
        style.GrabMinSize = 10.0f;

        style.WindowRounding = 8.0f;
        style.ChildRounding = 8.0f;
        style.FrameRounding = 7.0f;
        style.PopupRounding = 8.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding = 7.0f;
        style.TabRounding = 7.0f;

        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;
        style.TabBorderSize = 0.0f;

        style.SeparatorTextBorderSize = 1.0f;
        style.SeparatorTextPadding = ImVec2(10, 4);
        style.DisabledAlpha = 0.50f;

        // ---- Palette ----
        const ImVec4 bg00 = rgba(14, 15, 18);      // viewport / empty bg
        const ImVec4 bg01 = rgba(18, 20, 24);      // window bg
        const ImVec4 bg02 = rgba(23, 26, 32);      // frame bg
        const ImVec4 bg03 = rgba(29, 33, 41);      // hover
        const ImVec4 bg04 = rgba(38, 43, 54);      // active
        const ImVec4 brd = rgba(47, 53, 66, 230); // borders

        const ImVec4 tx1 = rgba(230, 234, 242);   // primary text
        const ImVec4 tx2 = rgba(167, 176, 192);   // secondary
        const ImVec4 tx3 = rgba(123, 132, 148);   // muted

        const ImVec4 acc = rgba(75, 187, 240);         // accent
        const ImVec4 accHover = lerp(acc, rgba(255, 255, 255), 0.10f);
        const ImVec4 accActive = lerp(acc, rgba(0, 0, 0), 0.15f);

        const ImVec4 ok = rgba(76, 175, 80);
        const ImVec4 warn = rgba(255, 193, 7);
        const ImVec4 err = rgba(244, 67, 54);

        ImVec4* c = style.Colors;

        // Text
        c[ImGuiCol_Text] = tx1;
        c[ImGuiCol_TextDisabled] = tx3;

        // Windows / panels
        c[ImGuiCol_WindowBg] = bg01;                                  // main & platform windows
        c[ImGuiCol_ChildBg] = bg00;
        c[ImGuiCol_PopupBg] = ImVec4(bg01.x, bg01.y, bg01.z, 0.98f);
        c[ImGuiCol_ModalWindowDimBg] = ImVec4(bg00.x, bg00.y, bg00.z, 0.85f);

        // Borders
        c[ImGuiCol_Border] = brd;
        c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        // Headers / menus / selectable
        c[ImGuiCol_MenuBarBg] = bg00;
        c[ImGuiCol_Header] = ImVec4(bg02.x, bg02.y, bg02.z, 0.95f);
        c[ImGuiCol_HeaderHovered] = bg03;
        c[ImGuiCol_HeaderActive] = bg04;

        // Frames (inputs, sliders, combo, color)
        c[ImGuiCol_FrameBg] = bg02;
        c[ImGuiCol_FrameBgHovered] = bg03;
        c[ImGuiCol_FrameBgActive] = bg04;

        // Buttons
        c[ImGuiCol_Button] = bg02;
        c[ImGuiCol_ButtonHovered] = bg03;
        c[ImGuiCol_ButtonActive] = ImVec4(acc.x, acc.y, acc.z, 0.90f);

        // Check / radio / sliders
        c[ImGuiCol_CheckMark] = acc;
        c[ImGuiCol_SliderGrab] = lerp(acc, bg02, 0.35f);
        c[ImGuiCol_SliderGrabActive] = accActive;

        // Tabs
        c[ImGuiCol_Tab] = rgba(33, 37, 46);
        c[ImGuiCol_TabHovered] = rgba(40, 45, 56);
        c[ImGuiCol_TabActive] = rgba(36, 41, 51);
        c[ImGuiCol_TabUnfocused] = rgba(24, 26, 30);
        c[ImGuiCol_TabUnfocusedActive] = rgba(30, 33, 40);

        // Title bars
        c[ImGuiCol_TitleBg] = bg00;
        c[ImGuiCol_TitleBgActive] = bg02;
        c[ImGuiCol_TitleBgCollapsed] = ImVec4(bg00.x, bg00.y, bg00.z, 0.75f);

        // Separators
        c[ImGuiCol_Separator] = ImVec4(brd.x, brd.y, brd.z, 0.65f);
        c[ImGuiCol_SeparatorHovered] = accHover;
        c[ImGuiCol_SeparatorActive] = accActive;

        // Scroll bars
        c[ImGuiCol_ScrollbarBg] = ImVec4(bg00.x, bg00.y, bg00.z, 0.60f);
        c[ImGuiCol_ScrollbarGrab] = rgba(51, 56, 66);
        c[ImGuiCol_ScrollbarGrabHovered] = rgba(58, 64, 76);
        c[ImGuiCol_ScrollbarGrabActive] = rgba(66, 72, 86);

        // Resize grips
        c[ImGuiCol_ResizeGrip] = ImVec4(acc.x, acc.y, acc.z, 0.22f);
        c[ImGuiCol_ResizeGripHovered] = ImVec4(accHover.x, accHover.y, accHover.z, 0.78f);
        c[ImGuiCol_ResizeGripActive] = accActive;

        // ---- Docking (DockSpace & nodes) ----
        c[ImGuiCol_DockingPreview] = ImVec4(acc.x, acc.y, acc.z, 0.38f);   // drop highlight
        c[ImGuiCol_DockingEmptyBg] = bg00;                                  // central node background

        // Tables
        c[ImGuiCol_TableHeaderBg] = rgba(31, 34, 42);
        c[ImGuiCol_TableBorderStrong] = rgba(44, 49, 61);
        c[ImGuiCol_TableBorderLight] = rgba(34, 37, 45);
        c[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_TableRowBgAlt] = ImVec4(1, 1, 1, 0.03f);

        // Selection / drag & drop / nav
        c[ImGuiCol_TextSelectedBg] = ImVec4(acc.x, acc.y, acc.z, 0.35f);
        c[ImGuiCol_DragDropTarget] = acc;
        c[ImGuiCol_NavHighlight] = ImVec4(acc.x, acc.y, acc.z, 0.90f);
        c[ImGuiCol_NavWindowingHighlight] = ImVec4(acc.x, acc.y, acc.z, 0.35f);
        c[ImGuiCol_NavWindowingDimBg] = ImVec4(bg00.x, bg00.y, bg00.z, 0.60f);

        // Plots (optional)
        c[ImGuiCol_PlotLines] = acc;
        c[ImGuiCol_PlotLinesHovered] = accHover;
        c[ImGuiCol_PlotHistogram] = ok;
        c[ImGuiCol_PlotHistogramHovered] = lerp(ok, rgba(255, 255, 255), 0.10f);

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            // Platform windows should be opaque and keep rounding.
            style.WindowRounding = 8.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;   // fully opaque on platform windows
            // Optional: remove small border artifacts on some WMs
            style.Colors[ImGuiCol_Border].w = 1.0f;
        }

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

    CustomUIControl::Responsive  CustomUIControl::GetResponsive(float minEm, float maxEm, float spacingEm)
    {
        const float em = ImGui::GetFontSize();
        const float spacing = spacingEm * em;
        CustomUIControl::Responsive R;
        R.em = em;
        R.spacing = spacing;
        R.cardMinW = minEm * em;
        R.cardMaxW = maxEm * em;
        return R;
    }

    float CustomUIControl::ComputeCardWidth(float minW, float maxW, float spacing)
    {
        float avail = ImGui::GetContentRegionAvail().x;
        if (avail <= minW) return minW;
        float denom = (minW + spacing);
        int cols = (int)((avail + spacing) / (denom > 1e-6f ? denom : 1.0f));
        if (cols < 1) cols = 1;
        float cardW = (avail - (cols - 1) * spacing) / (float)cols;
        if (cardW < minW) cardW = minW;
        if (cardW > maxW) cardW = maxW;
        return cardW;
    }

    // --- Modern, responsive controls ---

    bool CustomUIControl::DrawFloat3(const char* label, glm::vec3& v, float resetValue, float labelWidth, float speed, float minV, float maxV, const char* fmt)
    {
        auto R = GetResponsive();
        bool changed = false;

        ImGui::PushID(label);
        if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("ctrls", ImGuiTableColumnFlags_WidthStretch);

            // --- Label column
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            // --- Controls column
            ImGui::TableNextColumn();

            const float btnW = 2.0f * R.em;                      // global reset button width
            const float availX = ImGui::GetContentRegionAvail().x; // total width in this column
            const float fieldsW = availX - btnW - R.spacing;       // space for 3 fields
            float each = (fieldsW - 2.0f * R.spacing) / 3.0f;
            if (each < 1.0f) each = fieldsW / 3.0f;

            // Colors for axis labels (UE-style vibe)
            const ImVec4 colX = ImVec4(0.92f, 0.28f, 0.26f, 1.0f);
            const ImVec4 colY = ImVec4(0.34f, 0.80f, 0.36f, 1.0f);
            const ImVec4 colZ = ImVec4(0.18f, 0.56f, 1.00f, 1.0f);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(R.spacing, ImGui::GetStyle().ItemSpacing.y));

            auto axisField =
                [&](const char* id, float& val, const ImVec4& axisColor, const char axisName)
                {
                    // Colored axis prefix (non-interactive text)
                    ImGui::PushStyleColor(ImGuiCol_Text, axisColor);
                    ImGui::TextUnformatted(&axisName, &axisName + 1);
                    ImGui::PopStyleColor();
                    ImGui::SameLine(0.0f, R.spacing * 0.6f);

                    // Field
                    ImGui::SetNextItemWidth(each - ImGui::GetFontSize() - R.spacing * 0.6f);
                    bool edited = ImGui::DragFloat(id, &val, speed, minV, maxV, fmt);

                    // Alt+Click = reset this axis
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyAlt)
                    {
                        val = resetValue;
                        edited = true;
                    }

                    // Context menu for reset(s)
                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem("Reset", nullptr))
                        {
                            val = resetValue; edited = true;
                        }
                        if (ImGui::MenuItem("Reset All"))
                        {
                            v = glm::vec3(resetValue); edited = true;
                        }
                        ImGui::EndPopup();
                    }

                    // Tooltip hints
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Drag (%c). Alt+Click to reset to %.3f\nRight click for more", axisName, resetValue);

                    return edited;
                };

            // X | Y | Z fields
            changed |= axisField("##X", v.x, colX, 'X');
            ImGui::SameLine(0.0f, R.spacing);
            changed |= axisField("##Y", v.y, colY, 'Y');
            ImGui::SameLine(0.0f, R.spacing);
            changed |= axisField("##Z", v.z, colZ, 'Z');

            // Global reset (↺) aligned to the right
            ImGui::SameLine();
            float cursorX = ImGui::GetCursorPosX();
            ImGui::SetCursorPosX(cursorX + (fieldsW - 3.0f * each - 2.0f * R.spacing)); // ensure we’re actually at the end
            ImGui::SameLine(0.0f, R.spacing);
            if (ImGui::Button("↺", ImVec2(btnW, 0)))
            {
                v = glm::vec3(resetValue);
                changed = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Reset all to %.3f", resetValue);

            ImGui::PopStyleVar();
            ImGui::EndTable();
        }
        ImGui::PopID();
        return changed;
    }

    bool CustomUIControl::DrawFloat(const char* label, float& value, float minValue, float maxValue, float speed, float labelWidth, const char* fmt)
    {
        auto R = GetResponsive();
        bool changed = false;

        ImGui::PushID(label);
        if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("ctrl", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            ImGui::TableNextColumn();
            float avail = ImGui::GetContentRegionAvail().x;
            ImGui::SetNextItemWidth(avail);
            changed = ImGui::DragFloat("##v", &value, speed, minValue, maxValue, fmt);

            ImGui::EndTable();
        }
        ImGui::PopID();
        return changed;
    }

    bool CustomUIControl::TextBox(const char* label, std::string& textValue, bool isReadOnly, size_t maxLen, float columnWidth)
    {
        auto R = GetResponsive();
        bool changed = false;

        ImGui::PushID(label);
        if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
            ImGui::TableSetupColumn("ctrl", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            ImGui::TableNextColumn();
            float avail = ImGui::GetContentRegionAvail().x;
            ImGui::SetNextItemWidth(avail);
            ImGuiInputTextFlags flags = isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0;
            static std::vector<char> buffer;
            buffer.assign(textValue.begin(), textValue.end());
            buffer.push_back('\0');
            buffer.resize((size_t)maxLen + 1u, '\0');
            if (ImGui::InputText("##txt", buffer.data(), buffer.size(), flags))
            {
                textValue = buffer.data();
                changed = true;
            }

            ImGui::EndTable();
        }
        ImGui::PopID();
        return changed;
    }

    bool CustomUIControl::ComboBox(const char* label, int& currentItem, const std::vector<std::string>& items, float labelWidth /*= 120.0f*/, float comboWidth /*= -1.0f*/)
    {
        auto R = GetResponsive();
        bool changed = false;

        ImGui::PushID(label);
        if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("ctrl", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            ImGui::TableNextColumn();
            float avail = ImGui::GetContentRegionAvail().x;
            if (comboWidth > 0.0f) ImGui::SetNextItemWidth(comboWidth);
            else ImGui::SetNextItemWidth(avail);

            const char* preview = (currentItem >= 0 && currentItem < (int)items.size()) ? items[currentItem].c_str() : "";
            if (ImGui::BeginCombo("##combo", preview))
            {
                for (int i = 0; i < (int)items.size(); ++i)
                {
                    bool selected = (i == currentItem);
                    if (ImGui::Selectable(items[i].c_str(), selected))
                    {
                        currentItem = i;
                        changed = true;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::EndTable();
        }
        ImGui::PopID();
        return changed;
    }

    bool CustomUIControl::DrawQuatEuler(const char* label, glm::quat& q, float resetDeg, float labelWidth, float speed, const char* fmt)
    {
        auto R = GetResponsive();
        bool changed = false;

        ImGui::PushID(label);
        if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("ctrls", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            ImGui::TableNextColumn();
            glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(q));

            float avail = ImGui::GetContentRegionAvail().x;
            float each = (avail - 2.0f * R.spacing) / 3.0f;
            if (each < 1.0f) each = avail / 3.0f;
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(R.spacing, ImGui::GetStyle().ItemSpacing.y));

            ImGui::SetNextItemWidth(each);
            changed |= ImGui::DragFloat("##Pitch", &eulerDeg.x, speed, -360.0f, 360.0f, fmt);
            ImGui::SameLine(0.0f, R.spacing);
            ImGui::SetNextItemWidth(each);
            changed |= ImGui::DragFloat("##Yaw", &eulerDeg.y, speed, -360.0f, 360.0f, fmt);
            ImGui::SameLine(0.0f, R.spacing);
            ImGui::SetNextItemWidth(each);
            changed |= ImGui::DragFloat("##Roll", &eulerDeg.z, speed, -360.0f, 360.0f, fmt);

            ImGui::PopStyleVar();

            if (changed)
            {
                auto wrap = [](float a)->float { while (a > 180.0f) a -= 360.0f; while (a < -180.0f) a += 360.0f; return a; };
                eulerDeg.x = wrap(eulerDeg.x);
                eulerDeg.y = wrap(eulerDeg.y);
                eulerDeg.z = wrap(eulerDeg.z);
                q = glm::quat(glm::radians(eulerDeg));
            }

            ImGui::EndTable();
        }
        ImGui::PopID();
        return changed;
    }

    bool CustomUIControl::ColorEdit3(const char* label, glm::vec3& color, float labelWidth, float pickerWidth)
    {
        auto R = GetResponsive();
        bool changed = false;

        ImGui::PushID(label);
        if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("ctrl", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            ImGui::TableNextColumn();
            float avail = ImGui::GetContentRegionAvail().x;
            if (pickerWidth > 0.0f) ImGui::SetNextItemWidth(pickerWidth);
            else ImGui::SetNextItemWidth(avail);
            changed = ImGui::ColorEdit3("##col", (float*)&color, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_Float);

            ImGui::EndTable();
        }
        ImGui::PopID();
        return changed;
    }

    void CustomUIControl::TextureSlotCard(const char* label, std::shared_ptr<ITexture>& texture, const std::function<void()>& onLoad)
    {
        using namespace Motion;
        ImGui::PushID(label);
        ImGui::PushID(texture.get());

        auto R = GetResponsive();
        const float thumb = 6.0f * R.em;
        const float cardH = 8.8f * R.em;
        const float btn = 2.0f * R.em;
        const float cardW = ComputeCardWidth(R.cardMinW, R.cardMaxW, R.spacing);

        if (cardW + R.spacing <= ImGui::GetContentRegionAvail().x)
            ImGui::SameLine(0.0f, R.spacing);

        const ImVec2 cardSize(cardW, cardH);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + cardSize.x, p0.y + cardSize.y);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        dl->AddRectFilled(ImVec2(p0.x, p1.y - 6.0f), ImVec2(p1.x, p1.y + 8.0f), IM_COL32(0, 0, 0, 50));
        ImU32 bg = ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        dl->AddRectFilled(p0, p1, bg, 10.0f);

        ImGui::BeginChild("##card", cardSize, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
            dl->AddRect(p0, p1, ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_Border)), 10.0f, 0, 1.0f);

        const bool stacked = (cardW < (R.cardMinW + 20.0f));

        auto draw_thumb = [&]() {
            const float yPad = (cardH - thumb) * 0.5f - ImGui::GetStyle().FramePadding.y;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (yPad > 0 ? yPad : 0));
            ImVec2 t0 = ImGui::GetCursorScreenPos();
            ImVec2 t1 = ImVec2(t0.x + thumb, t0.y + thumb);
            const float tile = 0.7f * R.em;
            const ImU32 c0 = IM_COL32(60, 60, 60, 255), c1 = IM_COL32(80, 80, 80, 255);
            for (float y = t0.y; y < t1.y; y += tile)
                for (float x = t0.x; x < t1.x; x += tile)
                    dl->AddRectFilled(ImVec2(x, y), ImVec2((x + tile < t1.x ? x + tile : t1.x), (y + tile < t1.y ? y + tile : t1.y)), (((int((x - t0.x) / tile) + int((y - t0.y) / tile)) & 1) ? c0 : c1));
            if (texture && texture->GetID() != 0)
                ImGui::Image((ImTextureID)(uintptr_t)texture->GetID(), ImVec2(thumb, thumb));
            else
                ImGui::Dummy(ImVec2(thumb, thumb));
            };

        if (!stacked)
        {
            if (ImGui::BeginTable("##row", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableSetupColumn("thumb", ImGuiTableColumnFlags_WidthFixed, thumb + 8.0f);
                ImGui::TableSetupColumn("details", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthFixed, btn + 8.0f);

                ImGui::TableNextColumn();
                draw_thumb();

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH"))
                    {
                        if (payload->Data && payload->DataSize > 0)
                        {
                            const char* path = (const char*)payload->Data;
                            if (path && *path)
                            {
                                if (texture)
                                {
                                    auto& spec = texture->GetSpecification();
                                    texture->ReloadFromFile(path, spec.Type, spec.FlipOnLoadDefault);
                                }
                                else
                                {
                                    texture = ITexture::Create(path, TextureType::BaseColorTexture, true);
                                }
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::TableNextColumn();
                {
                    if (texture)
                    {
                        auto& spec = texture->GetSpecification();
                        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetColumnWidth());
                        ImGui::TextUnformatted(spec.Name.empty() ? label : spec.Name.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::Text("Size: %d x %d", spec.Width, spec.Height);
                        ImGui::Text("Type: %s", GetTextureTypeString(spec.Type).c_str());

                        if (!spec.TextureFile.empty())
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.7f, 0.9f, 1.0f));
                            ImGui::TextUnformatted(spec.TextureFile.c_str());
                            ImGui::PopStyleColor();
                        }

                        ImGui::Spacing();
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.6f * R.em, 0.4f * R.em));
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                        bool flip = spec.FlipOnLoadDefault;
                        if (ImGui::Checkbox("Flip vertically", &flip)) spec.FlipOnLoadDefault = flip;
                        ImGui::SameLine();
                        if (spec.Type == TextureType::NormalTexture)
                        {
                            bool invY = spec.InvertGreen;
                            if (ImGui::Checkbox("Invert normal Y", &invY)) spec.InvertGreen = invY;
                            ImGui::SameLine();
                        }

                        if (ImGui::SmallButton(ICON_MD_REFRESH " Reload"))
                        {
                            if (spec.Source == TextureSource::TextureFile && !spec.TextureFile.empty())
                                texture->ReloadFromFile(spec.TextureFile, spec.Type, spec.FlipOnLoadDefault);
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton(ICON_MD_UPLOAD " Replace…"))
                        {
                            if (onLoad) onLoad();
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton(ICON_MD_CLEAR " Clear"))
                        {
                            texture.reset();
                        }

                        ImGui::PopStyleVar(2);
                    }
                    else
                    {
                        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetColumnWidth());
                        ImGui::TextUnformatted(label);
                        ImGui::PopTextWrapPos();
                        ImGui::Text("Size: %d x %d", 0, 0);
                        ImGui::Text("Type: %s", "Not Set");
                        ImGui::Text("Source: %s", "—");

                        ImGui::Spacing();
                        if (ImGui::SmallButton(ICON_MD_UPLOAD " Load…"))
                        {
                            if (onLoad) onLoad();
                        }
                    }
                }

                ImGui::TableNextColumn();
                {
                    float curY = ImGui::GetCursorPosY();
                    float tgtY = curY + (cardH - btn) * 0.5f - ImGui::GetStyle().FramePadding.y;
                    if (curY < tgtY) ImGui::SetCursorPosY(tgtY);

                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                    if (ImGui::Button(ICON_MD_UPLOAD, ImVec2(btn, btn)))
                    {
                        if (onLoad) onLoad();
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load / Replace");
                    ImGui::PopStyleVar();
                }

                ImGui::EndTable();
            }
        }
        else
        {
            if (ImGui::BeginTable("##stack", 1, ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Dummy(ImVec2(0, 0.25f * R.em));
                ImVec2 cur = ImGui::GetCursorPos();
                ImGui::SetCursorPosX(cur.x + (cardW - thumb) * 0.5f);
                if (texture && texture->GetID() != 0)
                    ImGui::Image((ImTextureID)(uintptr_t)texture->GetID(), ImVec2(thumb, thumb));
                else
                    ImGui::Dummy(ImVec2(thumb, thumb));

                ImGui::Dummy(ImVec2(0, 0.5f * R.em));

                if (texture)
                {
                    auto& spec = texture->GetSpecification();
                    ImGui::TextUnformatted(spec.Name.empty() ? label : spec.Name.c_str());
                    ImGui::Text("Size: %d x %d", spec.Width, spec.Height);
                    ImGui::Text("Type: %s", GetTextureTypeString(spec.Type).c_str());
                }
                else
                {
                    ImGui::TextUnformatted(label);
                    ImGui::Text("Size: %d x %d", 0, 0);
                    ImGui::Text("Type: %s", "Not Set");
                }

                if (ImGui::Button(ICON_MD_UPLOAD " Replace…"))
                {
                    if (onLoad) onLoad();
                }

                ImGui::EndTable();
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopID();
        ImGui::PopID();
    }

    // Overload without callback keeps your existing API intact
    void CustomUIControl::TextureSlotCard(const char* label, std::shared_ptr<ITexture>& texture)
    {
        TextureSlotCard(label, texture, nullptr);
    }

}