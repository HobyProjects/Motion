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

    class Model final : public AssetBase<IAsset>
    {
        public:
            Model() = default;
            Model(UUID uuid, const std::string& name, const std::filesystem::path& modelFile);
            virtual ~Model() = default;

            [[nodiscard]] std::size_t GetMeshesCount() const noexcept { return m_Meshes.size(); }
            [[nodiscard]] const glm::vec3& GetMinBounds() const noexcept { return m_MIN; }
            [[nodiscard]] const glm::vec3& GetMaxBounds() const noexcept { return m_MAX; }

            void Render() const;
            void PushMesh(const std::shared_ptr<Mesh>& mesh) { m_Meshes.push_back(mesh); }

            std::vector<std::shared_ptr<Mesh>>::iterator begin() { return m_Meshes.begin(); }
            std::vector<std::shared_ptr<Mesh>>::iterator end() { return m_Meshes.end(); }
            std::vector<std::shared_ptr<Mesh>>::const_iterator begin() const { return m_Meshes.begin(); }
            std::vector<std::shared_ptr<Mesh>>::const_iterator end() const { return m_Meshes.end(); }

            const std::vector<glm::vec3>& GetPositions() const { return m_Vertices; }
            const std::vector<glm::uint32_t>& GetFaces() const { return m_Indices; }

            std::shared_ptr<Mesh>& operator[](std::size_t idx) { return m_Meshes[idx]; }
            const std::shared_ptr<Mesh>& operator[](std::size_t idx) const { return m_Meshes[idx]; }

        private:
            std::vector<std::shared_ptr<Mesh>> m_Meshes;
            std::vector<glm::vec3> m_Vertices;
            std::vector<glm::uint32_t> m_Indices;
            glm::vec3 m_MIN{ FLT_MAX, FLT_MAX, FLT_MAX };
            glm::vec3 m_MAX{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
            friend class Importer;
    };

}