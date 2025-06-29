#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "UUID.hpp"
#include "Model.hpp"

namespace Motion::Core
{
    struct TagComponent
    {
        UUID ID{0};
        std::string Tag{ "unamed" };
        bool IsActive{ false };

        TagComponent() = default;
        TagComponent(const std::string& tag) : Tag(tag) { ID = UniqueIdentity::GetUniqueID();}
        TagComponent(const std::string& tag, bool isActive) : Tag(tag), IsActive(isActive) { ID = UniqueIdentity::GetUniqueID(); }
        ~TagComponent() = default;
    };

    struct TransformComponent
    {
        glm::vec3 Translation{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };

        TransformComponent() = default;
        ~TransformComponent() = default;

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
        std::string Name{ "unamed" };
        std::shared_ptr<Model> Object{ nullptr };

        MeshComponent() = default;
        ~MeshComponent() = default;
    };

    struct DirectionalLightComponent 
    {
        glm::vec3 Direction{ -0.2f, -1.0f, -0.3f };
        glm::vec3 Color{ 1.0f };
        float Intensity = 1.0f;
        bool CastShadows = true;
    };

    struct PointLightComponent 
    {
        glm::vec3 Position{0.0f};
        glm::vec3 Color{1.0f};
        float Intensity = 1.0f;
        float Radius = 10.0f;
        float Falloff = 1.0f;
    };

    struct SpotLightComponent 
    {
        glm::vec3 Position{0.0f};
        glm::vec3 Direction{0.0f, -1.0f, 0.0f};
        glm::vec3 Color{1.0f};
        float Intensity = 1.0f;
        float InnerCutoff = 12.5f; // degrees
        float OuterCutoff = 17.5f;
        float Range = 15.0f;
    };
}