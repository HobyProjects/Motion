#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <filesystem>

#include "Mesh.hpp"

namespace Motion::Core
{
    class Importer;

    class Model
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
                explicit SubMeshMaterial(uint32_t materialIndex, const std::shared_ptr<Material>& material)
                    : MaterialIndex(materialIndex), Materials(std::move(material)){}
                ~SubMeshMaterial() = default;

                uint32_t MaterialIndex{0};
                std::shared_ptr<Material> Materials{nullptr};
            };

        public:
            Model(const std::string& modelName, const std::string& shaderName);
            ~Model() = default;

            void Render(const glm::mat4& modelTransForm);

        private:
            std::shared_ptr<IShader> m_Shader{ nullptr };
            std::vector<std::shared_ptr<SubMesh>> m_SubMeshes{};
            std::unordered_map<uint32_t, SubMeshMaterial> m_SubMeshMaterialMapping{};
            std::string Name{ "" };

            friend class Importer;
    };
}