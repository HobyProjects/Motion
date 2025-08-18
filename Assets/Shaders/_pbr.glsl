#type vertex
#version 460 core

layout (location = 0) in vec3 a_Positions;
layout (location = 1) in vec2 a_TextureCoords;
layout (location = 2) in vec3 a_Normals;
layout (location = 3) in vec3 a_Tangents;
layout (location = 4) in vec3 a_Bitangents;

//======================================

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;
uniform mat4 u_NormalMatrix;

//======================================

out v_OUT
{
    vec3 WorldPosition;
    vec2 UVs;
    vec3 Normals;
    vec3 Tangents;
    vec3 Bitangents;

} VOUT;

void main()
{
    vec4 world          = u_ModelMatrix * vec4(a_Positions, 1.0);
    VOUT.WorldPosition  = world.xyz;
    VOUT.UVs            = a_TextureCoords;

    vec3 T = normalize(u_NormalMatrix * a_Tangents);
    vec3 B = normalize(u_NormalMatrix * a_Bitangents);
    vec3 N = normalize(u_NormalMatrix * a_Normals);

    VOUT.Tangents      = T;
    VOUT.Bitangents    = B;
    VOUT.Normals       = N;

    gl_Position = u_ProjectionMatrix * u_ViewMatrix * world;
}

// ==========================================================================================================================

#type fragment
#version 460 core

layout (location = 0) out vec4 FragColor;

in v_OUT
{
    vec3 WorldPosition;
    vec2 UVs;
    vec3 Normals;
    vec3 Tangents;
    vec3 Bitangents;

} FIN;

struct SunLight
{
    vec3 Direction;
    vec3 Color;
    float Intensity;
};

struct Textures
{
    sampler2D BaseColorTexture;
    sampler2D AOTexture;
    sampler2D MetallicTexture;
    sampler2D RoughnessTexture;
    sampler2D NormalMapTexture;
    sampler2D EmissiveTexture;
    sampler2D OpacityTexture;

    sampler2D ORMTexture;

    sampler2D ClearcoatTexture;       
    sampler2D ClearcoatRoughnessTexture;      
    sampler2D SpecularColorTexture;   
    sampler2D SpecularTexture;        
    sampler2D SheenColorTexture;      
    sampler2D SheenRoughnessTexture;          
    sampler2D TransmissionTexture;    
    sampler2D ThicknessTexture;       
};

struct Attributes
{
    vec3 BaseColor;
    vec3 SpecularColor;
    vec3 SheenColor;
    vec3 AttenuationColor;

    float MetallicFactor;
    float RoughnessFactor;
    float Opacity;
    float ClearcoatFactor;
    float ClearcoatRoughnessFactor;
    float SpecularLevel;
    float SheenRoughnessFactor;
    float Transmission;
    float Thickness;
    float AttenuationDistance;
    float IOR;
}

const int TB_BaseColor      = 1 << 0;
const int TB_Metallic       = 1 << 1;
const int TB_Roughness      = 1 << 2;
const int TB_Normal         = 1 << 3;
const int TB_AO             = 1 << 4;
const int TB_Emissive       = 1 << 5;
const int TB_Opacity        = 1 << 6;
const int TB_ORM            = 1 << 7;
const int TB_Clearcoat      = 1 << 8;
const int TB_ClearcoatR     = 1 << 9;
const int TB_SpecColor      = 1 << 10;
const int TB_Spec           = 1 << 11;
const int TB_SheenColor     = 1 << 12;
const int TB_SheenR         = 1 << 13;
const int TB_Trans          = 1 << 14;
const int TB_Thick          = 1 << 15;

const float PI = 3.14159265359;

//===========================================

uniform SunLight        u_SunLight;
uniform vec3            u_CameraPosition;
uniform float           u_NormalYFlip;

uniform samplerCube     u_IrradianceTexture;
uniform samplerCube     u_PrefilteredTexture;
uniform sampler2D       u_BRDFLUTTexture;
uniform float           u_IBLIntensity_Diffuse;
uniform float           u_IBLIntensity_Specular;
uniform float           u_IBLMipLevels; // < Get it from the Environment class

