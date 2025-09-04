#pragma once

#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

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

    struct FitResult
    {
        glm::vec3 Translation{0.0f}; // world translation
        glm::vec3 Rotation{0.0f};    // euler XYZ (radians). Kept zero here (no re-orient)
        glm::vec3 Scale{1.0f};       // per-axis scale
    };

    inline FitResult FitToWorldBox(const Bounds& b, FitMode mode, const glm::vec3& targetSize = glm::vec3(0.1f), const glm::vec3& targetCenter = glm::vec3(0.0f))
    {
        constexpr float kEps = 1e-6f;

        const glm::vec3 srcSize   = b.Max - b.Min;
        const glm::vec3 srcCenter = (b.Max + b.Min) * 0.5f;

        // Guard against degenerate bounds
        glm::vec3 safeSrcSize = glm::max(srcSize, glm::vec3(kEps));

        glm::vec3 scale(1.0f);

        switch (mode)
        {
            case FitMode::UniformLongestSide:
            {
                float srcMax = glm::compMax(safeSrcSize);
                float dstMax = glm::compMax(glm::max(targetSize, glm::vec3(kEps)));
                float s = dstMax / srcMax;
                scale = glm::vec3(s);
            } break;

            case FitMode::UniformHeight:
            {
                float s = glm::max(targetSize.y, kEps) / safeSrcSize.y;
                scale = glm::vec3(s);
            } break;

            case FitMode::NonUniformToBox:
            {
                scale.x = glm::max(targetSize.x, kEps) / safeSrcSize.x;
                scale.y = glm::max(targetSize.y, kEps) / safeSrcSize.y;
                scale.z = glm::max(targetSize.z, kEps) / safeSrcSize.z;
            } break;
        }


        FitResult out;
        out.Scale       = scale;
        out.Rotation    = glm::vec3(0.0f);
        out.Translation = targetCenter - (srcCenter * scale);
        return out;
    }


}