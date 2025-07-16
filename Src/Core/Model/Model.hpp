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

    class StaticMesh final : public AssetBase<IAsset>
    {
    public:
        struct MeshPart
        {
            MeshPart() = default;
            ~MeshPart() = default;

            std::uint32_t SubMeshIndex{ 0 };
            std::uint32_t SubMaterialIndex{ 0 };
            std::shared_ptr<Mesh> MeshSelf{ nullptr };
            std::vector<std::shared_ptr<Material>> SubMeshMaterials{};
        };

    public:
        StaticMesh() = default;
        StaticMesh(const std::string& name, const std::filesystem::path& modelFile);
        StaticMesh(UUID uuid, const std::string& name, const std::filesystem::path& modelFile);
        virtual ~StaticMesh() = default;


        std::vector<std::shared_ptr<MeshPart>>::iterator begin() { return m_SubMeshes.begin(); }
        std::vector<std::shared_ptr<MeshPart>>::iterator end() { return m_SubMeshes.end(); }
        std::vector<std::shared_ptr<MeshPart>>::const_iterator cbegin() const { return m_SubMeshes.cbegin(); }
        std::vector<std::shared_ptr<MeshPart>>::const_iterator cend() const { return m_SubMeshes.cend(); }

        void Render(const glm::mat4& modelTransForm, const glm::mat4& cameraMatrix);

    private:
        std::vector<std::shared_ptr<MeshPart>> m_SubMeshes;
        friend class Importer;
    };

}