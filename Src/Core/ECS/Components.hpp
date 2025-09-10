#pragma once

#include <limits>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp> // for glm::toMat4

#include "UUID.hpp"
#include "Model.hpp"
#include "Entity.hpp"

#include "Colliders.hpp"

namespace Motion
{
    struct Units 
    {
        static constexpr float METERS_PER_UNIT = 1.0f;
        static constexpr float g_mps2 = 9.80665f;

        static inline float     ToMeters(float u)             { return u * METERS_PER_UNIT; }
        static inline glm::vec3 ToMeters(const glm::vec3& u)  { return u * METERS_PER_UNIT; }
        static inline float     FromMeters(float m)           { return m / METERS_PER_UNIT; }
        static inline glm::vec3 FromMeters(const glm::vec3& m){ return m / METERS_PER_UNIT; }
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

    struct MeshComponent
    {
        UUID                        ID{ 0 };
        std::string                 Name{ "unamed" };
        std::shared_ptr<Model>      Model{ nullptr };

        MeshComponent() : ID(UniqueIdentity::GetUniqueID()) {}
        MeshComponent(const std::string& name, const std::shared_ptr<Motion::Model>& model) : Name(name), Model(model), ID(UniqueIdentity::GetUniqueID()) {}
        ~MeshComponent() = default;
    };

    struct TransformComponent 
    {
        UUID        ID{0};
        glm::vec3   Translation{0.0f};
        glm::quat   Rotation{1.0f, 0.0f, 0.0f, 0.0f}; 
        glm::vec3   Scale{1.0f};                     

        TransformComponent() : ID(UniqueIdentity::GetUniqueID()) {}
        TransformComponent(const glm::vec3& t, const glm::quat& r, const glm::vec3& s)
            : Translation(t), Rotation(glm::normalize(r)), Scale(s), ID(UniqueIdentity::GetUniqueID()) {}

        glm::mat4 GetTransform() const 
        {
            return  glm::translate(glm::mat4(1.0f), Translation) *
                    glm::toMat4(glm::normalize(Rotation)) *
                    glm::scale(glm::mat4(1.0f), Scale);
        }

        glm::mat3 GetR() const { return glm::mat3_cast(glm::normalize(Rotation)); }
    };

    struct TransformHistoryComponent
    {
        glm::vec3 PrevTranslation{0.0f};
        glm::quat PrevRotation{1,0,0,0};
        glm::vec3 PrevScale{1.0f};
    };


    struct RigidBodyComponent
    {
        enum class PhysicsBody { Static, Kinematic, Dynamic };

        UUID      ID{ 0 };
        bool      IsEnabled{false};

        PhysicsBody Type{PhysicsBody::Dynamic};
        bool        UseGravity{true};
        float       GravityScale{1.0f};

        glm::vec3 LinearVelocity{0.0f};
        glm::vec3 AngularVelocity{0.0f};

        glm::vec3 ForceAccum{0.0f};
        glm::vec3 TorqueAccum{0.0f};

        float Mass{1.0f};
        float InvMass{1.0f};
        float Density{1.0f};

        glm::mat3 IBodyInv{1.0f};
        glm::mat3 IWorldInv{1.0f};

        glm::bvec3 LockLinear{false,false,false};
        glm::bvec3 LockAngular{false,false,false};
        float MaxLinearSpeed{std::numeric_limits<float>::infinity()};
        float MaxAngularSpeed{std::numeric_limits<float>::infinity()};

        bool  IsSleeping{false};
        float SleepTimer{0.0f};
        float SleepThresholdLin{0.01f};
        float SleepThresholdAng{0.01f};

        glm::vec3 KinematicTargetPos{0.0f};
        glm::quat KinematicTargetRot{1,0,0,0};

        bool  CCDEnabled{false};
        float CCDMotionThreshold{0.01f};
        float SweptSphereRadius{0.0f};

        RigidBodyComponent(): ID(UniqueIdentity::GetUniqueID()){}
        ~RigidBodyComponent() = default;

        inline bool Static() const 
        { 
            return InvMass == 0.0f; 
        }

        inline void SetMass(float m)
        {
            Mass    = m;
            InvMass = (m > 0.0f) ? 1.0f / m : 0.0f;
        }

        inline void SyncInertia(const TransformComponent& tc)
        {
            glm::mat3 R = tc.GetR();
            IWorldInv   = R * IBodyInv * glm::transpose(R);
        }
    };

    enum class CombineMode : std::uint8_t 
    { 
        Average, 
        Minimum, 
        Maximum, 
        Multiply 
    };

    struct PhysicalMaterial
    {
        float Restitution{0.20f};
        float FrictionStatic{0.60f};
        float FrictionDynamic{0.45f};

        CombineMode FrictionCombine{CombineMode::Average};
        CombineMode RestitutionCombine{CombineMode::Maximum};
    };

    struct ColliderComponent
    {
        UUID                ID{0};
        bool                IsEnabled{true};
        bool                ShowCollider{false};

        ColliderType        Type{ColliderType::AABB};   
        uint32_t            Layer{0x00000001};
        uint32_t            Mask {0xFFFFFFFF};
        bool                IsTrigger{false};

        glm::vec3           LocalOffset{0.0f};
        glm::quat           LocalRotation{1,0,0,0};

        Collider            Shape{};

        AABB                WorldAABB{};
        AABB                SweptAABB{};               
        bool                DirtyAABB{true};

        PhysicalMaterial    MaterialBase{};
    };

    struct DampingComponent
    {
        UUID  ID{0};
        float Linear{0.02f}; 
        float Angular{0.02f}; 

        DampingComponent(): ID(UniqueIdentity::GetUniqueID()){}
        ~DampingComponent() = default;
    };
}
