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
        Shadow          = 1,
        DepthOnly       = 2,
        Transparent     = 3,
    };

    enum class RenderFlags : std::uint32_t
    {
        None           = 0,
        Tessellation   = MOTION_BIT(0),
        AlphaTest      = MOTION_BIT(1),
        Transparent    = MOTION_BIT(2),
        Skinned        = MOTION_BIT(3),
    };

    inline RenderPass operator|(RenderPass a, RenderPass b){ return static_cast<RenderPass>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b)); }
    inline RenderPass operator&(RenderPass a, RenderPass b){ return static_cast<RenderPass>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b)); }
    inline RenderPass& operator|=(RenderPass& a, RenderPass b){ return a = a | b; }
    inline RenderPass& operator&=(RenderPass& a, RenderPass b){ return a = a & b; }

    inline RenderFlags operator|(RenderFlags a, RenderFlags b){ return static_cast<RenderFlags>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b)); }
    inline RenderFlags operator&(RenderFlags a, RenderFlags b){ return static_cast<RenderFlags>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b)); }
    inline RenderFlags& operator|=(RenderFlags& a, RenderFlags b){ return a = a | b; }
    inline RenderFlags& operator&=(RenderFlags& a, RenderFlags b){ return a = a & b; }

#pragma pack(push, 1)
    struct CameraViewProjection
    {
        glm::mat4 uView;
        glm::mat4 uProj;
        glm::vec3 uCameraPos; 
        float _pad0;
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
        glm::vec3  uSunDirection;  float _padS0; 
        glm::vec3  uSunColor;      float _padS1; 
        float      uSunIntensity;  float _padS2; float _padS3; float _padS4;
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct MaterialAttributes
    {
        glm::vec4   uBaseColorFactor; 
        float       uAlphaCutOff;    
        float       uNormalScale;      
        float       uMetallicFactor;   
        float       uRoughnessFactor;   
        float       uAOFactor;         
        float       uOpacityFactor;    
        float       uEmissiveStrength;  
        float       uSpecularStrength;  
        glm::vec3   uEmissiveColor;     
        float       _padM0;
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
    };

}