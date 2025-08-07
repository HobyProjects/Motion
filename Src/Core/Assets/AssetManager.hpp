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
        /**
         * @brief Creates a shader object based on the current rendering API.
         *
         * This static function attempts to create a shader object using the specified UUID, name, and source file path.
         * The creation process depends on the rendering API currently in use (e.g., OpenGL, Vulkan, DirectX).
         *
         * @param uuid The unique identifier for the shader.
         * @param name The name to assign to the shader.
         * @param sourceFile The filesystem path to the shader source file.
         * @return std::shared_ptr<IShader> A shared pointer to the created shader instance, or nullptr if creation fails or the API is unsupported.
         *
         * @note Currently, only the OpenGL API is supported. For other APIs, an error is logged and nullptr is returned.
         * @note If the shader source file is invalid or empty, an error is logged and nullptr is returned.
         */
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

        /**
         * @brief Creates a shader object based on the current rendering API.
         *
         * This static function attempts to create a shader object using the specified UUID, name,
         * vertex shader source file path, and fragment shader source file path.
         * The creation process depends on the rendering API currently in use (e.g., OpenGL, Vulkan, DirectX).
         *
         * @param uuid The unique identifier for the shader.
         * @param name The name to assign to the shader.
         * @param vertexPath The filesystem path to the vertex shader source file.
         * @param fragmentPath The filesystem path to the fragment shader source file.
         * @return std::shared_ptr<IShader> A shared pointer to the created shader instance, or nullptr if creation fails or the API is unsupported.
         *
         * @note Currently, only the OpenGL API is supported. For other APIs, an error is logged and nullptr is returned.
         */
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
    struct AssetBackendsBuilder<StaticMesh>
    {
        /**
         * @brief Creates a new StaticMesh instance with a specified UUID and returns a shared pointer to it.
         *
         * This function constructs a StaticMesh object with the given UUID, name, and path to the model file.
         * The StaticMesh is managed by a std::shared_ptr for automatic memory management.
         *
         * @param uuid The unique identifier for the static mesh.
         * @param name The name of the static mesh.
         * @param modelFile The filesystem path to the model file.
         * @return std::shared_ptr<StaticMesh> A shared pointer to the newly created StaticMesh instance.
         */
        static std::shared_ptr<StaticMesh> Create(UUID uuid, const std::string& name, const std::filesystem::path& modelFile)
        {
            return std::make_shared<StaticMesh>(uuid, name, modelFile);
        }
    };

    template<>
    struct AssetBackendsBuilder<PhysicalBasedMaterial>
    {
        /**
         * @brief Creates a new PhysicalBasedMaterial instance with a specified UUID and returns a shared pointer to it.
         *
         * This function constructs a PhysicalBasedMaterial object with the given UUID, name, and path to the material file.
         * The PhysicalBasedMaterial is managed by a std::shared_ptr for automatic memory management.
         *
         * @param uuid The unique identifier for the material.
         * @param name The name of the material.
         * @param materialFile The filesystem path to the material file.
         * @return std::shared_ptr<PhysicalBasedMaterial> A shared pointer to the newly created PhysicalBasedMaterial instance.
         */
        static std::shared_ptr<PhysicalBasedMaterial> Create(UUID uuid, const std::string& name, const std::filesystem::path& materialFile)
        {
            return std::make_shared<PhysicalBasedMaterial>(uuid, name, materialFile);
        }
    };

    template<>
    struct AssetBackendsBuilder<StandardMaterial>
    {
        /**
         * @brief Creates a new StandardMaterial instance with a specified UUID and returns a shared pointer to it.
         *
         * This function constructs a StandardMaterial object with the given UUID, name, and path to the material file.
         * The StandardMaterial is managed by a std::shared_ptr for automatic memory management.
         *
         * @param uuid The unique identifier for the material.
         * @param name The name of the material.
         * @param materialFile The filesystem path to the material file.
         * @return std::shared_ptr<StandardMaterial> A shared pointer to the newly created StandardMaterial instance.
         */
        static std::shared_ptr<StandardMaterial> Create(UUID uuid, const std::string& name, const std::filesystem::path& materialFile)
        {
            return std::make_shared<StandardMaterial>(uuid, name, materialFile);
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


        /**
         * @brief Retrieves the singleton instance of the AssetManager.
         *
         * This static method ensures that only one instance of AssetManager exists
         * throughout the application's lifetime. It provides global access to that instance.
         *
         * @return Reference to the singleton AssetManager instance.
         */
        static AssetManager& GetInstance()
        {
            static AssetManager instance;
            return instance;
        }

        /**
         * @brief Creates a new asset of type T with the specified name and constructor arguments.
         *
         * If an asset with the given name already exists, a warning is logged and the existing asset is returned.
         * Otherwise, a new asset is constructed using the provided arguments, registered, and returned.
         *
         * @tparam T The asset type to create. Must satisfy the AssetExpected concept.
         * @tparam Args Variadic template parameters for the constructor arguments of T.
         * @param name The unique name for the asset.
         * @param args Arguments to forward to the constructor of T.
         * @return std::shared_ptr<T> A shared pointer to the created or existing asset.
         */
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

        /**
         * @brief Retrieves a shared pointer to an asset of the specified type by its name.
         *
         * This function looks up the asset by its name and attempts to cast it to the expected type.
         * If the asset is not found, an error is logged and nullptr is returned.
         *
         * @tparam T The expected asset type, constrained by AssetExpected.
         * @param name The name of the asset to retrieve.
         * @return std::shared_ptr<T> Shared pointer to the asset if found and cast is successful, nullptr otherwise.
         */
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

        /**
         * @brief Retrieves an asset of the specified type associated with the given UUID.
         *
         * This function searches for an asset in the asset manager using the provided UUID.
         * If the asset is found, it attempts to cast it to the requested type T and returns
         * a shared pointer to it. If the asset is not found, an error is logged and nullptr is returned.
         *
         * @tparam T The expected asset type, constrained by AssetExpected.
         * @param id The UUID of the asset to retrieve.
         * @return std::shared_ptr<T> Shared pointer to the asset of type T if found and cast is successful; nullptr otherwise.
         */
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

        /**
         * @brief Checks if an asset with the given name exists in the asset manager.
         *
         * @param name The name of the asset to check for existence.
         * @return true if the asset exists, false otherwise.
         */
        bool Exists(const std::string& name) const
        {
            return m_NameToUUID.contains(name);
        }

        /**
         * @brief Checks if an asset with the specified UUID exists in the asset manager.
         *
         * @param uuid The universally unique identifier (UUID) of the asset to check.
         * @return true if the asset exists; false otherwise.
         */
        bool Exists(UUID uuid) const
        {
            return m_Assets.contains(uuid);
        }

        /**
         * @brief Clears all assets managed by the AssetManager.
         *
         * This function removes all loaded assets and their associated name-to-UUID mappings
         * from the manager, effectively resetting its state. A warning message is logged
         * to indicate that the asset manager is being cleared.
         */
        void Clear()
        {
            MOTION_CORE_WARN("Clearing asset manager");
            m_Assets.clear();
            m_NameToUUID.clear();
        }

    private:
        /**
         * @brief Registers an asset with the given UUID and name.
         *
         * Associates the provided asset with the specified UUID and stores a mapping from the asset's name to its UUID.
         *
         * @param uuid The universally unique identifier for the asset.
         * @param name The name of the asset.
         * @param asset A shared pointer to the asset to be registered.
         */
        void Register(const UUID& uuid, const std::string& name, std::shared_ptr<IAsset> asset)
        {
            m_Assets[uuid] = std::move(asset);
            m_NameToUUID[name] = uuid;
        }

        std::unordered_map<UUID, std::shared_ptr<IAsset>> m_Assets;
        std::unordered_map<std::string, UUID> m_NameToUUID;
    };
}