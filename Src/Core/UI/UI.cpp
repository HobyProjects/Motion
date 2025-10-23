#include "CorePCH.hpp"

namespace Motion
{
    void UserInterface::Init(WindowHandle windowHandle) noexcept
    {
        auto& windowManager = WindowManager::GetInstance();
        std::weak_ptr<IWindow> window = windowManager.GetWindow(windowHandle);
        if (!window.expired())
        {
            auto windowPtr = window.lock();
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImPlot::CreateContext();
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
                        {
                            ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)windowPtr->GetNativeWindow(), true);
                            ImGui_ImplOpenGL3_Init("#version 460 core");
                            break;
                        }
                        case RenderingAPI::Vulkan:   MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break;
                        case RenderingAPI::DirectX:  MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
                        default:                     MOTION_ASSERT(false, "Unknown rendering API!"); break;
                    }
                    break;
                }
                case PlatformBaseAPIs::Win32:   MOTION_ASSERT(false, "Win32 is not implemented yet!"); break;
                default:                        MOTION_ASSERT(false, "Unknown base API!"); break;
            }

            MOTION_CORE_INFO("IMGUI initialized successfully. IMGUI VERSION: {0}", IMGUI_VERSION);
            return;
        }

        MOTION_ASSERT(false, "Failed to initialize IMGUI");
    }

    void UserInterface::Quit() noexcept
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

        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    static std::string GetFullPath(const char* path)
    {
        std::filesystem::path fullPath = std::filesystem::absolute(path);
        MOTION_CORE_INFO("Full path resolved: {0}", fullPath.string());
        return fullPath.string();
    }

    void UserInterface::LoadDefaultFonts(const char* fontPath, float sizePx) noexcept
    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->TexGlyphPadding = 1; 

        ImFontConfig textCfg{};
        textCfg.OversampleH = 3;          
        textCfg.OversampleV = 3;
        textCfg.PixelSnapH  = false;     

        ImFont* base = nullptr;
        if (fontPath && *fontPath)
            base = io.Fonts->AddFontFromFileTTF(fontPath, sizePx, &textCfg);

        if (!base)
            base = io.Fonts->AddFontDefault();

        static const ImWchar kMaterialIconsRange[] = 
        {
            (ImWchar)ICON_MIN_MD, (ImWchar)ICON_MAX_MD, 0
        };

        static const ImWchar kFontAwesomeRange[] = 
        {
            (ImWchar)ICON_MIN_FA, (ImWchar)ICON_MAX_FA, 0
        };


        const float iconSizePx  = sizePx; 
        const float iconYOffset = -0.5f;


        {
            ImFontConfig iconCfg{};
            iconCfg.MergeMode        = true;     
            iconCfg.PixelSnapH       = true;     
            iconCfg.OversampleH      = 1;        
            iconCfg.OversampleV      = 1;
            iconCfg.GlyphOffset      = ImVec2(0.0f, iconYOffset);
            iconCfg.GlyphMinAdvanceX = 0.0f;    

            (void)io.Fonts->AddFontFromFileTTF(
                "Assets/Fonts/IconFonts/MaterialIcons-Regular.ttf",
                iconSizePx, &iconCfg, kMaterialIconsRange
            );
        }
        {
            ImFontConfig iconCfg{};
            iconCfg.MergeMode        = true;
            iconCfg.PixelSnapH       = true;
            iconCfg.OversampleH      = 1;
            iconCfg.OversampleV      = 1;
            iconCfg.GlyphOffset      = ImVec2(0.0f, iconYOffset);
            iconCfg.GlyphMinAdvanceX = 0.0f;

            (void)io.Fonts->AddFontFromFileTTF(
                "Assets/Fonts/IconFonts/fa-regular-400.ttf",
                iconSizePx, &iconCfg, kFontAwesomeRange
            );
        }

        (void)io.Fonts->Build();
    }

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

    static void SetImGuizmoStyleForEditor(const ImVec4& accent, float dpiScale)
    {
        const ImVec4 N00 = ImVec4(1.00f, 1.00f, 1.00f, 1.0f); 
        const ImVec4 N01 = ImVec4(0.98f, 0.98f, 0.98f, 1.0f); 
        const ImVec4 N02 = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);
        const ImVec4 N03 = ImVec4(0.88f, 0.88f, 0.88f, 1.0f); 
        const ImVec4 N04 = ImVec4(0.80f, 0.80f, 0.80f, 1.0f);  
        const ImVec4 BRD = ImVec4(0.25f, 0.27f, 0.30f, 0.35f);
        const ImVec4 TXT = ImVec4(0.10f, 0.11f, 0.12f, 1.0f);  

        const ImVec4 AX_X = Mix(ImVec4(0.92f, 0.26f, 0.26f, 1.0f), N03, 0.08f);
        const ImVec4 AX_Y = Mix(ImVec4(0.30f, 0.86f, 0.36f, 1.0f), N03, 0.08f);
        const ImVec4 AX_Z = Mix(ImVec4(0.27f, 0.60f, 0.98f, 1.0f), N03, 0.08f);

        ImGuizmo::Style& s = ImGuizmo::GetStyle();

        s.TranslationLineThickness   = 3.0f  * dpiScale;
        s.TranslationLineArrowSize   = 12.0f * dpiScale;
        s.RotationLineThickness      = 3.0f  * dpiScale;
        s.RotationOuterLineThickness = 3.5f  * dpiScale;
        s.ScaleLineThickness         = 3.0f  * dpiScale;
        s.ScaleLineCircleSize        = 8.0f  * dpiScale;
        s.HatchedAxisLineThickness   = 2.0f  * dpiScale;
        s.CenterCircleSize           = 5.0f  * dpiScale;

        s.Colors[ImGuizmo::DIRECTION_X] = AX_X;
        s.Colors[ImGuizmo::DIRECTION_Y] = AX_Y;
        s.Colors[ImGuizmo::DIRECTION_Z] = AX_Z;

        s.Colors[ImGuizmo::PLANE_X] = ImVec4(AX_X.x, AX_X.y, AX_X.z, 0.40f);
        s.Colors[ImGuizmo::PLANE_Y] = ImVec4(AX_Y.x, AX_Y.y, AX_Y.z, 0.40f);
        s.Colors[ImGuizmo::PLANE_Z] = ImVec4(AX_Z.x, AX_Z.y, AX_Z.z, 0.40f);

        s.Colors[ImGuizmo::SELECTION] = ImVec4(accent.x, accent.y, accent.z, 0.95f);

        const ImVec4 inactiveBase = Mix(N02, N04, 0.40f);
        s.Colors[ImGuizmo::INACTIVE] = ImVec4(inactiveBase.x, inactiveBase.y, inactiveBase.z, 0.70f);

        const ImVec4 lineIdle   = Mix(N04, ImVec4(0.0f,0.0f,0.0f,1.0f), 0.25f);
        const ImVec4 lineBright = Mix(N04, ImVec4(0.0f,0.0f,0.0f,1.0f), 0.40f); 

        s.Colors[ImGuizmo::TRANSLATION_LINE] = ImVec4(lineIdle.x,   lineIdle.y,   lineIdle.z,   0.85f);
        s.Colors[ImGuizmo::SCALE_LINE]       = ImVec4(lineBright.x, lineBright.y, lineBright.z, 0.90f);

        s.Colors[ImGuizmo::ROTATION_USING_BORDER] = ImVec4(0.10f, 0.10f, 0.10f, 0.85f);
        s.Colors[ImGuizmo::ROTATION_USING_FILL]   = ImVec4(0.10f, 0.10f, 0.10f, 0.08f);

        s.Colors[ImGuizmo::HATCHED_AXIS_LINES] = ImVec4(BRD.x, BRD.y, BRD.z, 0.85f);
        s.Colors[ImGuizmo::TEXT]               = TXT;
        s.Colors[ImGuizmo::TEXT_SHADOW]        = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
    }


    void UserInterface::UseColorLight(const ImVec4& accent) noexcept
    {
        ImGui::StyleColorsLight();
        ImGuiStyle& style = ImGui::GetStyle();

        style.AntiAliasedFill        = true;
        style.AntiAliasedLines       = true;
        style.AntiAliasedLinesUseTex = true;
        style.FontScaleDpi           = 1.18f;

        // Layout metrics
        style.WindowPadding      = ImVec2(8, 8);
        style.FramePadding       = ImVec2(6, 6);
        style.ItemSpacing        = ImVec2(8, 5);
        style.ItemInnerSpacing   = ImVec2(6, 4);
        style.IndentSpacing      = 8.0f;
        style.GrabMinSize        = 15.0f;

        // Borders
        style.WindowBorderSize   = 1.0f;
        style.ChildBorderSize    = 1.0f;
        style.PopupBorderSize    = 1.0f;
        style.FrameBorderSize    = 1.0f;

        // Rounding
        style.WindowRounding     = 12.0f;
        style.ChildRounding      = 6.0f;
        style.FrameRounding      = 8.0f;
        style.PopupRounding      = 5.0f;
        style.GrabRounding       = 8.0f;

        // Scrollbar
        style.ScrollbarSize      = 12.0f;
        style.ScrollbarRounding  = 8.0f;
        style.ScrollbarPadding   = 2.0f;

        // Tabs
        style.TabBorderSize      = 1.0f;
        style.TabBarBorderSize   = 1.0f;
        style.TabRounding        = 5.0f;

        // Tables
        style.CellPadding        = ImVec2(15, 5);
        style.DisabledAlpha      = 0.50f;

        // Light palette
        const ImVec4 N00 = ImVec4(1.00f, 1.00f, 1.00f, 1.0f); // white
        const ImVec4 N01 = ImVec4(0.98f, 0.98f, 0.98f, 1.0f); // window bg
        const ImVec4 N02 = ImVec4(0.94f, 0.94f, 0.94f, 1.0f); // frame/button bg
        const ImVec4 N03 = ImVec4(0.88f, 0.88f, 0.88f, 1.0f); // hover
        const ImVec4 N04 = ImVec4(0.80f, 0.80f, 0.80f, 1.0f); // active
        const ImVec4 N05 = ImVec4(0.72f, 0.72f, 0.72f, 1.0f); // header bg
        const ImVec4 BRD = ImVec4(0.25f, 0.27f, 0.30f, 0.35f); // subtle darker border

        const ImVec4 TXT  = ImVec4(0.10f, 0.11f, 0.12f, 1.0f); // primary text (near-black)
        const ImVec4 TXT2 = ImVec4(0.30f, 0.32f, 0.36f, 1.0f); // secondary
        const ImVec4 TXT3 = ImVec4(0.55f, 0.57f, 0.60f, 1.0f); // disabled

        const ImVec4 acc       = accent;
        const ImVec4 accHover  = Mix(acc, ImVec4(1,1,1,1), 0.10f);
        const ImVec4 accActive = Mix(acc, ImVec4(0,0,0,1), 0.20f);

        ImVec4* c = style.Colors;
        c[ImGuiCol_Text]                 = TXT;
        c[ImGuiCol_TextDisabled]         = TXT3;

        c[ImGuiCol_WindowBg]             = ImVec4(N01.x, N01.y, N01.z, 1.00f);
        c[ImGuiCol_ChildBg]              = N00;
        c[ImGuiCol_PopupBg]              = ImVec4(N00.x, N00.y, N00.z, 0.98f);
        c[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.10f, 0.10f, 0.10f, 0.20f);
        c[ImGuiCol_DockingEmptyBg]       = N00;
        c[ImGuiCol_DockingPreview]       = ImVec4(acc.x, acc.y, acc.z, 0.25f);

        c[ImGuiCol_Border]               = BRD;
        c[ImGuiCol_BorderShadow]         = ImVec4(0,0,0,0);
        c[ImGuiCol_Separator]            = ImVec4(BRD.x, BRD.y, BRD.z, 0.50f);
        c[ImGuiCol_SeparatorHovered]     = accHover;
        c[ImGuiCol_SeparatorActive]      = accActive;

        c[ImGuiCol_Header]               = N02;
        c[ImGuiCol_HeaderHovered]        = N03;
        c[ImGuiCol_HeaderActive]         = N04;

        c[ImGuiCol_FrameBg]              = N02;
        c[ImGuiCol_FrameBgHovered]       = N03;
        c[ImGuiCol_FrameBgActive]        = N04;

        c[ImGuiCol_Button]               = N02;
        c[ImGuiCol_ButtonHovered]        = N03;
        c[ImGuiCol_ButtonActive]         = Mix(acc, N04, 0.20f);

        c[ImGuiCol_Tab]                  = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
        c[ImGuiCol_TabHovered]           = N03;
        c[ImGuiCol_TabActive]            = ImVec4(0.92f, 0.92f, 0.92f, 1.0f);
        c[ImGuiCol_TabUnfocused]         = ImVec4(0.97f, 0.97f, 0.97f, 1.0f);
        c[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);

        c[ImGuiCol_TitleBg]              = N01;
        c[ImGuiCol_TitleBgActive]        = N02;
        c[ImGuiCol_TitleBgCollapsed]     = ImVec4(N01.x, N01.y, N01.z, 0.80f);

        c[ImGuiCol_CheckMark]            = acc;
        c[ImGuiCol_SliderGrab]           = Mix(acc, N02, 0.25f);
        c[ImGuiCol_SliderGrabActive]     = accActive;

        c[ImGuiCol_ScrollbarBg]          = ImVec4(0.96f, 0.96f, 0.96f, 1.0f);
        c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.86f, 0.86f, 0.86f, 1.0f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.78f, 0.78f, 0.78f, 1.0f);
        c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);

        c[ImGuiCol_TableHeaderBg]        = N05;
        c[ImGuiCol_TableBorderStrong]    = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
        c[ImGuiCol_TableBorderLight]     = ImVec4(0.92f, 0.92f, 0.92f, 1.0f);
        c[ImGuiCol_TableRowBg]           = ImVec4(0,0,0,0);
        c[ImGuiCol_TableRowBgAlt]        = ImVec4(0,0,0,0.03f);

        c[ImGuiCol_TextSelectedBg]       = ImVec4(acc.x, acc.y, acc.z, 0.25f);
        c[ImGuiCol_DragDropTarget]       = acc;
        c[ImGuiCol_NavHighlight]         = ImVec4(acc.x, acc.y, acc.z, 0.70f);
        c[ImGuiCol_NavWindowingHighlight]= ImVec4(acc.x, acc.y, acc.z, 0.20f);
        c[ImGuiCol_NavWindowingDimBg]    = ImVec4(0.10f, 0.10f, 0.10f, 0.15f);

        c[ImGuiCol_PlotLines]            = acc;
        c[ImGuiCol_PlotLinesHovered]     = accHover;
        c[ImGuiCol_PlotHistogram]        = Mix(acc, ImVec4(1,1,1,1), 0.15f);
        c[ImGuiCol_PlotHistogramHovered] = accHover;

        style.MouseCursorScale = 1.4f;
        SetImGuizmoStyleForEditor(accent, ImGui::GetIO().FontGlobalScale);
    }

}
