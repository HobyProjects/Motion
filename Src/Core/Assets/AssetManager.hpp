#pragma once

#include <string>
#include <concepts>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <algorithm>

#include "UUID.hpp"
#include "Asset.hpp"

#include "GL_Shaders.hpp"
#include "GL_Texture.hpp"

#include "Material.hpp"
#include "Model.hpp"

namespace Motion
{
    template<>
    struct AssetBackendsBuilder<IShader>
    {
        static std::shared_ptr<IShader> Create(UUID uuid, const std::string& name, const std::filesystem::path& sourceFile)
        {
            switch (Renderer::GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                auto shaderSources = IShader::ReadFullShaderFile(sourceFile);

                if (shaderSources.empty())
                {
                    MOTION_CORE_ERROR("No valid shader sources found in file: {0}", sourceFile.string());
                    return nullptr;
                }

                return std::make_shared<GL_Shader>(uuid, name, shaderSources, sourceFile);
            }
            case RenderingAPI::Vulkan:
            {
                MOTION_CORE_ERROR("Vulkan API is not yet supported for shader creation!");
                return nullptr;
            }
            case RenderingAPI::DirectX:
            {
                MOTION_CORE_ERROR("DirectX API is not yet supported for shader creation!");
                return nullptr;
            }
            default:
            {
                MOTION_CORE_ERROR("Unsupported rendering API for shader creation!");
                return nullptr;
            }
            }
        }

        static std::shared_ptr<IShader> Create(UUID uuid, const std::string& name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
        {
            switch (Renderer::GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                auto shaderSources = IShader::ReadShaderFiles(vertexPath, fragmentPath);

                if (shaderSources.empty())
                {
                    MOTION_CORE_ERROR("No valid shader sources found in files: {0} and {1}", vertexPath.string(), fragmentPath.string());
                    return nullptr;
                }

                return std::make_shared<GL_Shader>(uuid, name, shaderSources, vertexPath);
            }
            case RenderingAPI::Vulkan:
            {
                MOTION_CORE_ERROR("Vulkan API is not yet supported for shader creation!");
                return nullptr;
            }
            case RenderingAPI::DirectX:
            {
                MOTION_CORE_ERROR("DirectX API is not yet supported for shader creation!");
                return nullptr;
            }
            default:
            {
                MOTION_CORE_ERROR("Unsupported rendering API for shader creation!");
                return nullptr;
            }
            }
        }
    };

    template<>
    struct AssetBackendsBuilder<Model>
    {
        static std::shared_ptr<Model> Create(UUID uuid, const std::string& name, const std::filesystem::path& modelFile)
        {
            return std::make_shared<Model>(uuid, name, modelFile);
        }
    };

    template<>
    struct AssetBackendsBuilder<BaseMaterial>
    {
        static std::shared_ptr<BaseMaterial> Create(UUID uuid, const std::string& name, const std::filesystem::path& materialFile)
        {
            return std::make_shared<BaseMaterial>(uuid, name, materialFile);
        }
    };

    class AssetManager
    {
    public:
        AssetManager() = default;
        ~AssetManager() = default;

        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager(AssetManager&&) = delete;
        AssetManager& operator=(AssetManager&&) = delete;


        static AssetManager& GetInstance()
        {
            static AssetManager instance;
            return instance;
        }

        template<AssetExpected T, typename... Args>
        std::shared_ptr<T> Create(const std::string& name, Args&&... args)
        {
            if (Exists(name)) {
                MOTION_CORE_WARN("Asset with name '{}' already exists!", name);
            }

            UUID uuid = UniqueIdentity::GetUniqueID();
            std::shared_ptr<T> asset = AssetBackendsBuilder<T>::Create(uuid, name, std::forward<Args>(args)...);
            if (asset)
                Register(asset->GetUUID(), asset->GetName(), asset);

            return asset;
        }

        template<AssetExpected T>
        std::shared_ptr<T> Get(const std::string& name) const
        {
            auto it = m_NameToUUID.find(name);
            if (it == m_NameToUUID.end()) {
                MOTION_CORE_ERROR("Asset '{}' not found!", name);
                return nullptr;
            }
            return std::dynamic_pointer_cast<T>(m_Assets.at(it->second));
        }

        template<AssetExpected T>
        std::shared_ptr<T> Get(UUID id) const
        {
            auto it = m_Assets.find(id);
            if (it == m_Assets.end()) {
                MOTION_CORE_ERROR("Asset with UUID {} not found!", id);
                return nullptr;
            }
            return std::dynamic_pointer_cast<T>(it->second);
        }

        bool Exists(const std::string& name) const
        {
            return m_NameToUUID.contains(name);
        }

        bool Exists(UUID uuid) const
        {
            return m_Assets.contains(uuid);
        }

        void Clear()
        {
            MOTION_CORE_WARN("Clearing asset manager");
            m_Assets.clear();
            m_NameToUUID.clear();
        }

    private:
        void Register(const UUID& uuid, const std::string& name, std::shared_ptr<IAsset> asset)
        {
            m_Assets[uuid] = std::move(asset);
            m_NameToUUID[name] = uuid;
        }

        std::unordered_map<UUID, std::shared_ptr<IAsset>> m_Assets;
        std::unordered_map<std::string, UUID> m_NameToUUID;
    };
}