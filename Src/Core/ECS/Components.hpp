#pragma once

#include <limits>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <reactphysics3d/reactphysics3d.h>

#include "UUID.hpp"
#include "Mesh.hpp"

namespace Motion
{
    class Entity;
    
    struct Units 
    {
        static constexpr float METERS_PER_UNIT = 1.0f;
        static constexpr float MPS_PER_UNIT    = 1.0f;
        static constexpr float KGS_PER_UNIT    = 1.0f;
        static constexpr float DEG_PER_UNIT    = 1.0f;
        static constexpr float SI_GRAVITY      = 9.80665f;

        static inline float     ToMeters(float u)             { return u * METERS_PER_UNIT; }
        static inline glm::vec3 ToMeters(const glm::vec3& u)  { return u * METERS_PER_UNIT; }
        static inline float     FromMeters(float m)           { return m / METERS_PER_UNIT; }
        static inline glm::vec3 FromMeters(const glm::vec3& m){ return m / METERS_PER_UNIT; }

        static inline float     ToMetersPerSecond(float u)             { return u * METERS_PER_UNIT; }
        static inline glm::vec3 ToMetersPerSecond(const glm::vec3& u)  { return u * METERS_PER_UNIT; }
        static inline float     FromMetersPerSecond(float m)           { return m / METERS_PER_UNIT; }
        static inline glm::vec3 FromMetersPerSecond(const glm::vec3& m){ return m / METERS_PER_UNIT; }

        static inline float     ToKilograms(float u)             { return u * KGS_PER_UNIT; }
        static inline glm::vec3 ToKilograms(const glm::vec3& u)  { return u * KGS_PER_UNIT; }
        static inline float     FromKilograms(float k)           { return k / KGS_PER_UNIT; }
        static inline glm::vec3 FromKilograms(const glm::vec3& k){ return k / KGS_PER_UNIT; }

    };
    
    struct TagComponent
    {
        UUID        ID{ 0 };
        std::string Tag{ "unamed" };
        bool        IsActive{ true };

        TagComponent() = default;
        TagComponent(const std::string& tag) : Tag(tag) { ID = UniqueIdentity::GetUniqueID(); }
        TagComponent(const std::string& tag, bool isActive) : Tag(tag), IsActive(isActive) { ID = UniqueIdentity::GetUniqueID(); }
        ~TagComponent() = default;
        
    };

    struct NodeComponent
    {
        UUID ID{UniqueIdentity::GetUniqueID()};
        std::shared_ptr<Entity> EnTTNext{nullptr};
        bool IsRoot{false};
    };

    struct MaterialComponent
    {
        UUID ID{UniqueIdentity::GetUniqueID()};
        std::shared_ptr<Material> MaterialPointer{};
    };

    struct TransformComponent 
    {
        UUID        ID{UniqueIdentity::GetUniqueID()};
        glm::vec3   Translation{0.0f};
        glm::quat   Rotation{1.0f, 0.0f, 0.0f, 0.0f}; 
        glm::vec3   Scale{1.0f};                     

        inline glm::mat4 GetTransform() const 
        {
            return  glm::translate(glm::mat4(1.0f), Translation) *
                    glm::toMat4(glm::normalize(Rotation)) *
                    glm::scale(glm::mat4(1.0f), Scale);
        }
    };

    enum class BodyType : std::uint8_t
    {
        Static,
        Dynamic
    };

    struct RigidBodyComponent
    {
        UUID ID{UniqueIdentity::GetUniqueID()};
        BodyType Type{BodyType::Dynamic};

        float LinearDamping{0.2f};
        float AngularDamping{0.05f};

        bool LockX{false}, LockY{false}, LockZ{false};
        bool LockRotX{false}, LockRotY{false}, LockRotZ{false};

        rp3d::RigidBody* PhysicsBody{nullptr};
    };

    enum class ShapeType { Box, Sphere, Capsule, Convex, Concave, HightField };

    struct ColliderComponent
    {
        UUID ID{UniqueIdentity::GetUniqueID()};
        
        ShapeType Type{ShapeType::Box};
        rp3d::CollisionShape* Shape{nullptr};
        rp3d::Collider* Collider{nullptr};

        glm::vec3 BoxHalfExtents{0.5f, 0.5f, 0.5f};
        float SphereRadius{0.5f};
        struct 
        {
            float Radius{0.5f};
            float Height{1.0f};
            std::int32_t Axis{1};
        }Capsule;

        glm::vec3 LastAppliedScale{1.0f};
        glm::vec3 LocalTransform{0.0f};
        glm::quat LocalRotation{1.0f, 0.0f, 0.0f, 0.0f};

        float Friction{0.5f};
        float Restitution{0.2f};
        float MassDensity{500.0f};
    };

    struct MeshComponent
    {
        UUID ID{UniqueIdentity::GetUniqueID()};
        std::shared_ptr<Mesh> MeshPointer{nullptr};
    };
}
