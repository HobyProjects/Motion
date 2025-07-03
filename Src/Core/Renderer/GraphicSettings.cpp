#include "CorePCH.hpp"
#include "GraphicSettings.hpp"

namespace Motion::Core
{
    static GraphicSettings::AntiAliasingLevel ParseAAType(const std::string& aaType)
    {
        if (aaType == "None") return GraphicSettings::AntiAliasingLevel::None;
        if (aaType == "MSAAx2") return GraphicSettings::AntiAliasingLevel::MSAAx2;
        if (aaType == "MSAAx4") return GraphicSettings::AntiAliasingLevel::MSAAx4;
        if (aaType == "MSAAx8") return GraphicSettings::AntiAliasingLevel::MSAAx8;
        return GraphicSettings::AntiAliasingLevel::MSAAx4; // Default
    }

    static GraphicSettings::ScalingMode ParseScaling(const std::string& scaling)
    {
        if (scaling == "None") return GraphicSettings::ScalingMode::None;
        if (scaling == "DLSS") return GraphicSettings::ScalingMode::DLSS;
        if (scaling == "FSR2") return GraphicSettings::ScalingMode::FSR2;
        if (scaling == "Bicubic") return GraphicSettings::ScalingMode::Bicubic;
        return GraphicSettings::ScalingMode::None; // Default
    }

    static GraphicSettings::QualityPreset ParsePreset(const std::string& preset)
    {
        if (preset == "Low") return GraphicSettings::QualityPreset::Low;
        if (preset == "Medium") return GraphicSettings::QualityPreset::Medium;
        if (preset == "High") return GraphicSettings::QualityPreset::High;
        if (preset == "Ultra") return GraphicSettings::QualityPreset::Ultra;
        if (preset == "Auto") return GraphicSettings::QualityPreset::Auto;
        return GraphicSettings::QualityPreset::High; // Default
    }

    static std::string ToString(GraphicSettings::AntiAliasingLevel aaType)
    {
        switch (aaType)
        {
            case GraphicSettings::AntiAliasingLevel::None: return "None";
            case GraphicSettings::AntiAliasingLevel::MSAAx2: return "MSAAx2";
            case GraphicSettings::AntiAliasingLevel::MSAAx4: return "MSAAx4";
            case GraphicSettings::AntiAliasingLevel::MSAAx8: return "MSAAx8";
            default: return "MSAAx4"; // Default
        }
    }

    static std::string ToString(GraphicSettings::ScalingMode scaling)
    {
        switch (scaling)
        {
            case GraphicSettings::ScalingMode::None: return "None";
            case GraphicSettings::ScalingMode::DLSS: return "DLSS";
            case GraphicSettings::ScalingMode::FSR2: return "FSR2";
            case GraphicSettings::ScalingMode::Bicubic: return "Bicubic";
            default: return "None"; // Default
        }
    }

    static std::string ToString(GraphicSettings::QualityPreset preset)
    {
        switch (preset)
        {
            case GraphicSettings::QualityPreset::Low: return "Low";
            case GraphicSettings::QualityPreset::Medium: return "Medium";
            case GraphicSettings::QualityPreset::High: return "High";
            case GraphicSettings::QualityPreset::Ultra: return "Ultra";
            case GraphicSettings::QualityPreset::Auto: return "Auto";
            default: return "High"; // Default
        }
    }

    void Serialize(const GraphicSettings& settings, const std::filesystem::path& filePath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "AntiAliasing" << YAML::Value << ToString(settings.AntiAliasing);
        out << YAML::Key << "Upscaling" << YAML::Value << ToString(settings.Upscaling);
        out << YAML::Key << "AnisotropicLevel" << YAML::Value << settings.AnisotropicLevel;
        out << YAML::Key << "Bloom" << YAML::Value << settings.Bloom;
        out << YAML::Key << "SSAO" << YAML::Value << settings.SSAO;
        out << YAML::Key << "AmbientOcclusion" << YAML::Value << settings.AmbientOcclusion;
        out << YAML::Key << "DepthOfField" << YAML::Value << settings.DepthOfField;
        out << YAML::Key << "MotionBlur" << YAML::Value << settings.MotionBlur;
        out << YAML::Key << "Preset" << YAML::Value << ToString(settings.Preset);
        out << YAML::EndMap;

        std::ofstream file(filePath);
        if (file.is_open())
        {
            file << out.c_str();
            file.close();
            MOTION_CORE_INFO("Graphic settings serialized to: {}", filePath.string());
        }
        else
        {
            MOTION_CORE_ERROR("Failed to open file for writing: {}", filePath.string());
        }
    }

