#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "UUID.hpp"
#include "Model.hpp"

namespace Motion::Core
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

    struct TransformComponent
    {
        UUID ID{ 0 };
        glm::vec3 Translation{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };

        TransformComponent() : ID(UniqueIdentity::GetUniqueID()) {};
        ~TransformComponent() = default;

        /**
         * @brief Computes the transformation matrix for the component.
         *
         * This function constructs a transformation matrix by combining translation, rotation, and scale.
         * - Translation is applied first, using the `Translation` vector.
         * - Rotation is applied next, converting the `Rotation` vector from degrees to radians and applying
         *   rotations around the X, Y, and Z axes in that order.
         * - Scaling is applied last, using the `Scale` vector.
         *
         * @return glm::mat4 The resulting transformation matrix (T * R * S).
         */
        glm::mat4 GetTransform() const
        {
            glm::mat4 T{ glm::translate(glm::mat4(1.0f), Translation) };

            glm::vec3 radiantRotation = glm::radians(Rotation);
            glm::mat4 R{ glm::rotate(glm::mat4(1.0f), radiantRotation.x, glm::vec3(1, 0, 0)) *
                         glm::rotate(glm::mat4(1.0f), radiantRotation.y, glm::vec3(0, 1, 0)) *
                         glm::rotate(glm::mat4(1.0f), radiantRotation.z, glm::vec3(0, 0, 1)) };

            glm::mat4 S{ glm::scale(glm::mat4(1.0f), Scale) };

            glm::mat4 TRS{ T * R * S };
            return TRS;
        }
    };

    struct MeshComponent
    {
        UUID ID{ 0 };
        std::string Name{ "unamed" };
        std::shared_ptr<StaticMesh> Mesh{ nullptr };

        MeshComponent() : ID(UniqueIdentity::GetUniqueID()) {};
        MeshComponent(const std::string& name, const std::shared_ptr<StaticMesh>& mesh) : Name(name), Mesh(mesh), ID(UniqueIdentity::GetUniqueID()) {}
        ~MeshComponent() = default;
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