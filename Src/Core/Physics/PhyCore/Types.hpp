#pragma once

#include <memory>
#include <cstdint>
#include <glm/glm.hpp>

#include "Base.hpp"

namespace Motion
{
    struct AABB
    {
        glm::vec3 MIN{0.0f};
        glm::vec3 MAX{0.0f};
    };

    struct MassProperties
    {
        float MASS{0.0f};
        glm::mat3 Inertia{1.0f};
        glm::vec3 COM{0.0f};
    };

    struct MaterialBaseProperties
    {
        float Friction{0.6f};
        float Restitution{0.1f};
    };

    enum class ShapeType : std::uint8_t
    {
        Box     = BIT(0),
        Sphere  = BIT(1),
        Capsule = BIT(2)
    };

    template<>
    struct enable_bitmask_operations<ShapeType> : std::true_type {};

    struct Shape
    {
        ShapeType Type;
        float ConvexRadius{0.02f};

        Shape() = default;
        virtual ~Shape() = default;

        virtual MassProperties Mass(float density) const = 0;
        virtual AABB LocalAABB() const = 0;
        virtual glm::vec3 SupportLocal(const glm::vec3& dir) const = 0;
    };

    struct Collider
    {
        const Shape* ColliderShape{nullptr};
        glm::mat4 LocalPose{1.0f};
        MaterialBaseProperties MaterialProp{};
        std::uint32_t Filter{0xFFFFFFFF};
    };
}