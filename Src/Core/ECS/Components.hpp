#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "UUID.hpp"
#include "Model.hpp"
#include "Entity.hpp"

namespace Motion
{
    struct TagComponent
    {
        UUID ID{ 0 };
        std::string Tag{ "unamed" };
        bool IsActive{ false };

        TagComponent() = default;
        TagComponent(const std::string& tag) : Tag(tag) { ID = UniqueIdentity::GetUniqueID(); }
        TagComponent(const std::string& tag, bool isActive) : Tag(tag), IsActive(isActive) { ID = UniqueIdentity::GetUniqueID(); }
        ~TagComponent() = default;
    };

    struct MeshNodeComponent
    {
        UUID ID{ 0 };
        std::shared_ptr<MeshNode> Node{ nullptr };
        Entity* EntityPointer{ nullptr };

        MeshNodeComponent() : ID(UniqueIdentity::GetUniqueID()) {};
        MeshNodeComponent(const std::shared_ptr<MeshNode>& meshNode) : Node(meshNode) { ID = UniqueIdentity::GetUniqueID(); }
        ~MeshNodeComponent() = default;
    };

    struct MeshComponent
    {
        UUID ID{ 0 };
        std::shared_ptr<StaticMesh::MeshSegment> MeshSegment{ nullptr };
        Entity* EntityPointer{ nullptr };

        MeshComponent() : ID(UniqueIdentity::GetUniqueID()) {};
        MeshComponent(const std::shared_ptr<StaticMesh::MeshSegment>& meshSegment) : MeshSegment(meshSegment) { ID = UniqueIdentity::GetUniqueID(); }
        ~MeshComponent() = default;
    };

    struct MeshCollectionComponent
    {
        UUID ID{ 0 };
        std::vector<std::shared_ptr<Entity>> Meshes{ nullptr };
        Entity* EntityPointer{ nullptr };

        MeshCollectionComponent() : ID(UniqueIdentity::GetUniqueID()) {};
        MeshCollectionComponent(std::vector<std::shared_ptr<Entity>>& meshes)
        {
            ID = UniqueIdentity::GetUniqueID();
            Meshes.swap(meshes);
        }

        ~MeshCollectionComponent() = default;
    };

    struct StaticMeshComponent
    {
        UUID ID{ 0 };
        std::string Name{ "unamed" };
        std::shared_ptr<StaticMesh> Model{ nullptr };
        Entity* EntityPointer{ nullptr };

        StaticMeshComponent() : ID(UniqueIdentity::GetUniqueID()) {};
        StaticMeshComponent(const std::string& name, const std::shared_ptr<StaticMesh>& model) : Name(name), Model(model), ID(UniqueIdentity::GetUniqueID()) {}
        ~StaticMeshComponent() = default;
    };

    struct TransformComponent
    {
        UUID ID{ 0 };
        glm::vec3 Translation{ 0.0f };
        glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f }; // Identity quaternion
        glm::vec3 Scale{ 1.0f };

        Entity* EntityPointer{ nullptr };

        TransformComponent()
            : ID(UniqueIdentity::GetUniqueID()) {
        }

        TransformComponent(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
            : Translation(translation), Rotation(rotation), Scale(scale), ID(UniqueIdentity::GetUniqueID()) {
        }

        ~TransformComponent() = default;

        /**
         * @brief Computes and returns the transformation matrix for the entity.
         *
         * This function constructs the transformation matrix (TRS) by combining translation,
         * rotation, and scale components. Additionally, if the entity has a MeshCollectionComponent,
         * it synchronizes the transformation data with all associated MeshNodeComponents.
         *
         * @return glm::mat4 The combined transformation matrix (TRS).
         */
        glm::mat4 GetTransform() const
        {
            glm::mat4 T = glm::translate(glm::mat4(1.0f), Translation);
            glm::mat4 R = glm::toMat4(Rotation);
            glm::mat4 S = glm::scale(glm::mat4(1.0f), Scale);
            glm::mat4 TRS = T * R * S;

            // Optional sync to MeshNode if available
            if (EntityPointer && EntityPointer->HasComponent<MeshCollectionComponent>())
            {
                auto& meshCollection = EntityPointer->GetComponent<MeshCollectionComponent>();
                for (const auto& meshEntity : meshCollection.Meshes)
                {
                    if (meshEntity && meshEntity->HasComponent<MeshNodeComponent>())
                    {
                        auto& meshNode = meshEntity->GetComponent<MeshNodeComponent>();
                        if (meshNode.Node)
                        {
                            meshNode.Node->Transform = TRS;

                            //[TODO]: Need to double-check if this is correct
                            meshNode.Node->Position = Translation;
                            meshNode.Node->Rotation = Rotation;
                            meshNode.Node->Scale = Scale;
                        }
                    }
                }
            }

            return TRS;
        }
    };

    struct PhysicsBodyComponent
    {
        enum class BodyType { Static, Dynamic };

        float Mass = 1.0f;
        BodyType Type = BodyType::Dynamic;

        glm::vec3 Velocity = glm::vec3(0.0f);
        glm::vec3 ForceAccum = glm::vec3(0.0f);
        bool Active = false;

        PhysicsBodyComponent(float mass = 1.0f) : Mass(mass)
        {
            Type = (Mass <= 0.0f) ? BodyType::Static : BodyType::Dynamic;
        }
    };
}