#include "CorePCH.hpp"
#include "AssetManager.hpp"

namespace Motion::Core
{
    static std::unordered_map<UUID, std::shared_ptr<IAsset>> s_AssetRegistry{};
    static std::unordered_map<std::string, UUID> s_AssetNameUUIDMap{};

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

    std::shared_ptr<IShader> AssetManager::CreateShader(const std::string& name, const std::filesystem::path& shaderFile)
    {
        if(s_AssetRegistry.find(s_AssetNameUUIDMap[name]) != s_AssetRegistry.end())
        {
            MOTION_CORE_WARN("Shader {0} already exists!", name);
            return std::dynamic_pointer_cast<IShader>(s_AssetRegistry[s_AssetNameUUIDMap[name]]);
        }

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

                for (; it != end; ++it) 
                {
                    std::string typeStr = (*it)[1];
                    ShaderType shaderType = GetShaderTypeFromString(typeStr);
                    if (shaderType == ShaderType::None) 
                    {
                        MOTION_ASSERT(false, "Unknown shader type: {0}", typeStr);
                        continue;
                    }
                    shaderPositions.emplace_back(it->position(), shaderType);
                }

                for (size_t i = 0; i < shaderPositions.size(); ++i)
                {
                    size_t begin = source.find('\n', shaderPositions[i].first) + 1;
                    size_t end = (i + 1 < shaderPositions.size()) ? shaderPositions[i + 1].first : source.size();
                    shaderSources[shaderPositions[i].second] = source.substr(begin, end - begin);
                }

                std::shared_ptr<IShader> shaderAsset = std::make_shared<GL_Shader>(name, shaderSources, shaderFile);
                s_AssetRegistry[shaderAsset->GetMetaData().AssetUUID] = shaderAsset;
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


    std::shared_ptr<ITexture> AssetManager::CreateTextureFromFile(const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flipOnLoading)
    {
        if(s_AssetRegistry.find(s_AssetNameUUIDMap[name]) != s_AssetRegistry.end())
        {
            MOTION_CORE_WARN("Texture {0} already exists!", name);
            return std::dynamic_pointer_cast<ITexture>(s_AssetRegistry[s_AssetNameUUIDMap[name]]);
        }

        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         
            {
                std::shared_ptr<ITexture> textureAsset = std::make_shared<GL_Texture>(name, textureFile, type, flipOnLoading);
                if(textureAsset)
                {
                    s_AssetRegistry[textureAsset->GetMetaData().AssetUUID] = textureAsset;
                    MOTION_CORE_INFO("Texture created: {0} from file {1}", name, textureFile.string());
                }
                else
                {
                    MOTION_CORE_ERROR("Failed to create texture: {0}", name);
                    return nullptr;
                }

                return textureAsset;
            }
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        }
    }

