#include "CorePCH.hpp"

namespace Motion
{
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

            // sensible defaults
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            io.ConfigWindowsMoveFromTitleBarOnly = true;
            io.ConfigDockingAlwaysTabBar = true;

            // Backends
            auto& coreAPI = CoreAPI::GetInstance();
            switch (coreAPI.API())
            {
            case PlatformBaseAPIs::GLFW:
            {
                switch (Renderer::GetAPI())
                {
                case RenderingAPI::OpenGL:
                    ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)windowPtr->GetNativeWindow(), true);
                    ImGui_ImplOpenGL3_Init("#version 460 core");
                    break;
                case RenderingAPI::Vulkan:   MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break;
                case RenderingAPI::DirectX:  MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
                default:                     MOTION_ASSERT(false, "Unknown rendering API!"); break;
                }
                break;
            }
            case PlatformBaseAPIs::Win32:   MOTION_ASSERT(false, "Win32 is not implemented yet!"); break;
            default:                        MOTION_ASSERT(false, "Unknown base API!"); break;
            }

            LoadDefaultFonts("Assets/Fonts/JetBrainsMono/JetBrainsMono-Regular.ttf", 18.0f);
            ApplyTheme(Theme::Dark); // your preferred default

            MOTION_CORE_INFO("IMGUI initialized successfully. IMGUI VERSION: {0}", IMGUI_VERSION);
            return;
        }

        MOTION_ASSERT(false, "Failed to initialize IMGUI");
    }

    void UserInterfaceInitializer::Quit() noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:  ImGui_ImplOpenGL3_Shutdown(); break;
        case RenderingAPI::Vulkan:  MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break;
        case RenderingAPI::DirectX: MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
        default:                    MOTION_ASSERT(false, "Unknown rendering API!"); break;
        }

        auto& coreAPI = CoreAPI::GetInstance();
        switch (coreAPI.API())
        {
        case PlatformBaseAPIs::GLFW:  ImGui_ImplGlfw_Shutdown(); break;
        case PlatformBaseAPIs::Win32: MOTION_ASSERT(false, "Win32 is not implemented yet!"); break;
        default:                      MOTION_ASSERT(false, "Unknown base API!"); break;
        }

        ImGui::DestroyContext();
    }

    static std::string GetFullPath(const char* path)
    {
        std::filesystem::path fullPath = std::filesystem::absolute(path);
        MOTION_CORE_INFO("Full path resolved: {0}", fullPath.string());
        return fullPath.string();
    }

    // -------- Fonts ----------------------------------------------------------

    void UserInterfaceInitializer::LoadDefaultFonts(const char* fontPath, float sizePx) noexcept
    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        ImFontConfig cfg{};
        cfg.OversampleH = 3;
        cfg.OversampleV = 3;
        cfg.PixelSnapH = false;

        // 1) Base text font (use Dear ImGui default if path is null)
        ImFont* base = nullptr;
        if (fontPath && *fontPath)
            base = io.Fonts->AddFontFromFileTTF(GetFullPath(fontPath).c_str(), sizePx, &cfg);
        if (!base) base = io.Fonts->AddFontDefault();

        // 2) Merge Material Icons (monoscaled) into base
        static const ImWchar icon_ranges[] = { (ImWchar)ICON_MIN_MD, (ImWchar)ICON_MAX_MD, 0 };
        ImFontConfig iconCfg{};
        iconCfg.MergeMode = true;
        iconCfg.PixelSnapH = true;
        iconCfg.GlyphMinAdvanceX = sizePx * 1.0f; // make icons readable next to text
        io.Fonts->AddFontFromFileTTF(GetFullPath("Assets/Fonts/MaterialIconFonts/MaterialIcons-Regular.ttf").c_str(), sizePx, &iconCfg, icon_ranges);

        // Build atlas
        (void)io.Fonts->Build();
    }

    // -------- Themes ---------------------------------------------------------

    void UserInterfaceInitializer::ApplyTheme(Theme t, ImVec4 accent) noexcept
    {
        if (t == Theme::Auto)
            t = Theme::Dark;

        if (t == Theme::Dark) UseColorDarkImpl(accent);
        else                  UseColorLightImpl(accent);

        ImGuiStyle& style = ImGui::GetStyle();
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 8.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
            style.Colors[ImGuiCol_Border].w = 1.0f;
        }
    }

    void UserInterfaceInitializer::UseColorDark() noexcept { ApplyTheme(Theme::Dark); }
    void UserInterfaceInitializer::UseColorLight() noexcept { ApplyTheme(Theme::Light); }

    void UserInterfaceInitializer::UseColorDarkImpl(const ImVec4& accent) noexcept
    {
        ImGuiStyle& style = ImGui::GetStyle();

        auto rgba = [](int r, int g, int b, float a = 1.0f) -> ImVec4 {
            return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
            };
        auto lerp = [](const ImVec4& a, const ImVec4& b, float t) -> ImVec4 {
            return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
            };

        style.AntiAliasedFill = true;
        style.AntiAliasedLines = true;
        style.AntiAliasedLinesUseTex = true;

        style.ScrollbarSize = 12.0f;
        style.GrabMinSize = 10.0f;

        style.WindowRounding = 8.0f;
        style.ChildRounding = 8.0f;
        style.FrameRounding = 5.0f;
        style.PopupRounding = 5.0f;
        style.ScrollbarRounding = 5.0f;
        style.GrabRounding = 5.0f;
        style.TabRounding = 5.0f;

        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;
        style.TabBorderSize = 0.0f;

        style.SeparatorTextBorderSize = 1.0f;
        style.SeparatorTextPadding = ImVec2(10, 4);
        style.DisabledAlpha = 0.50f;

        const ImVec4 bg00 = rgba(30, 30, 30);
        const ImVec4 bg01 = rgba(40, 40, 40);
        const ImVec4 bg02 = rgba(50, 50, 50);
        const ImVec4 bg03 = rgba(60, 60, 60);
        const ImVec4 bg04 = rgba(70, 70, 70);
        const ImVec4 brd = rgba(90, 90, 90, 230);

        const ImVec4 tx1 = rgba(200, 200, 200);
        const ImVec4 tx3 = rgba(120, 120, 120);

        const ImVec4 acc = accent;
        const ImVec4 accHover = lerp(acc, rgba(255, 255, 255), 0.10f);
        const ImVec4 accActive = lerp(acc, rgba(0, 0, 0), 0.15f);
        const ImVec4 ok = rgba(80, 160, 80);

        ImVec4* c = style.Colors;
        c[ImGuiCol_Text] = tx1;
        c[ImGuiCol_TextDisabled] = tx3;

        c[ImGuiCol_WindowBg] = bg01;
        c[ImGuiCol_ChildBg] = bg00;
        c[ImGuiCol_PopupBg] = ImVec4(bg01.x, bg01.y, bg01.z, 0.98f);
        c[ImGuiCol_ModalWindowDimBg] = ImVec4(bg00.x, bg00.y, bg00.z, 0.85f);

        c[ImGuiCol_Border] = brd;
        c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        c[ImGuiCol_MenuBarBg] = bg00;
        c[ImGuiCol_Header] = ImVec4(bg02.x, bg02.y, bg02.z, 0.95f);
        c[ImGuiCol_HeaderHovered] = bg03;
        c[ImGuiCol_HeaderActive] = bg04;

        c[ImGuiCol_FrameBg] = bg02;
        c[ImGuiCol_FrameBgHovered] = bg03;
        c[ImGuiCol_FrameBgActive] = bg04;

        c[ImGuiCol_Button] = bg02;
        c[ImGuiCol_ButtonHovered] = bg03;
        c[ImGuiCol_ButtonActive] = ImVec4(acc.x, acc.y, acc.z, 0.90f);

        c[ImGuiCol_CheckMark] = acc;
        c[ImGuiCol_SliderGrab] = ImVec4((acc.x + bg02.x) * 0.65f, (acc.y + bg02.y) * 0.65f, (acc.z + bg02.z) * 0.65f, 1.0f);
        c[ImGuiCol_SliderGrabActive] = accActive;

        c[ImGuiCol_Tab] = rgba(40, 45, 55);
        c[ImGuiCol_TabHovered] = rgba(50, 55, 70);
        c[ImGuiCol_TabActive] = rgba(45, 50, 60);
        c[ImGuiCol_TabUnfocused] = rgba(35, 40, 50);
        c[ImGuiCol_TabUnfocusedActive] = rgba(40, 45, 55);

        c[ImGuiCol_TitleBg] = bg00;
        c[ImGuiCol_TitleBgActive] = bg02;
        c[ImGuiCol_TitleBgCollapsed] = ImVec4(bg00.x, bg00.y, bg00.z, 0.75f);

        c[ImGuiCol_Separator] = ImVec4(brd.x, brd.y, brd.z, 0.65f);
        c[ImGuiCol_SeparatorHovered] = accHover;
        c[ImGuiCol_SeparatorActive] = accActive;

        c[ImGuiCol_ScrollbarBg] = ImVec4(bg00.x, bg00.y, bg00.z, 0.60f);
        c[ImGuiCol_ScrollbarGrab] = rgba(50, 55, 65);
        c[ImGuiCol_ScrollbarGrabHovered] = rgba(60, 65, 80);
        c[ImGuiCol_ScrollbarGrabActive] = rgba(70, 75, 90);

        c[ImGuiCol_ResizeGrip] = ImVec4(acc.x, acc.y, acc.z, 0.22f);
        c[ImGuiCol_ResizeGripHovered] = ImVec4(accHover.x, accHover.y, accHover.z, 0.78f);
        c[ImGuiCol_ResizeGripActive] = accActive;

        c[ImGuiCol_DockingPreview] = ImVec4(acc.x, acc.y, acc.z, 0.38f);
        c[ImGuiCol_DockingEmptyBg] = bg00;

        c[ImGuiCol_TableHeaderBg] = rgba(40, 45, 55);
        c[ImGuiCol_TableBorderStrong] = rgba(50, 55, 70);
        c[ImGuiCol_TableBorderLight] = rgba(40, 45, 55);
        c[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_TableRowBgAlt] = ImVec4(1, 1, 1, 0.03f);

        c[ImGuiCol_TextSelectedBg] = ImVec4(acc.x, acc.y, acc.z, 0.35f);
        c[ImGuiCol_DragDropTarget] = acc;
        c[ImGuiCol_NavHighlight] = ImVec4(acc.x, acc.y, acc.z, 0.90f);
        c[ImGuiCol_NavWindowingHighlight] = ImVec4(acc.x, acc.y, acc.z, 0.35f);
        c[ImGuiCol_NavWindowingDimBg] = ImVec4(bg00.x, bg00.y, bg00.z, 0.60f);

        c[ImGuiCol_PlotLines] = acc;
        c[ImGuiCol_PlotLinesHovered] = lerp(acc, rgba(255, 255, 255), 0.10f);
        c[ImGuiCol_PlotHistogram] = ok;
        c[ImGuiCol_PlotHistogramHovered] = lerp(ok, rgba(255, 255, 255), 0.10f);
    }

    void UserInterfaceInitializer::UseColorLightImpl(const ImVec4& accent) noexcept
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = 6.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 5.0f;
        style.PopupRounding = 5.0f;
        style.ScrollbarRounding = 5.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;

        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.TabBorderSize = 0.0f;

        ImVec4* c = style.Colors;

        const ImVec4 bg0 = ImVec4(0.96f, 0.97f, 0.99f, 1.00f);
        const ImVec4 bg1 = ImVec4(0.98f, 0.98f, 0.99f, 1.00f);
        const ImVec4 mid = ImVec4(0.87f, 0.92f, 0.97f, 1.00f);
        const ImVec4 midH = ImVec4(0.81f, 0.86f, 0.94f, 1.00f);

        c[ImGuiCol_Text] = ImVec4(0.10f, 0.12f, 0.14f, 1.00f);
        c[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.57f, 0.60f, 1.00f);

        c[ImGuiCol_WindowBg] = bg0;
        c[ImGuiCol_ChildBg] = bg1;
        c[ImGuiCol_PopupBg] = ImVec4(0.97f, 0.98f, 1.00f, 1.00f);

        c[ImGuiCol_Border] = ImVec4(0.85f, 0.87f, 0.90f, 1.00f);
        c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        c[ImGuiCol_FrameBg] = ImVec4(0.93f, 0.95f, 0.98f, 1.00f);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.88f, 0.92f, 0.97f, 1.00f);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.81f, 0.86f, 0.95f, 1.00f);

        c[ImGuiCol_TitleBg] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
        c[ImGuiCol_TitleBgActive] = ImVec4(0.81f, 0.85f, 0.95f, 1.00f);
        c[ImGuiCol_TitleBgCollapsed] = bg0;
        c[ImGuiCol_MenuBarBg] = ImVec4(0.95f, 0.96f, 0.99f, 1.00f);

        c[ImGuiCol_ScrollbarBg] = ImVec4(0.97f, 0.97f, 0.99f, 1.00f);
        c[ImGuiCol_ScrollbarGrab] = midH;
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.72f, 0.80f, 0.91f, 1.00f);
        c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.67f, 0.76f, 0.89f, 1.00f);

        c[ImGuiCol_CheckMark] = accent;
        c[ImGuiCol_SliderGrab] = midH;
        c[ImGuiCol_SliderGrabActive] = accent;

        c[ImGuiCol_Button] = ImVec4(0.92f, 0.95f, 0.98f, 1.00f);
        c[ImGuiCol_ButtonHovered] = midH;
        c[ImGuiCol_ButtonActive] = accent;

        c[ImGuiCol_Header] = mid;
        c[ImGuiCol_HeaderHovered] = ImVec4(0.72f, 0.80f, 0.91f, 1.00f);
        c[ImGuiCol_HeaderActive] = accent;

        c[ImGuiCol_Separator] = ImVec4(0.80f, 0.85f, 0.93f, 1.00f);
        c[ImGuiCol_SeparatorHovered] = ImVec4(accent.x, accent.y, accent.z, 0.65f);
        c[ImGuiCol_SeparatorActive] = accent;

        c[ImGuiCol_ResizeGrip] = ImVec4(0.72f, 0.80f, 0.91f, 0.25f);
        c[ImGuiCol_ResizeGripHovered] = ImVec4(accent.x, accent.y, accent.z, 0.65f);
        c[ImGuiCol_ResizeGripActive] = accent;

        c[ImGuiCol_Tab] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
        c[ImGuiCol_TabHovered] = ImVec4(0.72f, 0.80f, 0.91f, 1.00f);
        c[ImGuiCol_TabActive] = ImVec4(0.81f, 0.86f, 0.94f, 1.00f);
        c[ImGuiCol_TabUnfocused] = bg0;
        c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);

        c[ImGuiCol_PlotLines] = ImVec4(0.32f, 0.35f, 0.38f, 1.00f);
        c[ImGuiCol_PlotLinesHovered] = accent;
        c[ImGuiCol_PlotHistogram] = accent;
        c[ImGuiCol_PlotHistogramHovered] = accent;

        c[ImGuiCol_TableHeaderBg] = ImVec4(0.94f, 0.96f, 0.99f, 1.00f);
        c[ImGuiCol_TableBorderStrong] = ImVec4(0.80f, 0.85f, 0.93f, 1.00f);
        c[ImGuiCol_TableBorderLight] = ImVec4(0.87f, 0.92f, 0.97f, 0.35f);
        c[ImGuiCol_TableRowBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
        c[ImGuiCol_TableRowBgAlt] = ImVec4(0.93f, 0.95f, 0.98f, 0.45f);

        c[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.25f);
        c[ImGuiCol_DragDropTarget] = accent;
        c[ImGuiCol_NavHighlight] = accent;
        c[ImGuiCol_NavWindowingHighlight] = ImVec4(accent.x, accent.y, accent.z, 0.70f);
        c[ImGuiCol_NavWindowingDimBg] = ImVec4(0.96f, 0.97f, 0.99f, 0.40f);
        c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.96f, 0.97f, 0.99f, 0.70f);
    }

}
