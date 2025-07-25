#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Base.hpp"
#include "Model.hpp"

namespace Motion
{
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
        static std::shared_ptr<StaticMesh> ImportModel(const std::string& modelName, const std::filesystem::path& path);

    private:
        static void LoadCurrentNodeMeshes(const aiScene* currentScene, std::uint32_t meshIndex, aiMesh* currentMesh, const std::shared_ptr<StaticMesh>& staticMesh);
        static void LoadNode(const std::shared_ptr<StaticMesh>& modelPtr, aiNode* node, const aiScene* scene);
        static void LoadMaterials(std::uint32_t meshIndex, std::uint32_t materialIndex, const std::shared_ptr<StaticMesh::MeshSegment>& meshSegment, const aiScene* scene);
    };
}