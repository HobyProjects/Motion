#include "CorePCH.hpp"

#include "SceneSerializer.hpp"
#include "ComponentSerializer.hpp"
#include "HierarchyManager.hpp"
#include "EntityBuilder.hpp"

#include "YAML_SerializationStrategy.hpp"

namespace Motion
{
    std::shared_ptr<ISerializationStrategy> SceneSerializer::s_Strategy = std::make_shared<YAMLSerializationStrategy>();
    std::shared_ptr<IComponentSerializer> SceneSerializer::s_ComponentSerializer = std::make_shared<ComponentSerializer>();
    std::shared_ptr<IHierarchyManager> SceneSerializer::s_HierarchyManager = std::make_shared<HierarchyManager>();
    std::shared_ptr<IEntityBuilder> SceneSerializer::s_EntityBuilder = nullptr;
    
    SceneSerializer::SceneSerializer()
    {
        if (!s_EntityBuilder)
        {
            s_EntityBuilder = std::make_shared<EntityBuilder>(
                s_ComponentSerializer,
                s_HierarchyManager
            );
        }
    }

    SceneSerializer::~SceneSerializer() = default;
    
    SerializationResult SceneSerializer::Serialize(Scene* scene, const std::filesystem::path& path)
    {
        if (!scene)
        {
            return SerializationResult::Fail("Cannot serialize null scene");
        }

        if (path.empty())
        {
            return SerializationResult::Fail("Cannot serialize to empty path");
        }

        try
        {
            SerializedScene sceneData = ExtractSceneData(scene);
            auto validationResult = ValidateSceneData(sceneData);
            if (!validationResult.Success)
            {
                return validationResult;
            }

            auto result = s_Strategy->Serialize(sceneData, path);
            if (result.Success)
            {
                scene->GetContext().Specification->SavedPath = std::filesystem::absolute(path);
            }

            return result;
        }
        catch (const std::exception& e)
        {
            return SerializationResult::Fail(
                std::format("Serialization failed: {}", e.what())
            );
        }
    }

    std::shared_ptr<Scene> SceneSerializer::Deserialize(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            MOTION_CORE_ERROR("Scene file not found: {}", path.string());
            return nullptr;
        }

