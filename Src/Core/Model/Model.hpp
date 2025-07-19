#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include <glm/glm.hpp>

#include "Mesh.hpp"
#include "Asset.hpp"

namespace Motion::Core
{
    class Importer; // forward declaration

    class StaticMesh final : public AssetBase<IAsset>
    {
    public:
        struct MeshSegment
        {
            MeshSegment() = default;
            ~MeshSegment() = default;

            std::uint32_t MeshIndex{ 0 };
            std::uint32_t MaterialIndex{ 0 };
            std::shared_ptr<Mesh> MeshSelf{ nullptr };
            std::shared_ptr<Material> Materials{};
        };

    public:
        StaticMesh() = default;
        StaticMesh(const std::string& name, const std::filesystem::path& modelFile);
        StaticMesh(UUID uuid, const std::string& name, const std::filesystem::path& modelFile);
        virtual ~StaticMesh() = default;

        std::vector<std::shared_ptr<MeshSegment>>::iterator begin() { return m_Meshes.begin(); }
        std::vector<std::shared_ptr<MeshSegment>>::iterator end() { return m_Meshes.end(); }
        std::vector<std::shared_ptr<MeshSegment>>::const_iterator cbegin() const { return m_Meshes.cbegin(); }
        std::vector<std::shared_ptr<MeshSegment>>::const_iterator cend() const { return m_Meshes.cend(); }

    private:
        std::vector<std::shared_ptr<MeshSegment>> m_Meshes;
        friend class Importer;
    };

}