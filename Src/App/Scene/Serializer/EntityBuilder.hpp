#pragma once

#include "Buffers.hpp"
#include "ModelImporter.hpp"

#include "SceneSerializer.hpp"
#include "SceneUtils.hpp"
#include "ComponentSerializer.hpp"
#include "HierarchyManager.hpp"
#include "ModelImporter.hpp"

namespace Motion
{
    class EntityBuilder : public IEntityBuilder
    {
    public:
        EntityBuilder(
            std::shared_ptr<IComponentSerializer> componentSerializer,
            std::shared_ptr<IHierarchyManager> hierarchyManager
        )
            : m_ComponentSerializer(componentSerializer)
            , m_HierarchyManager(hierarchyManager)
        {
        }

        ~EntityBuilder() override = default;

        std::optional<entt::entity> BuildEntity(Scene* scene, const SerializedEntity& data) override
        {
            if (!scene)
            {
                MOTION_CORE_ERROR("Cannot build entity: null scene");
                return std::nullopt;
            }

            if (!ValidateEntityData(data))
            {
                MOTION_CORE_ERROR("Invalid entity data for: {}", data.Tag.Tag);
                return std::nullopt;
            }

            std::shared_ptr<ImportedResults> modelData = nullptr;
            if (data.HasModel && !data.Model.FilePath.empty())
            {
                modelData = ImportModel(data.Model.FilePath);
                if (!modelData)
                {
                    MOTION_CORE_ERROR("Failed to import model: {}", data.Model.FilePath.string());
                    return std::nullopt;
                }

                if (data.Model.MeshCount != modelData->MeshCount)
                {
                    MOTION_CORE_WARN("Mesh count mismatch for '{}': expected {}, got {}",
                                     data.Model.FilePath.string(), data.Model.MeshCount, modelData->MeshCount);
                }
            }

            auto& registry = scene->GetContext().Entities->Registry;
            entt::entity rootEntity = registry.create();
            m_ComponentSerializer->DeserializeEntity(registry, rootEntity, data);

            std::vector<entt::entity> children;
            if (modelData && !data.Nodes.empty())
            {
                children = BuildChildNodes(scene, data.Nodes, modelData);
            }

            if (!children.empty())
            {
                m_HierarchyManager->BuildHierarchy(registry, rootEntity, children);
            }

            return rootEntity;
        }

    private:
        bool ValidateEntityData(const SerializedEntity& data) const
        {
            if (data.Tag.Tag.empty())
            {
                MOTION_CORE_ERROR("Entity has empty tag");
                return false;
            }

            if (data.HasModel && data.Model.FilePath.empty())
            {
                MOTION_CORE_ERROR("Entity has model component but empty file path");
                return false;
            }

            if (data.HasModel && !std::filesystem::exists(data.Model.FilePath))
            {
                MOTION_CORE_ERROR("Model file not found: {}", data.Model.FilePath.string());
                return false;
            }

            return true;
        }

        std::shared_ptr<ImportedResults> ImportModel(const std::filesystem::path& path)
        {
            if (!std::filesystem::exists(path))
            {
                MOTION_CORE_ERROR("Model file not found: {}", path.string());
                return nullptr;
            }

            ImportSettings settings;
            settings.ShouldExport = false;
            settings.ExportPath = std::filesystem::path{};
            settings.FilePath = path;

            auto result = Importer::ImportEntity(settings);
            if (!result)
            {
                MOTION_CORE_ERROR("Failed to import model: {}", path.string());
                return nullptr;
            }

            return result;
        }

        std::vector<entt::entity> BuildChildNodes(Scene* scene, const std::vector<SerializedNode>& nodes, const  std::shared_ptr<ImportedResults>& modelData)
        {
            std::vector<entt::entity> children;
            auto& registry = scene->GetContext().Entities->Registry;

            std::unordered_map<uint32_t, const SerializedNode*> nodesByIndex;
            for (const auto& node : nodes)
            {
                if (node.HasMesh)
                    nodesByIndex[node.Mesh.MeshIndex] = &node;
            }

            const BufferLayout layout
            {
                { "a_Position",   BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Position)  },
                { "a_TexCoords",  BufferComponents::UV,   BufferStride::F2, false, offsetof(Vertex, TexCoord)  },
                { "a_Normals",    BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Normal)    },
                { "a_Tangents",   BufferComponents::XYZW, BufferStride::F4, false, offsetof(Vertex, Tangent)   },
                { "a_Bitangents", BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Bitangent) },
            };

            for (const auto& [index, meshData] : modelData->Meshes)
            {
                const SerializedNode* nodeData = nullptr;
                if (auto it = nodesByIndex.find(index); it != nodesByIndex.end())
                    nodeData = it->second;

                if (!nodeData)
                {
                    MOTION_CORE_WARN("No node data found for mesh index {}", index);
                    continue;
                }

                entt::entity childEntity = registry.create();
                m_ComponentSerializer->DeserializeNode(registry, childEntity, *nodeData);

                if (nodeData->HasMesh)
                {
                    auto& meshComponent = registry.get<MeshComponent>(childEntity);
                    meshComponent.MeshPointer = Mesh::Create(
                        meshData.Vertices.data(), meshData.Vertices.size(),
                        meshData.Indices.data(), meshData.Indices.size(),
                        layout
                    );
                    meshComponent.MinBounds = meshData.MIN;
                    meshComponent.MaxBounds = meshData.MAX;
                }

                if (nodeData->HasRigidBody && nodeData->HasCollider)
                {
                    SetupPhysics(scene, childEntity, *nodeData, meshData);
                }

                children.push_back(childEntity);
            }

            return children;
        }

        void SetupPhysics(Scene* scene, entt::entity entity, const SerializedNode& nodeData, const MeshAsset& meshData)
        {
            auto& registry = scene->GetContext().Entities->Registry;
            auto* physicsWorld = scene->GetContext().Physics->World;
            auto* physicsCommon = &scene->GetContext().Physics->Properties;

            CreateRigidBody(physicsWorld, &registry, entity);
            auto& collider = registry.get<ColliderComponent>(entity);
            
            switch (collider.Type)
            {
                case ShapeType::Convex:
                {
                    std::vector<glm::vec3> vertices;
                    vertices.reserve(meshData.Vertices.size());
                    
                    std::transform(
                        meshData.Vertices.begin(), meshData.Vertices.end(),
                        std::back_inserter(vertices),
                        [](const Vertex& v) { return v.Position; }
                    );

                    CreateConvexCollider(physicsCommon, &registry, entity, vertices);
                    break;
                }

                case ShapeType::Box:
                    CreateBoxCollider(physicsCommon, &registry, entity);
                    break;

                case ShapeType::Sphere:
                    CreateSphereCollider(physicsCommon, &registry, entity);
                    break;

                case ShapeType::Capsule:
                    CreateCapsuleCollider(physicsCommon, &registry, entity);
                    break;

                default:
                    MOTION_CORE_WARN("Unsupported collider type for entity");
                    break;
            }
        }

    private:
        std::shared_ptr<IComponentSerializer> m_ComponentSerializer;
        std::shared_ptr<IHierarchyManager> m_HierarchyManager;
    };

} // namespace Motion