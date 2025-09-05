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

    struct StaticMeshComponent
    {
        UUID ID{ 0 };
        std::string Name{ "unamed" };
        std::shared_ptr<StaticMesh> Model{ nullptr };

        StaticMeshComponent() : ID(UniqueIdentity::GetUniqueID()) {};
        StaticMeshComponent(const std::string& name, const std::shared_ptr<StaticMesh>& model) : Name(name), Model(model), ID(UniqueIdentity::GetUniqueID()) {}
        ~StaticMeshComponent() = default;
    };

    struct TransformComponent
    {
        UUID ID{ 0 };
        
        glm::vec3 Translation{ 0.0f };
        glm::quat Rotation{ 0.0f, 0.0f, 0.0f, 0.0f }; 
        glm::vec3 Scale{ 0.01f };

        TransformComponent()
            : ID(UniqueIdentity::GetUniqueID()) {}

        TransformComponent(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
            : Translation(translation), Rotation(rotation), Scale(scale), ID(UniqueIdentity::GetUniqueID()) {}

        ~TransformComponent() = default;

        glm::mat4 GetTransform(bool degree = false) const
        {
            glm::mat4 T = glm::translate(glm::mat4(1.0f), Translation);
            glm::mat4 R = glm::toMat4(Rotation);
            glm::mat4 S = glm::scale(glm::mat4(1.0f), Scale);

            
            glm::mat4 TRS = T * R * S;
            return TRS;
        }
    };

    struct RigidBodyComponent
    {
        UUID ID{ 0 };

        float       Mass{1.0f};                // kg (0 => static/immovable)
        float       InvMass{1.0f};             // computed from mass
        glm::vec3   Velocity{0.0f};             // m/s
        glm::vec3   ForceAccum{0.0f};           // N (cleared each step)
        float       LinearDamping{0.02f};      // simple drag; unitless

        bool    Sleeping{false};
        float   SleepTimer{0.0f};

        glm::vec3 AngularVelocity{0.0f};
        glm::vec3 TorqueAccum{0.0f};
        float AngularDamping{0.05f};

        glm::vec3 InertiaDiag{1.0f};
        glm::vec3 InvInertiaDiag{1.0f};

        void SetMass(float m) 
        {
            Mass = m;
            InvMass = (m > 0.0f) ? 1.0f / m : 0.0f;
        }

        void SetBoxInertia(const glm::vec3& halfExtents) 
        {
            // box dimensions (full extents)
            const glm::vec3 s   = 2.0f * halfExtents;
            const float x2      = s.x * s.x, y2 = s.y * s.y, z2 = s.z * s.z;

            // I_box = (1/12) m * diag(y^2+z^2, x^2+z^2, x^2+y^2)
            glm::vec3 I     = (Mass * (1.0f/12.0f)) * glm::vec3(y2+z2, x2+z2, x2+y2);
            InertiaDiag     = I;
            InvInertiaDiag  = glm::vec3(
                I.x > 0 ? 1.0f/I.x : 0.0f,
                I.y > 0 ? 1.0f/I.y : 0.0f,
                I.z > 0 ? 1.0f/I.z : 0.0f
            );
        }

        void SetSphereInertia(float radius) 
        {
            // solid sphere: I = (2/5) m r^2
            const float I   = (2.0f/5.0f) * Mass * radius * radius;
            InertiaDiag     = glm::vec3(I);
            InvInertiaDiag  = glm::vec3(I > 0 ? 1.0f/I : 0.0f);
        }

        RigidBodyComponent()
            : ID(UniqueIdentity::GetUniqueID()) {}

        ~RigidBodyComponent() = default;
    };

    enum class CombineMode : std::uint8_t
    {
        Average, Minimum, Maximum, Multiply
    };

    struct PhysicalMaterial
    {
        float Restitution{0.20f};
        float FrictionStatic{0.60f};
        float FrictionDynamic{0.45f};
        // (Optional later: rollingFriction, anisotropic, etc.)

        CombineMode FrictionCombine{CombineMode::Average};
        CombineMode RestitutionCombine{CombineMode::Maximum};
    };

    enum class ColliderType { None, Sphere, Box, Capsule };

    struct SphereCollider 
    { 
        float Radius{0.5f}; 
    };

    struct BoxCollider 
    { 
        glm::vec3 HalfExtents{0.5f}; 
    };

    struct CapsuleCollider 
    {
        float Radius{0.4f};     
        float HalfHeight{0.9f}; 
    };


    struct AABB
    {
        glm::vec3 MIN{0.0f};
        glm::vec3 MAX{0.0f};
    };

    struct ColliderComponent
    {
        ColliderType    Type{ColliderType::None};
        
        SphereCollider  Sphere{};
        BoxCollider     Box{};
        CapsuleCollider Capsule{};

        AABB WorldAABB{};
        PhysicalMaterial MaterialBase{};
    };
}