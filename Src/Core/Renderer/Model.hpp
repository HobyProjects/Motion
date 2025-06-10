#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <filesystem>

#include "Mesh.hpp"
#include "Asset.hpp"

namespace Motion::Core
{
    class Importer;

    class Model final : public AssetBase<IAsset>
    {
        public:
            struct SubMesh
            {
                explicit SubMesh(uint32_t meshIndex, uint32_t materialIndex, const std::shared_ptr<Mesh>& mesh)
                    : MeshIndex(meshIndex), MaterialIndex(materialIndex), MeshPtr(std::move(mesh)){}
                ~SubMesh() = default;
            
                uint32_t MeshIndex{0};
                uint32_t MaterialIndex{0};
                std::shared_ptr<Mesh> MeshPtr{nullptr};
            };

            struct SubMeshMaterial
            {
                explicit SubMeshMaterial(uint32_t materialIndex, const std::string& name, const std::string& materialFile)
                    : MaterialIndex(materialIndex), Materials(std::make_shared<Material>(name, materialFile)){}
                ~SubMeshMaterial() = default;

                uint32_t MaterialIndex{0};
                std::shared_ptr<Material> Materials{nullptr};
            };

        public:
            Model(const std::string& name, const std::filesystem::path& modelFile):
                AssetBase<IAsset>(name, AssetType::Model, modelFile.string()){}
            virtual ~Model() = default;

            void Render(const glm::mat4& modelTransForm);

        private:
            std::vector<std::shared_ptr<SubMesh>> m_SubMeshes{};
            std::unordered_map<uint32_t, std::shared_ptr<SubMeshMaterial>> m_SubMeshMaterialMapping{};
            std::string Name{ "" };

            friend class Importer;
    };
}