uniform Textures        u_Textures;
uniform Attributes      u_Attributes;
uniform int             u_TextureBitmask;

//===============================================

bool HasTexture(int bitMask)
{
    return (u_TextureBitmask & bitMask) != 0;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(float N, float H, float roughness)
{
    float a         = roughness * roughness;
    float a2        = a * a;
    float NdotH     = max(dot(N, H), 0.0);
    float NdotH2    = NdotH * NdotH;

    float num       = a2;
    float denom     = (NdotH2 * (a2 - 1.0) + 1.0);
    denom           = PI * denom * denom;

    return num / max(denom, 1e-6);
}

float GeometrySmithSchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySmithSchlickGGX(NdotV, rough);
    float ggx1  = GeometrySmithSchlickGGX(NdotL, rough);

    return ggx1 * ggx2;
}

float ApplyNormalMaps(vec3 N, vec3 T, vec3 B, vec2 UVs)
{
    if(!HasTexture(TB_Normal))
    {
        return normalize(N);
    }
    else
    {
        vec3 n = texture(u_Textures.NormalMapTexture, UVs).xyz * 2.0 - 1.0;
        n.y = mix(n.y, -n.y, u_NormalYFlip);
        mat3 TBN = mat3(normalize(T), normalize(B), normalize(N));

        return normalize(TBN * n);
    }
}

float DistributionCharlie(float NdotH, float alpha)
{
    float invAlpha = 1.0 / alpha;
    float cos2h = NdotH;
    float sin2h = sqrt(max(1.0 - cos2h*cos2h, 0.0));
    return (2.0 + invAlpha) * pow(sin2h, invAlpha) / (2.0 * PI);
}

float VisibilitySheen(float NdotL, float NdotV)
{
    return 1.0 / (4.0 * (NdotL + NdotV - NdotL*NdotV));
}

vec3 Attenuation(vec3 attColor, float attDist, float distance)
{
    if (attDist <= 0.0) return vec3(1.0);
    vec3 sigma_a = -log(max(attColor, vec3(1e-4))) / max(attDist, 1e-4);
    return exp(-sigma_a * distance);
}

vec3 RefractEnvironment(vec3 V, vec3 N, float ETA)
{
    return refract(-V, N, 1.0/ETA);
}

