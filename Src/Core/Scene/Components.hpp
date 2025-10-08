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
    struct TagComponent
    {
        UUID        ID{UniqueIdentity::GetUniqueID()};
        std::string Tag{ "unamed" };
        bool        IsActive{ true };

        TagComponent() = default;
        TagComponent(const std::string& tag) : Tag(tag) { ID = UniqueIdentity::GetUniqueID(); }
        TagComponent(const std::string& tag, bool isActive) : Tag(tag), IsActive(isActive) { ID = UniqueIdentity::GetUniqueID(); }
        ~TagComponent() = default;
        
    };

    struct TransformComponent 
    {
        UUID        ID{UniqueIdentity::GetUniqueID()};
        glm::vec3   Translation{0.0f};
        glm::quat   Rotation{1.0f, 0.0f, 0.0f, 0.0f}; 
        glm::vec3   Scale{1.0f};                     

        glm::mat4 Local{1.0f};
        glm::mat4 World{1.0f};
        bool Dirty = true;
        
        static glm::mat4 Compose(const glm::vec3& t, const glm::quat& r, const glm::vec3& s) 
        {
            return glm::translate(glm::mat4(1.0f), t) *
            glm::toMat4(glm::normalize(r)) *
            glm::scale(glm::mat4(1.0f), s);
        }
        
        void RebuildLocal() 
        { 
            Local = Compose(Translation, Rotation, Scale); 
            Dirty = true; 
        }

        glm::mat4 GetLocalTransform() const 
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

        rp3d::ConvexMesh* ConvexMesh{nullptr};
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
        float MassDensity{7.5f};
    };

    struct MeshComponent
    {
        UUID ID{UniqueIdentity::GetUniqueID()};
        std::string Name{};
        std::shared_ptr<Mesh> MeshPointer{nullptr};
        glm::vec3 MinBounds{};
        glm::vec3 MaxBounds{};
    };

    struct MaterialComponent
    {
        UUID ID{UniqueIdentity::GetUniqueID()};
        std::shared_ptr<Material> MaterialPointer{};
    };

    struct ModelComponent
    {
        UUID ID{UniqueIdentity::GetUniqueID()};
        std::filesystem::path FilePath{};
        std::uint32_t MeshCount{0};
        glm::vec3 MinBounds{};
        glm::vec3 MaxBounds{};
    };

    struct HierarchyComponent 
    {
        entt::entity Parent{entt::null};
        entt::entity FirstChild{entt::null};
        entt::entity NextSibling{entt::null};

        HierarchyComponent() = default;
        HierarchyComponent(entt::entity parent, entt::entity child, entt::entity next) 
            : Parent(parent), FirstChild(child), NextSibling(next) {}
        ~HierarchyComponent() = default;
    };
}