    std::shared_ptr<ITexture> AssetManager::CreatePlainTexture(const std::string& name, uint32_t width, uint32_t height)
    {
        if(s_AssetRegistry.find(s_AssetNameUUIDMap[name]) != s_AssetRegistry.end())
        {
            MOTION_CORE_WARN("Texture {0} already exists!", name);
            return std::dynamic_pointer_cast<ITexture>(s_AssetRegistry[s_AssetNameUUIDMap[name]]);
        }

        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         
            {
                std::shared_ptr<ITexture> textureAsset = std::make_shared<GL_Texture>(name, width, height);
                if(textureAsset)
                {
                    s_AssetRegistry[textureAsset->GetMetaData().AssetUUID] = textureAsset;
                    MOTION_CORE_INFO("Texture created: {0}", name);
                }
                else
                {
                    MOTION_CORE_ERROR("Failed to create texture: {0}", name);
                    return nullptr;
                }

                return textureAsset;
            }
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        }
    }

    std::shared_ptr<Model> AssetManager::LoadModel(const std::string& name, std::filesystem::path& modelFile)
    {
        if(s_AssetRegistry.find(s_AssetNameUUIDMap[name]) != s_AssetRegistry.end())
        {
            MOTION_CORE_WARN("Model {0} already exists!", name);
            return std::dynamic_pointer_cast<Model>(s_AssetRegistry[s_AssetNameUUIDMap[name]]);
        }

        std::shared_ptr<Model> modelAsset = Importer::ImportModel(name, modelFile);
        if(modelAsset)
        {
            s_AssetRegistry[modelAsset->GetMetaData().AssetUUID] = modelAsset;
            MOTION_CORE_INFO("Model created: {0}", name);
            return modelAsset;
        }
        else
        {
            MOTION_CORE_ERROR("Failed to create model: {0}", name);
            return nullptr;
        }
    }

    std::shared_ptr<IAsset> AssetManager::GetAsset(const UUID& uuid)
    {
        if(s_AssetRegistry.find(uuid) == s_AssetRegistry.end())
        {
            MOTION_CORE_ERROR("Asset {0} does not exist!", uuid);
            return nullptr;
        }

        return s_AssetRegistry[uuid];
    }

    std::shared_ptr<IAsset> AssetManager::GetAsset(const std::string& name)
    {
        if(s_AssetRegistry.find(s_AssetNameUUIDMap[name]) == s_AssetRegistry.end())
        {
            MOTION_CORE_ERROR("Asset {0} does not exist!", name);
            return nullptr;
        }

        return s_AssetRegistry[s_AssetNameUUIDMap[name]];
    }
    std::shared_ptr<IShader> AssetManager::GetShader(const UUID& uuid)
    {
        if(s_AssetRegistry.find(uuid) == s_AssetRegistry.end())
        {
            MOTION_CORE_ERROR("Shader {0} does not exist!", uuid);
            return nullptr;
        }

        return std::dynamic_pointer_cast<IShader>(s_AssetRegistry[uuid]);
    }

    std::shared_ptr<ITexture> AssetManager::GetTexture(const UUID& uuid)
    {
        if(s_AssetRegistry.find(uuid) == s_AssetRegistry.end())
        {
            MOTION_CORE_ERROR("Texture {0} does not exist!", uuid);
            return nullptr;
        }

        return std::dynamic_pointer_cast<ITexture>(s_AssetRegistry[uuid]);
    }

    std::shared_ptr<Model> AssetManager::GetModel(const UUID& uuid)
    {
        if(s_AssetRegistry.find(uuid) == s_AssetRegistry.end())
        {
            MOTION_CORE_ERROR("Model {0} does not exist!", uuid);
            return nullptr;
        }

        return std::dynamic_pointer_cast<Model>(s_AssetRegistry[uuid]);
    }

    std::shared_ptr<IShader> AssetManager::GetShader(const std::string& name)
    {
        if(s_AssetRegistry.find(s_AssetNameUUIDMap[name]) == s_AssetRegistry.end())
        {
            MOTION_CORE_ERROR("Shader {0} does not exist!", name);
            return nullptr;
        }

        return std::dynamic_pointer_cast<IShader>(s_AssetRegistry[s_AssetNameUUIDMap[name]]);
    }

    std::shared_ptr<ITexture> AssetManager::GetTexture(const std::string& name)
    {
        if(s_AssetRegistry.find(s_AssetNameUUIDMap[name]) == s_AssetRegistry.end())
        {
            MOTION_CORE_ERROR("Texture {0} does not exist!", name);
            return nullptr;
        }

       return std::dynamic_pointer_cast<ITexture>(s_AssetRegistry[s_AssetNameUUIDMap[name]]);
    }

    std::shared_ptr<Model> AssetManager::GetModel(const std::string & name)
    {
        if(s_AssetRegistry.find(s_AssetNameUUIDMap[name]) == s_AssetRegistry.end())
        {
            MOTION_CORE_ERROR("Model {0} does not exist!", name);
            return nullptr;
        }

        return std::dynamic_pointer_cast<Model>(s_AssetRegistry[s_AssetNameUUIDMap[name]]);
    }

    void AssetManager::Clear()
    {
        MOTION_CORE_WARN("Clearing asset registry!");
        s_AssetRegistry.clear();
        s_AssetNameUUIDMap.clear();
    }
}


