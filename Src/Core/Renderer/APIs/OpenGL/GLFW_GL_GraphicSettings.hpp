#pragma once

#include "GraphicSettings.hpp"

namespace Motion::Core
{
    class GLFW_GL_GraphicSettings : public IGraphic
    {
        public:
            GLFW_GL_GraphicSettings() = default;
            virtual ~GLFW_GL_GraphicSettings() = default;

            virtual void UseSettings(const GraphicSettings& settings) override;
            virtual void GetSystemPreferredSettings() override;
            virtual GraphicSettings& GetSettings() override { return m_Settings; }

        private:
            GraphicSettings m_Settings{};
    };
}

