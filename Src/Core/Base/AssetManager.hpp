#pragma once

#include <string>

#include "Asset.hpp"
#include "Shaders.hpp"
#include "Texture.hpp"
#include "Model.hpp"
#include "UUID.hpp"

namespace Motion::Core
{
    class AssetManager
    {
        private:
            AssetManager() = default;
            ~AssetManager() = default;

            AssetManager(const AssetManager&) = delete;
            AssetManager& operator=(const AssetManager&) = delete;

        public:
            static std::shared_ptr<IShader> CreateShader(const std::string& name, const std::filesystem::path& shaderFile);
            static std::shared_ptr<IShader> CreateShader(UUID uuid, const std::string& name, const std::filesystem::path& shaderFile);
            static std::shared_ptr<ITexture> CreateTextureFromFile(const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flipOnLoading = true);
            static std::shared_ptr<ITexture> CreateTextureFromFile(UUID uuid, const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flipOnLoading = true);
            static std::shared_ptr<ITexture> CreatePlainTexture(const std::string& name, uint32_t width, uint32_t height);
            static std::shared_ptr<ITexture> CreatePlainTexture(UUID uuid, const std::string& name, uint32_t width, uint32_t height);
            static std::shared_ptr<Model> LoadModel(const std::string& name, std::filesystem::path& modelFile);
            static std::shared_ptr<Model> LoadModel(UUID uuid, const std::string& name, std::filesystem::path& modelFile);

            static std::shared_ptr<IAsset> GetAsset(const UUID& uuid);
            static std::shared_ptr<IAsset> GetAsset(const std::string& name);

            static std::shared_ptr<IShader> GetShader(const UUID& uuid);
            static std::shared_ptr<ITexture> GetTexture(const UUID& uuid);
            static std::shared_ptr<Model> GetModel(const UUID& uuid);

            static std::shared_ptr<IShader> GetShader(const std::string& name);
            static std::shared_ptr<ITexture> GetTexture(const std::string& name);
            static std::shared_ptr<Model> GetModel(const std::string& name);

            static void Clear();
    };
}