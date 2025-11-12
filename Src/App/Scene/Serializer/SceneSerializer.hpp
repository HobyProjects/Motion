#pragma once

#include <filesystem>
#include <memory>
#include <functional>
#include <optional>
#include <vector>

#include "Scene.hpp"

namespace Motion
{
    class ISerializationStrategy;
    class EntityBuilder;
    class HierarchyManager;
    
    struct SerializationResult
    {
        bool Success{false};
        std::string Message{};
        std::vector<std::string> Warnings{};
        
        static SerializationResult Ok(const std::string& msg = "Success")
        {
            return SerializationResult{true, msg, {}};
        }
        
        static SerializationResult Fail(const std::string& msg)
        {
            return SerializationResult{false, msg, {}};
        }
        
        void AddWarning(const std::string& warning)
        {
            Warnings.push_back(warning);
        }
    };
    
    struct SerializedNode
    {
        TagComponent         Tag{};
        TransformComponent   Transform{};
        MeshComponent        Mesh{};
        MaterialComponent    Material{};
        RigidBodyComponent   RigidBody{};
        ColliderComponent    Collider{};
        
        bool HasMesh{false};
        bool HasMaterial{false};
        bool HasRigidBody{false};
        bool HasCollider{false};
    };

    struct SerializedEntity
    {
        TagComponent         Tag{};
        ModelComponent       Model{};
        TransformComponent   Transform{};
        std::vector<SerializedNode> Nodes{};
        
        bool HasModel{false};
    };

    struct SerializedScene
    {
        UUID SceneID{};
        std::string Name{};
        std::filesystem::path SavedPath{};
        ScenePhysicsWorld::WorldLighting Lighting{};
        rp3d::PhysicsWorld::WorldSettings PhysicsSettings{};
        std::vector<SerializedEntity> Entities{};
    };
    
    class IComponentSerializer
    {
        public:
            virtual ~IComponentSerializer() = default;
            virtual void SerializeEntity(const entt::registry& registry, entt::entity entity, SerializedEntity& data) = 0;
            virtual void SerializeNode(const entt::registry& registry, entt::entity entity, SerializedNode& data) = 0;
            virtual void DeserializeEntity(entt::registry& registry, entt::entity entity, const SerializedEntity& data) = 0;
            virtual void DeserializeNode(entt::registry& registry, entt::entity entity, const SerializedNode& data) = 0;
    };
    
    class IHierarchyManager
    {
        public:
            virtual ~IHierarchyManager() = default;
            virtual void BuildHierarchy(entt::registry& registry, entt::entity root, const std::vector<entt::entity>& children) = 0;
            virtual std::vector<entt::entity> GetChildren(const entt::registry& registry, entt::entity root) const = 0;
            virtual bool IsRoot(const entt::registry& registry, entt::entity entity) const = 0;
    };
    
    class ISerializationStrategy
    {
        public:
            virtual ~ISerializationStrategy() = default;
            virtual SerializationResult Serialize(const SerializedScene& sceneData, const std::filesystem::path& path) = 0;
            virtual std::optional<SerializedScene> Deserialize(const std::filesystem::path& path) = 0;
    };
    
    class IEntityBuilder
    {
        public:
            virtual ~IEntityBuilder() = default;
            virtual std::optional<entt::entity> BuildEntity(Scene* scene, const SerializedEntity& data) = 0;
    };
    
    class SceneSerializer
    {
        public:
            SceneSerializer();
            ~SceneSerializer();

            static SerializationResult Serialize(Scene* scene, const std::filesystem::path& path);
            static std::shared_ptr<Scene> Deserialize(const std::filesystem::path& path);
            
            static SerializationResult SerializeRuntime(Scene* scene, const std::filesystem::path& path);
            static SerializationResult DeserializeRuntime(Scene* scene, const std::filesystem::path& path);
            static void SetSerializationStrategy(std::shared_ptr<ISerializationStrategy> strategy);

        private:
            static SerializedScene ExtractSceneData(Scene* scene);
            static bool RestoreSceneData(Scene* scene, const SerializedScene& data);
            static SerializationResult ValidateSceneData(const SerializedScene& data);

            static std::shared_ptr<ISerializationStrategy> s_Strategy;
            static std::shared_ptr<IComponentSerializer> s_ComponentSerializer;
            static std::shared_ptr<IHierarchyManager> s_HierarchyManager;
            static std::shared_ptr<IEntityBuilder> s_EntityBuilder;
    };

}