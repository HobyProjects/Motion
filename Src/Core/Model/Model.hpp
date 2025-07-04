#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <filesystem>

#include "Mesh.hpp"
#include "Asset.hpp"

namespace Motion::Core
{
    class Importer; // forward declaration

    class Model final : public AssetBase<IAsset>
    {
        public:
            struct SubMesh
            {
                SubMesh() = default;
                SubMesh(uint32_t meshIndex, uint32_t materialIndex, const std::shared_ptr<Mesh>& mesh)
                    : MeshIndex(meshIndex), MaterialIndex(materialIndex), MeshPtr(std::move(mesh)){}
                ~SubMesh() = default;
            
                uint32_t MeshIndex{0};
                uint32_t MaterialIndex{0};
                std::shared_ptr<Mesh> MeshPtr{nullptr};
                Model* ParentModel{nullptr};
            };

            struct SubMeshMaterial
            {
                SubMeshMaterial() = default;
                SubMeshMaterial(uint32_t materialIndex, uint32_t meshIndex, const std::string& name, const std::string& materialFile)
                    : MaterialIndex(materialIndex), MeshIndex(meshIndex), Materials(std::make_shared<Material>(name, materialFile)){}
                ~SubMeshMaterial() = default;

                uint32_t MaterialIndex{0};
                uint32_t MeshIndex{0};
                std::shared_ptr<Material> Materials{nullptr};
                Model* ParentModel{nullptr};
            };

        public:
            Model(const std::string& name, const std::filesystem::path& modelFile):
                AssetBase<IAsset>(UniqueIdentity::GetUniqueID(), name, AssetType::Model, modelFile.string()){}
            Model(UUID uuid, const std::string& name, const std::filesystem::path& modelFile):
                AssetBase<IAsset>(uuid, name, AssetType::Model, modelFile.string()){}
            virtual ~Model() = default;

            
            std::vector<std::shared_ptr<SubMesh>>::iterator begin() { return m_SubMeshes.begin(); }
            std::vector<std::shared_ptr<SubMesh>>::iterator end() { return m_SubMeshes.end(); }
            std::vector<std::shared_ptr<SubMesh>>::const_iterator begin() const { return m_SubMeshes.begin(); }
            std::vector<std::shared_ptr<SubMesh>>::const_iterator end() const { return m_SubMeshes.end(); }
            
            void Render(const glm::mat4& modelTransForm, const glm::mat4& cameraMatrix);
            void InsertMaterial(const std::shared_ptr<Material>& material);
            std::shared_ptr<SubMeshMaterial> GetSubMeshMaterial(uint32_t subMeshIndex) const;


        private:
            std::vector<std::shared_ptr<SubMesh>> m_SubMeshes;
            std::unordered_map<uint32_t, std::shared_ptr<SubMeshMaterial>> m_SubMeshMaterialMapping;
            std::string Name{ "" };

            friend class Importer;
    };

    class ModelsManager
    {
        private:
            ModelsManager() = default;
            ~ModelsManager() = default;

            ModelsManager(const ModelsManager&) = delete;
            ModelsManager& operator=(const ModelsManager&) = delete;
            ModelsManager(ModelsManager&&) = delete;
            ModelsManager& operator=(ModelsManager&&) = delete;

        public:
            static void InsertModel(const UUID& uuid, const std::shared_ptr<Model>& model);
            static std::shared_ptr<Model> GetModel(const UUID& uuid);
            static std::shared_ptr<Model> GetModel(const std::string& name);
    };
}