#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Motion 
{
    struct PhyConfig 
    {
        float FixedDeltaTime{1.0f/60.0f};
        bool UseFixedDeltaTime{false};

        int   VelocityIterations{8};
        int   PositionIterations{3};
        int   MaxSubsteps{1};

        glm::vec3 Gravity{0,-9.81f, 0};

        float FatAABBVelocityPad{0.5f};

        float LinearSlop{0.005f};
        float AngularSlop{0.0349f}; // ~2°
        float Baumgarte{0.2f};
        float RestitutionVelocityThreshold{1.0f};

        enum class CombineRule : std::uint8_t { Average = 0, Min, Max, Multiply, GeometricMean };
        CombineRule FrictionRule{CombineRule::GeometricMean};
        CombineRule RestitutionRule{CombineRule::Max};

        float SleepLinearThreshold{0.05f};
        float SleepAngularThreshold{0.05f};
        int   SleepFrames{60};
    };

    inline float Combine(float a, float b, PhyConfig::CombineRule rule) 
    {
        switch (rule) 
        {
            case PhyConfig::CombineRule::Average:       return 0.5f * (a + b);
            case PhyConfig::CombineRule::Min:           return a < b ? a : b;
            case PhyConfig::CombineRule::Max:           return a > b ? a : b;
            case PhyConfig::CombineRule::Multiply:      return a*b;
            case PhyConfig::CombineRule::GeometricMean: return sqrtf(a * b);
            default:
        }
    }
}
