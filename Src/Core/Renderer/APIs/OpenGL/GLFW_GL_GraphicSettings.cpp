#include "CorePCH.hpp"

namespace Motion::Core
{
    static const char* GetSystemGPU()
    {
        return reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    }

    void GLFW_GL_GraphicSettings::UseSettings(const GraphicSettings& settings)
    {
        m_Settings = settings;
    }

    void GLFW_GL_GraphicSettings::GetSystemPreferredSettings()
    {
        std::string gpu = GetSystemGPU();
        std::transform(gpu.begin(), gpu.end(), gpu.begin(), ::tolower);

        if (gpu.find("intel") != std::string::npos) {
            m_Settings.Preset = GraphicSettings::QualityPreset::Low;
        } else if (gpu.find("gtx 10") != std::string::npos || gpu.find("rx 5") != std::string::npos) {
            m_Settings.Preset = GraphicSettings::QualityPreset::Medium;
        } else if (gpu.find("rtx") != std::string::npos || gpu.find("rx 6") != std::string::npos) {
            m_Settings.Preset = GraphicSettings::QualityPreset::Ultra;
        } else {
            m_Settings.Preset = GraphicSettings::QualityPreset::Medium; // Default fallback
        }

        MOTION_CORE_INFO("Detected GPU: {0} -> Preset: {1}", gpu, static_cast<int>(m_Settings.Preset));

        switch (m_Settings.Preset) 
        {
            case GraphicSettings::QualityPreset::Low:
                m_Settings.AntiAliasing = GraphicSettings::AntiAliasingLevel::None;
                m_Settings.AnisotropicLevel = 1;
                m_Settings.Bloom = false; m_Settings.SSAO = false; m_Settings.DepthOfField = false; m_Settings.MotionBlur = false;
                m_Settings.Upscaling = GraphicSettings::ScalingMode::Bicubic;
                break;

            case GraphicSettings::QualityPreset::Medium:
                m_Settings.AntiAliasing = GraphicSettings::AntiAliasingLevel::MSAAx2;
                m_Settings.AnisotropicLevel = 4;
                m_Settings.Bloom = true; m_Settings.SSAO = true; m_Settings.DepthOfField = false;
                m_Settings.Upscaling = GraphicSettings::ScalingMode::None;
                break;

            case GraphicSettings::QualityPreset::High:
                m_Settings.AntiAliasing = GraphicSettings::AntiAliasingLevel::MSAAx4;
                m_Settings.AnisotropicLevel = 8;
                m_Settings.Bloom = true; m_Settings.SSAO = true; m_Settings.DepthOfField = true;
                m_Settings.Upscaling = GraphicSettings::ScalingMode::None;
                break;

            case GraphicSettings::QualityPreset::Ultra:
                m_Settings.AntiAliasing = GraphicSettings::AntiAliasingLevel::MSAAx8;
                m_Settings.AnisotropicLevel = 16;
                m_Settings.Bloom = true; m_Settings.SSAO = true; m_Settings.DepthOfField = true; m_Settings.MotionBlur = true;
                m_Settings.Upscaling = GraphicSettings::ScalingMode::DLSS;
                break;

            default:
                break;
        }

        if (GL_EXT_texture_filter_anisotropic) {
            GLfloat maxAniso = 0.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            GLfloat targetAniso = std::min((GLfloat)m_Settings.AnisotropicLevel, maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, targetAniso);
        }

        MOTION_CORE_INFO("GraphicsSettings Applied configuration. GPU: {0}", GetSystemGPU());
    }
}