void main()
{
    vec2 UV     = FIN.UVs;
    vec3  V     = normalize(u_CameraPosition - FIN.WorldPosition);
    vec3  N     = ApplyNormalMaps(FIN.Normals, FIN.Tangents, FIN.Bitangents, UV); 


    vec3 baseColor          = vec3(1.0);
    vec3 specularColor      = vec3(0.4);
    vec3 sheenColor         = vec3(0.0);
    vec3 attenuationColor   = vec3(1.0);
    vec3 emissiveColor      = vec3(0.0);

    float metallicFactor    = 0.0;
    float roughnessFactor   = 0.0;
    float opacity           = 1.0;
    float AO                = 1.0;

    float clearcoatFactor               = 0.0;
    float clearcoatRoughnessFactor      = 0.1;
    float sheenRoughnessFactor          = 0.5;
    float transmissionFactor            = 0.0;
    float thicknessFactor               = 0.0;
    float IOR                           = 1.5;
    float attenuationDistance           = 1.0;

    if(HasTexture(TB_BaseColor))
        baseColor = texture(u_Textures.BaseColorTexture, UV).rgb * u_Attributes.BaseColor;

    if(HasTexture(TB_ORM) && !HasTexture(TB_Roughness) && !HasTexture(TB_Metallic))
    {
        vec3 ORM = texture(u_Textures.ORMTexture, UV).rgb;
        metallicFactor = ORM.b;
        roughnessFactor = ORM.g;
    }

    if(HasTexture(TB_Roughness))
        roughnessFactor = texture(u_Textures.RoughnessTexture, UV).r * u_Attributes.RoughnessFactor;
        
    if(HasTexture(TB_Metallic))
        metallicFactor = texture(u_Textures.MetallicTexture, UV).r * u_Attributes.MetallicFactor;

    if(HasTexture(TB_Opacity))
        opacity = texture(u_Textures.OpacityTexture, UV).r * u_Attributes.Opacity;



    vec3 F0 = mix(vec3(0.04), baseColor, metallicFactor);

    if(HasTexture(TB_SpecColor))
        F0 = texture(u_Textures.SpecularColorTexture, UV).rgb * u_Attributes.SpecularColor;
    else
        F0 = mix(F0, specularColor, 1.0);

    if(HasTexture(TB_Spec))
        F0 *= texture(u_Textures.SpecularTexture, UV).r * u_Attributes.SpecularLevel;
    else
        F0 *= specularColor;
    
    vec3 L = normalize(-u_SunLight.Direction);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    float D = DistributionGGX(N, H, roughnessFactor);
    float G = GeometrySmith(N, V, L, roughnessFactor);
    vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallicFactor);

    vec3 specular   = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-6);
    vec3 Lo         = (kD * baseColor / PI + specular) * (u_SunLight.Color * u_SunLight.Intensity) * NdotL;

    vec3 NN = normalize(N);
    vec3  R = reflect(-V, NN);

    vec3 irradiance = texture(u_IrradianceTexture, NN).rgb * u_IBLIntensity_Diffuse;
    vec3 differIBL  = irradiance * baseColor;

    float mipLevel = 5.0;
    if(u_IBLMipLevels > 0.0)
        mipLevel = u_IBLMipLevels;

    float LOD               = roughnessFactor * mipLevel;
    vec3 prefilteredColor   = textureLod(u_PrefilteredTexture, R, LOD).rgb * u_IBLIntensity_Specular;
    vec2 BRDF               = texture(u_BRDFLUTTexture, vec2(NdotV, roughnessFactor)).rg;
    vec3 specularIBL        = prefilteredColor * (FresnelSchlickRoughness(NdotV, F0, roughnessFactor) * BRDF.x + BRDF.y);

    if(HasTexture(TB_AO))   AO = texture(u_Textures.AOTexture, UV).r;
    if(HasTexture(TB_ORM))  AO = texture(u_Textures.ORMTexture, UV).r;
    vec3 ambient = (differIBL * (1.0 - metallicFactor) / PI + specularIBL) * AO;

    if(HasTexture(TB_Clearcoat) && HasTexture(TB_ClearcoatR))
    {
        clearcoatFactor             = texture(u_Textures.ClearcoatTexture, UV).r * u_Attributes.ClearcoatFactor;
        clearcoatRoughnessFactor    = texture(u_Textures.ClearcoatRoughnessTexture, UV).r * u_Attributes.ClearcoatRoughnessFactor;
        
        clearcoatFactor             = clamp(clearcoatFactor, 0.0, 1.0);
        clearcoatRoughnessFactor    = clamp(clearcoatRoughnessFactor, 0.0, 1.0);

        vec3 F0C            = vec3(0.04);
        float DC            = DistributionGGX(N, H, clearcoatRoughnessFactor);
        float GC            = GeometrySmith(N, V, L, clearcoatRoughnessFactor);
        vec3 FC             = FresnelSchlick(max(dot(H, V), 0.0), F0C);
        vec3 specularCC     = (DC * GC * FC) / max(4.0 * NdotV * NdotL, 1e-6);

        vec3 coatAttenuate  = (vec3(1.0) - FC * clearcoatFactor);
        Lo                  *= coatAttenuate;
        ambient             *= coatAttenuate;
        Lo                  +=  specularCC * (u_SunLight.Color * u_SunLight.Intensity) * NdotL * clearcoatFactor;

        float LODC              = clearcoatRoughnessFactor * mipLevel;
        vec3 prefilteredColorC  = textureLod(u_PrefilteredTexture, R, LODC).rgb * u_IBLIntensity_Specular;
        vec2 BRDFC              = texture(u_BRDFLUTTexture, vec2(NdotV, clearcoatRoughnessFactor)).rg;
        vec3 specularIBLC       = prefilteredColorC * (FresnelSchlickRoughness(NdotV, F0C, clearcoatRoughnessFactor) * BRDFC.x + BRDFC.y);
        ambient                 += specularIBLC * clearcoatFactor;
    }

    if(HasTexture(TB_SheenColor) && HasTexture(TB_SheenR))
    {
        sheenColor              = texture(u_Textures.SheenColorTexture, UV).rgb * u_Attributes.SheenColor;
        sheenRoughnessFactor    = texture(u_Textures.SheenRoughnessTexture, UV).r * u_Attributes.SheenRoughnessFactor;
        sheenRoughnessFactor    = clamp(sheenRoughnessFactor, 0.0, 1.0);

        if(length(sheenColor) > 1e-4)
        {
            float alpha     = max(1e-4, sheenRoughnessFactor);
            float NdotH     = max(dot(N, H), 0.0);
            float DS        = DistributionCharlie(NdotH, alpha);
            float VS        = VisibilitySheen(NdotL, NdotV);

            vec3 FS             = sheenColor;
            vec3 sheenSpecular  = (DS * VS) * FS;

            Lo              += sheenSpecular * NdotL;
            float LODS      = clamp(1.0 * sheenRoughnessFactor, 0.0, 1.0) * mipLevel;
            float PRES      = textureLod(u_PrefilteredTexture, R, LODS).r * u_IBLIntensity_Specular;
            ambient         += PRES * sheenColor * 0.25;
        }
    }

    if(HasTexture(TB_Trans) && HasTexture(TB_Thick))
    {
        transmissionFactor = texture(u_Textures.TransmissionTexture, UV).r * u_Attributes.TransmissionFactor;
        thicknessFactor    = texture(u_Textures.ThicknessTexture, UV).r * u_Attributes.ThicknessFactor;
        transmissionFactor = clamp(transmissionFactor, 0.0, 1.0);
        thicknessFactor    = clamp(thicknessFactor, 0.0, 1.0);

        if(transmissionFactor > 0.0 && opacity > 0.0)
        {
            if(u_Attributes.IOR > 0.0)
                IOR = u_Attributes.IOR;

            float ior                       = max(IOR, 1.0);
            vec3 RTR                        = RefractEnvironment(V, NN, ior);
            float LODT                      = roughnessFactor * mipLevel;
            vec3 transmissionEnvironment    = textureLod(u_PrefilteredTexture, RTR, LODT).rgb * u_IBLIntensity_Specular;

            attenuationDistance = u_Attributes.AttenuationDistance;
            attenuationColor    = u_Attributes.AttenuationColor;

            float DIST          = attenuationDistance * thicknessFactor;
            float ATTE          = attenuation(attenuationColor, attenuationDistance, DIST);
            vec3 transmitted    = transmissionEnvironment * baseColor * ATTE;

            float reflectWeight     = max(max(F0.r, max(F0.g, F0.b)), 0.04);
            float transmitWeight    = (1.0 - reflectWeight) * transmissionFactor;

            ambient     = mix(ambient, transmitted, transmitWeight);
            Lo          = (1.0 - transmitWeight) * 0.5;
            opacity     = mix(opacity, opacity * (1.0 - transmissionFactor * 0.5), 1.0);
        }
    }

    if(HasTexture(TB_Emissive))
    {
        emissiveColor = texture(u_Textures.EmissiveTexture, UV).rgb;
        emissiveColor *= u_IBLIntensity_Specular;
    }

    vec3 finalColor = ambient + Lo + emissiveColor;
    FragColor = vec4(finalColor, opacity);    
}























