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
#include "Entity.hpp"
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
        std::unordered_map<MeshAssetID, MeshAsset> Meshes{};
        std::string Name{};
    };

    inline std::pair<glm::vec3, glm::vec3> ModelBounds(const std::vector<Vertex>& vertices)
    {
        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
        for (const auto& v : vertices)
        {
            min = glm::min(min, v.Position);
            max = glm::max(max, v.Position);
        }
        return { min, max };
    }

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
            static std::shared_ptr<ImportedResults> ImportModelAsync(const std::filesystem::path& path, bool shouldExport, const std::string& exportPath);
    };
}