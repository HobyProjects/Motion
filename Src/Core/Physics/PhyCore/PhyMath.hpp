#pragma once

#include <glm/glm.hpp>
#include <cmath>

namespace Motion
{
    inline void OrthonormalBasis(const glm::vec3& n, glm::vec3& t1, glm::vec3& t2) 
    {
        glm::vec3 h = (std::abs(n.x) < 0.577f) ? glm::vec3(1,0,0) :
                      (std::abs(n.y) < 0.577f) ? glm::vec3(0,1,0) : glm::vec3(0,0,1);

        t1 = glm::normalize(glm::cross(h, n));
        t2 = glm::cross(n, t1);
    }
}