        try
        {
            auto sceneDataOpt = s_Strategy->Deserialize(path);
            if (!sceneDataOpt)
            {
                MOTION_CORE_ERROR("Failed to deserialize scene data");
                return nullptr;
            }

            SerializedScene& sceneData = *sceneDataOpt;
            SceneSpecification spec{};
            spec.ID = sceneData.SceneID;
            spec.Name = sceneData.Name;
            spec.SavedPath = sceneData.SavedPath;

            auto scene = std::make_shared<Scene>(spec);
            if (!scene)
            {
                MOTION_CORE_ERROR("Failed to create scene");
                return nullptr;
            }

            if (!RestoreSceneData(scene.get(), sceneData))
            {
                MOTION_CORE_ERROR("Failed to restore scene data");
                return nullptr;
            }

            MOTION_CORE_INFO("Scene loaded successfully: {}", sceneData.Name);
            return scene;
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_CRITICAL("Deserialization failed: {}", e.what());
            return nullptr;
        }
    }

    SerializationResult SceneSerializer::SerializeRuntime(Scene* scene, const std::filesystem::path& path)
    {
        if (!scene)
        {
            return SerializationResult::Fail("Cannot serialize null scene");
        }

        if (path.empty())
        {
            return SerializationResult::Fail("Cannot serialize to empty path");
        }

        try
        {
            SerializedScene sceneData;
            sceneData.SceneID = scene->GetContext().Specification->ID;
            sceneData.Name = scene->GetContext().Specification->Name;
            sceneData.SavedPath = scene->GetContext().Specification->SavedPath;
            sceneData.Lighting = scene->GetContext().Physics->SunLight;
            sceneData.PhysicsSettings = scene->GetContext().Physics->Settings;

            auto& registry = scene->GetContext().Entities->Registry;
            scene->ForEachRootEntity([&](entt::entity root)
            {
                if (!registry.valid(root)) return;

                SerializedEntity entity;
                entity.Tag = *registry.try_get<TagComponent>(root);
                entity.Transform = *registry.try_get<TransformComponent>(root);
                auto children = s_HierarchyManager->GetChildren(registry, root);
                
                for (entt::entity child : children)
                {
                    SerializedNode node;
                    node.Tag = *registry.try_get<TagComponent>(child);
                    node.Transform = *registry.try_get<TransformComponent>(child);
                    
                    if (auto* rb = registry.try_get<RigidBodyComponent>(child))
                    {
                        node.RigidBody = *rb;
                        node.HasRigidBody = true;
                    }
                    
                    if (auto* col = registry.try_get<ColliderComponent>(child))
                    {
                        node.Collider = *col;
                        node.HasCollider = true;
                    }
                    
                    entity.Nodes.push_back(node);
                }

                sceneData.Entities.push_back(entity);
            });

            return s_Strategy->Serialize(sceneData, path);
        }
        catch (const std::exception& e)
        {
            return SerializationResult::Fail(
                std::format("Runtime serialization failed: {}", e.what())
            );
        }
    }

    SerializationResult SceneSerializer::DeserializeRuntime(Scene* scene, const std::filesystem::path& path)
    {
        if (!scene)
        {
            return SerializationResult::Fail("Cannot deserialize into null scene");
        }

        if (!std::filesystem::exists(path))
        {
            return SerializationResult::Fail(
                std::format("Runtime scene file not found: {}", path.string())
            );
        }

        try
        {
            auto sceneDataOpt = s_Strategy->Deserialize(path);
            if (!sceneDataOpt)
            {
                return SerializationResult::Fail("Failed to deserialize runtime data");
            }

            SerializedScene& runtimeData = *sceneDataOpt;
            scene->GetContext().Physics->SunLight = runtimeData.Lighting;
            scene->GetContext().Physics->Settings = runtimeData.PhysicsSettings;

            auto& registry = scene->GetContext().Entities->Registry;
            std::unordered_map<UUID, SerializedEntity*> entityLookup;
            for (auto& entity : runtimeData.Entities)
            {
                entityLookup[entity.Tag.ID] = &entity;
            }

            scene->ForEachRootEntity([&](entt::entity root)
            {
                auto* rootTag = registry.try_get<TagComponent>(root);
                if (!rootTag) return;
                SerializedEntity* entityData = nullptr;
                if (auto it = entityLookup.find(rootTag->ID); it != entityLookup.end())
                {
                    entityData = it->second;
                }

                if (!entityData) return;
                if (auto* tr = registry.try_get<TransformComponent>(root))
                {
                    tr->Translation = entityData->Transform.Translation;
                    tr->Rotation = entityData->Transform.Rotation;
                    tr->Scale = entityData->Transform.Scale;
                    tr->RebuildLocal();
                }

                std::unordered_map<UUID, SerializedNode*> nodeLookup;
                for (auto& node : entityData->Nodes)
                {
                    nodeLookup[node.Tag.ID] = &node;
                }

                auto children = s_HierarchyManager->GetChildren(registry, root);
                for (entt::entity child : children)
                {
                    auto* childTag = registry.try_get<TagComponent>(child);
                    if (!childTag) continue;

                    SerializedNode* nodeData = nullptr;
                    if (auto it = nodeLookup.find(childTag->ID); it != nodeLookup.end())
                    {
                        nodeData = it->second;
                    }

                    if (!nodeData) continue;
                    if (auto* tr = registry.try_get<TransformComponent>(child))
                    {
                        tr->Translation = nodeData->Transform.Translation;
                        tr->Rotation = nodeData->Transform.Rotation;
                        tr->Scale = nodeData->Transform.Scale;
                        tr->RebuildLocal();
                    }

                    auto* rb = registry.try_get<RigidBodyComponent>(child);
                    if (nodeData->HasRigidBody && rb)
                    {
                        rb->Type = nodeData->RigidBody.Type;
                        rb->LinearDamping = nodeData->RigidBody.LinearDamping;
                        rb->AngularDamping = nodeData->RigidBody.AngularDamping;
                        rb->LockX = nodeData->RigidBody.LockX;
                        rb->LockY = nodeData->RigidBody.LockY;
                        rb->LockZ = nodeData->RigidBody.LockZ;
                        rb->LockRotX = nodeData->RigidBody.LockRotX;
                        rb->LockRotY = nodeData->RigidBody.LockRotY;
                        rb->LockRotZ = nodeData->RigidBody.LockRotZ;

                        if (rb->PhysicsBody)
                        {
                            rb->PhysicsBody->resetForce();
                            rb->PhysicsBody->resetTorque();
                            rb->PhysicsBody->setLinearVelocity(rp3d::Vector3(0, 0, 0));
                            rb->PhysicsBody->setAngularVelocity(rp3d::Vector3(0, 0, 0));
                        }
                    }

                    auto* col = registry.try_get<ColliderComponent>(child);
                    if (nodeData->HasCollider && col)
                    {
                        col->Friction = nodeData->Collider.Friction;
                        col->Restitution = nodeData->Collider.Restitution;
                        col->MassDensity = nodeData->Collider.MassDensity;
                        col->Collider->setLocalToBodyTransform(rp3d::Transform::identity());
                    }
                }
            });

            scene->RefreshPhysicBodies();
            return SerializationResult::Ok("Runtime state restored");
        }
        catch (const std::exception& e)
        {
            return SerializationResult::Fail(
                std::format("Runtime deserialization failed: {}", e.what())
            );
        }
    }
    
    void SceneSerializer::SetSerializationStrategy(std::shared_ptr<ISerializationStrategy> strategy)
    {
        if (strategy)
        {
            s_Strategy = strategy;
            MOTION_CORE_INFO("Serialization strategy updated");
        }
        else
        {
            MOTION_CORE_WARN("Attempted to set null serialization strategy");
        }
    }
    
    SerializedScene SceneSerializer::ExtractSceneData(Scene* scene)
    {
        SerializedScene sceneData;
        sceneData.SceneID = scene->GetContext().Specification->ID;
        sceneData.Name = scene->GetContext().Specification->Name;
        sceneData.SavedPath = scene->GetContext().Specification->SavedPath;
        sceneData.Lighting = scene->GetContext().Physics->SunLight;
        sceneData.PhysicsSettings = scene->GetContext().Physics->Settings;

        auto& registry = scene->GetContext().Entities->Registry;
        scene->ForEachRootEntity([&](entt::entity root)
        {
            if (!registry.valid(root)) return;

            SerializedEntity entity;
            s_ComponentSerializer->SerializeEntity(registry, root, entity);
            auto children = s_HierarchyManager->GetChildren(registry, root);
            
            for (entt::entity child : children)
            {
                SerializedNode node;
                s_ComponentSerializer->SerializeNode(registry, child, node);
                entity.Nodes.push_back(node);
            }

            sceneData.Entities.push_back(entity);
        });

        return sceneData;
    }

    bool SceneSerializer::RestoreSceneData(Scene* scene, const SerializedScene& data)
    {
        scene->GetContext().Physics->SunLight = data.Lighting;
        scene->GetContext().Physics->Settings = data.PhysicsSettings;
        if (!s_EntityBuilder)
        {
            s_EntityBuilder = std::make_shared<EntityBuilder>(
                s_ComponentSerializer,
                s_HierarchyManager
            );
        }

        for (const auto& entityData : data.Entities)
        {
            auto entityOpt = s_EntityBuilder->BuildEntity(scene, entityData);
            if (!entityOpt)
            {
                MOTION_CORE_WARN("Failed to build entity: {}", entityData.Tag.Tag);
                continue;
            }

            scene->EmplaceEntity(*entityOpt);
        }

        return true;
    }

    SerializationResult SceneSerializer::ValidateSceneData(const SerializedScene& data)
    {
        SerializationResult result = SerializationResult::Ok();

        if (data.Name.empty())
        {
            result.AddWarning("Scene has empty name");
        }

        if (data.Entities.empty())
        {
            result.AddWarning("Scene has no entities");
        }

        for (const auto& entity : data.Entities)
        {
            if (entity.Tag.Tag.empty())
            {
                result.AddWarning("Entity has empty tag");
            }

            if (entity.HasModel && entity.Model.FilePath.empty())
            {
                std::string warn = std::format("Entity '{}' has model but empty file path", entity.Tag.Tag);
                result.AddWarning(warn);
            }

            if (entity.HasModel && !std::filesystem::exists(entity.Model.FilePath))
            {
                std::string warn = std::format("Model file not found for '{}': {}", entity.Tag.Tag, entity.Model.FilePath.string());
                result.AddWarning(warn);
            }
        }

        return result;
    }

} 