#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Base.hpp"
#include "Model.hpp"

namespace Motion::Core
{
    class Importer
    {
        private:
            Importer() = default;
            ~Importer() = default;
        
        public:
            static std::shared_ptr<Model> ImportModel(const std::string& modelName, const std::filesystem::path& path);

        private:
            static void LoadMesh(const std::shared_ptr<Model>& modelPtr, aiMesh* mesh, const aiScene* scene);
            static void LoadNode(const std::shared_ptr<Model>& modelPtr, aiNode* node, const aiScene* scene);
            static void LoadMaterials(const std::shared_ptr<Model>& modelPtr, const aiScene* scene);
    };
}