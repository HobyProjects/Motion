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

namespace Motion::Core
{
    template<>
    struct AssetBackendsBuilder<IShader>
    {
        /**
         * @brief Creates a shader instance based on the current rendering API.
         *
         * This static function attempts to create a shader object using the specified shader name and source file path.
         * The creation process depends on the rendering API currently in use (e.g., OpenGL, Vulkan, DirectX).
         *
         * @param name The name to assign to the shader.
         * @param sourceFile The filesystem path to the shader source file.
         * @return std::shared_ptr<IShader> A shared pointer to the created shader instance, or nullptr if creation fails or the API is unsupported.
         *
         * @note Currently, only the OpenGL API is supported. For other APIs, an error is logged and nullptr is returned.
         * @note If the shader source file is invalid or empty, an error is logged and nullptr is returned.
         */
        static std::shared_ptr<IShader> Create(const std::string& name, const std::filesystem::path& sourceFile)
        {
            auto& renderer = Renderer::GetInstance();
            switch (renderer.GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                auto shaderSources = ShaderBuilder::ReadFullShaderFile(sourceFile);
                if (shaderSources.empty())
                {
                    MOTION_CORE_ERROR("No valid shader sources found in file: {0}", sourceFile.string());
                    return nullptr;
                }

                UUID uuid = UniqueIdentity::GetUniqueID();
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
        static std::shared_ptr<IShader> Create(const UUID& uuid, const std::string& name, const std::filesystem::path& sourceFile)
        {
            auto& renderer = Renderer::GetInstance();
            switch (renderer.GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                auto shaderSources = ShaderBuilder::ReadFullShaderFile(sourceFile);
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
         * This static function attempts to create and return a shared pointer to an IShader implementation,
         * using the provided shader name and file paths for the vertex and fragment shader source files.
         * The function supports different rendering APIs (OpenGL, Vulkan, DirectX), but currently only
         * OpenGL is implemented. For unsupported APIs, an error is logged and nullptr is returned.
         *
         * @param name The name to assign to the shader.
         * @param vertexPath The filesystem path to the vertex shader source file.
         * @param fragmentPath The filesystem path to the fragment shader source file.
         * @return std::shared_ptr<IShader> A shared pointer to the created shader object, or nullptr if creation failed or the API is unsupported.
         */
        static std::shared_ptr<IShader> Create(const std::string& name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
        {
            auto& renderer = Renderer::GetInstance();
            switch (renderer.GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                auto shaderSources = ShaderBuilder::ReadShaderFiles(vertexPath, fragmentPath);
                if (shaderSources.empty())
                {
                    MOTION_CORE_ERROR("No valid shader sources found in files: {0} and {1}", vertexPath.string(), fragmentPath.string());
                    return nullptr;
                }

                UUID uuid = UniqueIdentity::GetUniqueID();
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
        static std::shared_ptr<IShader> Create(const UUID& uuid, const std::string& name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
        {
            auto& renderer = Renderer::GetInstance();
            switch (renderer.GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                auto shaderSources = ShaderBuilder::ReadShaderFiles(vertexPath, fragmentPath);
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
    struct AssetBackendsBuilder<ITexture>
    {
        /**
         * @brief Creates a texture object based on the current rendering API.
         *
         * This static function instantiates a texture object with the specified name, width, and height.
         * The type of texture created depends on the active rendering API (OpenGL, Vulkan, or DirectX).
         * Currently, only OpenGL is supported; Vulkan and DirectX will log an error and return nullptr.
         *
         * @param name The name to assign to the texture.
         * @param width The width of the texture in pixels.
         * @param height The height of the texture in pixels.
         * @return std::shared_ptr<ITexture> A shared pointer to the created texture object, or nullptr if the API is unsupported.
         */
        static std::shared_ptr<ITexture> Create(const std::string& name, std::uint32_t width, std::uint32_t height)
        {
            auto& renderer = Renderer::GetInstance();
            switch (renderer.GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                UUID uuid = UniqueIdentity::GetUniqueID();
                return std::make_shared<GL_Texture>(uuid, name, width, height);
            }
            case RenderingAPI::Vulkan:
            {
                MOTION_CORE_ERROR("Vulkan API is not yet supported for texture creation!");
                return nullptr;
            }
            case RenderingAPI::DirectX:
            {
                MOTION_CORE_ERROR("DirectX API is not yet supported for texture creation!");
                return nullptr;
            }
            default:
            {
                MOTION_CORE_ERROR("Unsupported rendering API for texture creation!");
                return nullptr;
            }
            }
        }

        /**
         * @brief Creates a texture object based on the current rendering API.
         *
         * This static function instantiates a texture object with the specified UUID, name, width, and height.
         * The type of texture created depends on the active rendering API (OpenGL, Vulkan, or DirectX).
         * Currently, only OpenGL is supported; Vulkan and DirectX will log an error and return nullptr.
         *
         * @param uuid The unique identifier for the texture.
         * @param name The name to assign to the texture.
         * @param width The width of the texture in pixels.
         * @param height The height of the texture in pixels.
         * @return std::shared_ptr<ITexture> A shared pointer to the created texture object, or nullptr if the API is unsupported.
         */
        static std::shared_ptr<ITexture> Create(const UUID& uuid, const std::string& name, std::uint32_t width, std::uint32_t height)
        {
            auto& renderer = Renderer::GetInstance();
            switch (renderer.GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                return std::make_shared<GL_Texture>(uuid, name, width, height);
            }
            case RenderingAPI::Vulkan:
            {
                MOTION_CORE_ERROR("Vulkan API is not yet supported for texture creation!");
                return nullptr;
            }
            case RenderingAPI::DirectX:
            {
                MOTION_CORE_ERROR("DirectX API is not yet supported for texture creation!");
                return nullptr;
            }
            default:
            {
                MOTION_CORE_ERROR("Unsupported rendering API for texture creation!");
                return nullptr;
            }
            }
        }

        /**
         * @brief Creates a texture object based on the current rendering API.
         *
         * This static function instantiates a texture object of the appropriate type
         * (e.g., OpenGL, Vulkan, DirectX) depending on the active rendering API.
         * For unsupported APIs, an error is logged and nullptr is returned.
         *
         * @param name        The name identifier for the texture.
         * @param textureFile The file path to the texture resource.
         * @param type        The type of the texture (e.g., diffuse, specular).
         * @param flip        Whether to vertically flip the texture on load (default: true).
         * @return std::shared_ptr<ITexture> A shared pointer to the created texture object,
         *         or nullptr if the API is unsupported.
         */
        static std::shared_ptr<ITexture> Create(const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip = true)
        {
            auto& renderer = Renderer::GetInstance();
            switch (renderer.GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                UUID uuid = UniqueIdentity::GetUniqueID();
                return std::make_shared<GL_Texture>(uuid, name, textureFile, type, flip);
            }
            case RenderingAPI::Vulkan:
            {
                MOTION_CORE_ERROR("Vulkan API is not yet supported for texture creation!");
                return nullptr;
            }
            case RenderingAPI::DirectX:
            {
                MOTION_CORE_ERROR("DirectX API is not yet supported for texture creation!");
                return nullptr;
            }
            default:
            {
                MOTION_CORE_ERROR("Unsupported rendering API for texture creation!");
                return nullptr;
            }
            }
        }

        /**
         * @brief Creates a texture object based on the current rendering API.
         *
         * This static function instantiates a texture object with the specified UUID, name,
         * texture file path, type, and flip option. The type of texture created depends on the
         * active rendering API (OpenGL, Vulkan, or DirectX). Currently, only OpenGL is supported;
         * Vulkan and DirectX will log an error and return nullptr.
         *
         * @param uuid        The unique identifier for the texture.
         * @param name        The name to assign to the texture.
         * @param textureFile The filesystem path to the texture source file.
         * @param type        The type of the texture (e.g., diffuse, specular).
         * @param flip        Whether to vertically flip the texture on load (default: true).
         * @return std::shared_ptr<ITexture> A shared pointer to the created texture object,
         *         or nullptr if the API is unsupported.
         */
        static std::shared_ptr<ITexture> Create(const UUID& uuid, const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip = true)
        {
            auto& renderer = Renderer::GetInstance();
            switch (renderer.GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                return std::make_shared<GL_Texture>(uuid, name, textureFile, type, flip);
            }
            case RenderingAPI::Vulkan:
            {
                MOTION_CORE_ERROR("Vulkan API is not yet supported for texture creation!");
                return nullptr;
            }
            case RenderingAPI::DirectX:
            {
                MOTION_CORE_ERROR("DirectX API is not yet supported for texture creation!");
                return nullptr;
            }
            default:
            {
                MOTION_CORE_ERROR("Unsupported rendering API for texture creation!");
                return nullptr;
            }
            }
        }
    };

    template<>
    struct AssetBackendsBuilder<ICubeMapTexture>
    {
        /**
         * @brief Creates a cube map texture object based on the current rendering API.
         *
         * This static function instantiates a cube map texture object with the specified name
         * and source file path. The type of texture created depends on the active rendering API
         * (OpenGL, Vulkan, or DirectX). Currently, only OpenGL is supported; Vulkan and DirectX
         * will log an error and return nullptr.
         *
         * @param name The name to assign to the cube map texture.
         * @param textureFile The filesystem path to the cube map texture source file.
         * @return std::shared_ptr<ICubeMapTexture> A shared pointer to the created cube map texture object,
         *         or nullptr if the API is unsupported.
         */
        static std::shared_ptr<ICubeMapTexture> Create(const std::string& name, const std::filesystem::path& textureFile)
        {
            auto& renderer = Renderer::GetInstance();
            switch (renderer.GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                UUID uuid = UniqueIdentity::GetUniqueID();
                return std::make_shared<GL_CubeMapTexture>(uuid, name, textureFile);
            }
            case RenderingAPI::Vulkan:
            {
                MOTION_CORE_ERROR("Vulkan API is not yet supported for cube map texture creation!");
                return nullptr;
            }
            case RenderingAPI::DirectX:
            {
                MOTION_CORE_ERROR("DirectX API is not yet supported for cube map texture creation!");
                return nullptr;
            }
            default:
            {
                MOTION_CORE_ERROR("Unsupported rendering API for cube map texture creation!");
                return nullptr;
            }
            }
        }

        /**
         * @brief Creates a cube map texture object based on the current rendering API.
         *
         * This static function instantiates a cube map texture object with the specified UUID,
         * name, and source file path. The type of texture created depends on the active rendering API
         * (OpenGL, Vulkan, or DirectX). Currently, only OpenGL is supported; Vulkan and DirectX
         * will log an error and return nullptr.
         *
         * @param uuid The unique identifier for the cube map texture.
         * @param name The name to assign to the cube map texture.
         * @param textureFile The filesystem path to the cube map texture source file.
         * @return std::shared_ptr<ICubeMapTexture> A shared pointer to the created cube map texture object,
         *         or nullptr if the API is unsupported.
         */
        static std::shared_ptr<ICubeMapTexture> Create(const UUID& uuid, const std::string& name, const std::filesystem::path& textureFile)
        {
            auto& renderer = Renderer::GetInstance();
            switch (renderer.GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                return std::make_shared<GL_CubeMapTexture>(uuid, name, textureFile);
            }
            case RenderingAPI::Vulkan:
            {
                MOTION_CORE_ERROR("Vulkan API is not yet supported for cube map texture creation!");
                return nullptr;
            }
            case RenderingAPI::DirectX:
            {
                MOTION_CORE_ERROR("DirectX API is not yet supported for cube map texture creation!");
                return nullptr;
            }
            default:
            {
                MOTION_CORE_ERROR("Unsupported rendering API for cube map texture creation!");
                return nullptr;
            }
            }
        }
    };


    template<>
    struct AssetBackendsBuilder<Mesh>
    {
        /**
         * @brief Creates a new Mesh instance and returns a shared pointer to it.
         *
         * This function constructs a Mesh object with a unique identifier, the specified name,
         * vertex and index data, buffer layout, and an optional parent model. The Mesh is managed
         * by a std::shared_ptr for automatic memory management.
         *
         * @param name The name of the mesh.
         * @param vertices Pointer to the array of vertex data.
         * @param verticesSize The size (in bytes or elements, as required by Mesh) of the vertex data array.
         * @param indices Pointer to the array of index data.
         * @param indicesCount The number of indices in the index data array.
         * @param layout The buffer layout describing the structure of the vertex data.
         * @param parentModel Shared pointer to the parent StaticMesh, or nullptr if there is no parent.
         * @return std::shared_ptr<Mesh> A shared pointer to the newly created Mesh instance.
         */
        std::shared_ptr<Mesh> Create(const std::string& name, float* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel)
        {
            return std::make_shared<Mesh>(UniqueIdentity::GetUniqueID(), name, vertices, verticesSize, indices, indicesCount, layout, parentModel);
        }

        /**
         * @brief Creates a new Mesh object and returns a shared pointer to it.
         *
         * @param uuid Unique identifier for the mesh.
         * @param name Name of the mesh.
         * @param vertices Pointer to the array of vertex data.
         * @param verticesSize Size of the vertex data array (in floats).
         * @param indices Pointer to the array of index data.
         * @param indicesCount Number of indices in the index array.
         * @param layout Buffer layout describing the structure of the vertex data.
         * @param parentModel Shared pointer to the parent StaticMesh object.
         * @return std::shared_ptr<Mesh> Shared pointer to the newly created Mesh object.
         */
        std::shared_ptr<Mesh> Create(const UUID& uuid, const std::string& name, float* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel)
        {
            return std::make_shared<Mesh>(uuid, name, vertices, verticesSize, indices, indicesCount, layout, parentModel);
        }
    };

    template<>
    struct AssetBackendsBuilder<Material>
    {
        /**
         * @brief Creates a new Material instance and returns a shared pointer to it.
         *
         * This function constructs a Material object with a unique identifier, the specified name,
         * and an optional shading method. The Material is managed by a std::shared_ptr for automatic memory management.
         *
         * @param name The name of the material.
         * @return std::shared_ptr<Material> A shared pointer to the newly created Material instance.
         */
        static std::shared_ptr<Material> Create(const std::string& name)
        {
            UUID uuid = UniqueIdentity::GetUniqueID();
            return std::make_shared<Material>(uuid, name);
        }

        /**
         * @brief Creates a new Material instance with a specified UUID and returns a shared pointer to it.
         *
         * This function constructs a Material object with the given UUID, name, and an optional shading method.
         * The Material is managed by a std::shared_ptr for automatic memory management.
         *
         * @param uuid The unique identifier for the material.
         * @param name The name of the material.
         * @return std::shared_ptr<Material> A shared pointer to the newly created Material instance.
         */
        static std::shared_ptr<Material> Create(const UUID& uuid, const std::string& name)
        {
            return std::make_shared<Material>(uuid, name);
        }
    };

    template<>
    struct AssetBackendsBuilder<StaticMesh>
    {
        /**
         * @brief Creates a new StaticMesh instance and returns a shared pointer to it.
         *
         * This function constructs a StaticMesh object with a unique identifier, the specified name,
         * and the path to the model file. The StaticMesh is managed by a std::shared_ptr for automatic memory management.
         *
         * @param name The name of the static mesh.
         * @param modelFile The filesystem path to the model file.
         * @return std::shared_ptr<StaticMesh> A shared pointer to the newly created StaticMesh instance.
         */
        static std::shared_ptr<StaticMesh> Create(const std::string& name, const std::filesystem::path& modelFile)
        {
            UUID uuid = UniqueIdentity::GetUniqueID();
            return std::make_shared<StaticMesh>(uuid, name, modelFile);
        }

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
        static std::shared_ptr<StaticMesh> Create(const UUID& uuid, const std::string& name, const std::filesystem::path& modelFile)
        {
            return std::make_shared<StaticMesh>(uuid, name, modelFile);
        }
    };

    template<>
    struct AssetBackendsBuilder<IFrameTexture>
    {
        /**
         * @brief Creates a new frame texture object based on the current rendering API.
         *
         * This static function instantiates a frame texture using the appropriate implementation
         * for the active rendering API (e.g., OpenGL). If the API is not supported, it logs an error
         * and returns nullptr.
         *
         * @param name The name to assign to the frame texture.
         * @param textureID Pointer to a TextureID object to be associated with the frame texture.
         * @param useMultiSampling Boolean flag indicating whether multisampling should be enabled.
         * @return std::shared_ptr<IFrameTexture> Shared pointer to the created frame texture object,
         *         or nullptr if the rendering API is not supported.
         */
        static std::shared_ptr<IFrameTexture> Create(const std::string& name, TextureID* textureID, bool useMultiSampling)
        {
            auto& renderer = Renderer::GetInstance();
            switch (renderer.GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                UUID uuid = UniqueIdentity::GetUniqueID();
                return std::make_shared<GL_FrameTexture>(uuid, name, textureID, useMultiSampling);
            }
            case RenderingAPI::Vulkan:
            {
                MOTION_CORE_ERROR("Vulkan API is not yet supported for frame texture creation!");
                return nullptr;
            }
            case RenderingAPI::DirectX:
            {
                MOTION_CORE_ERROR("DirectX API is not yet supported for frame texture creation!");
                return nullptr;
            }
            default:
            {
                MOTION_CORE_ERROR("Unsupported rendering API for frame texture creation!");
                return nullptr;
            }
            }
        }

        /**
         * @brief Creates a new frame texture object based on the current rendering API.
         *
         * This static function instantiates a frame texture using the appropriate implementation
         * for the active rendering API (e.g., OpenGL). If the API is not supported, it logs an error
         * and returns nullptr.
         *
         * @param uuid The unique identifier for the frame texture.
         * @param name The name to assign to the frame texture.
         * @param textureID Pointer to a TextureID object to be associated with the frame texture.
         * @param useMultiSampling Boolean flag indicating whether multisampling should be enabled.
         * @return std::shared_ptr<IFrameTexture> Shared pointer to the created frame texture object,
         *         or nullptr if the rendering API is not supported.
         */
        static std::shared_ptr<IFrameTexture> Create(const UUID& uuid, const std::string& name, TextureID* textureID, bool useMultiSampling)
        {
            auto& renderer = Renderer::GetInstance();
            switch (renderer.GetAPI())
            {
            case RenderingAPI::OpenGL:
            {
                return std::make_shared<GL_FrameTexture>(uuid, name, textureID, useMultiSampling);
            }
            case RenderingAPI::Vulkan:
            {
                MOTION_CORE_ERROR("Vulkan API is not yet supported for frame texture creation!");
                return nullptr;
            }
            case RenderingAPI::DirectX:
            {
                MOTION_CORE_ERROR("DirectX API is not yet supported for frame texture creation!");
                return nullptr;
            }
            default:
            {
                MOTION_CORE_ERROR("Unsupported rendering API for frame texture creation!");
                return nullptr;
            }
            }
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
                MOTION_CORE_WARN("Asset '{}' already exists!", name);
                return Get<T>(name);
            }

            UUID uuid = UniqueIdentity::GetUniqueID();
            auto asset = AssetBackendsBuilder<T>::Create(name, std::forward<Args>(args)...);

            if (asset)
                Register(uuid, name, asset);

            return asset;
        }

        /**
         * @brief Creates a new asset of type T with the specified UUID, name, and constructor arguments.
         *
         * If an asset with the given UUID already exists, a warning is logged and the existing asset is returned.
         * Otherwise, a new asset is constructed using the provided arguments, registered, and returned.
         *
         * @tparam T The asset type to create. Must satisfy the AssetExpected concept.
         * @tparam Args Variadic template parameters for the constructor arguments of T.
         * @param uuid The unique identifier for the asset.
         * @param name The unique name for the asset.
         * @param args Arguments to forward to the constructor of T.
         * @return std::shared_ptr<T> A shared pointer to the created or existing asset.
         */
        template<AssetExpected T, typename... Args>
        std::shared_ptr<T> Create(const UUID& uuid, const std::string& name, Args&&... args)
        {
            if (Exists(uuid)) {
                MOTION_CORE_WARN("Asset with UUID {} already exists!", uuid);
                return Get<T>(uuid);
            }

            auto asset = AssetBackendsBuilder<T>::Create(uuid, name, std::forward<Args>(args)...);

            if (asset)
                Register(uuid, name, asset);

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