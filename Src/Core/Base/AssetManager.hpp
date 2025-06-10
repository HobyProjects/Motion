#pragma once

#include <string>

#include "Asset.hpp"
#include "Shaders.hpp"
#include "Texture.hpp"
#include "Model.hpp"
#include "UUID.hpp"

namespace Motion::Core
{
    class AssetFactory
    {
        public:
            static std::shared_ptr<IShader> CreateShader(const std::string& name, const std::string& filePath);
            static std::shared_ptr<ITexture> CreateTxture(const std::string& name, const std::string& filePath);
    };

    class AssetManager
    {
        public:
            AssetManager() = default;
            ~AssetManager() = default;

            std::shared_ptr<IShader> CreateShader(const std::string& name, const std::filesystem::path& shaderFile);
            std::shared_ptr<ITexture> CreateTextureFromFile(const std::string& name, const std::filesystem::path& textureFile, bool flipOnLoading = true);
            std::shared_ptr<ITexture> CreatePlainTexture(const std::string& name, uint32_t width, uint32_t height);
            std::shared_ptr<Model> LoadModel(const std::string& name, std::filesystem::path& modelFile);

        private:
            std::unordered_map<UUID, std::shared_ptr<IAsset>> m_AssetRegistry{};
            std::unordered_map<std::string, UUID> m_AssetNameUUIDMap{};
    };
}