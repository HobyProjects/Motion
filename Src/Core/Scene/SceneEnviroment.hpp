#pragma once

#include <memory>
#include <glm/glm.hpp>

namespace Motion
{
    struct DirectLight
    {
        glm::vec3 Direction{ -100.0f, -100.0f, -50.0f };
        glm::vec3 Color{ 1.0f,  1.0f,  1.0f };
        float Intensity{ 3.0f };

        bool CastShadow{false};
        bool ShowDir{false};

        void SetFromPosition(const glm::vec3& position, const glm::vec3& target = glm::vec3(0.0f))
        {
            Direction = glm::normalize(target - position);
        }
    };

    struct SceneEnvironment
    {
        DirectLight Sun{};
    };
}