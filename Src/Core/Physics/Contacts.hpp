#pragma once

#include <glm/glm.hpp>
#include "Entity.hpp"

namespace Motion
{
    struct Contact
    {
        std::shared_ptr<Entity> A{};
        std::shared_ptr<Entity> B{};

        glm::vec3 Normal{0.0f};
        glm::vec3 Point{0.0f};
        float Penetration{0.0f};
    };
}