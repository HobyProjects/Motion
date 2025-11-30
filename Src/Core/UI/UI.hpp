#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <imgui/imgui.h>
#include <glm/glm.hpp>

#include "Window.hpp"
#include "KeyCodes.hpp"

namespace Motion
{
    class UserInterface
    {
    public:
        static void Init(WindowHandle windowHandle) noexcept;
        static void Quit() noexcept;
        static void EnableMicaEffect(WindowHandle windowHandle) noexcept;

        class FontManager
        {
        public:
            struct FontConfig
            {
                std::string Name;
                std::string FilePath;
                float SizePx;
                bool IsIconFont;
                const ImWchar* GlyphRanges;
            };

            static ImFont* LoadFont(const std::string& name, const std::string& filePath, float sizePx = 16.0f) noexcept;
            static ImFont* LoadFontWithIcons(const std::string& name, const std::string& filePath, float sizePx = 16.0f, bool includeMaterialIcons = true, bool includeFontAwesome = true) noexcept;
            static void MergeIconFont(const std::string& filePath, float sizePx, const ImWchar* glyphRanges) noexcept;
            
            static ImFont* GetFont(const std::string& name) noexcept;
            static ImFont* GetFontOrDefault(const std::string& name) noexcept;
            static ImFont* GetDefaultFont() noexcept;
            
            static void SetAsDefaultFont(ImFont* font) noexcept;
            static void SetAsDefaultFont(const std::string& name) noexcept;
           
            static void BeginFontLoading() noexcept;  
            static void Build() noexcept;          
            
            
            static bool HasFont(const std::string& name) noexcept;
            static void Clear() noexcept;
            static std::vector<std::string> GetFontNames() noexcept;
            
            static inline std::string MaterialIconsPath = "Assets/Fonts/IconFonts/MaterialIcons-Regular.ttf";
            static inline std::string FontAwesomePath   = "Assets/Fonts/IconFonts/fa-regular-400.ttf";

        private:
            static inline std::unordered_map<std::string, ImFont*> s_Fonts;
            static inline ImFont* s_DefaultFont = nullptr;
            static inline ImFont* s_LastLoadedFont = nullptr;
            static inline bool s_FontLoadingStarted = false;
        };

        class ThemeManager
        {
        public:
            struct ColorScheme
            {
                ImVec4 Accent;
                ImVec4 Background;
                ImVec4 Surface;
                ImVec4 Text;
                ImVec4 TextDisabled;
                ImVec4 Border;
                ImVec4 Hover;
                ImVec4 Active;
            };

            static void UseColorScheme(const ColorScheme& scheme) noexcept;
            static void ApplyDarkTheme() noexcept;
            static void ApplyLightTheme() noexcept;
            static void ApplyClassicTheme() noexcept;
            
            static ColorScheme GetDefaultDarkScheme() noexcept;
            static ColorScheme GetDefaultLightScheme() noexcept;
            static ColorScheme GetMaterialDesignScheme() noexcept;
            static ColorScheme GetNeumorphicScheme() noexcept;
            
            static ColorScheme CreateSchemeWithAccent(const ImVec4& accent, bool darkMode = true) noexcept;
            
            static void SetRounding(float rounding) noexcept;
            static void SetSpacing(float spacing) noexcept;
            static void SetPadding(float padding) noexcept;
            
            static ImVec4 GetAccentColor() noexcept;
            static void SetAccentColor(const ImVec4& color) noexcept;

        private:
            static void ApplyBaseStyle() noexcept;
            static ImVec4 AdjustAlpha(const ImVec4& color, float alpha) noexcept;

        private:
            static inline ImVec4 s_CurrentAccent = ImVec4(0.13f, 0.59f, 0.95f, 1.0f);
        };

    private:
        static inline bool s_Initialized;
    };
}