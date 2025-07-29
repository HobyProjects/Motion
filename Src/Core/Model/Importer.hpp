#pragma once

#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
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
        static std::shared_ptr<StaticMesh> ImportModel(const std::filesystem::path& path, const std::string& exportPath = "default");

    private:
        static bool ConvertToGLB(aiScene* scene, const std::filesystem::path& outputPath);
        static std::shared_ptr<StaticMesh> ReadGLB(const std::filesystem::path& outputPath);
    };
}