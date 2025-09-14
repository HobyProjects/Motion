#pragma once

#include <limits>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp> 

#include "UUID.hpp"
#include "Model.hpp"
#include "Entity.hpp"
#include "KinetiX.hpp"

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

        inline glm::mat4 GetTransform() const 
        {
            return  glm::translate(glm::mat4(1.0f), Translation) *
                    glm::toMat4(glm::normalize(Rotation)) *
                    glm::scale(glm::mat4(1.0f), Scale);
        }

        inline glm::mat3 GetR() const { return glm::mat3_cast(glm::normalize(Rotation)); }

        inline static glm::vec3 GetTransformPoint(const TransformComponent& t, const glm::vec3& p) 
        {
            return t.Translation + (t.Rotation * (t.Scale * p));
        }

        inline static glm::vec3 GetTransformVector(const TransformComponent& t, const glm::vec3& v) 
        {
            return t.Rotation * (t.Scale * v);
        }

    };
}
