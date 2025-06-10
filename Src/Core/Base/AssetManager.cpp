#include "CorePCH.hpp"

namespace Motion::Core
{
    static ShaderType GetShaderTypeFromString(const std::string& typeStr)
    {
        if (typeStr == "vertex") return ShaderType::Vertex;
        if (typeStr == "fragment") return ShaderType::Fragment;
        if (typeStr == "geometry") return ShaderType::Geometry;
        if (typeStr == "compute") return ShaderType::Compute;
        if (typeStr == "tessellation_control") return ShaderType::TessellationControl;
        if (typeStr == "tessellation_evaluation") return ShaderType::TessellationEvaluation;
        return ShaderType::None;
    }

    std::shared_ptr<IShader> AssetManager::CreateShader(const std::string & name, const std::filesystem::path & shaderFile)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:
            {
                if(!std::filesystem::exists(shaderFile))
                {
                    MOTION_ASSERT(false, "Shader file does not exist: {0}", shaderFile.string());
                    return nullptr;
                }

                std::string source = ShaderCompiler::ReadShaderFiles(shaderFile);
                if (source.empty())
                    return nullptr;

                std::unordered_map<ShaderType, std::string> shaderSources;
                std::regex typeRegex(R"(#type\s+(\w+))");
                std::sregex_iterator it(source.begin(), source.end(), typeRegex);
                std::sregex_iterator end;
                std::vector<std::pair<size_t, ShaderType>> shaderPositions;

                for (; it != end; ++it) {
                    std::string typeStr = (*it)[1];
                    ShaderType shaderType = GetShaderTypeFromString(typeStr);
                    if (shaderType == ShaderType::None) {
                        MOTION_ASSERT(false, "Unknown shader type: {0}", typeStr);
                        continue;
                    }
                    shaderPositions.emplace_back(it->position(), shaderType);
                }

                for (size_t i = 0; i < shaderPositions.size(); ++i) {
                    size_t begin = source.find('\n', shaderPositions[i].first) + 1;
                    size_t end = (i + 1 < shaderPositions.size()) ? shaderPositions[i + 1].first : source.size();
                    shaderSources[shaderPositions[i].second] = source.substr(begin, end - begin);
                }

                std::shared_ptr<IShader> shaderAsset = std::make_shared<GL_Shader>(name, shaderSources, shaderFile);
                m_AssetRegistry[shaderAsset->GetMetaData().AssetUUID] = shaderAsset;
                MOTION_CORE_INFO("Shader created: {0} from file {1}", name, shaderFile.string());

                return shaderAsset;
            }
            case RenderingAPI::Vulkan:
            {
                MOTION_ASSERT(false, "Vulkan is not implemented yet!");
                return nullptr;
            }
            case RenderingAPI::DirectX:
            {
                MOTION_ASSERT(false, "DirectX is not implemented yet!");
                return nullptr;
            }
            default:
            {
                MOTION_ASSERT(false, "Unknown rendering API!");
                return nullptr;
            }
        }
    }
}