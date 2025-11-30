#include "CorePCH.hpp"

#ifdef _WIN32
    #include <Windows.h>
    #include <dwmapi.h>
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
#include "UI.hpp"
    #pragma comment(lib, "dwmapi.lib")
#endif

namespace Motion
{
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
    
    static inline ImVec4 Lighten(const ImVec4& color, float amount = 0.1f)
    {
        return ImVec4(
            std::min(1.0f, color.x + amount),
            std::min(1.0f, color.y + amount),
            std::min(1.0f, color.z + amount),
            color.w
        );
    }

    static inline ImVec4 Darken(const ImVec4& color, float amount = 0.1f)
    {
        return ImVec4(
            std::max(0.0f, color.x - amount),
            std::max(0.0f, color.y - amount),
            std::max(0.0f, color.z - amount),
            color.w
        );
    }

    static inline ImVec4 SoftShadow(const ImVec4& base, float alpha = 0.2f)
    {
        ImVec4 dark = Darken(base, 0.15f);
        dark.w = alpha;
        return dark;
    }

    static inline ImVec4 SoftHighlight(const ImVec4& base, float alpha = 0.15f)
    {
        ImVec4 light = Lighten(base, 0.15f);
        light.w = alpha;
        return light;
    }

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

            s_Initialized = true;
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

