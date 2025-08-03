#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include <glm/glm.hpp>

#include "Mesh.hpp"
#include "Asset.hpp"

namespace Motion
{
    class Importer; // forward declaration

    class StaticMesh final : public AssetBase<IAsset>
    {
    public:
        struct MeshSegment
        {
            MeshSegment() = default;
            ~MeshSegment() = default;

            std::uint32_t MeshIndex{ std::numeric_limits<std::uint32_t>::max() };
            std::shared_ptr<Mesh> MeshSelf{ nullptr };
            std::shared_ptr<MaterialInstance> Materials{ nullptr };
        };

    public:
        StaticMesh() = default;
        StaticMesh(UUID uuid, const std::string& name, const std::filesystem::path& modelFile);
        virtual ~StaticMesh() = default;

        [[nodiscard]] std::size_t GetMeshesCount() const noexcept { return m_Meshes.size(); }

        std::vector<MeshSegment>::iterator begin() { return m_Meshes.begin(); }
        std::vector<MeshSegment>::iterator end() { return m_Meshes.end(); }
        std::vector<MeshSegment>::const_iterator cbegin() const { return m_Meshes.cbegin(); }
        std::vector<MeshSegment>::const_iterator cend() const { return m_Meshes.cend(); }
        std::vector<MeshSegment>::reverse_iterator rbegin() { return m_Meshes.rbegin(); }
        std::vector<MeshSegment>::reverse_iterator rend() { return m_Meshes.rend(); }
        std::vector<MeshSegment>::const_reverse_iterator crbegin() const { return m_Meshes.crbegin(); }
        std::vector<MeshSegment>::const_reverse_iterator crend() const { return m_Meshes.crend(); }

    private:
        std::vector<MeshSegment> m_Meshes;
        friend class Importer;
    };

}