#include "CorePCH.hpp"

#ifdef _WIN32
    #include <Windows.h>
    #include <dwmapi.h>
     #define GLFW_EXPOSE_NATIVE_WIN32
     #include <GLFW/glfw3native.h>
     #pragma comment(lib, "dwmapi.lib")
 #endif

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

            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            io.ConfigWindowsMoveFromTitleBarOnly = true;
            io.ConfigDockingAlwaysTabBar = true;
            io.ConfigViewportsNoDecoration = false; 

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

    void UserInterface::LoadDefaultFonts(const char* fontPath, float sizePx) noexcept
    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->TexGlyphPadding = 2; 

        ImFontConfig textCfg{};
        textCfg.OversampleH = 4;  
        textCfg.OversampleV = 4;
        textCfg.PixelSnapH  = false;

        ImFont* base = nullptr;
        if (fontPath && *fontPath)
        {
            base = io.Fonts->AddFontFromFileTTF(fontPath, sizePx, &textCfg);
        }

        if (!base)
        {
            base = io.Fonts->AddFontDefault();
        }

        static const ImWchar kMaterialIconsRange[] = 
        {
            (ImWchar)ICON_MIN_MD, (ImWchar)ICON_MAX_MD, 0
        };
        static const ImWchar kFontAwesomeRange[] = 
        {
            (ImWchar)ICON_MIN_FA, (ImWchar)ICON_MAX_FA, 0
        };

        const float iconSizePx  = sizePx;
        const float iconYOffset = 0.0f;  
        {
            ImFontConfig iconCfg{};
            iconCfg.MergeMode        = true;
            iconCfg.PixelSnapH       = true;
            iconCfg.OversampleH      = 2;
            iconCfg.OversampleV      = 2;
            iconCfg.GlyphOffset      = ImVec2(0.0f, iconYOffset);
            iconCfg.GlyphMinAdvanceX = iconSizePx;

            io.Fonts->AddFontFromFileTTF(
                "Assets/Fonts/IconFonts/MaterialIcons-Regular.ttf",
                iconSizePx, &iconCfg, kMaterialIconsRange
            );
        }
        {
            ImFontConfig iconCfg{};
            iconCfg.MergeMode        = true;
            iconCfg.PixelSnapH       = true;
            iconCfg.OversampleH      = 2;
            iconCfg.OversampleV      = 2;
            iconCfg.GlyphOffset      = ImVec2(0.0f, iconYOffset);
            iconCfg.GlyphMinAdvanceX = iconSizePx;

            io.Fonts->AddFontFromFileTTF(
                "Assets/Fonts/IconFonts/fa-regular-400.ttf",
                iconSizePx, &iconCfg, kFontAwesomeRange
            );
        }

        io.Fonts->Build();
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

    static inline ImVec4 Acrylic(const ImVec4& base, float opacity = 0.85f)
    {
        return ImVec4(base.x, base.y, base.z, opacity);
    }

    static inline ImVec4 Reveal(const ImVec4& base, float amount = 0.15f)
    {
        return Mix(base, ImVec4(1, 1, 1, base.w), amount);
    }
    
    static inline void DrawShadow(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, 
                                  float rounding, float shadowSize = 8.0f, float opacity = 0.3f)
    {
        const ImU32 shadowCol = ImGui::GetColorU32(ImVec4(0, 0, 0, opacity));
        const int segments = 12;
        
        for (int i = 0; i < segments; ++i)
        {
            float t = (float)i / segments;
            float alpha = (1.0f - t) * opacity;
            float offset = t * shadowSize;
            
            ImU32 col = ImGui::GetColorU32(ImVec4(0, 0, 0, alpha));
            drawList->AddRect(
                ImVec2(min.x - offset, min.y - offset),
                ImVec2(max.x + offset, max.y + offset),
                col, rounding + offset, 0, 1.0f
            );
        }
    }

    static void SetImGuizmoStyleForFluent(const ImVec4& accent, float dpiScale)
    {
        const ImVec4 N00 = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
        const ImVec4 N01 = RGBA(249, 249, 249);
        const ImVec4 N02 = RGBA(243, 243, 243); 
        const ImVec4 N03 = RGBA(237, 237, 237); 
        const ImVec4 N04 = RGBA(230, 230, 230);
        const ImVec4 BRD = RGBA(229, 229, 229, 0.4f); 
        const ImVec4 TXT = RGBA(32, 33, 36); 

        const ImVec4 AX_X = RGBA(232, 17, 35); 
        const ImVec4 AX_Y = RGBA(16, 137, 62);  
        const ImVec4 AX_Z = RGBA(0, 120, 215); 

        ImGuizmo::Style& s = ImGuizmo::GetStyle();

        s.TranslationLineThickness   = 4.0f  * dpiScale;
        s.TranslationLineArrowSize   = 14.0f * dpiScale;
        s.RotationLineThickness      = 4.0f  * dpiScale;
        s.RotationOuterLineThickness = 4.5f  * dpiScale;
        s.ScaleLineThickness         = 4.0f  * dpiScale;
        s.ScaleLineCircleSize        = 10.0f * dpiScale;
        s.HatchedAxisLineThickness   = 3.0f  * dpiScale;
        s.CenterCircleSize           = 6.0f  * dpiScale;

        s.Colors[ImGuizmo::DIRECTION_X] = AX_X;
        s.Colors[ImGuizmo::DIRECTION_Y] = AX_Y;
        s.Colors[ImGuizmo::DIRECTION_Z] = AX_Z;

        s.Colors[ImGuizmo::PLANE_X] = ImVec4(AX_X.x, AX_X.y, AX_X.z, 0.50f);
        s.Colors[ImGuizmo::PLANE_Y] = ImVec4(AX_Y.x, AX_Y.y, AX_Y.z, 0.50f);
        s.Colors[ImGuizmo::PLANE_Z] = ImVec4(AX_Z.x, AX_Z.y, AX_Z.z, 0.50f);

        s.Colors[ImGuizmo::SELECTION] = ImVec4(accent.x, accent.y, accent.z, 1.0f);

        const ImVec4 inactiveBase = Mix(N02, N04, 0.50f);
        s.Colors[ImGuizmo::INACTIVE] = ImVec4(inactiveBase.x, inactiveBase.y, inactiveBase.z, 0.60f);

        const ImVec4 lineColor = Mix(N04, TXT, 0.20f);
        s.Colors[ImGuizmo::TRANSLATION_LINE] = ImVec4(lineColor.x, lineColor.y, lineColor.z, 0.90f);
        s.Colors[ImGuizmo::SCALE_LINE]       = ImVec4(lineColor.x, lineColor.y, lineColor.z, 0.95f);

        s.Colors[ImGuizmo::ROTATION_USING_BORDER] = ImVec4(TXT.x, TXT.y, TXT.z, 0.90f);
        s.Colors[ImGuizmo::ROTATION_USING_FILL]   = ImVec4(accent.x, accent.y, accent.z, 0.15f);

        s.Colors[ImGuizmo::HATCHED_AXIS_LINES] = ImVec4(BRD.x, BRD.y, BRD.z, 0.90f);
        s.Colors[ImGuizmo::TEXT]               = TXT;
        s.Colors[ImGuizmo::TEXT_SHADOW]        = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);  
    }

    void UserInterface::UseColor(const ImVec4& accent) noexcept
    {
        ImGui::StyleColorsLight();
        ImGuiStyle& style = ImGui::GetStyle();

        style.AntiAliasedFill        = true;
        style.AntiAliasedLines       = true;
        style.AntiAliasedLinesUseTex = true;
        style.FontScaleDpi           = 1.2f; 
        
        style.WindowPadding      = ImVec2(12, 12);
        style.FramePadding       = ImVec2(8, 6);
        style.ItemSpacing        = ImVec2(8, 6);
        style.ItemInnerSpacing   = ImVec2(6, 6);
        style.IndentSpacing      = 12.0f;
        style.GrabMinSize        = 16.0f;
        style.TouchExtraPadding  = ImVec2(0, 0);
        
        style.WindowBorderSize   = 1.0f;
        style.ChildBorderSize    = 1.0f;
        style.PopupBorderSize    = 1.0f;
        style.FrameBorderSize    = 0.0f;  
        style.TabBorderSize      = 0.0f;
        style.TabBarBorderSize   = 0.0f;
        
        style.WindowRounding     = 8.0f;  
        style.ChildRounding      = 8.0f;
        style.FrameRounding      = 6.0f;   
        style.PopupRounding      = 8.0f;
        style.ScrollbarRounding  = 10.0f;
        style.GrabRounding       = 6.0f;
        style.TabRounding        = 6.0f;
        
        style.ScrollbarSize      = 14.0f;      
        style.CellPadding        = ImVec2(8, 6);
        style.WindowTitleAlign   = ImVec2(0.5f, 0.5f);  
        style.WindowMenuButtonPosition = ImGuiDir_None;  
        style.Alpha              = 1.0f;
        style.DisabledAlpha      = 0.50f;

        const ImVec4 MICA_BG     = RGBA(249, 249, 249, 0.95f);  
        const ImVec4 CARD_BG     = RGBA(255, 255, 255, 0.90f);  
        const ImVec4 CONTROL_BG  = RGBA(251, 251, 251);         
        const ImVec4 HOVER_BG    = RGBA(246, 246, 246);     
        const ImVec4 ACTIVE_BG   = RGBA(243, 243, 243);        
        const ImVec4 SELECTED_BG = RGBA(240, 240, 240);   

        const ImVec4 TEXT_PRIMARY    = RGBA(32, 33, 36);       
        const ImVec4 TEXT_SECONDARY  = RGBA(96, 94, 92);        
        const ImVec4 TEXT_DISABLED   = RGBA(161, 159, 157);    
        const ImVec4 TEXT_ON_ACCENT  = RGBA(255, 255, 255);   

        const ImVec4 BORDER         = RGBA(229, 229, 229, 0.50f);
        const ImVec4 BORDER_LIGHT   = RGBA(237, 237, 237, 0.30f);
        const ImVec4 DIVIDER        = RGBA(229, 229, 229, 0.60f);

        const ImVec4 ACCENT        = accent;
        const ImVec4 ACCENT_HOVER  = Reveal(accent, 0.12f);
        const ImVec4 ACCENT_ACTIVE = Mix(accent, ImVec4(0, 0, 0, 1), 0.15f);
        const ImVec4 ACCENT_DIM    = Mix(accent, CARD_BG, 0.85f); 

        const ImVec4 SUCCESS = RGBA(16, 137, 62);
        const ImVec4 WARNING = RGBA(255, 185, 0);
        const ImVec4 ERROR_   = RGBA(232, 17, 35);

        const ImVec4 SHADOW  = ImVec4(0.0f, 0.0f, 0.0f, 0.08f);
        const ImVec4 OVERLAY = ImVec4(0.0f, 0.0f, 0.0f, 0.50f);

        
        ImVec4* c = style.Colors;

        c[ImGuiCol_Text]                 = TEXT_PRIMARY;
        c[ImGuiCol_TextDisabled]         = TEXT_DISABLED;
        c[ImGuiCol_TextSelectedBg]       = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.30f);

        c[ImGuiCol_WindowBg]             = MICA_BG;
        c[ImGuiCol_ChildBg]              = CARD_BG;
        c[ImGuiCol_PopupBg]              = Acrylic(CARD_BG, 0.98f);
        c[ImGuiCol_MenuBarBg]            = Acrylic(CONTROL_BG, 0.95f);

        c[ImGuiCol_Border]               = BORDER;
        c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);

        c[ImGuiCol_FrameBg]              = CONTROL_BG;
        c[ImGuiCol_FrameBgHovered]       = HOVER_BG;
        c[ImGuiCol_FrameBgActive]        = ACTIVE_BG;

        c[ImGuiCol_TitleBg]              = CONTROL_BG;
        c[ImGuiCol_TitleBgActive]        = MICA_BG;
        c[ImGuiCol_TitleBgCollapsed]     = Acrylic(CONTROL_BG, 0.80f);

        c[ImGuiCol_ScrollbarBg]          = Acrylic(CONTROL_BG, 0.50f);
        c[ImGuiCol_ScrollbarGrab]        = Mix(CONTROL_BG, TEXT_DISABLED, 0.30f);
        c[ImGuiCol_ScrollbarGrabHovered] = Mix(CONTROL_BG, TEXT_SECONDARY, 0.40f);
        c[ImGuiCol_ScrollbarGrabActive]  = Mix(CONTROL_BG, TEXT_PRIMARY, 0.50f);

        c[ImGuiCol_SliderGrab]           = ACCENT;
        c[ImGuiCol_SliderGrabActive]     = ACCENT_ACTIVE;

        c[ImGuiCol_Button]               = CONTROL_BG;
        c[ImGuiCol_ButtonHovered]        = HOVER_BG;
        c[ImGuiCol_ButtonActive]         = Mix(ACCENT, ACTIVE_BG, 0.80f);

        c[ImGuiCol_Header]               = HOVER_BG;
        c[ImGuiCol_HeaderHovered]        = Mix(ACCENT, HOVER_BG, 0.90f);
        c[ImGuiCol_HeaderActive]         = Mix(ACCENT, ACTIVE_BG, 0.85f);

        c[ImGuiCol_Separator]            = DIVIDER;
        c[ImGuiCol_SeparatorHovered]     = ACCENT_HOVER;
        c[ImGuiCol_SeparatorActive]      = ACCENT_ACTIVE;

        c[ImGuiCol_ResizeGrip]           = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.25f);
        c[ImGuiCol_ResizeGripHovered]    = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.50f);
        c[ImGuiCol_ResizeGripActive]     = ACCENT;

        c[ImGuiCol_Tab]                  = CONTROL_BG;
        c[ImGuiCol_TabHovered]           = HOVER_BG;
        c[ImGuiCol_TabActive]            = CARD_BG;
        c[ImGuiCol_TabUnfocused]         = Mix(CONTROL_BG, MICA_BG, 0.50f);
        c[ImGuiCol_TabUnfocusedActive]   = Mix(CARD_BG, MICA_BG, 0.70f);

        c[ImGuiCol_DockingPreview]       = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.35f);
        c[ImGuiCol_DockingEmptyBg]       = MICA_BG;

        c[ImGuiCol_TableHeaderBg]        = CONTROL_BG;
        c[ImGuiCol_TableBorderStrong]    = BORDER;
        c[ImGuiCol_TableBorderLight]     = BORDER_LIGHT;
        c[ImGuiCol_TableRowBg]           = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_TableRowBgAlt]        = ImVec4(HOVER_BG.x, HOVER_BG.y, HOVER_BG.z, 0.50f);

        c[ImGuiCol_CheckMark]            = ACCENT;
        c[ImGuiCol_DragDropTarget]       = ACCENT;

        c[ImGuiCol_NavHighlight]         = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.80f);
        c[ImGuiCol_NavWindowingHighlight]= ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.30f);
        c[ImGuiCol_NavWindowingDimBg]    = OVERLAY;

        c[ImGuiCol_ModalWindowDimBg]     = OVERLAY;

        c[ImGuiCol_PlotLines]            = ACCENT;
        c[ImGuiCol_PlotLinesHovered]     = ACCENT_HOVER;
        c[ImGuiCol_PlotHistogram]        = Mix(ACCENT, CARD_BG, 0.30f);
        c[ImGuiCol_PlotHistogramHovered] = ACCENT_HOVER;
  
        style.MouseCursorScale = 1.0f;
        SetImGuizmoStyleForFluent(accent, ImGui::GetIO().FontGlobalScale);
    }

    void UserInterface::EnableMicaEffect(WindowHandle windowHandle) noexcept
    {
#ifdef _WIN32
        auto& windowManager = WindowManager::GetInstance();
        std::weak_ptr<IWindow> window = windowManager.GetWindow(windowHandle);
        if (!window.expired())
        {
            auto windowPtr = window.lock();
            
            // Get the native GLFW window and extract Win32 HWND
            GLFWwindow* glfwWindow = (GLFWwindow*)windowPtr->GetNativeWindow();
            if (glfwWindow)
            {
                HWND hwnd = glfwGetWin32Window(glfwWindow);
                if (hwnd)
                {
                    // Enable Windows 11 Mica backdrop material
                    // Note: Requires Windows 11 build 22000 or later
                    typedef enum _DWM_SYSTEMBACKDROP_TYPE {
                        DWMSBT_AUTO = 0,           // Let DWM automatically decide
                        DWMSBT_NONE = 1,           // No backdrop
                        DWMSBT_MAINWINDOW = 2,     // Mica
                        DWMSBT_TRANSIENTWINDOW = 3,// Acrylic
                        DWMSBT_TABBEDWINDOW = 4    // Tabbed Mica
                    } DWM_SYSTEMBACKDROP_TYPE;

                    const DWORD DWMWA_SYSTEMBACKDROP_TYPE = 38;
                    DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_MAINWINDOW; // Mica effect

                    HRESULT hr = DwmSetWindowAttribute(
                        hwnd,
                        DWMWA_SYSTEMBACKDROP_TYPE,
                        &backdropType,
                        sizeof(backdropType)
                    );

                    if (SUCCEEDED(hr))
                    {
                        MOTION_CORE_INFO("Windows 11 Mica effect enabled successfully");
                    }
                    else
                    {
                        MOTION_CORE_WARN("Failed to enable Mica effect. This feature requires Windows 11 build 22000+");
                    }
                }
            }
        }
#else
        MOTION_CORE_WARN("EnableMicaEffect is only supported on Windows 11");
#endif
    }
}