//----------------------------------------------------------------------------------------------------------------------------------

#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;

uniform mat4 u_Model;
uniform mat3 u_NormalMatrix;
uniform mat4 u_View;
uniform mat4 u_Proj;

out VS_OUT 
{
    vec3 WorldPos;
    vec2 UV;
    vec3 N;
    vec3 T;
    vec3 B;

} vs_out;

void main()
{
    vec4 world = u_Model * vec4(a_Position, 1.0);
    vs_out.WorldPos = world.xyz;
    vs_out.UV = a_TexCoord;

    vec3 N = normalize(u_NormalMatrix * a_Normal);
    vec3 T = normalize(u_NormalMatrix * a_Tangent);
    T = normalize(T - dot(T, N) * N);      // re-orthogonalize
    vec3 B = normalize(cross(N, T));

    vs_out.N = N;
    vs_out.T = T;
    vs_out.B = B;

    gl_Position = u_Proj * u_View * world;
}

#type fragment
#version 460 core

in VS_OUT 
{
    vec3 WorldPos;
    vec2 UV;
    vec3 N;
    vec3 T;
    vec3 B;

} fs_in;

layout(location = 0) out vec4 FragColor;

// ── light & camera ──────────────────────────────────────────────────────────
struct SunLight 
{
    vec3 direction;
    float intensity;
    vec3 color;
};

