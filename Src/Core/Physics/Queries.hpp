#pragma once

#include <glm/glm.hpp>
#include "Entity.hpp"

namespace Motion
{
    struct Ray
    {
        glm::vec3 Origin{0.0f};
        glm::vec3 Direction{0.0f}; // normalized;
        float     MaxDistance{1e6f};
    };

    struct RayCastHit
    {
        bool Hit{false};
        std::shared_ptr<Entity> EnTT;
        glm::vec3 Point{0.0f};
        glm::vec3 Normal{0.0f};
        float Distance{0.0f};
    };

    RayCastHit RayCast(const std::vector<std::shared_ptr<Entity>>& e, const Ray& ray);
    bool RayCastEntity(const std::shared_ptr<Entity>& entt, const Ray& ray, RayCastHit& out);

}