        FontManager::Clear();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        s_Initialized = false;
    }

    void UserInterface::EnableMicaEffect(WindowHandle windowHandle) noexcept
    {
#ifdef _WIN32
        auto& windowManager = WindowManager::GetInstance();
        std::weak_ptr<IWindow> window = windowManager.GetWindow(windowHandle);
        if (!window.expired())
        {
            auto windowPtr = window.lock();
            
            GLFWwindow* glfwWindow = (GLFWwindow*)windowPtr->GetNativeWindow();
            if (glfwWindow)
            {
                HWND hwnd = glfwGetWin32Window(glfwWindow);
                if (hwnd)
                {
                    typedef enum _DWM_SYSTEMBACKDROP_TYPE {
                        DWMSBT_AUTO = 0,
                        DWMSBT_NONE = 1,
                        DWMSBT_MAINWINDOW = 2,
                        DWMSBT_TRANSIENTWINDOW = 3,
                        DWMSBT_TABBEDWINDOW = 4
                    } DWM_SYSTEMBACKDROP_TYPE;

                    const DWORD DWMWA_SYSTEMBACKDROP_TYPE = 38;
                    DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_MAINWINDOW;

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

    void UserInterface::FontManager::BeginFontLoading() noexcept
    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        s_FontLoadingStarted = true;
        s_Fonts.clear();
        s_DefaultFont = nullptr;
        s_LastLoadedFont = nullptr;
        MOTION_CORE_INFO("Font loading started - font atlas cleared");
    }
    
    ImFont* UserInterface::FontManager::LoadFont(const std::string& name, const std::string& filePath, float sizePx) noexcept
    {
        ImGuiIO& io = ImGui::GetIO();
        
        ImFontConfig fontConfig{};
        fontConfig.OversampleH = 4;
        fontConfig.OversampleV = 4;
        fontConfig.PixelSnapH = false;
        
        ImFont* font = io.Fonts->AddFontFromFileTTF(filePath.c_str(), sizePx, &fontConfig);
        
        if (font)
        {
            s_Fonts[name] = font;
            s_LastLoadedFont = font;
            
            if (s_DefaultFont == nullptr)
            {
                s_DefaultFont = font;
                io.FontDefault = font;
            }
            
            MOTION_CORE_INFO("Font '{0}' loaded successfully from {1} at {2}px", name, filePath, sizePx);
            return font;
        }
        else
        {
            MOTION_CORE_ERROR("Failed to load font '{0}' from {1}", name, filePath);
            return nullptr;
        }
    }
    
    ImFont* UserInterface::FontManager::LoadFontWithIcons(
        const std::string& name, 
        const std::string& filePath, 
        float sizePx,
        bool includeMaterialIcons,
        bool includeFontAwesome) noexcept
    {
        ImGuiIO& io = ImGui::GetIO();
        
        ImFontConfig fontConfig{};
        fontConfig.OversampleH = 4;
        fontConfig.OversampleV = 4;
        fontConfig.PixelSnapH = false;
        
        ImFont* baseFont = io.Fonts->AddFontFromFileTTF(filePath.c_str(), sizePx, &fontConfig);
        
        if (!baseFont)
        {
            MOTION_CORE_ERROR("Failed to load font '{0}' from {1}", name, filePath);
            return nullptr;
        }
        
        // ====================================================================
        // Merge Material Icons
        // ====================================================================
        if (includeMaterialIcons)
        {
            static const ImWchar materialRange[] = { (ImWchar)ICON_MIN_MD, (ImWchar)ICON_MAX_MD, 0 };
            
            ImFontConfig iconCfg{};
            iconCfg.MergeMode = true;           // ✅ CRITICAL
            iconCfg.PixelSnapH = true;
            iconCfg.OversampleH = 2;
            iconCfg.OversampleV = 2;
            iconCfg.GlyphMinAdvanceX = sizePx;
            iconCfg.GlyphMaxAdvanceX = sizePx;
            iconCfg.GlyphOffset = ImVec2(0.0f, 2.0f);  // Vertical adjustment
            
            ImFont* materialFont = io.Fonts->AddFontFromFileTTF(
                MaterialIconsPath.c_str(),
                sizePx,
                &iconCfg,
                materialRange
            );
            
            if (materialFont)
            {
                MOTION_CORE_INFO("  - Material Icons merged (range: 0x{:x}-0x{:x})", ICON_MIN_MD, ICON_MAX_MD);
            }
            else
            {
                MOTION_CORE_WARN("  - Failed to merge Material Icons from {}", MaterialIconsPath);
            }
        }
        

        if (includeFontAwesome)
        {
            static const ImWchar faRange[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
            
            ImFontConfig iconCfg{};
            iconCfg.MergeMode = true;           // ✅ CRITICAL
            iconCfg.PixelSnapH = true;
            iconCfg.OversampleH = 2;
            iconCfg.OversampleV = 2;
            iconCfg.GlyphMinAdvanceX = sizePx;
            iconCfg.GlyphMaxAdvanceX = sizePx;
            iconCfg.GlyphOffset = ImVec2(0.0f, 2.0f);
            
            ImFont* faFont = io.Fonts->AddFontFromFileTTF(
                FontAwesomePath.c_str(),
                sizePx,
                &iconCfg,
                faRange
            );
            
            if (faFont)
            {
                MOTION_CORE_INFO("  - Font Awesome merged (range: 0x{:x}-0x{:x})", ICON_MIN_FA, ICON_MAX_FA);
            }
            else
            {
                MOTION_CORE_WARN("  - Failed to merge Font Awesome from {}", FontAwesomePath);
            }
        }
        
        s_Fonts[name] = baseFont;
        s_LastLoadedFont = baseFont;
        
        if (s_DefaultFont == nullptr)
        {
            s_DefaultFont = baseFont;
            io.FontDefault = baseFont;
        }
        
        MOTION_CORE_INFO("Font '{0}' loaded with icons from {1} at {2}px", name, filePath, sizePx);
        return baseFont;
    }
    
    void UserInterface::FontManager::MergeIconFont(const std::string& filePath, float sizePx, const ImWchar* glyphRanges) noexcept
    {
        if (s_LastLoadedFont == nullptr)
        {
            MOTION_CORE_ERROR("Cannot merge icon font - no regular font loaded yet");
            return;
        }
        
        ImGuiIO& io = ImGui::GetIO();
        
        ImFontConfig iconCfg{};
        iconCfg.MergeMode           = true;      
        iconCfg.PixelSnapH          = true;
        iconCfg.OversampleH         = 2;
        iconCfg.OversampleV         = 2;
        iconCfg.GlyphMinAdvanceX    = sizePx;
        iconCfg.GlyphMaxAdvanceX    = sizePx;
        iconCfg.GlyphOffset         = ImVec2(0.0f, 2.0f);
        
        ImFont* iconFont = io.Fonts->AddFontFromFileTTF(
            filePath.c_str(),
            sizePx,
            &iconCfg,
            glyphRanges
        );
        
        if (iconFont)
        {
            MOTION_CORE_INFO("Icon font merged from {0} into last loaded font", filePath);
        }
        else
        {
            MOTION_CORE_ERROR("Failed to merge icon font from {0}", filePath);
        }
    }
    
    void UserInterface::FontManager::Build() noexcept
    {
        ImGuiIO& io = ImGui::GetIO();
        
        if (!io.Fonts->Build())
        {
            MOTION_CORE_ERROR("Failed to build font atlas!");
            return;
        }
        
        s_FontLoadingStarted = false;
        
        MOTION_CORE_INFO("Font atlas built successfully:");
        MOTION_CORE_INFO("  - Total fonts: {0}", io.Fonts->Fonts.Size);
        MOTION_CORE_INFO("  - Named fonts: {0}", s_Fonts.size());
    }
    
    ImFont* UserInterface::FontManager::GetFont(const std::string& name) noexcept
    {
        auto it = s_Fonts.find(name);
        return (it != s_Fonts.end()) ? it->second : nullptr;
    }
    
    ImFont* UserInterface::FontManager::GetFontOrDefault(const std::string& name) noexcept
    {
        auto font = GetFont(name);
        return font ? font : GetDefaultFont();
    }
    
    ImFont* UserInterface::FontManager::GetDefaultFont() noexcept
    {
        return s_DefaultFont ? s_DefaultFont : ImGui::GetFont();
    }
    
    void UserInterface::FontManager::SetAsDefaultFont(ImFont* font) noexcept
    {
        if (font)
        {
            ImGuiIO& io     = ImGui::GetIO();
            io.FontDefault  = font;
            s_DefaultFont   = font;
        }
    }
    
    void UserInterface::FontManager::SetAsDefaultFont(const std::string& name) noexcept
    {
        ImFont* font = GetFont(name);
        if (font)
        {
            SetAsDefaultFont(font);
        }
        else
        {
            MOTION_CORE_WARN("Cannot set default font - '{0}' not found", name);
        }
    }
    
    bool UserInterface::FontManager::HasFont(const std::string& name) noexcept
    {
        return s_Fonts.find(name) != s_Fonts.end();
    }
    
    void UserInterface::FontManager::Clear() noexcept
    {
        s_Fonts.clear();
        s_DefaultFont = nullptr;
        s_LastLoadedFont = nullptr;
        s_FontLoadingStarted = false;
    }
    
    std::vector<std::string> UserInterface::FontManager::GetFontNames() noexcept
    {
        std::vector<std::string> names;
        names.reserve(s_Fonts.size());
        for (const auto& [name, font] : s_Fonts)
        {
            names.push_back(name);
        }
        return names;
    }

    void UserInterface::ThemeManager::UseColorScheme(const ColorScheme& scheme) noexcept
    {
        s_CurrentAccent = scheme.Accent;
        
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* c = style.Colors;
        ApplyBaseStyle();
        
        const ImVec4 BASE       = scheme.Background;
        const ImVec4 SURFACE    = scheme.Surface;
        const ImVec4 RAISED     = Lighten(SURFACE, 0.05f);
        const ImVec4 PRESSED    = Darken(SURFACE, 0.05f);
        
        const ImVec4 SHADOW_DARK    = SoftShadow(BASE, 0.40f);
        const ImVec4 SHADOW_LIGHT   = SoftHighlight(BASE, 0.15f);
        
        const ImVec4 TEXT_PRIMARY   = scheme.Text;
        const ImVec4 TEXT_SECONDARY = AdjustAlpha(scheme.Text, 0.85f);
        const ImVec4 TEXT_DISABLED  = scheme.TextDisabled;
        
        const ImVec4 ACCENT         = scheme.Accent;
        const ImVec4 ACCENT_LIGHT   = Lighten(ACCENT, 0.15f);
        const ImVec4 ACCENT_DARK    = Darken(ACCENT, 0.15f);
        const ImVec4 ACCENT_SUBTLE  = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.25f);
        
        const ImVec4 OVERLAY = RGBA(0, 0, 0, 0.60f);
        
        // ========================================================================
        // Text Colors
        // ========================================================================
        c[ImGuiCol_Text]                = TEXT_PRIMARY;
        c[ImGuiCol_TextDisabled]        = TEXT_DISABLED;
        c[ImGuiCol_TextSelectedBg]      = Mix(ACCENT_SUBTLE, SURFACE, 0.50f);
        
        // ========================================================================
        // Window Colors
        // ========================================================================
        c[ImGuiCol_WindowBg]            = BASE;
        c[ImGuiCol_ChildBg]             = SURFACE;
        c[ImGuiCol_PopupBg]             = RAISED;
        c[ImGuiCol_Border]              = Mix(BASE, SHADOW_DARK, 0.30f);
        c[ImGuiCol_BorderShadow]        = SHADOW_DARK;
        
        // ========================================================================
        // Frame Colors
        // ========================================================================
        c[ImGuiCol_FrameBg]             = PRESSED;
        c[ImGuiCol_FrameBgHovered]      = Mix(PRESSED, ACCENT_SUBTLE, 0.20f);
        c[ImGuiCol_FrameBgActive]       = Mix(PRESSED, ACCENT_SUBTLE, 0.40f);
        
        // ========================================================================
        // Title Bar Colors
        // ========================================================================
        c[ImGuiCol_TitleBg]             = SURFACE;
        c[ImGuiCol_TitleBgActive]       = RAISED;
        c[ImGuiCol_TitleBgCollapsed]    = Darken(SURFACE, 0.03f);
        
        // ========================================================================
        // Scrollbar Colors
        // ========================================================================
        c[ImGuiCol_ScrollbarBg]         = PRESSED;
        c[ImGuiCol_ScrollbarGrab]       = Mix(SURFACE, TEXT_SECONDARY, 0.25f);
        c[ImGuiCol_ScrollbarGrabHovered]= Mix(SURFACE, TEXT_PRIMARY, 0.35f);
        c[ImGuiCol_ScrollbarGrabActive] = Mix(SURFACE, ACCENT, 0.50f);
        
        // ========================================================================
        // Slider Colors
        // ========================================================================
        c[ImGuiCol_SliderGrab]          = ACCENT;
        c[ImGuiCol_SliderGrabActive]    = ACCENT_DARK;
        
        // ========================================================================
        // Button Colors
        // ========================================================================
        c[ImGuiCol_Button]              = RAISED;
        c[ImGuiCol_ButtonHovered]       = Lighten(RAISED, 0.03f);
        c[ImGuiCol_ButtonActive]        = PRESSED;
        
        // ========================================================================
        // Header Colors
        // ========================================================================
        c[ImGuiCol_Header]              = Mix(SURFACE, ACCENT_SUBTLE, 0.15f);
        c[ImGuiCol_HeaderHovered]       = Mix(SURFACE, ACCENT_SUBTLE, 0.30f);
        c[ImGuiCol_HeaderActive]        = Mix(SURFACE, ACCENT_SUBTLE, 0.50f);
        
        // ========================================================================
        // Separator Colors
        // ========================================================================
        c[ImGuiCol_Separator]           = Mix(BASE, SHADOW_DARK, 0.40f);
        c[ImGuiCol_SeparatorHovered]    = Mix(ACCENT_SUBTLE, SHADOW_DARK, 0.50f);
        c[ImGuiCol_SeparatorActive]     = ACCENT;
        
        // ========================================================================
        // Resize Grip Colors
        // ========================================================================
        c[ImGuiCol_ResizeGrip]          = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.20f);
        c[ImGuiCol_ResizeGripHovered]   = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.45f);
        c[ImGuiCol_ResizeGripActive]    = ACCENT;
        
        // ========================================================================
        // Tab Colors
        // ========================================================================
        c[ImGuiCol_Tab]                 = PRESSED;
        c[ImGuiCol_TabHovered]          = SURFACE;
        c[ImGuiCol_TabActive]           = RAISED;
        c[ImGuiCol_TabUnfocused]        = Darken(SURFACE, 0.03f);
        c[ImGuiCol_TabUnfocusedActive]  = SURFACE;
        
        // ========================================================================
        // Docking Colors
        // ========================================================================
        c[ImGuiCol_DockingPreview]      = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.30f);
        c[ImGuiCol_DockingEmptyBg]      = BASE;
        
        // ========================================================================
        // Table Colors
        // ========================================================================
        c[ImGuiCol_TableHeaderBg]       = RAISED;
        c[ImGuiCol_TableBorderStrong]   = Mix(BASE, SHADOW_DARK, 0.35f);
        c[ImGuiCol_TableBorderLight]    = Mix(BASE, SHADOW_LIGHT, 0.20f);
        c[ImGuiCol_TableRowBg]          = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_TableRowBgAlt]       = ImVec4(SURFACE.x, SURFACE.y, SURFACE.z, 0.30f);
        
        // ========================================================================
        // Interactive Element Colors
        // ========================================================================
        c[ImGuiCol_CheckMark]           = ACCENT;
        c[ImGuiCol_DragDropTarget]      = ACCENT;
        
        // ========================================================================
        // Navigation Colors
        // ========================================================================
        c[ImGuiCol_NavHighlight]        = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.70f);
        c[ImGuiCol_NavWindowingHighlight] = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.40f);
        c[ImGuiCol_NavWindowingDimBg]   = OVERLAY;
        
        // ========================================================================
        // Modal Colors
        // ========================================================================
        c[ImGuiCol_ModalWindowDimBg]    = OVERLAY;
        
        // ========================================================================
        // Plot Colors
        // ========================================================================
        c[ImGuiCol_PlotLines]           = ACCENT;
        c[ImGuiCol_PlotLinesHovered]    = ACCENT_LIGHT;
        c[ImGuiCol_PlotHistogram]       = Mix(ACCENT, SURFACE, 0.40f);
        c[ImGuiCol_PlotHistogramHovered]= ACCENT;
        
        // ========================================================================
        // ImPlot Style Configuration
        // ========================================================================
        ImPlot::GetStyle().LineWeight           = 2.5f;
        ImPlot::GetStyle().MarkerSize           = 5.0f;
        ImPlot::GetStyle().MarkerWeight         = 1.5f;
        ImPlot::GetStyle().FillAlpha            = 0.30f;
        ImPlot::GetStyle().ErrorBarSize         = 6.0f;
        ImPlot::GetStyle().ErrorBarWeight       = 2.0f;
        ImPlot::GetStyle().DigitalBitHeight     = 10.0f;
        ImPlot::GetStyle().DigitalBitGap        = 5.0f;
        ImPlot::GetStyle().PlotBorderSize       = 0.0f;
        ImPlot::GetStyle().MinorAlpha           = 0.30f;
        ImPlot::GetStyle().MajorTickLen         = ImVec2(6.0f, 6.0f);
        ImPlot::GetStyle().MinorTickLen         = ImVec2(3.0f, 3.0f);
        ImPlot::GetStyle().MajorTickSize        = ImVec2(1.5f, 1.5f);
        ImPlot::GetStyle().MinorTickSize        = ImVec2(1.0f, 1.0f);
        ImPlot::GetStyle().MajorGridSize        = ImVec2(1.2f, 1.2f);
        ImPlot::GetStyle().MinorGridSize        = ImVec2(1.0f, 1.0f);
        ImPlot::GetStyle().PlotPadding          = ImVec2(12.0f, 12.0f);
        ImPlot::GetStyle().LabelPadding         = ImVec2(8.0f, 8.0f);
        ImPlot::GetStyle().LegendPadding        = ImVec2(12.0f, 12.0f);
        ImPlot::GetStyle().LegendInnerPadding   = ImVec2(6.0f, 6.0f);
        ImPlot::GetStyle().LegendSpacing        = ImVec2(5.0f, 0.0f);
        ImPlot::GetStyle().MousePosPadding      = ImVec2(12.0f, 12.0f);
        ImPlot::GetStyle().AnnotationPadding    = ImVec2(4.0f, 4.0f);
        ImPlot::GetStyle().FitPadding           = ImVec2(0.0f, 0.0f);
        ImPlot::GetStyle().PlotDefaultSize      = ImVec2(450, 300);
        ImPlot::GetStyle().PlotMinSize          = ImVec2(300, 225);
        
        // ========================================================================
        // ImPlot Colors
        // ========================================================================
        ImVec4* pc = ImPlot::GetStyle().Colors;
        
        pc[ImPlotCol_Line]              = ACCENT;
        pc[ImPlotCol_Fill]              = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.25f);
        pc[ImPlotCol_MarkerOutline]     = ACCENT_DARK;
        pc[ImPlotCol_MarkerFill]        = ACCENT_LIGHT;
        pc[ImPlotCol_ErrorBar]          = Mix(ACCENT, TEXT_SECONDARY, 0.50f);
        pc[ImPlotCol_FrameBg]           = PRESSED;
        pc[ImPlotCol_PlotBg]            = SURFACE;
        pc[ImPlotCol_PlotBorder]        = Mix(BASE, SHADOW_DARK, 0.30f);
        pc[ImPlotCol_LegendBg]          = RAISED;
        pc[ImPlotCol_LegendBorder]      = Mix(BASE, SHADOW_DARK, 0.25f);
        pc[ImPlotCol_LegendText]        = TEXT_PRIMARY;
        pc[ImPlotCol_TitleText]         = TEXT_PRIMARY;
        pc[ImPlotCol_InlayText]         = TEXT_SECONDARY;
        pc[ImPlotCol_AxisText]          = TEXT_SECONDARY;
        pc[ImPlotCol_AxisGrid]          = Mix(BASE, SHADOW_LIGHT, 0.40f);
        pc[ImPlotCol_AxisTick]          = TEXT_SECONDARY;
        pc[ImPlotCol_AxisBg]            = SURFACE;
        pc[ImPlotCol_AxisBgHovered]     = Lighten(SURFACE, 0.02f);
        pc[ImPlotCol_AxisBgActive]      = PRESSED;
        pc[ImPlotCol_Selection]         = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.35f);
        pc[ImPlotCol_Crosshairs]        = Mix(ACCENT, TEXT_PRIMARY, 0.60f);
    }

    void UserInterface::ThemeManager::ApplyDarkTheme() noexcept
    {
        UseColorScheme(GetDefaultDarkScheme());
    }

    void UserInterface::ThemeManager::ApplyLightTheme() noexcept
    {
        UseColorScheme(GetDefaultLightScheme());
    }

    void UserInterface::ThemeManager::ApplyClassicTheme() noexcept
    {
        // Classic theme with a traditional blue accent
        ColorScheme classic{
            ImVec4(0.26f, 0.59f, 0.98f, 1.0f),  // Accent
            RGBA(50, 50, 55),                    // Background
            RGBA(60, 60, 65),                    // Surface
            RGBA(255, 255, 255, 1.0f),          // Text
            RGBA(128, 128, 128, 0.50f),         // TextDisabled
            RGBA(110, 110, 128, 0.80f),         // Border
            RGBA(70, 70, 75, 1.0f),             // Hover
            RGBA(80, 80, 85, 1.0f)              // Active
        };
        UseColorScheme(classic);
    }

    UserInterface::ThemeManager::ColorScheme UserInterface::ThemeManager::CreateSchemeWithAccent(const ImVec4& accent, bool darkMode) noexcept
    {
        if (darkMode)
        {
            return ColorScheme{
                accent,                              // Accent
                RGBA(42, 42, 48),                    // Background
                RGBA(48, 48, 54),                    // Surface
                RGBA(230, 230, 235, 1.0f),          // Text
                RGBA(120, 120, 130, 0.50f),         // TextDisabled
                RGBA(60, 60, 66, 0.80f),            // Border
                RGBA(60, 60, 70, 1.0f),             // Hover
                RGBA(35, 35, 40, 1.0f)              // Active
            };
        }
        else
        {
            return ColorScheme{
                accent,                              // Accent
                RGBA(240, 240, 245),                 // Background
                RGBA(250, 250, 255),                 // Surface
                RGBA(20, 20, 25, 1.0f),             // Text
                RGBA(120, 120, 130, 0.50f),         // TextDisabled
                RGBA(200, 200, 210, 0.80f),         // Border
                RGBA(230, 230, 240, 1.0f),          // Hover
                RGBA(210, 210, 220, 1.0f)           // Active
            };
        }
    }

    UserInterface::ThemeManager::ColorScheme UserInterface::ThemeManager::GetDefaultDarkScheme() noexcept
    {
        return ColorScheme{
            ImVec4(0.13f, 0.59f, 0.95f, 1.0f),  // Accent
            RGBA(42, 42, 48),                     // Background
            RGBA(48, 48, 54),                     // Surface
            RGBA(230, 230, 235, 1.0f),           // Text
            RGBA(120, 120, 130, 0.50f),          // TextDisabled
            RGBA(60, 60, 66, 0.80f),             // Border
            RGBA(60, 60, 70, 1.0f),              // Hover
            RGBA(35, 35, 40, 1.0f)               // Active
        };
    }

    UserInterface::ThemeManager::ColorScheme UserInterface::ThemeManager::GetDefaultLightScheme() noexcept
    {
        return ColorScheme{
            ImVec4(0.10f, 0.48f, 0.82f, 1.0f),  // Accent
            RGBA(240, 240, 245),                  // Background
            RGBA(250, 250, 255),                  // Surface
            RGBA(20, 20, 25, 1.0f),              // Text
            RGBA(120, 120, 130, 0.50f),          // TextDisabled
            RGBA(200, 200, 210, 0.80f),          // Border
            RGBA(230, 230, 240, 1.0f),           // Hover
            RGBA(210, 210, 220, 1.0f)            // Active
        };
    }

    UserInterface::ThemeManager::ColorScheme UserInterface::ThemeManager::GetMaterialDesignScheme() noexcept
    {
        return ColorScheme{
            ImVec4(0.25f, 0.53f, 0.96f, 1.0f),  // Material Blue
            RGBA(33, 33, 33),                     // Background
            RGBA(48, 48, 48),                     // Surface
            RGBA(255, 255, 255, 0.87f),          // Text
            RGBA(255, 255, 255, 0.38f),          // TextDisabled
            RGBA(255, 255, 255, 0.12f),          // Border
            RGBA(66, 66, 66, 1.0f),              // Hover
            RGBA(80, 80, 80, 1.0f)               // Active
        };
    }

    UserInterface::ThemeManager::ColorScheme UserInterface::ThemeManager::GetNeumorphicScheme() noexcept
    {
        return GetDefaultDarkScheme();
    }

    void UserInterface::ThemeManager::SetRounding(float rounding) noexcept
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = rounding;
        style.ChildRounding = rounding;
        style.FrameRounding = rounding;
        style.PopupRounding = rounding;
        style.ScrollbarRounding = rounding;
        style.GrabRounding = rounding;
        style.TabRounding = rounding;
    }

    void UserInterface::ThemeManager::SetSpacing(float spacing) noexcept
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.ItemSpacing = ImVec2(spacing, spacing);
        style.ItemInnerSpacing = ImVec2(spacing * 0.5f, spacing * 0.5f);
    }

    void UserInterface::ThemeManager::SetPadding(float padding) noexcept
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(padding, padding);
        style.FramePadding = ImVec2(padding, padding * 0.5f);
    }

    ImVec4 UserInterface::ThemeManager::GetAccentColor() noexcept
    {
        return s_CurrentAccent;
    }

    void UserInterface::ThemeManager::SetAccentColor(const ImVec4& color) noexcept
    {
        // Determine if current theme is dark or light based on background color
        ImVec4 currentBg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        bool isDark = (currentBg.x + currentBg.y + currentBg.z) / 3.0f < 0.5f;
        
        ColorScheme newScheme = CreateSchemeWithAccent(color, isDark);
        UseColorScheme(newScheme);
    }

    void UserInterface::ThemeManager::ApplyBaseStyle() noexcept
    {
        ImGuiStyle& style = ImGui::GetStyle();
        
        style.WindowRounding = 8.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;
        
        style.WindowBorderSize = 0.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.TabBorderSize = 0.0f;
        
        style.WindowPadding = ImVec2(12.0f, 12.0f);
        style.FramePadding = ImVec2(8.0f, 4.0f);
        style.CellPadding = ImVec2(6.0f, 4.0f);
        style.ItemSpacing = ImVec2(8.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
        style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
        style.IndentSpacing = 22.0f;
        style.ScrollbarSize = 14.0f;
        style.GrabMinSize = 12.0f;
        
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.5f);
        
        style.AntiAliasedLines = true;
        style.AntiAliasedLinesUseTex = true;
        style.AntiAliasedFill = true;
    }

    ImVec4 UserInterface::ThemeManager::AdjustAlpha(const ImVec4& color, float alpha) noexcept
    {
        return ImVec4(color.x, color.y, color.z, alpha);
    }
}