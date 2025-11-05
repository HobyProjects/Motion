#pragma once

#include "SceneSerializer.hpp"

namespace Motion
{
    class ComponentSerializer : public IComponentSerializer
    {
    public:
        ComponentSerializer() = default;
        ~ComponentSerializer() override = default;

        void SerializeEntity(const entt::registry& registry, entt::entity entity, SerializedEntity& data) override
        {
            if (const auto* tag = registry.try_get<TagComponent>(entity))
                data.Tag = *tag;

            if (const auto* transform = registry.try_get<TransformComponent>(entity))
                data.Transform = *transform;

            if (const auto* model = registry.try_get<ModelComponent>(entity))
            {
                data.Model = *model;
                data.HasModel = true;
            }
        }

        void SerializeNode(const entt::registry& registry, entt::entity entity, SerializedNode& data) override
        {
            if (const auto* tag = registry.try_get<TagComponent>(entity))
                data.Tag = *tag;

            if (const auto* transform = registry.try_get<TransformComponent>(entity))
                data.Transform = *transform;

            if (const auto* mesh = registry.try_get<MeshComponent>(entity))
            {
                data.Mesh = *mesh;
                data.HasMesh = true;
            }

            if (const auto* material = registry.try_get<MaterialComponent>(entity))
            {
                data.Material = *material;
                data.HasMaterial = true;
            }

            if (const auto* rb = registry.try_get<RigidBodyComponent>(entity))
            {
                data.RigidBody = *rb;
                data.HasRigidBody = true;
            }

            if (const auto* collider = registry.try_get<ColliderComponent>(entity))
            {
                data.Collider = *collider;
                data.HasCollider = true;
            }
        }

        void DeserializeEntity(entt::registry& registry, entt::entity entity, const SerializedEntity& data) override
        {
            auto& tag = registry.emplace_or_replace<TagComponent>(entity);
            tag = data.Tag;

            auto& transform = registry.emplace_or_replace<TransformComponent>(entity);
            transform = data.Transform;
            transform.RebuildLocal();

            if (data.HasModel)
            {
                auto& model = registry.emplace_or_replace<ModelComponent>(entity);
                model = data.Model;
            }
        }

        void DeserializeNode(entt::registry& registry, entt::entity entity, const SerializedNode& data) override
        {
            auto& tag = registry.emplace_or_replace<TagComponent>(entity);
            tag = data.Tag;

            auto& transform = registry.emplace_or_replace<TransformComponent>(entity);
            transform = data.Transform;
            transform.RebuildLocal();

            if (data.HasMesh)
            {
                auto& mesh = registry.emplace_or_replace<MeshComponent>(entity);
                mesh = data.Mesh;
            }

            if (data.HasMaterial)
            {
                auto& material = registry.emplace_or_replace<MaterialComponent>(entity);
                material = data.Material;
                
                if (!material.MaterialPointer)
                    material.MaterialPointer = Material::Create();
            }

            if (data.HasRigidBody)
            {
                auto& rb = registry.emplace_or_replace<RigidBodyComponent>(entity);
                rb = data.RigidBody;
                rb.PhysicsBody = nullptr;
            }

            if (data.HasCollider)
            {
                auto& collider = registry.emplace_or_replace<ColliderComponent>(entity);
                collider = data.Collider;
                collider.Shape = nullptr;
                collider.Collider = nullptr;
                collider.ConvexMesh = nullptr;
            }
        }
    };

} 