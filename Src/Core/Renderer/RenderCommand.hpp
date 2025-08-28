#pragma once

#include "Base.hpp"
#include "UUID.hpp"
#include "Model.hpp"
#include "Shaders.hpp"
#include "Texture.hpp"
#include "Environment.hpp"

namespace Motion
{
    enum class RenderPass : std::uint32_t
    {
        Opaque          = 0,
        AlphaTest       = 1,
        Transparent     = 2,
        DepthOnly       = 3,
        Shadow          = 4,
        ForwardLit      = 5,
        PostProcess     = 6,
        Overlay         = 7,
    };

    enum class RenderFlags : std::uint32_t
    {
        None             = 0,

        Skinned          = 1u << 0,
        MorphTarget      = 1u << 1,
        Tessellation     = 1u << 2,
        Instanced        = 1u << 3,

        AlphaTest        = 1u << 4,
        AlphaBlend       = 1u << 5,
        Premultiplied    = 1u << 6,
        Additive         = 1u << 7,

        DoubleSided      = 1u << 8,
        Wireframe        = 1u << 9,
        DepthWriteOff    = 1u << 10,
        DepthTestOff     = 1u << 11,
    };

    template <>
    struct enable_bitmask_operations<RenderFlags> : std::true_type {};
    
#pragma pack(push, 1)
    struct CameraViewProjection
    {
        glm::mat4 uView;
        glm::mat4 uProj;
        glm::vec3 uCameraPos; 
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct ModelMatrix
    {
        glm::mat4 uModel;
        glm::mat3 uNormal;
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct SunLighting
    {
        glm::vec3  uSunDirection;   
        glm::vec3  uSunColor;     
        float uSunIntensity; 
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct MaterialAttributes
    {
        glm::vec4  uBaseColorFactor;
        float      uAlphaCutOff;
        float      uNormalScale;
        float      uMetallicFactor;
        float      uRoughnessFactor;
        float      uAOFactor;
        float      uOpacityFactor;
        float      uEmissiveStrength;
        float      uSpecularStrength;
        glm::vec3  uEmissiveColor;
        float      uDisplacementScale;   
        float      uDisplacementBias;    
        float      uTessMin;             
        float      uTessMax;
        float      uPixPerEdge;         
        float      uLODNear;
        float      uLODFar;
    };
#pragma pack(pop)

    struct RenderCommand
    {
        RenderPass  Pass{RenderPass::Opaque};
        RenderFlags Flags{RenderFlags::None};
        UUID SortKey{0};

        Mesh*           MeshPointer{nullptr};
        Material*       MaterialPointer{nullptr};
        IEnvironment*   EnvPointer{nullptr};

        ModelMatrix ModelData{};
        CameraViewProjection CameraData{};
        SunLighting SunData{};

        bool operator<(const RenderCommand& other) const { return std::tie(SortKey, MaterialPointer, EnvPointer, MeshPointer) < std::tie(other.SortKey, other.MaterialPointer, other.EnvPointer, other.MeshPointer); }
        bool operator>(const RenderCommand& other) const { return std::tie(SortKey, MaterialPointer, EnvPointer, MeshPointer) > std::tie(other.SortKey, other.MaterialPointer, other.EnvPointer, other.MeshPointer); }
    };

}