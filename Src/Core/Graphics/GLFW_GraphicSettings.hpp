#pragma once

#include "GraphicSettings.hpp"

namespace Motion::Core
{
    class GLFW_GraphicSettings : public IGraphic
    {
        public:
            GLFW_GraphicSettings() = default;
            virtual ~GLFW_GraphicSettings() = default;

            virtual void ApplySettings(WindowHandle handle) override;
            virtual void UsePreset(GraphicSettings::QualityPreset preset) override;
            virtual void AutoDetect() override;
            virtual GraphicSettings& GetSettings() override { return m_Settings; }

        private:
            GraphicSettings m_Settings{};
    };
}

