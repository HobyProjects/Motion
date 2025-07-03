#pragma once

#include <filesystem>

namespace Motion::Core
{
    struct GraphicSettings
    {
        enum class AntiAliasingLevel : uint32_t { None = 1, MSAAx2 = 2, MSAAx4 = 4, MSAAx8 = 8 };
        enum class ScalingMode { None, DLSS, FSR2, Bicubic };
        enum class QualityPreset { Low, Medium, High, Ultra, Auto };

        AntiAliasingLevel AntiAliasing{AntiAliasingLevel::MSAAx4};
        ScalingMode Upscaling{ScalingMode::None};
        QualityPreset Preset{QualityPreset::High};
        bool Bloom{true};
        bool SSAO{true};
        bool AmbientOcclusion{true};
        bool DepthOfField{false};
        bool MotionBlur{false};
        uint32_t AnisotropicLevel{4};

        bool IsLooksGood{false};

        GraphicSettings() = default;
        ~GraphicSettings() = default;
    };

    class IGraphic
    {
        public:
            IGraphic() = default;
            virtual ~IGraphic() = default;

            virtual void UseSettings(const GraphicSettings& settings) = 0;
            virtual void GetSystemPreferredSettings() = 0;
            virtual GraphicSettings& GetSettings() = 0;
    };

    class GraphicFactory
    {
        public:
            static std::shared_ptr<IGraphic> CreateGraphic(const GraphicSettings& settings);
            static std::shared_ptr<IGraphic> CreateGraphic();
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