    GraphicSettings Deserialize(const std::filesystem::path& filePath)
    {
        GraphicSettings settings;
        if (!std::filesystem::exists(filePath))
        {
            MOTION_CORE_CRITICAL("Graphic settings file does not exist: {}", filePath.string());
            settings.IsLooksGood = false;
            return settings; // Return default settings if file does not exist
        }

        try
        {
            YAML::Node config = YAML::LoadFile(filePath.string());
            settings.AntiAliasing = ParseAAType(config["AntiAliasing"].as<std::string>());
            settings.Upscaling = ParseScaling(config["Upscaling"].as<std::string>());
            settings.AnisotropicLevel = config["AnisotropicLevel"].as<int>();
            settings.Bloom = config["Bloom"].as<bool>();
            settings.SSAO = config["SSAO"].as<bool>();
            settings.AmbientOcclusion = config["AmbientOcclusion"].as<bool>();
            settings.DepthOfField = config["DepthOfField"].as<bool>();
            settings.MotionBlur = config["MotionBlur"].as<bool>();
            settings.Preset = ParsePreset(config["Preset"].as<std::string>());
        }
        catch (const YAML::Exception& e)
        {
            MOTION_CORE_ERROR("Failed to parse graphic settings file: {}. Error: {}", filePath.string(), e.what());
            settings.IsLooksGood = false;
            return settings; // Return default settings if parsing fails
        }

        settings.IsLooksGood = true;
        return settings;
    }

    std::shared_ptr<IGraphic> GraphicFactory::CreateGraphic(const GraphicSettings& settings)
    {
        switch(CoreAPI::API())
        {
            case BaseAPIs::GLFW:
            {
                switch(Renderer::GetAPI())
                {
                    case RenderingAPI::OpenGL:
                    {
                        auto graphic = std::make_shared<GLFW_GL_GraphicSettings>();
                        if(!settings.IsLooksGood)
                        {
                            graphic->GetSystemPreferredSettings();
                            MOTION_CORE_WARN("Graphic settings are not valid, using system preferred settings.");
                            return graphic;
                        }
                        else
                        {
                            MOTION_CORE_INFO("Using provided graphic settings.");
                            graphic->UseSettings(settings);
                            return graphic;
                        }
                    }
                    case RenderingAPI::Vulkan:
                    {
                        MOTION_ASSERT(false, "Vulkan rendering API is not supported yet.");
                        return nullptr;
                    }
                    case RenderingAPI::DirectX:
                    {
                        MOTION_ASSERT(false, "DirectX rendering API is not supported yet.");
                        return nullptr;
                    }
                    default:
                    {
                        MOTION_ASSERT(false, "Unknown rendering API.");
                        return nullptr;
                    }
                }
            }
            case BaseAPIs::Win32:
            {
                MOTION_ASSERT(false, "Win32 is not supported yet.");
                return nullptr;
            }
            default:
            {
                MOTION_ASSERT(false, "Unknown base API.");
                return nullptr;
            }
        }
    }
    std::shared_ptr<IGraphic> GraphicFactory::CreateGraphic()
    {
                switch(CoreAPI::API())
        {
            case BaseAPIs::GLFW:
            {
                switch(Renderer::GetAPI())
                {
                    case RenderingAPI::OpenGL:
                    {
                        auto graphic = std::make_shared<GLFW_GL_GraphicSettings>();
                        MOTION_CORE_INFO("Creating graphic settings with system preferred settings.");
                        graphic->GetSystemPreferredSettings();
                        return graphic;
                    }
                    case RenderingAPI::Vulkan:
                    {
                        MOTION_ASSERT(false, "Vulkan rendering API is not supported yet.");
                        return nullptr;
                    }
                    case RenderingAPI::DirectX:
                    {
                        MOTION_ASSERT(false, "DirectX rendering API is not supported yet.");
                        return nullptr;
                    }
                    default:
                    {
                        MOTION_ASSERT(false, "Unknown rendering API.");
                        return nullptr;
                    }
                }
            }
            case BaseAPIs::Win32:
            {
                MOTION_ASSERT(false, "Win32 is not supported yet.");
                return nullptr;
            }
            default:
            {
                MOTION_ASSERT(false, "Unknown base API.");
                return nullptr;
            }
        }
    }
}

