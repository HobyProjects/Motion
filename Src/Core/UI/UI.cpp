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

        // ---- Base text font ----
        ImFontConfig textCfg{};
        textCfg.OversampleH = 3;
        textCfg.OversampleV = 3;
        textCfg.PixelSnapH  = false;

        ImFont* base = nullptr;
        if (fontPath && *fontPath)
            base = io.Fonts->AddFontFromFileTTF(GetFullPath(fontPath).c_str(), sizePx, &textCfg);
        if (!base) base = io.Fonts->AddFontDefault();

        // ---- Material Icons merged into base ----
        static const ImWchar icon_ranges[] = { (ImWchar)ICON_MIN_MD, (ImWchar)ICON_MAX_MD, 0 };

        // Tune these two to align with your theme
        const float iconScale   = 1.0f;           // icons slightly larger than text
        const float iconSizePx  = sizePx * iconScale;
        const float iconYOffset = 1.0f;           // move icons up/down in pixels (try -2..+2)

        ImFontConfig iconCfg{};
        iconCfg.MergeMode      = true;             // merge into *last* font (our base)
        iconCfg.PixelSnapH     = true;
        iconCfg.OversampleH    = 1;                // oversampling 1 is fine for icon glyphs
        iconCfg.OversampleV    = 1;
        iconCfg.GlyphMinAdvanceX = iconSizePx;     // keeps icons readable beside text
        iconCfg.GlyphOffset    = ImVec2(0.1f, iconYOffset);

        io.Fonts->AddFontFromFileTTF("Assets/Fonts/MaterialIconFonts/MaterialIcons-Regular.ttf", iconSizePx, &iconCfg, icon_ranges);

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

    static inline ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t) 
    {
        return ImVec4(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t
        );
    }

    static inline ImVec4 RGBA(int r, int g, int b, float a = 1.0f) 
    {
        return ImVec4(r / 255.f, g / 255.f, b / 255.f, a);
    }

    void UserInterfaceInitializer::UseColorDarkImpl(const ImVec4& accent) noexcept
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // Layout / geometry
        style.AntiAliasedFill = true;
        style.AntiAliasedLines = true;
        style.AntiAliasedLinesUseTex = true;

        style.WindowPadding      = ImVec2(10, 10);
        style.FramePadding       = ImVec2(8, 6);
        style.ItemSpacing        = ImVec2(8, 6);
        style.ItemInnerSpacing   = ImVec2(6, 5);

        style.ScrollbarSize      = 12.0f;
        style.GrabMinSize        = 10.0f;

        style.WindowRounding     = 10.0f;
        style.ChildRounding      = 10.0f;
        style.FrameRounding      = 8.0f;
        style.PopupRounding      = 8.0f;
        style.ScrollbarRounding  = 6.0f;
        style.GrabRounding       = 6.0f;
        style.TabRounding        = 6.0f;

        style.WindowBorderSize   = 0.0f;
        style.FrameBorderSize    = 0.0f;
        style.TabBorderSize      = 0.0f;

        style.DisabledAlpha      = 0.45f;

        // Neutral *dark* palette (no hue bias)
        const ImVec4 N00 = RGBA( 14,  14,  15);      // deepest
        const ImVec4 N01 = RGBA( 18,  18,  19);      // window bg
        const ImVec4 N02 = RGBA( 25,  26,  27);      // frame bg / passive
        const ImVec4 N03 = RGBA( 32,  33,  35);      // hover
        const ImVec4 N04 = RGBA( 42,  44,  47);      // active / headers
        const ImVec4 BRD = ImVec4(0.30f, 0.30f, 0.32f, 0.60f); // subtle border

        const ImVec4 TXT  = RGBA(235, 235, 235);
        const ImVec4 TXT2 = RGBA(170, 170, 170);
        const ImVec4 TXT3 = RGBA(120, 120, 120);

        const ImVec4 acc        = accent;
        const ImVec4 accHover   = Mix(acc, ImVec4(1,1,1,1), 0.12f);
        const ImVec4 accActive  = Mix(acc, ImVec4(0,0,0,1), 0.15f);

        ImVec4* c = style.Colors;

        // Text
        c[ImGuiCol_Text]                 = TXT;
        c[ImGuiCol_TextDisabled]         = TXT3;

        // Windows / popups / docking
        c[ImGuiCol_WindowBg]             = ImVec4(N01.x, N01.y, N01.z, 0.98f);
        c[ImGuiCol_ChildBg]              = N00;
        c[ImGuiCol_PopupBg]              = ImVec4(N01.x, N01.y, N01.z, 0.98f);
        c[ImGuiCol_ModalWindowDimBg]     = ImVec4(N00.x, N00.y, N00.z, 0.75f);
        c[ImGuiCol_DockingEmptyBg]       = N00;
        c[ImGuiCol_DockingPreview]       = ImVec4(acc.x, acc.y, acc.z, 0.35f);

        // Borders / separators
        c[ImGuiCol_Border]               = BRD;
        c[ImGuiCol_BorderShadow]         = ImVec4(0,0,0,0);
        c[ImGuiCol_Separator]            = ImVec4(BRD.x, BRD.y, BRD.z, 0.65f);
        c[ImGuiCol_SeparatorHovered]     = accHover;
        c[ImGuiCol_SeparatorActive]      = accActive;

        // Headers (collapsing, selectable, table header)
        c[ImGuiCol_Header]               = ImVec4(N02.x, N02.y, N02.z, 0.95f);
        c[ImGuiCol_HeaderHovered]        = N03;
        c[ImGuiCol_HeaderActive]         = N04;

        // Frames (input, sliders, combo)
        c[ImGuiCol_FrameBg]              = N02;
        c[ImGuiCol_FrameBgHovered]       = N03;
        c[ImGuiCol_FrameBgActive]        = N04;

        // Buttons
        c[ImGuiCol_Button]               = N02;
        c[ImGuiCol_ButtonHovered]        = N03;
        c[ImGuiCol_ButtonActive]         = ImVec4(acc.x, acc.y, acc.z, 0.90f);

        // Tabs
        c[ImGuiCol_Tab]                  = RGBA(28,29,30);
        c[ImGuiCol_TabHovered]           = N03;
        c[ImGuiCol_TabActive]            = RGBA(34,35,37);
        c[ImGuiCol_TabUnfocused]         = RGBA(24,25,26);
        c[ImGuiCol_TabUnfocusedActive]   = RGBA(28,29,30);

        // Titles
        c[ImGuiCol_TitleBg]              = N00;
        c[ImGuiCol_TitleBgActive]        = N02;
        c[ImGuiCol_TitleBgCollapsed]     = ImVec4(N00.x, N00.y, N00.z, 0.70f);

        // Widgets
        c[ImGuiCol_CheckMark]            = acc;
        c[ImGuiCol_SliderGrab]           = Mix(acc, N02, 0.35f);
        c[ImGuiCol_SliderGrabActive]     = accActive;

        // Scrollbar
        c[ImGuiCol_ScrollbarBg]          = ImVec4(N00.x, N00.y, N00.z, 0.55f);
        c[ImGuiCol_ScrollbarGrab]        = RGBA(48,49,52);
        c[ImGuiCol_ScrollbarGrabHovered] = RGBA(58,59,63);
        c[ImGuiCol_ScrollbarGrabActive]  = RGBA(68,69,74);

        // Tables
        c[ImGuiCol_TableHeaderBg]        = RGBA(30,31,33);
        c[ImGuiCol_TableBorderStrong]    = RGBA(44,46,48);
        c[ImGuiCol_TableBorderLight]     = RGBA(36,37,39);
        c[ImGuiCol_TableRowBg]           = ImVec4(0,0,0,0);
        c[ImGuiCol_TableRowBgAlt]        = ImVec4(1,1,1,0.03f);

        // Navigation / selections / misc
        c[ImGuiCol_TextSelectedBg]       = ImVec4(acc.x, acc.y, acc.z, 0.35f);
        c[ImGuiCol_DragDropTarget]       = acc;
        c[ImGuiCol_NavHighlight]         = ImVec4(acc.x, acc.y, acc.z, 0.85f);
        c[ImGuiCol_NavWindowingHighlight]= ImVec4(acc.x, acc.y, acc.z, 0.30f);
        c[ImGuiCol_NavWindowingDimBg]    = ImVec4(N00.x, N00.y, N00.z, 0.60f);

        // Plots
        c[ImGuiCol_PlotLines]            = acc;
        c[ImGuiCol_PlotLinesHovered]     = accHover;
        c[ImGuiCol_PlotHistogram]        = Mix(acc, ImVec4(1,1,1,1), 0.05f);
        c[ImGuiCol_PlotHistogramHovered] = accHover;
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
