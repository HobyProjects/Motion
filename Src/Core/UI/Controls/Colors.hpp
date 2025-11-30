#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace Colors
{
    // ============================================================================
    // BASIC COLORS
    // ============================================================================
    
    // White and Black
    inline constexpr ImVec4 White           = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // #FFFFFF
    inline constexpr ImVec4 Black           = ImVec4(0.00f, 0.00f, 0.00f, 1.00f); // #000000
    inline constexpr ImVec4 Transparent     = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // #00000000
    
    // Grayscale
    inline constexpr ImVec4 Gray            = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // #808080
    inline constexpr ImVec4 LightGray       = ImVec4(0.83f, 0.83f, 0.83f, 1.00f); // #D3D3D3
    inline constexpr ImVec4 DarkGray        = ImVec4(0.66f, 0.66f, 0.66f, 1.00f); // #A9A9A9
    inline constexpr ImVec4 DimGray         = ImVec4(0.41f, 0.41f, 0.41f, 1.00f); // #696969
    inline constexpr ImVec4 Silver          = ImVec4(0.75f, 0.75f, 0.75f, 1.00f); // #C0C0C0
    inline constexpr ImVec4 Gainsboro       = ImVec4(0.86f, 0.86f, 0.86f, 1.00f); // #DCDCDC
    inline constexpr ImVec4 WhiteSmoke      = ImVec4(0.96f, 0.96f, 0.96f, 1.00f); // #F5F5F5
    
    // ============================================================================
    // RED SPECTRUM
    // ============================================================================
    
    inline constexpr ImVec4 Red             = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); // #FF0000
    inline constexpr ImVec4 DarkRed         = ImVec4(0.55f, 0.00f, 0.00f, 1.00f); // #8B0000
    inline constexpr ImVec4 Crimson         = ImVec4(0.86f, 0.08f, 0.24f, 1.00f); // #DC143C
    inline constexpr ImVec4 Firebrick       = ImVec4(0.70f, 0.13f, 0.13f, 1.00f); // #B22222
    inline constexpr ImVec4 IndianRed       = ImVec4(0.80f, 0.36f, 0.36f, 1.00f); // #CD5C5C
    inline constexpr ImVec4 LightCoral      = ImVec4(0.94f, 0.50f, 0.50f, 1.00f); // #F08080
    inline constexpr ImVec4 Salmon          = ImVec4(0.98f, 0.50f, 0.45f, 1.00f); // #FA8072
    inline constexpr ImVec4 DarkSalmon      = ImVec4(0.91f, 0.59f, 0.48f, 1.00f); // #E9967A
    inline constexpr ImVec4 LightSalmon     = ImVec4(1.00f, 0.63f, 0.48f, 1.00f); // #FFA07A
    
    // ============================================================================
    // PINK SPECTRUM
    // ============================================================================
    
    inline constexpr ImVec4 Pink            = ImVec4(1.00f, 0.75f, 0.80f, 1.00f); // #FFC0CB
    inline constexpr ImVec4 LightPink       = ImVec4(1.00f, 0.71f, 0.76f, 1.00f); // #FFB6C1
    inline constexpr ImVec4 HotPink         = ImVec4(1.00f, 0.41f, 0.71f, 1.00f); // #FF69B4
    inline constexpr ImVec4 DeepPink        = ImVec4(1.00f, 0.08f, 0.58f, 1.00f); // #FF1493
    inline constexpr ImVec4 MediumVioletRed = ImVec4(0.78f, 0.08f, 0.52f, 1.00f); // #C71585
    inline constexpr ImVec4 PaleVioletRed   = ImVec4(0.86f, 0.44f, 0.58f, 1.00f); // #DB7093
    
    // ============================================================================
    // ORANGE SPECTRUM
    // ============================================================================
    
    inline constexpr ImVec4 Orange          = ImVec4(1.00f, 0.65f, 0.00f, 1.00f); // #FFA500
    inline constexpr ImVec4 DarkOrange      = ImVec4(1.00f, 0.55f, 0.00f, 1.00f); // #FF8C00
    inline constexpr ImVec4 Coral           = ImVec4(1.00f, 0.50f, 0.31f, 1.00f); // #FF7F50
    inline constexpr ImVec4 Tomato          = ImVec4(1.00f, 0.39f, 0.28f, 1.00f); // #FF6347
    inline constexpr ImVec4 OrangeRed       = ImVec4(1.00f, 0.27f, 0.00f, 1.00f); // #FF4500
    
    // ============================================================================
    // YELLOW SPECTRUM
    // ============================================================================
    
    inline constexpr ImVec4 Yellow          = ImVec4(1.00f, 1.00f, 0.00f, 1.00f); // #FFFF00
    inline constexpr ImVec4 Gold            = ImVec4(1.00f, 0.84f, 0.00f, 1.00f); // #FFD700
    inline constexpr ImVec4 LightYellow     = ImVec4(1.00f, 1.00f, 0.88f, 1.00f); // #FFFFE0
    inline constexpr ImVec4 LemonChiffon    = ImVec4(1.00f, 0.98f, 0.80f, 1.00f); // #FFFACD
    inline constexpr ImVec4 LightGoldenrod  = ImVec4(0.98f, 0.98f, 0.82f, 1.00f); // #FAFAD2
    inline constexpr ImVec4 PapayaWhip      = ImVec4(1.00f, 0.94f, 0.84f, 1.00f); // #FFEFD5
    inline constexpr ImVec4 Moccasin        = ImVec4(1.00f, 0.89f, 0.71f, 1.00f); // #FFE4B5
    inline constexpr ImVec4 PeachPuff       = ImVec4(1.00f, 0.85f, 0.73f, 1.00f); // #FFDAB9
    inline constexpr ImVec4 PaleGoldenrod   = ImVec4(0.93f, 0.91f, 0.67f, 1.00f); // #EEE8AA
    inline constexpr ImVec4 Khaki           = ImVec4(0.94f, 0.90f, 0.55f, 1.00f); // #F0E68C
    inline constexpr ImVec4 DarkKhaki       = ImVec4(0.74f, 0.72f, 0.42f, 1.00f); // #BDB76B
    
    // ============================================================================
    // BROWN SPECTRUM
    // ============================================================================
    
    inline constexpr ImVec4 Brown           = ImVec4(0.65f, 0.16f, 0.16f, 1.00f); // #A52A2A
    inline constexpr ImVec4 Maroon          = ImVec4(0.50f, 0.00f, 0.00f, 1.00f); // #800000
    inline constexpr ImVec4 SaddleBrown     = ImVec4(0.55f, 0.27f, 0.07f, 1.00f); // #8B4513
    inline constexpr ImVec4 Sienna          = ImVec4(0.63f, 0.32f, 0.18f, 1.00f); // #A0522D
    inline constexpr ImVec4 Chocolate       = ImVec4(0.82f, 0.41f, 0.12f, 1.00f); // #D2691E
    inline constexpr ImVec4 Peru            = ImVec4(0.80f, 0.52f, 0.25f, 1.00f); // #CD853F
    inline constexpr ImVec4 SandyBrown      = ImVec4(0.96f, 0.64f, 0.38f, 1.00f); // #F4A460
    inline constexpr ImVec4 BurlyWood       = ImVec4(0.87f, 0.72f, 0.53f, 1.00f); // #DEB887
    inline constexpr ImVec4 Tan             = ImVec4(0.82f, 0.71f, 0.55f, 1.00f); // #D2B48C
    inline constexpr ImVec4 RosyBrown       = ImVec4(0.74f, 0.56f, 0.56f, 1.00f); // #BC8F8F
    inline constexpr ImVec4 Wheat           = ImVec4(0.96f, 0.87f, 0.70f, 1.00f); // #F5DEB3
    inline constexpr ImVec4 Beige           = ImVec4(0.96f, 0.96f, 0.86f, 1.00f); // #F5F5DC
    
    // ============================================================================
    // GREEN SPECTRUM
    // ============================================================================
    
    inline constexpr ImVec4 Green           = ImVec4(0.00f, 0.50f, 0.00f, 1.00f); // #008000
    inline constexpr ImVec4 Lime            = ImVec4(0.00f, 1.00f, 0.00f, 1.00f); // #00FF00
    inline constexpr ImVec4 DarkGreen       = ImVec4(0.00f, 0.39f, 0.00f, 1.00f); // #006400
    inline constexpr ImVec4 ForestGreen     = ImVec4(0.13f, 0.55f, 0.13f, 1.00f); // #228B22
    inline constexpr ImVec4 LimeGreen       = ImVec4(0.20f, 0.80f, 0.20f, 1.00f); // #32CD32
    inline constexpr ImVec4 PaleGreen       = ImVec4(0.60f, 0.98f, 0.60f, 1.00f); // #98FB98
    inline constexpr ImVec4 LightGreen      = ImVec4(0.56f, 0.93f, 0.56f, 1.00f); // #90EE90
    inline constexpr ImVec4 SpringGreen     = ImVec4(0.00f, 1.00f, 0.50f, 1.00f); // #00FF7F
    inline constexpr ImVec4 MediumSpringGreen = ImVec4(0.00f, 0.98f, 0.60f, 1.00f); // #00FA9A
    inline constexpr ImVec4 SeaGreen        = ImVec4(0.18f, 0.55f, 0.34f, 1.00f); // #2E8B57
    inline constexpr ImVec4 MediumSeaGreen  = ImVec4(0.24f, 0.70f, 0.44f, 1.00f); // #3CB371
    inline constexpr ImVec4 DarkSeaGreen    = ImVec4(0.56f, 0.74f, 0.56f, 1.00f); // #8FBC8F
    inline constexpr ImVec4 YellowGreen     = ImVec4(0.60f, 0.80f, 0.20f, 1.00f); // #9ACD32
    inline constexpr ImVec4 OliveDrab       = ImVec4(0.42f, 0.56f, 0.14f, 1.00f); // #6B8E23
    inline constexpr ImVec4 Olive           = ImVec4(0.50f, 0.50f, 0.00f, 1.00f); // #808000
    inline constexpr ImVec4 DarkOliveGreen  = ImVec4(0.33f, 0.42f, 0.18f, 1.00f); // #556B2F
    inline constexpr ImVec4 LawnGreen       = ImVec4(0.49f, 0.99f, 0.00f, 1.00f); // #7CFC00
    inline constexpr ImVec4 Chartreuse      = ImVec4(0.50f, 1.00f, 0.00f, 1.00f); // #7FFF00
    inline constexpr ImVec4 GreenYellow     = ImVec4(0.68f, 1.00f, 0.18f, 1.00f); // #ADFF2F
    
    // ============================================================================
    // CYAN SPECTRUM
    // ============================================================================
    
    inline constexpr ImVec4 Cyan            = ImVec4(0.00f, 1.00f, 1.00f, 1.00f); // #00FFFF
    inline constexpr ImVec4 Aqua            = ImVec4(0.00f, 1.00f, 1.00f, 1.00f); // #00FFFF
    inline constexpr ImVec4 LightCyan       = ImVec4(0.88f, 1.00f, 1.00f, 1.00f); // #E0FFFF
    inline constexpr ImVec4 PaleTurquoise   = ImVec4(0.69f, 0.93f, 0.93f, 1.00f); // #AFEEEE
    inline constexpr ImVec4 Aquamarine      = ImVec4(0.50f, 1.00f, 0.83f, 1.00f); // #7FFFD4
    inline constexpr ImVec4 Turquoise       = ImVec4(0.25f, 0.88f, 0.82f, 1.00f); // #40E0D0
    inline constexpr ImVec4 MediumTurquoise = ImVec4(0.28f, 0.82f, 0.80f, 1.00f); // #48D1CC
    inline constexpr ImVec4 DarkTurquoise   = ImVec4(0.00f, 0.81f, 0.82f, 1.00f); // #00CED1
    inline constexpr ImVec4 DarkCyan        = ImVec4(0.00f, 0.55f, 0.55f, 1.00f); // #008B8B
    inline constexpr ImVec4 Teal            = ImVec4(0.00f, 0.50f, 0.50f, 1.00f); // #008080
    inline constexpr ImVec4 LightSeaGreen   = ImVec4(0.13f, 0.70f, 0.67f, 1.00f); // #20B2AA
    inline constexpr ImVec4 CadetBlue       = ImVec4(0.37f, 0.62f, 0.63f, 1.00f); // #5F9EA0
    
    // ============================================================================
    // BLUE SPECTRUM
    // ============================================================================
    
    inline constexpr ImVec4 Blue            = ImVec4(0.00f, 0.00f, 1.00f, 1.00f); // #0000FF
    inline constexpr ImVec4 DarkBlue        = ImVec4(0.00f, 0.00f, 0.55f, 1.00f); // #00008B
    inline constexpr ImVec4 MediumBlue      = ImVec4(0.00f, 0.00f, 0.80f, 1.00f); // #0000CD
    inline constexpr ImVec4 Navy            = ImVec4(0.00f, 0.00f, 0.50f, 1.00f); // #000080
    inline constexpr ImVec4 MidnightBlue    = ImVec4(0.10f, 0.10f, 0.44f, 1.00f); // #191970
    inline constexpr ImVec4 RoyalBlue       = ImVec4(0.25f, 0.41f, 0.88f, 1.00f); // #4169E1
    inline constexpr ImVec4 CornflowerBlue  = ImVec4(0.39f, 0.58f, 0.93f, 1.00f); // #6495ED
    inline constexpr ImVec4 LightSteelBlue  = ImVec4(0.69f, 0.77f, 0.87f, 1.00f); // #B0C4DE
    inline constexpr ImVec4 LightSlateGray  = ImVec4(0.47f, 0.53f, 0.60f, 1.00f); // #778899
    inline constexpr ImVec4 SlateGray       = ImVec4(0.44f, 0.50f, 0.56f, 1.00f); // #708090
    inline constexpr ImVec4 DodgerBlue      = ImVec4(0.12f, 0.56f, 1.00f, 1.00f); // #1E90FF
    inline constexpr ImVec4 AliceBlue       = ImVec4(0.94f, 0.97f, 1.00f, 1.00f); // #F0F8FF
    inline constexpr ImVec4 SteelBlue       = ImVec4(0.27f, 0.51f, 0.71f, 1.00f); // #4682B4
    inline constexpr ImVec4 LightSkyBlue    = ImVec4(0.53f, 0.81f, 0.98f, 1.00f); // #87CEFA
    inline constexpr ImVec4 SkyBlue         = ImVec4(0.53f, 0.81f, 0.92f, 1.00f); // #87CEEB
    inline constexpr ImVec4 DeepSkyBlue     = ImVec4(0.00f, 0.75f, 1.00f, 1.00f); // #00BFFF
    inline constexpr ImVec4 LightBlue       = ImVec4(0.68f, 0.85f, 0.90f, 1.00f); // #ADD8E6
    inline constexpr ImVec4 PowderBlue      = ImVec4(0.69f, 0.88f, 0.90f, 1.00f); // #B0E0E6
    
    // ============================================================================
    // PURPLE/VIOLET SPECTRUM
    // ============================================================================
    
    inline constexpr ImVec4 Purple          = ImVec4(0.50f, 0.00f, 0.50f, 1.00f); // #800080
    inline constexpr ImVec4 Indigo          = ImVec4(0.29f, 0.00f, 0.51f, 1.00f); // #4B0082
    inline constexpr ImVec4 DarkMagenta     = ImVec4(0.55f, 0.00f, 0.55f, 1.00f); // #8B008B
    inline constexpr ImVec4 DarkViolet      = ImVec4(0.58f, 0.00f, 0.83f, 1.00f); // #9400D3
    inline constexpr ImVec4 DarkSlateBlue   = ImVec4(0.28f, 0.24f, 0.55f, 1.00f); // #483D8B
    inline constexpr ImVec4 BlueViolet      = ImVec4(0.54f, 0.17f, 0.89f, 1.00f); // #8A2BE2
    inline constexpr ImVec4 DarkOrchid      = ImVec4(0.60f, 0.20f, 0.80f, 1.00f); // #9932CC
    inline constexpr ImVec4 Fuchsia         = ImVec4(1.00f, 0.00f, 1.00f, 1.00f); // #FF00FF
    inline constexpr ImVec4 Magenta         = ImVec4(1.00f, 0.00f, 1.00f, 1.00f); // #FF00FF
    inline constexpr ImVec4 SlateBlue       = ImVec4(0.42f, 0.35f, 0.80f, 1.00f); // #6A5ACD
    inline constexpr ImVec4 MediumSlateBlue = ImVec4(0.48f, 0.41f, 0.93f, 1.00f); // #7B68EE
    inline constexpr ImVec4 MediumOrchid    = ImVec4(0.73f, 0.33f, 0.83f, 1.00f); // #BA55D3
    inline constexpr ImVec4 MediumPurple    = ImVec4(0.58f, 0.44f, 0.86f, 1.00f); // #9370DB
    inline constexpr ImVec4 Orchid          = ImVec4(0.85f, 0.44f, 0.84f, 1.00f); // #DA70D6
    inline constexpr ImVec4 Violet          = ImVec4(0.93f, 0.51f, 0.93f, 1.00f); // #EE82EE
    inline constexpr ImVec4 Plum            = ImVec4(0.87f, 0.63f, 0.87f, 1.00f); // #DDA0DD
    inline constexpr ImVec4 Thistle         = ImVec4(0.85f, 0.75f, 0.85f, 1.00f); // #D8BFD8
    inline constexpr ImVec4 Lavender        = ImVec4(0.90f, 0.90f, 0.98f, 1.00f); // #E6E6FA
    
    // ============================================================================
    // NEUTRAL/EARTH TONES
    // ============================================================================
    
    inline constexpr ImVec4 MistyRose       = ImVec4(1.00f, 0.89f, 0.88f, 1.00f); // #FFE4E1
    inline constexpr ImVec4 AntiqueWhite    = ImVec4(0.98f, 0.92f, 0.84f, 1.00f); // #FAEBD7
    inline constexpr ImVec4 Linen           = ImVec4(0.98f, 0.94f, 0.90f, 1.00f); // #FAF0E6
    inline constexpr ImVec4 Bisque          = ImVec4(1.00f, 0.89f, 0.77f, 1.00f); // #FFE4C4
    inline constexpr ImVec4 BlanchedAlmond  = ImVec4(1.00f, 0.92f, 0.80f, 1.00f); // #FFEBCD
    inline constexpr ImVec4 NavajoWhite     = ImVec4(1.00f, 0.87f, 0.68f, 1.00f); // #FFDEAD
    inline constexpr ImVec4 OldLace         = ImVec4(0.99f, 0.96f, 0.90f, 1.00f); // #FDF5E6
    inline constexpr ImVec4 FloralWhite     = ImVec4(1.00f, 0.98f, 0.94f, 1.00f); // #FFFAF0
    inline constexpr ImVec4 Cornsilk        = ImVec4(1.00f, 0.97f, 0.86f, 1.00f); // #FFF8DC
    inline constexpr ImVec4 Ivory           = ImVec4(1.00f, 1.00f, 0.94f, 1.00f); // #FFFFF0
    inline constexpr ImVec4 Honeydew        = ImVec4(0.94f, 1.00f, 0.94f, 1.00f); // #F0FFF0
    inline constexpr ImVec4 MintCream       = ImVec4(0.96f, 1.00f, 0.98f, 1.00f); // #F5FFFA
    inline constexpr ImVec4 Azure           = ImVec4(0.94f, 1.00f, 1.00f, 1.00f); // #F0FFFF
    inline constexpr ImVec4 Snow            = ImVec4(1.00f, 0.98f, 0.98f, 1.00f); // #FFFAFA
    inline constexpr ImVec4 GhostWhite      = ImVec4(0.97f, 0.97f, 1.00f, 1.00f); // #F8F8FF
    inline constexpr ImVec4 Seashell        = ImVec4(1.00f, 0.96f, 0.93f, 1.00f); // #FFF5EE
    
    // ============================================================================
    // MATERIAL DESIGN COLORS
    // ============================================================================
    
    // Material Red
    inline constexpr ImVec4 MaterialRed50       = ImVec4(1.00f, 0.92f, 0.93f, 1.00f); // #FFEBEE
    inline constexpr ImVec4 MaterialRed100      = ImVec4(1.00f, 0.80f, 0.82f, 1.00f); // #FFCDD2
    inline constexpr ImVec4 MaterialRed200      = ImVec4(0.94f, 0.60f, 0.60f, 1.00f); // #EF9A9A
    inline constexpr ImVec4 MaterialRed300      = ImVec4(0.90f, 0.45f, 0.45f, 1.00f); // #E57373
    inline constexpr ImVec4 MaterialRed400      = ImVec4(0.94f, 0.32f, 0.31f, 1.00f); // #EF5350
    inline constexpr ImVec4 MaterialRed500      = ImVec4(0.96f, 0.26f, 0.21f, 1.00f); // #F44336
    inline constexpr ImVec4 MaterialRed600      = ImVec4(0.90f, 0.22f, 0.21f, 1.00f); // #E53935
    inline constexpr ImVec4 MaterialRed700      = ImVec4(0.83f, 0.18f, 0.18f, 1.00f); // #D32F2F
    inline constexpr ImVec4 MaterialRed800      = ImVec4(0.78f, 0.16f, 0.16f, 1.00f); // #C62828
    inline constexpr ImVec4 MaterialRed900      = ImVec4(0.72f, 0.11f, 0.11f, 1.00f); // #B71C1C
    
    // Material Pink
    inline constexpr ImVec4 MaterialPink50      = ImVec4(0.99f, 0.91f, 0.95f, 1.00f); // #FCE4EC
    inline constexpr ImVec4 MaterialPink100     = ImVec4(0.97f, 0.73f, 0.82f, 1.00f); // #F8BBD0
    inline constexpr ImVec4 MaterialPink200     = ImVec4(0.96f, 0.56f, 0.69f, 1.00f); // #F48FB1
    inline constexpr ImVec4 MaterialPink300     = ImVec4(0.94f, 0.38f, 0.57f, 1.00f); // #F06292
    inline constexpr ImVec4 MaterialPink400     = ImVec4(0.93f, 0.25f, 0.48f, 1.00f); // #EC407A
    inline constexpr ImVec4 MaterialPink500     = ImVec4(0.91f, 0.12f, 0.39f, 1.00f); // #E91E63
    inline constexpr ImVec4 MaterialPink600     = ImVec4(0.85f, 0.11f, 0.38f, 1.00f); // #D81B60
    inline constexpr ImVec4 MaterialPink700     = ImVec4(0.76f, 0.09f, 0.35f, 1.00f); // #C2185B
    inline constexpr ImVec4 MaterialPink800     = ImVec4(0.68f, 0.08f, 0.34f, 1.00f); // #AD1457
    inline constexpr ImVec4 MaterialPink900     = ImVec4(0.53f, 0.05f, 0.31f, 1.00f); // #880E4F
    
    // Material Purple
    inline constexpr ImVec4 MaterialPurple50    = ImVec4(0.95f, 0.90f, 0.96f, 1.00f); // #F3E5F5
    inline constexpr ImVec4 MaterialPurple100   = ImVec4(0.88f, 0.75f, 0.94f, 1.00f); // #E1BEE7
    inline constexpr ImVec4 MaterialPurple200   = ImVec4(0.81f, 0.57f, 0.89f, 1.00f); // #CE93D8
    inline constexpr ImVec4 MaterialPurple300   = ImVec4(0.73f, 0.40f, 0.83f, 1.00f); // #BA68C8
    inline constexpr ImVec4 MaterialPurple400   = ImVec4(0.67f, 0.28f, 0.79f, 1.00f); // #AB47BC
    inline constexpr ImVec4 MaterialPurple500   = ImVec4(0.61f, 0.15f, 0.69f, 1.00f); // #9C27B0
    inline constexpr ImVec4 MaterialPurple600   = ImVec4(0.56f, 0.14f, 0.67f, 1.00f); // #8E24AA
    inline constexpr ImVec4 MaterialPurple700   = ImVec4(0.48f, 0.12f, 0.66f, 1.00f); // #7B1FA2
    inline constexpr ImVec4 MaterialPurple800   = ImVec4(0.42f, 0.11f, 0.66f, 1.00f); // #6A1B9A
    inline constexpr ImVec4 MaterialPurple900   = ImVec4(0.29f, 0.08f, 0.55f, 1.00f); // #4A148C
    
    // Material Deep Purple
    inline constexpr ImVec4 MaterialDeepPurple50  = ImVec4(0.93f, 0.91f, 0.97f, 1.00f); // #EDE7F6
    inline constexpr ImVec4 MaterialDeepPurple100 = ImVec4(0.82f, 0.77f, 0.91f, 1.00f); // #D1C4E9
    inline constexpr ImVec4 MaterialDeepPurple200 = ImVec4(0.70f, 0.61f, 0.86f, 1.00f); // #B39DDB
    inline constexpr ImVec4 MaterialDeepPurple300 = ImVec4(0.60f, 0.46f, 0.81f, 1.00f); // #9575CD
    inline constexpr ImVec4 MaterialDeepPurple400 = ImVec4(0.49f, 0.35f, 0.76f, 1.00f); // #7E57C2
    inline constexpr ImVec4 MaterialDeepPurple500 = ImVec4(0.40f, 0.23f, 0.72f, 1.00f); // #673AB7
    inline constexpr ImVec4 MaterialDeepPurple600 = ImVec4(0.37f, 0.21f, 0.69f, 1.00f); // #5E35B1
    inline constexpr ImVec4 MaterialDeepPurple700 = ImVec4(0.32f, 0.18f, 0.66f, 1.00f); // #512DA8
    inline constexpr ImVec4 MaterialDeepPurple800 = ImVec4(0.28f, 0.16f, 0.64f, 1.00f); // #4527A0
    inline constexpr ImVec4 MaterialDeepPurple900 = ImVec4(0.19f, 0.11f, 0.57f, 1.00f); // #311B92
    
    // Material Indigo
    inline constexpr ImVec4 MaterialIndigo50    = ImVec4(0.91f, 0.92f, 0.97f, 1.00f); // #E8EAF6
    inline constexpr ImVec4 MaterialIndigo100   = ImVec4(0.77f, 0.79f, 0.91f, 1.00f); // #C5CAE9
    inline constexpr ImVec4 MaterialIndigo200   = ImVec4(0.62f, 0.65f, 0.86f, 1.00f); // #9FA8DA
    inline constexpr ImVec4 MaterialIndigo300   = ImVec4(0.48f, 0.51f, 0.80f, 1.00f); // #7986CB
    inline constexpr ImVec4 MaterialIndigo400   = ImVec4(0.36f, 0.42f, 0.75f, 1.00f); // #5C6BC0
    inline constexpr ImVec4 MaterialIndigo500   = ImVec4(0.25f, 0.32f, 0.71f, 1.00f); // #3F51B5
    inline constexpr ImVec4 MaterialIndigo600   = ImVec4(0.24f, 0.30f, 0.68f, 1.00f); // #3949AB
    inline constexpr ImVec4 MaterialIndigo700   = ImVec4(0.20f, 0.25f, 0.64f, 1.00f); // #303F9F
    inline constexpr ImVec4 MaterialIndigo800   = ImVec4(0.17f, 0.21f, 0.62f, 1.00f); // #283593
    inline constexpr ImVec4 MaterialIndigo900   = ImVec4(0.10f, 0.14f, 0.55f, 1.00f); // #1A237E
    
    // Material Blue
    inline constexpr ImVec4 MaterialBlue50      = ImVec4(0.89f, 0.95f, 0.99f, 1.00f); // #E3F2FD
    inline constexpr ImVec4 MaterialBlue100     = ImVec4(0.73f, 0.87f, 0.98f, 1.00f); // #BBDEFB
    inline constexpr ImVec4 MaterialBlue200     = ImVec4(0.56f, 0.79f, 0.98f, 1.00f); // #90CAF9
    inline constexpr ImVec4 MaterialBlue300     = ImVec4(0.39f, 0.71f, 0.96f, 1.00f); // #64B5F6
    inline constexpr ImVec4 MaterialBlue400     = ImVec4(0.25f, 0.65f, 0.96f, 1.00f); // #42A5F5
    inline constexpr ImVec4 MaterialBlue500     = ImVec4(0.13f, 0.59f, 0.95f, 1.00f); // #2196F3
    inline constexpr ImVec4 MaterialBlue600     = ImVec4(0.12f, 0.53f, 0.90f, 1.00f); // #1E88E5
    inline constexpr ImVec4 MaterialBlue700     = ImVec4(0.10f, 0.46f, 0.82f, 1.00f); // #1976D2
    inline constexpr ImVec4 MaterialBlue800     = ImVec4(0.08f, 0.40f, 0.76f, 1.00f); // #1565C0
    inline constexpr ImVec4 MaterialBlue900     = ImVec4(0.05f, 0.28f, 0.63f, 1.00f); // #0D47A1
    
    // Material Light Blue
    inline constexpr ImVec4 MaterialLightBlue50   = ImVec4(0.88f, 0.96f, 0.99f, 1.00f); // #E1F5FE
    inline constexpr ImVec4 MaterialLightBlue100  = ImVec4(0.70f, 0.90f, 0.99f, 1.00f); // #B3E5FC
    inline constexpr ImVec4 MaterialLightBlue200  = ImVec4(0.51f, 0.85f, 0.99f, 1.00f); // #81D4FA
    inline constexpr ImVec4 MaterialLightBlue300  = ImVec4(0.31f, 0.79f, 0.98f, 1.00f); // #4FC3F7
    inline constexpr ImVec4 MaterialLightBlue400  = ImVec4(0.16f, 0.75f, 0.98f, 1.00f); // #29B6F6
    inline constexpr ImVec4 MaterialLightBlue500  = ImVec4(0.01f, 0.71f, 0.96f, 1.00f); // #03A9F4
    inline constexpr ImVec4 MaterialLightBlue600  = ImVec4(0.01f, 0.66f, 0.92f, 1.00f); // #039BE5
    inline constexpr ImVec4 MaterialLightBlue700  = ImVec4(0.01f, 0.61f, 0.88f, 1.00f); // #0288D1
    inline constexpr ImVec4 MaterialLightBlue800  = ImVec4(0.01f, 0.53f, 0.82f, 1.00f); // #0277BD
    inline constexpr ImVec4 MaterialLightBlue900  = ImVec4(0.00f, 0.38f, 0.68f, 1.00f); // #01579B
    
    // Material Cyan
    inline constexpr ImVec4 MaterialCyan50      = ImVec4(0.88f, 0.97f, 0.98f, 1.00f); // #E0F7FA
    inline constexpr ImVec4 MaterialCyan100     = ImVec4(0.70f, 0.92f, 0.95f, 1.00f); // #B2EBF2
    inline constexpr ImVec4 MaterialCyan200     = ImVec4(0.50f, 0.87f, 0.92f, 1.00f); // #80DEEA
    inline constexpr ImVec4 MaterialCyan300     = ImVec4(0.30f, 0.81f, 0.89f, 1.00f); // #4DD0E1
    inline constexpr ImVec4 MaterialCyan400     = ImVec4(0.15f, 0.78f, 0.87f, 1.00f); // #26C6DA
    inline constexpr ImVec4 MaterialCyan500     = ImVec4(0.00f, 0.74f, 0.83f, 1.00f); // #00BCD4
    inline constexpr ImVec4 MaterialCyan600     = ImVec4(0.00f, 0.68f, 0.76f, 1.00f); // #00ACC1
    inline constexpr ImVec4 MaterialCyan700     = ImVec4(0.00f, 0.61f, 0.69f, 1.00f); // #0097A7
    inline constexpr ImVec4 MaterialCyan800     = ImVec4(0.00f, 0.53f, 0.62f, 1.00f); // #00838F
    inline constexpr ImVec4 MaterialCyan900     = ImVec4(0.00f, 0.38f, 0.48f, 1.00f); // #006064
    
    // Material Teal
    inline constexpr ImVec4 MaterialTeal50      = ImVec4(0.88f, 0.95f, 0.95f, 1.00f); // #E0F2F1
    inline constexpr ImVec4 MaterialTeal100     = ImVec4(0.70f, 0.88f, 0.86f, 1.00f); // #B2DFDB
    inline constexpr ImVec4 MaterialTeal200     = ImVec4(0.50f, 0.80f, 0.77f, 1.00f); // #80CBC4
    inline constexpr ImVec4 MaterialTeal300     = ImVec4(0.30f, 0.71f, 0.68f, 1.00f); // #4DB6AC
    inline constexpr ImVec4 MaterialTeal400     = ImVec4(0.15f, 0.66f, 0.62f, 1.00f); // #26A69A
    inline constexpr ImVec4 MaterialTeal500     = ImVec4(0.00f, 0.59f, 0.53f, 1.00f); // #009688
    inline constexpr ImVec4 MaterialTeal600     = ImVec4(0.00f, 0.54f, 0.49f, 1.00f); // #00897B
    inline constexpr ImVec4 MaterialTeal700     = ImVec4(0.00f, 0.48f, 0.45f, 1.00f); // #00796B
    inline constexpr ImVec4 MaterialTeal800     = ImVec4(0.00f, 0.42f, 0.40f, 1.00f); // #00695C
    inline constexpr ImVec4 MaterialTeal900     = ImVec4(0.00f, 0.30f, 0.32f, 1.00f); // #004D40
    
    // Material Green
    inline constexpr ImVec4 MaterialGreen50     = ImVec4(0.90f, 0.96f, 0.91f, 1.00f); // #E8F5E9
    inline constexpr ImVec4 MaterialGreen100    = ImVec4(0.78f, 0.90f, 0.79f, 1.00f); // #C8E6C9
    inline constexpr ImVec4 MaterialGreen200    = ImVec4(0.65f, 0.84f, 0.66f, 1.00f); // #A5D6A7
    inline constexpr ImVec4 MaterialGreen300    = ImVec4(0.51f, 0.78f, 0.53f, 1.00f); // #81C784
    inline constexpr ImVec4 MaterialGreen400    = ImVec4(0.40f, 0.73f, 0.42f, 1.00f); // #66BB6A
    inline constexpr ImVec4 MaterialGreen500    = ImVec4(0.30f, 0.69f, 0.31f, 1.00f); // #4CAF50
    inline constexpr ImVec4 MaterialGreen600    = ImVec4(0.27f, 0.63f, 0.28f, 1.00f); // #43A047
    inline constexpr ImVec4 MaterialGreen700    = ImVec4(0.24f, 0.57f, 0.25f, 1.00f); // #388E3C
    inline constexpr ImVec4 MaterialGreen800    = ImVec4(0.20f, 0.50f, 0.22f, 1.00f); // #2E7D32
    inline constexpr ImVec4 MaterialGreen900    = ImVec4(0.15f, 0.36f, 0.15f, 1.00f); // #1B5E20
    
    // Material Light Green
    inline constexpr ImVec4 MaterialLightGreen50  = ImVec4(0.95f, 0.98f, 0.91f, 1.00f); // #F1F8E9
    inline constexpr ImVec4 MaterialLightGreen100 = ImVec4(0.87f, 0.95f, 0.79f, 1.00f); // #DCEDC8
    inline constexpr ImVec4 MaterialLightGreen200 = ImVec4(0.78f, 0.90f, 0.66f, 1.00f); // #C5E1A5
    inline constexpr ImVec4 MaterialLightGreen300 = ImVec4(0.69f, 0.85f, 0.53f, 1.00f); // #AED581
    inline constexpr ImVec4 MaterialLightGreen400 = ImVec4(0.61f, 0.80f, 0.40f, 1.00f); // #9CCC65
    inline constexpr ImVec4 MaterialLightGreen500 = ImVec4(0.55f, 0.76f, 0.29f, 1.00f); // #8BC34A
    inline constexpr ImVec4 MaterialLightGreen600 = ImVec4(0.49f, 0.70f, 0.26f, 1.00f); // #7CB342
    inline constexpr ImVec4 MaterialLightGreen700 = ImVec4(0.41f, 0.62f, 0.22f, 1.00f); // #689F38
    inline constexpr ImVec4 MaterialLightGreen800 = ImVec4(0.33f, 0.55f, 0.18f, 1.00f); // #558B2F
    inline constexpr ImVec4 MaterialLightGreen900 = ImVec4(0.20f, 0.41f, 0.12f, 1.00f); // #33691E
    
    // Material Lime
    inline constexpr ImVec4 MaterialLime50      = ImVec4(0.98f, 0.98f, 0.91f, 1.00f); // #F9FBE7
    inline constexpr ImVec4 MaterialLime100     = ImVec4(0.94f, 0.96f, 0.79f, 1.00f); // #F0F4C3
    inline constexpr ImVec4 MaterialLime200     = ImVec4(0.90f, 0.94f, 0.65f, 1.00f); // #E6EE9C
    inline constexpr ImVec4 MaterialLime300     = ImVec4(0.86f, 0.92f, 0.51f, 1.00f); // #DCE775
    inline constexpr ImVec4 MaterialLime400     = ImVec4(0.83f, 0.88f, 0.38f, 1.00f); // #D4E157
    inline constexpr ImVec4 MaterialLime500     = ImVec4(0.80f, 0.86f, 0.22f, 1.00f); // #CDDC39
    inline constexpr ImVec4 MaterialLime600     = ImVec4(0.75f, 0.79f, 0.20f, 1.00f); // #C0CA33
    inline constexpr ImVec4 MaterialLime700     = ImVec4(0.69f, 0.71f, 0.17f, 1.00f); // #AFB42B
    inline constexpr ImVec4 MaterialLime800     = ImVec4(0.62f, 0.64f, 0.15f, 1.00f); // #9E9D24
    inline constexpr ImVec4 MaterialLime900     = ImVec4(0.51f, 0.47f, 0.09f, 1.00f); // #827717
    
    // Material Yellow
    inline constexpr ImVec4 MaterialYellow50    = ImVec4(1.00f, 0.99f, 0.91f, 1.00f); // #FFFDE7
    inline constexpr ImVec4 MaterialYellow100   = ImVec4(1.00f, 0.98f, 0.80f, 1.00f); // #FFF9C4
    inline constexpr ImVec4 MaterialYellow200   = ImVec4(1.00f, 0.96f, 0.67f, 1.00f); // #FFF59D
    inline constexpr ImVec4 MaterialYellow300   = ImVec4(1.00f, 0.95f, 0.53f, 1.00f); // #FFF176
    inline constexpr ImVec4 MaterialYellow400   = ImVec4(1.00f, 0.93f, 0.38f, 1.00f); // #FFEE58
    inline constexpr ImVec4 MaterialYellow500   = ImVec4(1.00f, 0.92f, 0.23f, 1.00f); // #FFEB3B
    inline constexpr ImVec4 MaterialYellow600   = ImVec4(0.99f, 0.87f, 0.21f, 1.00f); // #FDD835
    inline constexpr ImVec4 MaterialYellow700   = ImVec4(0.98f, 0.81f, 0.18f, 1.00f); // #FBC02D
    inline constexpr ImVec4 MaterialYellow800   = ImVec4(0.97f, 0.75f, 0.16f, 1.00f); // #F9A825
    inline constexpr ImVec4 MaterialYellow900   = ImVec4(0.96f, 0.65f, 0.14f, 1.00f); // #F57F17
    
    // Material Amber
    inline constexpr ImVec4 MaterialAmber50     = ImVec4(1.00f, 0.97f, 0.88f, 1.00f); // #FFF8E1
    inline constexpr ImVec4 MaterialAmber100    = ImVec4(1.00f, 0.93f, 0.70f, 1.00f); // #FFECB3
    inline constexpr ImVec4 MaterialAmber200    = ImVec4(1.00f, 0.88f, 0.51f, 1.00f); // #FFE082
    inline constexpr ImVec4 MaterialAmber300    = ImVec4(1.00f, 0.84f, 0.31f, 1.00f); // #FFD54F
    inline constexpr ImVec4 MaterialAmber400    = ImVec4(1.00f, 0.79f, 0.16f, 1.00f); // #FFCA28
    inline constexpr ImVec4 MaterialAmber500    = ImVec4(1.00f, 0.76f, 0.03f, 1.00f); // #FFC107
    inline constexpr ImVec4 MaterialAmber600    = ImVec4(1.00f, 0.70f, 0.00f, 1.00f); // #FFB300
    inline constexpr ImVec4 MaterialAmber700    = ImVec4(1.00f, 0.65f, 0.00f, 1.00f); // #FFA000
    inline constexpr ImVec4 MaterialAmber800    = ImVec4(1.00f, 0.56f, 0.00f, 1.00f); // #FF8F00
    inline constexpr ImVec4 MaterialAmber900    = ImVec4(1.00f, 0.44f, 0.00f, 1.00f); // #FF6F00
    
    // Material Orange
    inline constexpr ImVec4 MaterialOrange50    = ImVec4(1.00f, 0.95f, 0.88f, 1.00f); // #FFF3E0
    inline constexpr ImVec4 MaterialOrange100   = ImVec4(1.00f, 0.88f, 0.70f, 1.00f); // #FFE0B2
    inline constexpr ImVec4 MaterialOrange200   = ImVec4(1.00f, 0.80f, 0.50f, 1.00f); // #FFCC80
    inline constexpr ImVec4 MaterialOrange300   = ImVec4(1.00f, 0.72f, 0.30f, 1.00f); // #FFB74D
    inline constexpr ImVec4 MaterialOrange400   = ImVec4(1.00f, 0.65f, 0.15f, 1.00f); // #FFA726
    inline constexpr ImVec4 MaterialOrange500   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f); // #FF9800
    inline constexpr ImVec4 MaterialOrange600   = ImVec4(0.98f, 0.55f, 0.00f, 1.00f); // #FB8C00
    inline constexpr ImVec4 MaterialOrange700   = ImVec4(0.96f, 0.49f, 0.00f, 1.00f); // #F57C00
    inline constexpr ImVec4 MaterialOrange800   = ImVec4(0.94f, 0.43f, 0.00f, 1.00f); // #EF6C00
    inline constexpr ImVec4 MaterialOrange900   = ImVec4(0.90f, 0.32f, 0.00f, 1.00f); // #E65100
    
    // Material Deep Orange
    inline constexpr ImVec4 MaterialDeepOrange50  = ImVec4(0.98f, 0.91f, 0.88f, 1.00f); // #FBE9E7
    inline constexpr ImVec4 MaterialDeepOrange100 = ImVec4(1.00f, 0.80f, 0.74f, 1.00f); // #FFCCBC
    inline constexpr ImVec4 MaterialDeepOrange200 = ImVec4(1.00f, 0.67f, 0.57f, 1.00f); // #FFAB91
    inline constexpr ImVec4 MaterialDeepOrange300 = ImVec4(1.00f, 0.54f, 0.40f, 1.00f); // #FF8A65
    inline constexpr ImVec4 MaterialDeepOrange400 = ImVec4(1.00f, 0.44f, 0.26f, 1.00f); // #FF7043
    inline constexpr ImVec4 MaterialDeepOrange500 = ImVec4(1.00f, 0.34f, 0.13f, 1.00f); // #FF5722
    inline constexpr ImVec4 MaterialDeepOrange600 = ImVec4(0.96f, 0.32f, 0.11f, 1.00f); // #F4511E
    inline constexpr ImVec4 MaterialDeepOrange700 = ImVec4(0.90f, 0.29f, 0.10f, 1.00f); // #E64A19
    inline constexpr ImVec4 MaterialDeepOrange800 = ImVec4(0.84f, 0.26f, 0.09f, 1.00f); // #D84315
    inline constexpr ImVec4 MaterialDeepOrange900 = ImVec4(0.75f, 0.21f, 0.05f, 1.00f); // #BF360C
    
    // Material Brown
    inline constexpr ImVec4 MaterialBrown50     = ImVec4(0.94f, 0.93f, 0.91f, 1.00f); // #EFEBE9
    inline constexpr ImVec4 MaterialBrown100    = ImVec4(0.84f, 0.80f, 0.78f, 1.00f); // #D7CCC8
    inline constexpr ImVec4 MaterialBrown200    = ImVec4(0.74f, 0.67f, 0.64f, 1.00f); // #BCAAA4
    inline constexpr ImVec4 MaterialBrown300    = ImVec4(0.64f, 0.54f, 0.51f, 1.00f); // #A1887F
    inline constexpr ImVec4 MaterialBrown400    = ImVec4(0.55f, 0.43f, 0.40f, 1.00f); // #8D6E63
    inline constexpr ImVec4 MaterialBrown500    = ImVec4(0.47f, 0.33f, 0.28f, 1.00f); // #795548
    inline constexpr ImVec4 MaterialBrown600    = ImVec4(0.43f, 0.30f, 0.25f, 1.00f); // #6D4C41
    inline constexpr ImVec4 MaterialBrown700    = ImVec4(0.38f, 0.26f, 0.22f, 1.00f); // #5D4037
    inline constexpr ImVec4 MaterialBrown800    = ImVec4(0.31f, 0.22f, 0.18f, 1.00f); // #4E342E
    inline constexpr ImVec4 MaterialBrown900    = ImVec4(0.24f, 0.15f, 0.14f, 1.00f); // #3E2723
    
    // Material Grey
    inline constexpr ImVec4 MaterialGrey50      = ImVec4(0.98f, 0.98f, 0.98f, 1.00f); // #FAFAFA
    inline constexpr ImVec4 MaterialGrey100     = ImVec4(0.96f, 0.96f, 0.96f, 1.00f); // #F5F5F5
    inline constexpr ImVec4 MaterialGrey200     = ImVec4(0.93f, 0.93f, 0.93f, 1.00f); // #EEEEEE
    inline constexpr ImVec4 MaterialGrey300     = ImVec4(0.88f, 0.88f, 0.88f, 1.00f); // #E0E0E0
    inline constexpr ImVec4 MaterialGrey400     = ImVec4(0.74f, 0.74f, 0.74f, 1.00f); // #BDBDBD
    inline constexpr ImVec4 MaterialGrey500     = ImVec4(0.62f, 0.62f, 0.62f, 1.00f); // #9E9E9E
    inline constexpr ImVec4 MaterialGrey600     = ImVec4(0.46f, 0.46f, 0.46f, 1.00f); // #757575
    inline constexpr ImVec4 MaterialGrey700     = ImVec4(0.38f, 0.38f, 0.38f, 1.00f); // #616161
    inline constexpr ImVec4 MaterialGrey800     = ImVec4(0.26f, 0.26f, 0.26f, 1.00f); // #424242
    inline constexpr ImVec4 MaterialGrey900     = ImVec4(0.13f, 0.13f, 0.13f, 1.00f); // #212121
    
    // Material Blue Grey
    inline constexpr ImVec4 MaterialBlueGrey50  = ImVec4(0.93f, 0.95f, 0.96f, 1.00f); // #ECEFF1
    inline constexpr ImVec4 MaterialBlueGrey100 = ImVec4(0.81f, 0.85f, 0.86f, 1.00f); // #CFD8DC
    inline constexpr ImVec4 MaterialBlueGrey200 = ImVec4(0.69f, 0.75f, 0.77f, 1.00f); // #B0BEC5
    inline constexpr ImVec4 MaterialBlueGrey300 = ImVec4(0.56f, 0.64f, 0.68f, 1.00f); // #90A4AE
    inline constexpr ImVec4 MaterialBlueGrey400 = ImVec4(0.47f, 0.56f, 0.61f, 1.00f); // #78909C
    inline constexpr ImVec4 MaterialBlueGrey500 = ImVec4(0.38f, 0.49f, 0.55f, 1.00f); // #607D8B
    inline constexpr ImVec4 MaterialBlueGrey600 = ImVec4(0.33f, 0.43f, 0.48f, 1.00f); // #546E7A
    inline constexpr ImVec4 MaterialBlueGrey700 = ImVec4(0.27f, 0.35f, 0.39f, 1.00f); // #455A64
    inline constexpr ImVec4 MaterialBlueGrey800 = ImVec4(0.22f, 0.28f, 0.31f, 1.00f); // #37474F
    inline constexpr ImVec4 MaterialBlueGrey900 = ImVec4(0.15f, 0.20f, 0.22f, 1.00f); // #263238

}