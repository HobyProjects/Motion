#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <future>
#include <format>
#include <glm/glm.hpp>

#include "Base.hpp"
#include "Buffers.hpp"

namespace Motion
{
    using MeshAssetID = std::uint32_t;

    struct MeshAsset
    {
        MeshAssetID ID{ 0 };
        std::string Name{ "" };
        std::vector<Vertex> Vertices{};
        std::vector<std::uint32_t> Indices{};
        glm::vec3 MIN{0.0f};
        glm::vec3 MAX{0.0f};
    };

    struct ImportedResults
    {
        uint32_t MeshCount = 0;
        glm::vec3 MIN = {};
        glm::vec3 MAX = {};
        std::string Name{};
        std::filesystem::path FilePath{};
        std::unordered_map<MeshAssetID, MeshAsset> Meshes{};
    };

    struct ImportSettings
    {
        std::filesystem::path FilePath;
        bool ShouldExport{false};
        std::filesystem::path ExportPath;
    };
    
    class Importer
    {
        private:
            Importer() = default;
            ~Importer() = default;

            Importer(const Importer&) = delete;
            Importer& operator=(const Importer&) = delete;
            Importer(Importer&&) = delete;
            Importer& operator=(Importer&&) = delete;

        public:
            static std::shared_ptr<ImportedResults> ImportEntity(ImportSettings settings);
    };
}