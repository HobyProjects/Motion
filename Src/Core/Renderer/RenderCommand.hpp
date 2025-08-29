#pragma once

#include "Base.hpp"
#include "UUID.hpp"
#include "Model.hpp"
#include "Shaders.hpp"
#include "Texture.hpp"
#include "Environment.hpp"

namespace Motion
{   
    struct CameraViewProjection
    {
        glm::mat4 View;
        glm::mat4 Projection;
        glm::vec3 CameraPosition; 
    };

    struct ModelMatrix
    {
        glm::mat4 Model;
        glm::mat3 Normal;
    };

    struct DirectionalLight
    {
        glm::vec3   Direction;   
        glm::vec3   Color;     
        float       Intensity; 
    };

    struct MaterialAttributes
    {
        glm::vec4  BaseColorFactor;
        glm::vec3  EmissiveColor;
        
        float NormalScale;
        float MetallicFactor;
        float RoughnessFactor;
        float AOFactor;
        float OpacityFactor;
        float EmissiveStrength;
    };

    struct RenderCommand
    {
        UUID SortKey{0};

        Mesh*           MeshPointer{nullptr};
        Material*       MaterialPointer{nullptr};
        IEnvironment*   EnvPointer{nullptr};

        ModelMatrix             ModelData{};
        CameraViewProjection    CameraData{};
        DirectionalLight        LightData{};

        bool operator<(const RenderCommand& other) const { return std::tie(SortKey, MaterialPointer, EnvPointer, MeshPointer) < std::tie(other.SortKey, other.MaterialPointer, other.EnvPointer, other.MeshPointer); }
        bool operator>(const RenderCommand& other) const { return std::tie(SortKey, MaterialPointer, EnvPointer, MeshPointer) > std::tie(other.SortKey, other.MaterialPointer, other.EnvPointer, other.MeshPointer); }
    };

}