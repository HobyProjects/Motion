#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

#include "Components.hpp"

namespace Motion
{
    struct Bounds 
    {
        glm::vec3 Min{0.0f};
        glm::vec3 Max{0.0f};
    };

    enum class FitMode 
    {
        UniformLongestSide,  // keep proportions, longest side becomes targetMax
        UniformHeight,       // keep proportions, Y becomes targetMax
        NonUniformToBox      // scale each axis to match targetSize (no proportions)
    };

    inline glm::vec3 safeDiv(const glm::vec3& a, const glm::vec3& b, float eps = 1e-6f) 
    {
        return glm::vec3(
            b.x > eps ? a.x / b.x : 1.0f,
            b.y > eps ? a.y / b.y : 1.0f,
            b.z > eps ? a.z / b.z : 1.0f
        );
    }

    inline void FitTransformToWorldBox(TransformComponent& tr,  const Bounds& b, FitMode mode, const glm::vec3& target, const glm::vec3& worldPos, bool pivotAtCenter = true, bool sitOnGround = false, float groundY = 0.0f) 
    {
        const glm::vec3 size   = b.Max - b.Min;
        const glm::vec3 center = 0.5f * (b.Min + b.Max);
        const glm::vec3 pivot  = pivotAtCenter ? center : b.Min;

        glm::vec3 newScale(1.0f);
        switch (mode) 
        {
            case FitMode::UniformLongestSide: 
            {
                float maxDim = std::max({size.x, size.y, size.z});
                float s = (maxDim > 1e-6f) ? (target.x / maxDim) : 1.0f;
                newScale = glm::vec3(s);

            } break;
            case FitMode::UniformHeight: 
            {
                float h = size.y;
                float s = (h > 1e-6f) ? (target.x / h) : 1.0f;
                newScale = glm::vec3(s);

            } break;
            case FitMode::NonUniformToBox: 
            {
                newScale = safeDiv(target, size);
                
            } break;
        }

        const glm::mat3 R = glm::mat3_cast(tr.Rotation);
        const glm::mat3 S = glm::mat3(glm::vec3(newScale.x, 0, 0), glm::vec3(0, newScale.y, 0), glm::vec3(0, 0, newScale.z));
        const glm::mat3 RS = R * S;

        glm::vec3 translation = worldPos - RS * pivot;
        if (sitOnGround) 
        {
            const glm::vec3 minRel = b.Min - pivot;
            const float minYAfter = (RS * minRel).y + translation.y;
            const float dy = groundY - minYAfter;
            translation.y += dy;
        }

        tr.Scale = newScale;
        tr.Translation = translation;
    }
}