uniform SunLight u_Sun;
uniform vec3  u_CameraWorldPos;

// ── environment (IBL) ──────────────────────────────────────────────────────
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilteredEnvMap;
uniform sampler2D   u_BRDFLUT;
uniform float       u_IBLIntensity_Diffuse;
uniform float       u_IBLIntensity_Specular;

// ── base material scalars (fallbacks) ──────────────────────────────────────
uniform vec3  u_Material_BaseColor;
uniform float u_Material_Metallic;
uniform float u_Material_Roughness;
uniform float u_Material_Opacity;

// ── extended material scalars (see prerequisites) ──────────────────────────
uniform float u_Material_ClearcoatFactor;   // [0..1]
uniform float u_Material_ClearcoatRoughness;// [0..1]
uniform vec3  u_Material_SpecularColor;     // usually ~0.04
uniform float u_Material_SpecularLevel;     // multiplier on F0 (1.0 default)
uniform vec3  u_Material_SheenColor;
uniform float u_Material_SheenRoughness;    // [0..1]
uniform float u_Material_Transmission;      // [0..1] thin-surface
uniform float u_Material_Thickness;         // [0..1]
uniform vec3  u_Material_AttenuationColor;  // transmittance color
uniform float u_Material_AttenuationDist;   // meters (or scene units)
uniform float u_Material_IOR;               // e.g. 1.5 (glass)

// ── samplers for all maps ──────────────────────────────────────────────────

// base
uniform sampler2D u_BaseColorTex;
uniform sampler2D u_AOTex;
uniform sampler2D u_MetallicTex;
uniform sampler2D u_RoughnessTex;
uniform sampler2D u_NormalTex;
uniform sampler2D u_EmissiveTex;
uniform sampler2D u_OpacityTex;

// packed
uniform sampler2D u_ORMTex;

// extended
uniform sampler2D u_ClearcoatTex;       // factor
uniform sampler2D u_ClearcoatRTex;      // roughness
uniform sampler2D u_SpecularColorTex;   // RGB F0 override
uniform sampler2D u_SpecularTex;        // scalar level
uniform sampler2D u_SheenColorTex;      // color
uniform sampler2D u_SheenRTex;          // roughness
uniform sampler2D u_TransmissionTex;    // scalar
uniform sampler2D u_ThicknessTex;       // scalar

// ── map presence: bit mask from CPU ────────────────────
uniform int u_TexMask;

