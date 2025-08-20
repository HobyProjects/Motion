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
        StaticMesh() = default;
        StaticMesh(UUID uuid, const std::string& name, const std::filesystem::path& modelFile);
        virtual ~StaticMesh() = default;

        [[nodiscard]] std::size_t GetMeshesCount() const noexcept { return m_Meshes.size(); }
        [[nodiscard]] const glm::vec3& GetMinBounds() const noexcept { return m_MinBounds; }
        [[nodiscard]] const glm::vec3& GetMaxBounds() const noexcept { return m_MaxBounds; }

        void Render() const;

        std::vector<std::shared_ptr<Mesh>>::iterator begin() { return m_Meshes.begin(); }
        std::vector<std::shared_ptr<Mesh>>::iterator end() { return m_Meshes.end(); }
        std::vector<std::shared_ptr<Mesh>>::const_iterator begin() const { return m_Meshes.begin(); }
        std::vector<std::shared_ptr<Mesh>>::const_iterator end() const { return m_Meshes.end(); }

        std::shared_ptr<Mesh>& operator[](std::size_t idx) { return m_Meshes[idx]; }
        const std::shared_ptr<Mesh>& operator[](std::size_t idx) const { return m_Meshes[idx]; }

    private:
        std::vector<std::shared_ptr<Mesh>> m_Meshes;
        glm::vec3 m_MinBounds{ FLT_MAX, FLT_MAX, FLT_MAX };
        glm::vec3 m_MaxBounds{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
        friend class Importer;
    };

}