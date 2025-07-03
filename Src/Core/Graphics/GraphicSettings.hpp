#pragma once

#include <filesystem>

namespace Motion::Core
{
    struct GraphicSettings
    {
        enum class AAType { None, MSAAx2, MSAAx4, MSAAx8 };
        enum class ScalingMode { None, DLSS, FSR2, Bicubic };
        enum class QualityPreset { Low, Medium, High, Ultra, Auto };

        AAType AntiAliasing{AAType::MSAAx4};
        ScalingMode Upscaling{ScalingMode::None};
        uint32_t AnisotropicLevel{4};
        bool Bloom{true};
        bool SSAO{true};
        bool AmbientOcclusion{true};
        bool DepthOfField{false};
        bool MotionBlur{false};

        QualityPreset Preset{QualityPreset::High};

        GraphicSettings() = default;
        ~GraphicSettings() = default;
    };

    class IGraphic
    {
        public:
            IGraphic() = default;
            virtual ~IGraphic() = default;

            virtual void ApplySettings(WindowHandle window) = 0;
            virtual void UsePreset(GraphicSettings::QualityPreset preset) = 0;
            virtual void AutoDetect() = 0;
            virtual GraphicSettings& GetSettings() = 0;
    };

    class GraphicSettingSerializer
    {
        private:
            GraphicSettingSerializer() = default;
            ~GraphicSettingSerializer() = default;

            GraphicSettingSerializer(const GraphicSettingSerializer&) = delete;
            GraphicSettingSerializer& operator=(const GraphicSettingSerializer&) = delete;
            GraphicSettingSerializer(const GraphicSettingSerializer&&) = delete;
            GraphicSettingSerializer&& operator=(const GraphicSettingSerializer&&) = delete;

        public:
            static void Serialize(const GraphicSettings& settings, const std::filesystem::path& filePath);
            static GraphicSettings Deserialize(const std::filesystem::path& filePath);
    };
}