// must match CPU bit layout
const int TB_BaseColor  = 1 << 0;
const int TB_Metallic   = 1 << 1;
const int TB_Roughness  = 1 << 2;
const int TB_Normal     = 1 << 3;
const int TB_AO         = 1 << 4;
const int TB_Emissive   = 1 << 5;
const int TB_Opacity    = 1 << 6;
const int TB_ORM        = 1 << 7;
const int TB_Clearcoat  = 1 << 8;
const int TB_ClearcoatR = 1 << 9;
const int TB_SpecColor  = 1 << 10;
const int TB_Spec       = 1 << 11;
const int TB_SheenColor = 1 << 12;
const int TB_SheenR     = 1 << 13;
const int TB_Trans      = 1 << 14;
const int TB_Thick      = 1 << 15;

// UI-driven normal tweak
uniform float u_NormalYFlip; // 0.0 keep, 1.0 invert green

// ── helpers ────────────────────────────────────────────────────────────────
const float PI = 3.14159265359;

bool hasMap(int bit) { return (u_TexMask & bit) != 0; }

vec3 fresnel_schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnel_schlick_roughness(float cosTheta, vec3 F0, float roughness)
{
    // from Epic: more grazing reflectance at high roughness
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float distribution_ggx(vec3 N, vec3 H, float rough)
{
    float a  = rough * rough;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / max(denom, 1e-6);
}

float geometry_smith_schlick_ggx(float NdotV, float rough)
{
    float r = rough + 1.0;
    float k = (r*r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float rough)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_smith_schlick_ggx(NdotV, rough);
    float ggx1 = geometry_smith_schlick_ggx(NdotL, rough);
    return ggx1 * ggx2;
}

vec3 apply_normal_map(vec3 N, vec3 T, vec3 B, vec2 uv)
{
    if (!hasMap(TB_Normal)) return normalize(N);
    vec3 n = texture(u_NormalTex, uv).xyz * 2.0 - 1.0;
    n.y = mix(n.y, -n.y, clamp(u_NormalYFlip, 0.0, 1.0));
    mat3 TBN = mat3(normalize(T), normalize(B), normalize(N));
    return normalize(TBN * n);
}

// Charlie (sheen) distribution from Burley 2015 (approximation)
float D_Charlie(float NdotH, float alpha)
{
    float invAlpha = 1.0 / alpha;
    float cos2h = NdotH;
    float sin2h = sqrt(max(1.0 - cos2h*cos2h, 0.0));
    return (2.0 + invAlpha) * pow(sin2h, invAlpha) / (2.0 * PI);
}

// Sheen Visibility term (approx) — Burley
float V_Sheen(float NdotL, float NdotV)
{
    return 1.0 / (4.0 * (NdotL + NdotV - NdotL*NdotV)); // crude but stable
}

// Beer-Lambert attenuation
vec3 attenuation(vec3 attColor, float attDist, float distance)
{
    if (attDist <= 0.0) return vec3(1.0);
    vec3 sigma_a = -log(max(attColor, vec3(1e-4))) / max(attDist, 1e-4);
    return exp(-sigma_a * distance);
}

// Thin-surface refraction dir (approx): view refracted by IOR
vec3 refract_env(vec3 V, vec3 N, float eta)
{
    // GLSL refract expects IOR ratio (eta = n1/n2). For air→material: 1.0/IOR.
    return refract(-V, N, 1.0/eta);
}

// ── main ───────────────────────────────────────────────────────────────────
void main()
{
    vec2 uv = fs_in.UV;
    vec3  V = normalize(u_CameraWorldPos - fs_in.WorldPos);
    vec3  N = apply_normal_map(fs_in.N, fs_in.T, fs_in.B, uv);

    // base color
    vec3 baseColor = u_Material_BaseColor;
    if (hasMap(TB_BaseColor)) baseColor = texture(u_BaseColorTex, uv).rgb;

    // metallic / roughness (prefer separate; fall back to ORM channels)
    float metallic  = u_Material_Metallic;
    float roughness = u_Material_Roughness;

    if (hasMap(TB_ORM)) {
        vec3 orm = texture(u_ORMTex, uv).rgb;
        if (!hasMap(TB_Roughness)) roughness = orm.g;
        if (!hasMap(TB_Metallic))  metallic  = orm.b;
    }
    if (hasMap(TB_Metallic))  metallic  = texture(u_MetallicTex,  uv).r;
    if (hasMap(TB_Roughness)) roughness = texture(u_RoughnessTex, uv).r;
    metallic  = clamp(metallic,  0.0, 1.0);
    roughness = clamp(roughness, 0.04, 1.0);

    // opacity
    float opacity = u_Material_Opacity;
    if (hasMap(TB_Opacity)) opacity *= texture(u_OpacityTex, uv).r;

    // F0 (specular workflow overrides)
    vec3  F0 = mix(vec3(0.04), baseColor, metallic);
    if (hasMap(TB_SpecColor)) F0 = texture(u_SpecularColorTex, uv).rgb;
    else                      F0 = mix(F0, u_Material_SpecularColor, 1.0); // allow scalar override
    if (hasMap(TB_Spec))      F0 *= texture(u_SpecularTex, uv).r;
    else                      F0 *= u_Material_SpecularLevel;

    // direct light (sun)
    vec3 L = normalize(-u_Sun.direction);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    float  D  = distribution_ggx(N, H, roughness);
    float  G  = geometry_smith(N, V, L, roughness);
    vec3   F  = fresnel_schlick(max(dot(H, V), 0.0), F0);

    vec3  kS = F;
    vec3  kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3  specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-6);
    vec3  Lo = (kD * baseColor / PI + specular) * (u_Sun.color * u_Sun.intensity) * NdotL;

    // IBL
    vec3 Nn = normalize(N);
    vec3 R  = reflect(-V, Nn);

    // diffuse irradiance
    vec3 irradiance = texture(u_IrradianceMap, Nn).rgb * u_IBLIntensity_Diffuse;
    vec3 diffuseIBL = irradiance * baseColor;

    // specular IBL
    float mipCount = 5.0; // depends on prefilter chain
    float lod = roughness * mipCount;
    vec3 prefiltered = textureLod(u_PrefilteredEnvMap, R, lod).rgb * u_IBLIntensity_Specular;
    vec2 brdf = texture(u_BRDFLUT, vec2(NdotV, roughness)).rg;
    vec3 specIBL = prefiltered * (fresnel_schlick_roughness(NdotV, F0, roughness) * brdf.x + brdf.y);

    // AO (prefer AO map, else ORM.R, else 1)
    float ao = 1.0;
    if (hasMap(TB_AO))  ao = texture(u_AOTex, uv).r;
    else if (hasMap(TB_ORM)) ao = texture(u_ORMTex, uv).r;

    vec3 ambient = (diffuseIBL * (1.0 - metallic) / PI + specIBL) * ao;

    // ── CLEARCOAT (extra GGX lobe on top) ───────────────────────────────────
    float ccFactor = hasMap(TB_Clearcoat)  ? texture(u_ClearcoatTex,  uv).r : u_Material_ClearcoatFactor;
    float ccRough  = hasMap(TB_ClearcoatR) ? texture(u_ClearcoatRTex, uv).r : u_Material_ClearcoatRoughness;
    ccFactor = clamp(ccFactor, 0.0, 1.0);
    ccRough  = clamp(ccRough,  0.0, 1.0);

    if (ccFactor > 0.0)
    {
        // coat uses a near-dielectric F0 ~ 0.04..0.08; we’ll take 0.04
        vec3  F0c = vec3(0.04);
        float Dc  = distribution_ggx(N, H, ccRough);
        float Gc  = geometry_smith(N, V, L, ccRough);
        vec3  Fc  = fresnel_schlick(max(dot(H, V), 0.0), F0c);
        vec3  specCC = (Dc * Gc * Fc) / max(4.0 * NdotV * NdotL, 1e-6);

        // attenuate base reflection by (1 - Fc * ccFactor)
        vec3 coatAtten = (vec3(1.0) - Fc * ccFactor);
        Lo      *= coatAtten;
        ambient *= coatAtten;

        // add the coat lobe on top
        Lo += specCC * (u_Sun.color * u_Sun.intensity) * NdotL * ccFactor;

        // coat IBL (simple: use same R and BRDF with coat roughness)
        float lodC = ccRough * mipCount;
        vec3 preC  = textureLod(u_PrefilteredEnvMap, R, lodC).rgb * u_IBLIntensity_Specular;
        vec2 brdfC = texture(u_BRDFLUT, vec2(NdotV, ccRough)).rg;
        vec3 specIBL_C = preC * (fresnel_schlick_roughness(NdotV, F0c, ccRough) * brdfC.x + brdfC.y);
        ambient += specIBL_C * ccFactor;
    }

    // ── SHEEN (cloth-like grazing lobe) ─────────────────────────────────────
    vec3  sheenColor = hasMap(TB_SheenColor) ? texture(u_SheenColorTex, uv).rgb : u_Material_SheenColor;
    float sheenR     = hasMap(TB_SheenR)     ? texture(u_SheenRTex,    uv).r    : u_Material_SheenRoughness;
    sheenR = clamp(sheenR, 0.0, 1.0);

    if (length(sheenColor) > 1e-4)
    {
        float alpha = max(1e-4, sheenR);
        float NdotH = max(dot(N, H), 0.0);
        float Ds    = D_Charlie(NdotH, alpha);
        float Vs    = V_Sheen(NdotL, NdotV);
        // Fresnel for sheen often uses a simple color scale (no metallic coupling)
        vec3  Fs    = sheenColor;
        vec3  sheenSpec = (Ds * Vs) * Fs;

        Lo += sheenSpec * NdotL; // add to direct
        // IBL sheen (simple): use prefiltered spec with high roughness bias
        float lodS = clamp(0.5 + 0.5 * sheenR, 0.0, 1.0) * mipCount;
        vec3 preS  = textureLod(u_PrefilteredEnvMap, R, lodS).rgb * u_IBLIntensity_Specular;
        ambient += preS * sheenColor * 0.25; // scaled to avoid over-energy
    }

    // ── TRANSMISSION + THICKNESS (thin surface IBL) ─────────────────────────
    float transmission = hasMap(TB_Trans) ? texture(u_TransmissionTex, uv).r : u_Material_Transmission;
    float thickness    = hasMap(TB_Thick) ? texture(u_ThicknessTex,    uv).r : u_Material_Thickness;
    transmission = clamp(transmission, 0.0, 1.0);
    thickness    = clamp(thickness,    0.0, 1.0);

    if (transmission > 0.0 && opacity > 0.0)
    {
        // refracted env (thin surface)
        float ior = max(u_Material_IOR, 1.0);
        vec3  Rtr = refract_env(V, Nn, ior);
        float lodT = roughness * mipCount;
        vec3  transEnv = textureLod(u_PrefilteredEnvMap, Rtr, lodT).rgb;

        // attenuation by thickness via Beer-Lambert
        float dist = u_Material_AttenuationDist * thickness;
        vec3  att  = attenuation(u_Material_AttenuationColor, u_Material_AttenuationDist, dist);

        vec3 transmitted = transEnv * baseColor * att;

        // split energy between reflection and transmission (conservatively)
        // more correct is to use Fresnel at N·V; keep simple:
        float reflectWeight = max(max(F0.r, max(F0.g, F0.b)), 0.04);
        float transmitWeight = (1.0 - reflectWeight) * transmission;

        // put transmitted in ambient term (environment lighting side)
        ambient = mix(ambient, transmitted, transmitWeight);
        // also reduce direct Lo a bit so refraction isn't overbright
        Lo *= (1.0 - transmitWeight * 0.5);
        // and reduce opacity a touch to hint translucency
        opacity = mix(opacity, opacity * (1.0 - transmission * 0.5), 1.0);
    }

    // emissive
    vec3 emissive = vec3(0.0);
    if (hasMap(TB_Emissive)) emissive = texture(u_EmissiveTex, uv).rgb;

    vec3 color = ambient + Lo + emissive;

    FragColor = vec4(color, opacity);
}

